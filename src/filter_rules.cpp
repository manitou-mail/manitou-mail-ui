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

#include "filter_rules.h"
#include "db.h"
#include "sqlstream.h"
#include "users.h"
#include "tags.h"

filter_expr::filter_expr() : m_dirty(false), m_delete(false), m_new(false)
{
  m_expr_id=0;
  m_apply_order=0;
  m_direction='I';
}

filter_expr::~filter_expr()
{
}

filter_action::filter_action()
{
  m_action_order=0;
}

filter_action::~filter_action()
{
}

QString
filter_action::ui_text() const
{
  QString s;
  if (m_str_atype=="tag") {
    s = QObject::tr("Assign the tag: %1").arg(m_action_arg);
  }
  else if (m_str_atype=="redirect") {
    s = QObject::tr("Redirect message to: %1").arg(m_action_arg);
  }
  else if (m_str_atype=="status") {
    QStringList display_status;
    QStringList l=m_action_arg.split('+', QString::KeepEmptyParts);
    for (QStringList::Iterator it=l.begin(); it!=l.end(); ++it) {
      char c=(*it).at(0).toAscii();
      switch(c) {
      case 'R':
	display_status.append(QObject::tr("Read"));
	break;
      case 'T':
	display_status.append(QObject::tr("Trashed")); // obsolete
	break;
      case 'A':
	display_status.append(QObject::tr("Archived"));
	break;
      case 'F':
	display_status.append(QObject::tr("Forwarded"));
	break;
      case 'D':
	display_status.append(QObject::tr("Deleted")); // obsolete
	break;
      }
    }
    s = QObject::tr("Set status to %1").arg(display_status.join(","));
  }
  else if (m_str_atype=="priority") {
    if (m_action_arg.at(0)=='=') {
      s = QObject::tr("Set priority to %1").arg(m_action_arg.mid(1).toInt());
    }
    else if (m_action_arg.mid(0,2)=="+=") {
      int p=m_action_arg.mid(2).toInt();
      if (p>=0)
	s = QObject::tr("Add %1 to priority").arg(p);
      else
	s = QObject::tr("Subtract %1 from priority").arg(-p);
    }
  }
  else if (m_str_atype=="discard") {
    if (m_action_arg=="trash")
      s = QObject::tr("Move to the trash can");
    else
      s = QObject::tr("Delete message");
  }
  else if (m_str_atype=="stop") {
    s = QObject::tr("Stop filters");
  }
  else if (m_str_atype=="set header") {
    s = QObject::tr("Set header: unable to decode action");
    QRegExp rx("^([^:]+):(set|prepend|append) (.*)");
    int pos = rx.indexIn(m_action_arg);
    if (pos>-1) {
      QString op=rx.cap(2);
      if (op=="set")
	s = QObject::tr("Set header field %1 to %2").arg(rx.cap(1)).arg(rx.cap(3));
      else if (op=="prepend")
	s = QObject::tr("Prepend \"%2\" to header field %1").arg(rx.cap(1)).arg(rx.cap(3));
      else if (op=="append")
	s = QObject::tr("Append \"%2\" to header field %1").arg(rx.cap(1)).arg(rx.cap(3));
      
    }
    else {
      QStringList list = m_action_arg.split(':');
      if (list.size()==2) {
	s = QObject::tr("Set header field %1 to %2").arg(list.at(0)).arg(list.at(1));
      }
    }
  }
  else if (m_str_atype=="remove header") {
    s = QObject::tr("Remove header field %1").arg(m_action_arg);
  }
  else if (m_str_atype=="set identity") {
    s = QObject::tr("Set identity to %1").arg(m_action_arg);
  }
  return s;
}

void
filter_action::set_action_string_from_db(const QString arg)
{
  if (m_str_atype=="tag") {
    m_action_arg = message_tag::convert_separator_from_ascii(arg);
  }
  else
    m_action_arg = arg;
}

QString
filter_action::action_string_to_db()
{
  if (m_str_atype=="tag") {
    return message_tag::convert_separator_to_ascii(m_action_arg);
  }
  else
    return m_action_arg;
}

expr_list::expr_list()
{
}

expr_list::~expr_list()
{
}

