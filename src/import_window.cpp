/* Copyright (C) 2004-2024 Daniel Verite

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
#include "import_window.h"
#include "mbox_import_wizard.h"
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

import_window*
import_window::m_current_instance; // singleton

void
open_import_window()
{
  import_window* w = import_window::open_unique();
  w->show();
  w->activateWindow();
  w->raise();
}

import_window*
import_window::open_unique()
{
  if (!m_current_instance)
    m_current_instance = new import_window();
  return m_current_instance;
}

import_window::import_window()
{
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowIcon(UI_ICON(ICON16_IMPORT_MBOX));
  setWindowTitle(tr("Mail import"));

  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setMargin(5);

  QTreeWidget* w = new QTreeWidget();
  m_treewidget = w;
  top_layout->addWidget(w);

  QStringList labels;
  labels << tr("File") << tr("Status") << tr("Completion");
  w->setHeaderLabels(labels);
  w->header()->resizeSection(0, 200);
  w->setRootIsDecorated(false);
  w->setAllColumnsShowFocus(true);

  QHBoxLayout* buttons = new QHBoxLayout();
  top_layout->addLayout(buttons);

  m_btn_stop_restart = new QPushButton(tr("Stop"));
  buttons->addWidget(m_btn_stop_restart);
  connect(m_btn_stop_restart, SIGNAL(clicked()), this, SLOT(start_stop()));

  m_btn_purge = new QPushButton(tr("Purge"));
  buttons->addWidget(m_btn_purge);
  connect(m_btn_purge, SIGNAL(clicked()), this, SLOT(purge_import()));

  buttons->addStretch(5);

  m_btn_refresh = new QPushButton(tr("Refresh"));
  buttons->addWidget(m_btn_refresh);
  connect(m_btn_refresh, SIGNAL(clicked()), this, SLOT(refresh()));

  m_btn_new = new QPushButton(tr("New..."));
  buttons->addWidget(m_btn_new);
  connect(m_btn_new, SIGNAL(clicked()), this, SLOT(new_import()));

  connect(w, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	  this, SLOT(item_changed(QTreeWidgetItem*,QTreeWidgetItem*)));

#if 0
  w->setSortingEnabled(true);
  w->sortByColumn(import_window_widget_item::index_column_date, Qt::DescendingOrder);
#endif

  load();

  m_timer = new QTimer(this);
  m_timer->start(5000);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
  resize(600,300);
}

import_window::~import_window()
{
  m_current_instance=NULL;
}

void
import_window::item_changed(QTreeWidgetItem* item, QTreeWidgetItem* old)
{
  Q_UNUSED(old);
  if (item) {
    import_mbox* m = get_import_mbox(item->data(0, import_window_widget_item::import_mbox_id_role).toInt());
    update_buttons(m); // m may be NULL
  }
  else {
    update_buttons(NULL);
  }
}

void
import_window::update_buttons(import_mbox* m)
{
  if (m==NULL) {
    m_btn_purge->setEnabled(false);
    m_btn_stop_restart->setEnabled(false);
  }
  else {
    if (m->status()==import_mbox::status_not_started ||
	m->status()==import_mbox::status_stopped) {
      m_btn_stop_restart->setText(tr("Start"));
      m_btn_stop_restart->setEnabled(true);
    }
    else if (m->status()==import_mbox::status_running) {
      m_btn_stop_restart->setText(tr("Stop"));
      m_btn_stop_restart->setEnabled(true);
    }
    else if (m->status()==import_mbox::status_finished) {
      m_btn_stop_restart->setText(tr("Stop"));
      m_btn_stop_restart->setEnabled(false);
    }
    m_btn_purge->setEnabled(true);
  }
}

import_mbox*
import_window::get_import_mbox(int import_mbox_id)
{
  import_mbox_list::iterator iter = m_list.begin();
  for (; iter != m_list.end(); ++iter) {
    if (iter->id() == import_mbox_id)
      return &(*iter);
  }
  return NULL;
}

// retrieve the item in the treeview by the import_mbox_id
import_window_widget_item*
import_window::get_item(int import_mbox_id)
{
  QTreeWidgetItemIterator it(m_treewidget);
  while (*it) {
    if ((*it)->data(0,import_window_widget_item::import_mbox_id_role).toInt() == import_mbox_id)
      return static_cast<import_window_widget_item*>(*it);
    ++it;
  }
  return NULL;
}

import_mbox*
import_window::selected_item()
{
  QTreeWidgetItem* item = m_treewidget->currentItem();
  return item ? get_import_mbox(item->data(0, import_window_widget_item::import_mbox_id_role).toInt()) : NULL;  
}

// Populate the list with contents from the database
void
import_window::load()
{
  m_list.load();
  import_mbox_list::const_iterator iter = m_list.constBegin();
  for (; iter != m_list.constEnd(); ++iter) {
    add_item(&(*iter));
  }
}

import_window_widget_item*
import_window::add_item(const import_mbox* m)
{
  import_window_widget_item* item = new import_window_widget_item();
  QProgressBar* bar = new QProgressBar();
  m_progress_bars[m->id()] = bar;
  bar->setRange(0, 100);
  m_treewidget->addTopLevelItem(item);
  m_treewidget->setItemWidget(item, 2, bar);
  item->setData(0, import_window_widget_item::import_mbox_id_role, m->id());
#if 0
  QVariant vdate;
  vdate.setValue(m->creation_date());
  item->setData(import_window_widget_item::index_column_date,
		import_window_widget_item::import_mbox_date_role, vdate);
#endif
  display_entry(item, m);
  return item;
}

void
import_window::display_entry(QTreeWidgetItem* item, const import_mbox* m)
{
  if (!item)
    return;
  int col=0;
  item->setText(col++, m->filename());
//  item->setText(col++, m->creation_date().output_8());
  item->setText(col++, m->status_text());
  QProgressBar* bar = m_progress_bars[m->id()];
  if (bar) {
    bar->setValue((int)(m->m_completion*100));
    QString color="#1e8bd8"; // blue
    switch(m->status()) {
    case import_mbox::status_running:
      color="green";
      break;
    case import_mbox::status_stopped:
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
import_window::start_stop()
{
  import_mbox* m = selected_item();
  if (!m) return;
  if (m->status()==import_mbox::status_not_started || m->status()==import_mbox::status_stopped ) {
    m->instantiate_job();
  }
  else if (m->status()==import_mbox::status_running) {
    m->stop();
  }
  update_buttons(m);
  display_entry(m_treewidget->currentItem(), m);
}


// Refetch from the database the status and progress of running imports
void
import_window::refresh()
{
  import_mbox_list list;
  if (list.load()) {
    // foreach import_mbox, check if changes have occurred
    import_mbox_list::iterator iter = list.begin();
    for (; iter != list.end(); ++iter) {
      DBG_PRINTF(4, "import_mbox_id=%d in the list", iter->id());
      import_mbox* m = get_import_mbox(iter->id());
      if (!m) {
	// add new entry
	m_list.append(*iter); // a new import_mbox has appeared
	add_item(&(*iter));
      }
      else if (m->status()!=iter->status() || m->m_completion!=iter->m_completion) {
	// redisplay entry
	import_window_widget_item* item = get_item(iter->id());
	if (item)
	  display_entry(item, &(*iter));
	*m = *iter;
      }
    }

    /* foreach element on display, check if it's still in the fresh
       list If not, remove it from the display. We don't use
       QTreeWidgetItemIterator since it's not clear if it supports
       deletion of items during traversal */
    int i=0;
    while (i<m_treewidget->topLevelItemCount()) {
      QTreeWidgetItem* item = m_treewidget->topLevelItem(i);
      if (item) {
	int id=item->data(0,import_window_widget_item::import_mbox_id_role).toInt();
	if (!list.id_exists(id))
	  delete item; // index doesn't move
	else
	  i++;
      }
    }
  }

  update_buttons(selected_item());
}

