/* Copyright (C) 2004-2025 Daniel Verite

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

#include "date.h"
#include "db.h"
#include "main.h"
#include "threads_dialog.h"
#include <list>
#include <set>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <QDebug>

void
open_threads_actions_list()
{
  threads_dialog* window = new threads_dialog();
  window->show();
}

threads_dialog::threads_dialog(QWidget* parent) : QWidget(parent)
{
  setWindowTitle(tr("Actions on threads"));
  resize(500,300);
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  m_listw = new QTreeWidget();
  top_layout->addWidget(m_listw);

  fetch(m_threads_list);

  QStringList labels;
  labels << tr("Subject") << tr("Action") << tr("Created") << tr("Last message");
  m_listw->setHeaderLabels(labels);
  m_listw->header()->resizeSection(0, 300);

  m_listw->header()->setSortIndicatorShown(true);
  m_listw->sortByColumn(2, Qt::AscendingOrder);
  m_listw->setSortingEnabled(true);

  m_listw->setRootIsDecorated(false);
  m_listw->setAllColumnsShowFocus(true);

  m_listw->setSelectionMode(QTreeView::ExtendedSelection);
  m_listw->setSelectionBehavior(QTreeView::SelectRows);

  // fill
  for (const auto &entry : m_threads_list) {
    thread_lvitem* item = new thread_lvitem(m_listw);

    item->setText(0, entry.subject);
    QVariant v;
    v.setValue(entry);
    item->setData(0, Qt::UserRole, v);

    QString action_text;
    if (entry.action == mail_thread::action_auto_archive)
      action_text = tr("Archive");
    else if (entry.action == mail_thread::action_auto_trash)
      action_text = tr("Trash");
    item->setText(1, action_text);

    item->setText(2, entry.date_insert.Output());
    item->setData(2, Qt::UserRole, entry.date_insert);

    item->setText(3, entry.date_last_message.Output());
    item->setData(3, Qt::UserRole, entry.date_last_message);
  }

  QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok |
						  QDialogButtonBox::Help);
  m_del_btn = new QPushButton(tr("Delete"));
  m_del_btn->setEnabled(false);
  btn_box->addButton(m_del_btn, QDialogButtonBox::ActionRole);
  top_layout->addWidget(btn_box);

  connect(btn_box, SIGNAL(helpRequested()), this, SLOT(help()));
  connect(btn_box, SIGNAL(accepted()), this, SLOT(ok()));
  connect(btn_box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(btn_clicked(QAbstractButton*)));

  connect(m_listw, SIGNAL(itemSelectionChanged()),
	  this, SLOT(selection_changed()));
}

threads_dialog::~threads_dialog()
{
}

void
threads_dialog::selection_changed()
{
  m_del_btn->setEnabled(!m_listw->selectedItems().empty());
}

void
threads_dialog::btn_clicked(QAbstractButton* btn)
{
  if (btn == m_del_btn) {
    std::set<mail_thread> to_remove;
    std::list<QTreeWidgetItem*> items_to_remove;

    // build the list of thread actions to remove
    QModelIndexList list = m_listw->selectionModel()->selectedIndexes();
    for (int i=0; i < list.size(); i++) {
      const QModelIndex idx = list.at(i);
      if (idx.column()!=0)	// we only want one item per row
	continue;
      QVariant v = idx.data(Qt::UserRole);	// selected mail_thread
      if (v.isValid()) {
	mail_thread t = v.value<mail_thread>();
	to_remove.insert(t);
	items_to_remove.push_back(m_listw->topLevelItem(idx.row()));
      }
    }

    if (mail_thread::remove_auto_actions(to_remove)) { // remove from db
      for (auto ptr_item : items_to_remove) {
	// remove from list
	if (ptr_item)
	  delete ptr_item;
      }
    }
  }
}

void
threads_dialog::help()
{
}

void
threads_dialog::ok()
{
  this->close();
}

bool
threads_dialog::fetch(std::list<mail_thread>& list)
{
  db_cnx db;
  try {
    /* Fetch thread_action information and subjects of threads. Union
       future threads (no thread_id yet) with threads having already
       several messages */
    sql_stream s("SELECT m.subject, ta.thread_id, ta.mail_id, ta.action_type,"
		 "   to_char(ta.date_insert,'YYYYMMDDHH24MISS'),"
		 "   to_char(m.msg_date,'YYYYMMDDHH24MISS')"
		 " FROM thread_action ta JOIN mail m ON (ta.mail_id=m.mail_id)"
		 " UNION"
		 " SELECT"
		 "   subject,"
		 "   s.thread_id,"
		 "   mail.mail_id,"
		 "   s.action_type,"
		 "   to_char(s.date_insert,'YYYYMMDDHH24MISS'),"
		 "   to_char(mail.msg_date,'YYYYMMDDHH24MISS')"
		 "  FROM mail JOIN ("
		 "     select ta.thread_id,"
		 "	max(ta.date_insert) as date_insert,"
		 "	max(action_type) as action_type,"
		 "        max(m.mail_id) as mid"
		 "     FROM thread_action ta JOIN mail m ON (ta.thread_id=m.thread_id)"
		 "     GROUP BY ta.thread_id) s"
		 "   ON (mail.mail_id=s.mid)"
		 , db);

    while (!s.eos()) {
      mail_thread t;
      int action_code;
      QString sdate, mdate;
      s >> t.subject >> t.thread_id >> t.mail_id >> action_code >> sdate >> mdate;
      t.action = (enum mail_thread::mail_thread_action) action_code;
      t.date_insert = date(sdate);
      t.date_last_message = date(mdate);

      /* remove the Re: prefix from subjects */
      if (t.subject.startsWith("Re:", Qt::CaseInsensitive))
	t.subject = t.subject.mid(3).trimmed();

      list.push_back(t);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    list.clear();
    return false;
  }
  return true;
}

bool
threads_dialog_delete_action(mail_thread db_thread)
{
  std::set<mail_thread> t;
  t.insert(db_thread);
  return mail_thread::remove_auto_actions(t, NULL);
}

thread_lvitem::thread_lvitem(QTreeWidget* parent) : QTreeWidgetItem(parent)
{
}

thread_lvitem::~thread_lvitem()
{
}

bool
thread_lvitem::operator<(const QTreeWidgetItem& other) const
{
  int column = treeWidget()->sortColumn();

  switch(column) {
  case 0:			// subject
    return text(0).toLower() < other.text(0).toLower();
    break;
  case 2:			// date of action
    return data(2, Qt::UserRole).value<date>()
			       < other.data(2, Qt::UserRole).value<date>();
  case 3:			// date of last message
    return data(3, Qt::UserRole).value<date>()
			       < other.data(3, Qt::UserRole).value<date>();
    break;
  }
  return false;
}
