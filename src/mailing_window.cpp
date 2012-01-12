/* Copyright (C) 2004-2011 Daniel Verite

   This file is part of Manitou-Mail (see http://www.manitou-mail.org)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "main.h"
#include "mailing_window.h"
#include "mailing_viewer.h"
#include "mailing_wizard.h"
#include "icons.h"
#include "date.h"

#include <QTreeWidget>
#include <QPushButton>
#include <QBoxLayout>
#include <QProgressBar>
#include <QHeaderView>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <QMessageBox>

mailing_window*
mailing_window::m_current_instance; // singleton

void
open_mailing_window()
{
  mailing_window* w = mailing_window::open_unique();
  w->show();
  w->activateWindow();
  w->raise();
}

mailing_window*
mailing_window::open_unique()
{
  if (!m_current_instance)
    m_current_instance = new mailing_window();
  return m_current_instance;
}

mailing_window::mailing_window()
{
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowIcon(UI_ICON(ICON16_MAIL_MERGE));
  setWindowTitle(tr("Mailings"));

  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setMargin(5);

  QTreeWidget* w = new QTreeWidget();
  m_treewidget = w;
  top_layout->addWidget(w);

  QStringList labels;
  labels << tr("Title") << tr("Date") << tr("Status") << tr("Sent") << tr("Completion");
  w->setHeaderLabels(labels);
  w->header()->resizeSection(0, 200);
  w->setRootIsDecorated(false);
  w->setAllColumnsShowFocus(true);

  QHBoxLayout* buttons = new QHBoxLayout();
  top_layout->addLayout(buttons);

  m_btn_stop_restart = new QPushButton(tr("Stop"));
  buttons->addWidget(m_btn_stop_restart);
  connect(m_btn_stop_restart, SIGNAL(clicked()), this, SLOT(start_stop()));

  m_btn_delete = new QPushButton(tr("Delete"));
  buttons->addWidget(m_btn_delete);
  connect(m_btn_delete, SIGNAL(clicked()), this, SLOT(delete_mailing()));

  m_btn_view = new QPushButton(tr("View"));
  buttons->addWidget(m_btn_view);
  connect(m_btn_view, SIGNAL(clicked()), this, SLOT(view_mailing()));

#if 0
  m_btn_clone = new QPushButton(tr("Clone..."));
  buttons->addWidget(m_btn_clone);
  connect(m_btn_clone, SIGNAL(clicked()), this, SLOT(clone_mailing()));
#endif

  buttons->addStretch(5);

  m_btn_refresh = new QPushButton(tr("Refresh"));
  buttons->addWidget(m_btn_refresh);
  connect(m_btn_refresh, SIGNAL(clicked()), this, SLOT(refresh()));

  m_btn_new = new QPushButton(tr("New..."));
  buttons->addWidget(m_btn_new);
  connect(m_btn_new, SIGNAL(clicked()), this, SLOT(new_mailing()));

  connect(w, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	  this, SLOT(item_changed(QTreeWidgetItem*,QTreeWidgetItem*)));

  w->setSortingEnabled(true);
  w->sortByColumn(mailing_window_widget_item::index_column_date, Qt::DescendingOrder);

  load();

  m_timer = new QTimer(this);
  m_timer->start(5000);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
  resize(600,300);
}

mailing_window::~mailing_window()
{
  m_current_instance=NULL;
}

void
mailing_window::item_changed(QTreeWidgetItem* item, QTreeWidgetItem* old)
{
  Q_UNUSED(old);
  if (item) {
    mailing_db* m = get_mailing(item->data(0, mailing_window_widget_item::mailing_id_role).toInt());
    if (m) {
      update_buttons(m);
    }
  }
  else {
    DBG_PRINTF(2, "item_changed for NULL");
  }
}

void
mailing_window::update_buttons(mailing_db* m)
{
  if (m->status()==mailing_db::status_not_started || m->status()==mailing_db::status_stopped ) {
    m_btn_stop_restart->setText(tr("Start"));
    m_btn_stop_restart->setEnabled(true);
  }
  else if (m->status()==mailing_db::status_running) {
    m_btn_stop_restart->setText(tr("Stop"));
    m_btn_stop_restart->setEnabled(true);
  }
  else if (m->status()==mailing_db::status_finished) {
    m_btn_stop_restart->setText(tr("Stop"));
    m_btn_stop_restart->setEnabled(false);
  }
}

mailing_db*
mailing_window::get_mailing(int mailing_id)
{
  mailings::iterator iter = m_mailings.begin();
  for (; iter != m_mailings.end(); ++iter) {
    if (iter->id() == mailing_id)
      return &(*iter);
  }
  return NULL;
}

// retrieve the item in the treeview by the mailing_id
mailing_window_widget_item*
mailing_window::get_item(int mailing_id)
{
  QTreeWidgetItemIterator it(m_treewidget);
  while (*it) {
    if ((*it)->data(0,mailing_window_widget_item::mailing_id_role).toInt() == mailing_id)
      return static_cast<mailing_window_widget_item*>(*it);
    ++it;
  }
  return NULL;
}

mailing_db*
mailing_window::selected_item()
{
  QTreeWidgetItem* item = m_treewidget->currentItem();
  return item ? get_mailing(item->data(0, mailing_window_widget_item::mailing_id_role).toInt()) : NULL;  
}

// Populate the list with contents from the database
void
mailing_window::load()
{
  m_mailings.load();
  mailings::const_iterator iter = m_mailings.constBegin();
  for (; iter != m_mailings.constEnd(); ++iter) {
    add_item(&(*iter));
  }
}

mailing_window_widget_item*
mailing_window::add_item(const mailing_db* m)
{
  mailing_window_widget_item* item = new mailing_window_widget_item();
  QProgressBar* bar = new QProgressBar();
  m_progress_bars[m->id()] = bar;
  /* m_nb_sent should be lower or equal than m_nb_total, but if it
     happens to be greater, we take it as the upper boundary */
  bar->setRange(0, m->m_nb_total>=m->m_nb_sent ? m->m_nb_total : m->m_nb_sent);
  m_treewidget->addTopLevelItem(item);
  m_treewidget->setItemWidget(item, 4, bar);
  item->setData(0, mailing_window_widget_item::mailing_id_role, m->id());
  QVariant vdate;
  vdate.setValue(m->creation_date());
  item->setData(mailing_window_widget_item::index_column_date,
		mailing_window_widget_item::mailing_date_role, vdate);
  display_entry(item, m);
  return item;
}

