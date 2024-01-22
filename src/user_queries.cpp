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

#include "user_queries.h"
#include "db.h"
#include "sqlstream.h"
#include "selectmail.h"

#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>


//static
std::map<QString,QString>
user_queries_repository::m_map;

//static
bool user_queries_repository::m_uq_map_fetched = false;

// title is the user's choosen name for the query
void
save_filter_query(msgs_filter* filter, int mode, const QString title)
{
  save_query_box* w = new save_query_box(0, filter, mode, title);
  QString initial_title = title;

  int r=w->exec();
  if (r==QDialog::Accepted) {
    QString query;
    db_cnx db;
    try {
      if (!initial_title.isEmpty() && initial_title != w->m_name->text()) {
	// rename the title and replace the SQL sentence
	sql_stream s("UPDATE user_queries SET title=:p1,sql_stmt=:p2 WHERE title=:p3", db);
	s << w->m_name->text() << w->m_sql << initial_title;
	// remove entry, to add it with the new title (at the end of the outer block)
	user_queries_repository::m_map.erase(initial_title);
      }
      else if (user_queries_repository::name_exists(w->m_name->text())) {
	// replace (has been confirmed in the dialog)
	sql_stream s("UPDATE user_queries SET sql_stmt=:p1 WHERE title=:p2", db);
	s << w->m_sql << w->m_name->text();
      }
      else {
	// insert new
	sql_stream s("INSERT INTO user_queries(title, sql_stmt) VALUES (:p1,:p2)", db);
	s << w->m_name->text() << w->m_sql;
      }
      user_queries_repository::m_map[w->m_name->text()] = w->m_sql;
    }
    catch (db_excpt p) {
      DBEXCPT(p);
    }
  }
  delete w;
}

/*
  mode=0: new query, no title suggested and sql is read-only
  mode=1: edit query, title and sql can be updated
*/
save_query_box::save_query_box(QWidget* parent, msgs_filter* filter, int mode,
			       const QString title) :
  QDialog(parent)
{
  setWindowTitle(tr("User query"));
  QVBoxLayout* top_layout = new QVBoxLayout(this);

  if (filter)
    m_sql = filter->user_query();
  m_initial_title = title;

  QLabel* lq = new QLabel(tr("SQL statement:"), this);
  top_layout->addWidget(lq);

  m_sql_edit = new QTextEdit(this);
  top_layout->addWidget(m_sql_edit);
  m_sql_edit->setPlainText(m_sql);
  if (mode==0)
    m_sql_edit->setReadOnly(true);

  QLabel* l = new QLabel(tr("Enter a short description (title) for the query:"), this);
  top_layout->addWidget(l);
  m_name = new QLineEdit(this);
  m_name->setText(title);
  top_layout->addWidget(m_name);

  QHBoxLayout* hbl = new QHBoxLayout();
  top_layout->addLayout(hbl);
  hbl->addStretch(5);
  QPushButton* ok = new QPushButton(tr("Save"), this);
  connect(ok, SIGNAL(clicked()), SLOT(check_accept()));
  ok->setDefault(true);
  hbl->addWidget(ok);
  hbl->addStretch(5);

  QPushButton* cancel = new QPushButton(tr("Cancel"), this);
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));
  hbl->addWidget(cancel);
  hbl->addStretch(5);
}

save_query_box::~save_query_box()
{
}

/* Check for input correctness and accept() */
void
save_query_box::check_accept()
{
  QString title = m_name->text();
  if (title.isEmpty()) {
    QMessageBox::critical(NULL, tr("Input error"), tr("Please give a name to the query"));
    return;
  }

  if (title!=m_initial_title && user_queries_repository::name_exists(title)) {
    int rep=QMessageBox::question(NULL, tr("Confirmation"), 
				  tr("A query with this name already exists.\n"
				     "Are you sure you want to overwrite it?"),
				  QMessageBox::Yes, QMessageBox::No);
    if (rep==QMessageBox::No)
      return;
  }
  m_sql = m_sql_edit->toPlainText();
  accept();
}

/* Fill in the cache */
bool
user_queries_repository::fetch()
{
  bool result=true;
  if (!m_uq_map_fetched) {
    db_cnx db;
    try {
      m_map.clear();
      sql_stream s("SELECT title,sql_stmt FROM user_queries", db);
      while (!s.eos()) {
	QString ttl;
	QString sql;
	s >> ttl >> sql;
	m_map[ttl] = sql;
      }
      m_uq_map_fetched=true;
    }
    catch(db_excpt& p) {
      m_map.clear();
      m_uq_map_fetched=false;
      DBEXCPT(p);
      result=false;
    }
  }
  return result;
}

// static
void
user_queries_repository::reset()
{
  m_map.clear();
  m_uq_map_fetched=false;
}

/* Returns true if 'name' is already used in the user_queries table */
bool
user_queries_repository::name_exists(const QString name)
{
  fetch();
  std::map<QString,QString>::const_iterator iter = m_map.find(name);
  return (iter!=m_map.end());
}

bool
user_queries_repository::remove_query(const QString name)
{
  db_cnx db;
  try {
    sql_stream s("DELETE FROM user_queries WHERE title=:p1", db);
    s << name;
    user_queries_repository::m_map.erase(name);
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}
