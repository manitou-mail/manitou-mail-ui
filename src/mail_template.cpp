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

#include "mail_template.h"
#include "db.h"
#include "sqlstream.h"
#include <QBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>

bool
mail_template::load(int id)
{
  m_template_id = id;
  db_cnx db;
  try {
    sql_stream s("SELECT title, body_text,body_html,header FROM mail_template WHERE template_id=:i", db);
    s << id;
    if (!s.eos()) {
      s >> m_title >> m_text_body >> m_html_body >> m_header;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mail_template::insert()
{
  db_cnx db;
  try {
    QString query = "INSERT INTO mail_template(title, body_text, body_html, header)"
      " VALUES (:t1,:t2,:t3,:t4) RETURNING template_id";
    sql_stream s(query, db);
    s << m_title;
    if (!m_text_body.isEmpty())
      s << m_text_body;
    else
      s << sql_null();

    if (!m_html_body.isEmpty())
      s << m_html_body;
    else
      s << sql_null();

    if (!m_header.isEmpty())
      s << m_header;
    else
      s << sql_null();
    
    if (!s.eos()) {
      s >> m_template_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mail_template::update()
{
  db_cnx db;
  try {
    QString query = "UPDATE mail_template SET title=:t1,body_text=:t2,body_html=:t3,header=:t4"
      " WHERE template_id=:id";
    sql_stream s(query, db);
    s << m_title;
    if (!m_text_body.isEmpty())
      s << m_text_body;
    else
      s << sql_null();

    if (!m_html_body.isEmpty())
      s << m_html_body;
    else
      s << sql_null();

    if (!m_header.isEmpty())
      s << m_header;
    else
      s << sql_null();

    s << m_template_id;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

QString
mail_template::html_body()
{
  return m_html_body;
}

QString
mail_template::text_body()
{
  return m_text_body;
}

bool
mail_template_list::fetch_titles()
{
  db_cnx db;
  clear();
  try {
    sql_stream s("SELECT template_id, title FROM mail_template ORDER BY title", db);
    while (!s.eos()) {
      mail_template t;
      s >> t.m_template_id >> t.m_title;
      append(t);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

template_dialog::template_dialog() : QDialog(NULL)
{
  setWindowTitle(tr("Choose a template"));
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(new QLabel(tr("Select a template from the list:")));

  // list of template titles with the template ID as user data
  m_wlist = new QListWidget;
  layout->addWidget(m_wlist);
  mail_template_list mtl;
  mtl.fetch_titles();
  for (int i=0; i<mtl.size(); i++) {
    m_wlist->addItem(mtl.at(i).m_title);
    QListWidgetItem* item = m_wlist->item(i);
    item->setData(Qt::UserRole, QVariant(mtl.at(i).m_template_id));
  }

  connect(m_wlist, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));
  QHBoxLayout* hl = new QHBoxLayout;
  layout->addLayout(hl);
  hl->addStretch(1);
  QPushButton* ok = new QPushButton(tr("OK"));
  hl->addWidget(ok);
  hl->addStretch(1);
  QPushButton* cancel = new QPushButton(tr("Cancel"));
  hl->addWidget(cancel);
  hl->addStretch(1);
  connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

int
template_dialog::selected_template_id()
{
  QListWidgetItem* item = m_wlist->currentItem();
  return (item!=NULL) ? item->data(Qt::UserRole).toInt() : 0;
}