void
mailing_window::display_entry(QTreeWidgetItem* item, const mailing_db* m)
{
  if (!item)
    return;
  int col=0;
  item->setText(col++, m->title());
  item->setText(col++, m->creation_date().output_8());
  item->setText(col++, m->status_text());
  item->setText(col++, QString("%1 / %2").arg(m->m_nb_sent).arg(m->m_nb_total));
  QProgressBar* bar = m_progress_bars[m->id()];
  if (bar) {
    bar->setValue(m->m_nb_sent);
    QString color="#1e8bd8"; // blue
    switch(m->status()) {
    case mailing_db::status_running:
      color="green";
      break;
    case mailing_db::status_stopped:
      color="red";
      break;
    }
    QString style=QString("QProgressBar:horizontal {"
			  "border: 1px solid gray;"
			  "border-radius: 3px;"
			  "background: white;"
			  "padding: 1px;"
			  "text-align: center;"
			  "font-weight: bold;"
			  "}"
			  "QProgressBar::chunk:horizontal {"
			  "background: qlineargradient(x1: 0, y1: 0.5, x2: 1, y2: 0.5, stop: 0 %1, stop: 1 white);"
			  "}").arg(color);
    bar->setStyleSheet(style);
  }
}

void
mailing_window::start_stop()
{
  mailing_db* m = selected_item();
  if (!m) return;

  if (m->status()==mailing_db::status_not_started || m->status()==mailing_db::status_stopped ) {
    m->instantiate_job();
  }
  else if (m->status()==mailing_db::status_running) {
    m->stop();
  }
  update_buttons(m);
  display_entry(m_treewidget->currentItem(), m);
}


// Refetch from the database the status and progress of running mailings
void
mailing_window::refresh()
{
  mailings list;
  mailing_db* msel = selected_item();
  if (list.load()) {
    // foreach mailing, check if changes have occurred
    mailings::iterator iter = list.begin();
    for (; iter != list.end(); ++iter) {
      DBG_PRINTF(4, "mailing_id=%d in the list", iter->id());
      mailing_db* m = get_mailing(iter->id());
      if (!m) {
	// add new entry
	m_mailings.append(*iter); // a new mailing has appeared
	add_item(&(*iter));
      }
      else if (m->status()!=iter->status() || m->m_nb_sent!=iter->m_nb_sent) {
	// redisplay entry
	mailing_window_widget_item* item = get_item(iter->id());
	if (item)
	  display_entry(item, &(*iter));
	*m = *iter;
      }
      if (msel!=NULL && msel==m) {
	update_buttons(m);
      }
    }
  }
}

void
mailing_window::view_mailing()
{
  mailing_db* m = selected_item();
  if (!m) return;
  mailing_viewer* w = new mailing_viewer;
  w->setAttribute(Qt::WA_DeleteOnClose);
  w->show_merge_existing(m);
}

void
mailing_window::new_mailing()
{
  mailing_wizard* mw = new mailing_wizard();
  connect(mw, SIGNAL(accepted()), this, SLOT(new_mailing_started()));
  mw->show();
}

void
mailing_window::new_mailing_started()
{
  show();
  activateWindow();
  raise();
  refresh();
}

#if 0
void
mailing_window::clone_mailing()
{
  mailing_db* m = selected_item();
  if (!m) return;
  mailing_wizard* mw = new mailing_wizard();
  connect(mw, SIGNAL(accepted()), this, SLOT(refresh()));
  m->load_definition(&mw->m_options);
  mw->show();
}
#endif

void
mailing_window::delete_mailing()
{
  mailing_db* m = selected_item();
  if (!m)
    return;
  int rep = QMessageBox::question(this, tr("Confirmation"),
				  tr("Please confirm the complete removal of the mailing"),
				  QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

  if (rep==QMessageBox::Cancel) {
      return;
  }
  else {
    if (m->purge()) {
      QTreeWidgetItem* item = m_treewidget->currentItem();
      if (item) {
	int index = m_treewidget->indexOfTopLevelItem(item);
	m_treewidget->takeTopLevelItem(index);
      }
    }
  }
  
}

bool
mailing_window_widget_item::operator<(const QTreeWidgetItem& other) const
{
  int column = treeWidget()->header()->sortIndicatorSection();
  if (column==index_column_date) {
    QVariant v1 = data(column, mailing_date_role);
    QVariant v2 = other.data(column, mailing_date_role);
    return v1.value<date>() < v2.value<date>();
  }
  else
    return QTreeWidgetItem::operator<(other);
}