void
import_window::new_import()
{
  mbox_import_wizard* mw = new mbox_import_wizard();
  connect(mw, SIGNAL(accepted()), this, SLOT(new_import_started()));
  mw->show();
}

void
import_window::new_import_started()
{
  show();
  activateWindow();
  raise();
  refresh();
}

void
import_window::purge_import()
{
  import_mbox* m = selected_item();
  if (!m)
    return;
  int rep = QMessageBox::question(this, tr("Confirm purge"),
				  tr("Please confirm the purge of the import data from the database.\nMessages not yet imported will be lost."),
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

#if 0
bool
import_window_widget_item::operator<(const QTreeWidgetItem& other) const
{
  int column = treeWidget()->header()->sortIndicatorSection();
  if (column==index_column_date) {
    QVariant v1 = data(column, import_mbox_date_role);
    QVariant v2 = other.data(column, import_mbox_date_role);
    return v1.value<date>() < v2.value<date>();
  }
  else
    return QTreeWidgetItem::operator<(other);
}
#endif


bool
import_mbox_list::load()
{
  db_cnx db;
  try {
    sql_stream s("SELECT import_id,completion,status,filename FROM import_mbox ORDER BY import_id DESC", db);
    while (!s.eof()) {
      import_mbox m;
      s >> m.m_id >> m.m_completion >> m.m_status >> m.m_filename;
      append(m);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
import_mbox_list::id_exists(int id) const
{
  import_mbox_list::const_iterator iter = constBegin();
  for (; iter != constEnd(); ++iter) {
    if (id==iter->id())
      return true;
  }
  return false;
}

QString
import_mbox::status_text() const
{
  switch(m_status) {
  case 0: return QObject::tr("Not started");
  case 1: return QObject::tr("Running");
  case 2: return QObject::tr("Stopped");
  case 3: return QObject::tr("Finished");
  }
  return QObject::tr("Unknown");
}

bool
import_mbox::stop()
{
  db_cnx db;
  try {
    sql_stream s("UPDATE import_mbox SET status=2 WHERE import_id=:p1", db);
    s << id();
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  
  return true;
}

bool
import_mbox::purge()
{
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream sd1("DELETE FROM import_message WHERE import_id=:p1", db);
    sd1 << id();
    sql_stream sd("DELETE FROM import_mbox WHERE import_id=:p1", db);
    sd << id();
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }

  return true;
}

bool
import_mbox::instantiate_job()
{
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s1("UPDATE import_mbox SET status=:s WHERE import_id=:id", db);
    s1 << status_running << m_id;
    sql_stream s("INSERT INTO jobs_queue(job_type, job_args) VALUES(:t,:a)", db);
    s << "import_mailbox" << QString("%1").arg(m_id);
    sql_stream sn("NOTIFY job_request", db);
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  return true;    
}