bool
expr_list::fetch()
{
  bool result=true;
  db_cnx db;
  try {
    filter_expr* current=NULL;
    QString action_type, action_args, tmp, sdate;
    int expr_id;
    sql_stream s("SELECT fe.expr_id, fe.name, fe.expression, fe.direction, fe.apply_order, to_char(fe.last_hit, 'YYYYMMDDHH24MISS'), action_type,action_arg FROM filter_action fa RIGHT JOIN filter_expr fe ON fe.expr_id=fa.expr_id ORDER BY fe.apply_order,action_order", db);
    while (!s.eos()) {
      s >> expr_id;
      if (!current || current->m_expr_id != expr_id) {
	// new expr
	filter_expr e;
	e.m_expr_id = expr_id;
	push_back(e);
	current = &(back());
	s >> current->m_expr_name >> current->m_expr_text >> current->m_direction >> current->m_apply_order >> sdate;
	current->m_last_hit = date(sdate);
      }
      else {
	char c;
	float order;
	s >> tmp >> tmp >> c >> order >> tmp;
      }
      s >> action_type >> action_args;
      if (!action_type.isEmpty()) {
	filter_action a;
	a.m_str_atype = action_type;
	a.set_action_string_from_db(action_args);
	current->m_actions.push_back(a);
      }
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}


bool
expr_list::needs_save()
{
  std::list<filter_expr>::iterator it = begin();
  for (; it != end(); ++it) {
    if (!it->m_expr_id && !it->m_delete)
      return true;
    if (it->m_delete || it->m_dirty)
      return true;
  }
  return false;
}

bool
expr_list::update_db()
{
  bool result=true;
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s_upd("UPDATE filter_expr SET name=:name,expression=:expr, user_lastmod=:user, last_update=now(),direction=':d',apply_order=:a WHERE expr_id=:id", db);
    sql_stream s_del_expr("DELETE FROM filter_expr WHERE expr_id=:id", db);
    sql_stream s_ins("INSERT INTO filter_expr(expr_id,name,expression,direction,apply_order) VALUES (:id,:name,:expr,':dir',:a)", db);
    // TODO: use a sequence
    sql_stream s_seq("SELECT nextval('seq_filter_expr_id')", db, false);
    sql_stream s_del_act("DELETE FROM filter_action WHERE expr_id=:p1", db);
    sql_stream s_add_act("INSERT INTO filter_action(expr_id,action_arg,action_type,action_order) VALUES(:p1,:p2,:p3,:p4)", db);

    std::list<filter_expr>::iterator itd = begin();
    for (; itd != end(); ++itd) {
      if (itd->m_expr_id>0 && itd->m_delete) {
	s_del_act << itd->m_expr_id;
	s_del_expr << itd->m_expr_id;
      }
    }

    std::list<filter_expr>::iterator it = begin();
    for (; it != end(); ++it) {
      unsigned int db_expr_id=it->m_expr_id;
      if (!db_expr_id) {
	if (!it->m_delete) {
	  s_seq.execute();
	  s_seq >> db_expr_id;
	  s_ins << db_expr_id;
	  if (it->m_expr_name.isEmpty())
	    s_ins << sql_null();
	  else
	    s_ins << it->m_expr_name;
	  s_ins << it->m_expr_text << it->m_direction << it->m_apply_order;
	}
 	// else ignore the entry: it has been created then deleted.
      }
      if (it->m_dirty && !it->m_delete) {
	if (it->m_expr_name.isEmpty())
	  s_upd << sql_null();
	else
	  s_upd << it->m_expr_name;
	s_upd << it->m_expr_text << user::current_user_id() << it->m_direction << it->m_apply_order << db_expr_id;
	// on update, delete all old actions and insert the new ones below
	s_del_act << db_expr_id;
      }
      // insert actions
      if ((it->m_dirty || it->m_expr_id==0) && !it->m_delete) {
	int aorder=1;
	QList<filter_action>::iterator ait;
	for (ait=it->m_actions.begin(); ait!=it->m_actions.end(); ++ait) {
	  s_add_act << db_expr_id << ait->action_string_to_db();
	  s_add_act << ait->m_str_atype << aorder++;
	}
      }
    } // for each expr
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  return result;
}

const filter_expr*
expr_list::find_name(const QString& name) const
{
  std::list<filter_expr>::const_iterator it = begin();
  for (; it != end(); ++it) {
    if ((*it).m_expr_name == name)
      return &(*it);
  }
  return NULL;
}

float
expr_list::max_apply_order() const
{
  float m=0;
  std::list<filter_expr>::const_iterator it = begin();
  for (; it != end(); ++it) {
    if ((*it).m_apply_order > m)
      m=(*it).m_apply_order;
  }
  return m;
}
