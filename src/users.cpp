/* Copyright (C) 2004-2014 Daniel Verite

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

#include "users.h"
#include "db.h"
#include "sqlstream.h"

/* cached value of USERS.user_id for the user connected to the database
   -1 => user unknown yet
   0 => unable to find it in USERS
   >0 => real user_id found by matching current_user WITH users.login
*/
//static
int
user::m_current_user_id=-1;

//static
std::map<int,QString> users_repository::m_users_map;
int users_repository::m_users_map_fetched;

//static
int
user::current_user_id()
{
  if (m_current_user_id<0) {
    db_cnx db;
    try {
      sql_stream s("SELECT user_id FROM users WHERE login=current_user", db);
      if (!s.eos())
	s >> m_current_user_id;
      else
	m_current_user_id=0;
    }
    catch(db_excpt& p) {
      DBEXCPT (p);
      // if we're unable to get the user_id because of a db error,
      // we won't try again next time
      m_current_user_id=0;
    }
  }
  return m_current_user_id;
}

/*
  Test if the current user has an entry in the USERS table,
  and create one if it's not the case
*/
//static
bool
user::create_if_missing(const QString fullname)
{
  db_cnx db;
  try {
    sql_stream s("SELECT user_id FROM users WHERE login=current_user", db);
    if (s.eos()) {
      sql_stream si("INSERT INTO users(user_id,login,fullname) SELECT 1+coalesce(max(user_id),0), current_user, :p1 FROM users", db);
      if (!fullname.isEmpty())
	si << fullname;
      else
	si << sql_null();
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
  return true;
}

bool
user::fetch(int user_id)
{
  db_cnx db;
  try {
    sql_stream s("SELECT fullname,login,email,custom_field1,custom_field2,custom_field3 FROM users WHERE user_id=:p1", db);
    s << user_id;
    if (!s.eos()) {
      s >> m_fullname >> m_login >> m_email >> m_custom_field1 >> m_custom_field2 >> m_custom_field3;
      m_user_id = user_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
  return true;
}

bool
user::update_fields()
{
  db_cnx db;
  try {
    sql_stream s("UPDATE users SET fullname=:p1,login=:p2,email=:p3,custom_field1=:p4,custom_field2=:p5,custom_field3=:p6 WHERE user_id=:id", db);
    QString* fields[] = {&m_fullname, &m_login, &m_email, &m_custom_field1, &m_custom_field2, &m_custom_field3 };
    for (uint i=0; i<sizeof(fields)/sizeof(fields[0]); i++) {
      if (fields[i]->isEmpty())
	s << sql_null();
      else
	s << *fields[i];
    }
    s << m_user_id;
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
  return true;
}

//static
QString
user::name(int user_id)
{
  QString name;
  db_cnx db;
  try {
    sql_stream s("SELECT fullname FROM users WHERE user_id=:p1", db);
    s << user_id;
    if (!s.eos()) {
      s >> name;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
  return name;
}

/* Fill in the cache */
void
users_repository::fetch()
{
  db_cnx db;
  try {
    sql_stream s("SELECT user_id,fullname FROM users", db);
    while (!s.eos()) {
      int id;
      QString qname;
      s >> id >> qname;
      m_users_map[id] = qname;
    }
    m_users_map_fetched = 1;
  }
  catch(db_excpt& p) {
    m_users_map.clear();
    m_users_map_fetched = 0;
    DBEXCPT(p);
  }
}

/* Empty the cache */
void
users_repository::reset()
{
  m_users_map.clear();
  m_users_map_fetched = 0;
}

QString
users_repository::name(int id)
{
  std::map<int,QString>::const_iterator i;
  i = m_users_map.find(id);
  if (i!=m_users_map.end())
    return i->second;
  else
    return QString();
}

QList<user>
users_repository::get_list()
{
  QList<user> list;
  db_cnx db;
  try {
    sql_stream s("SELECT coalesce(u.user_id,0),fullname,pu.usename,login,email,custom_field1,custom_field2,custom_field3 FROM users u FULL JOIN pg_user pu ON (pu.usename=u.login) ORDER BY login", db);
    while (!s.eos()) {
      user u;
      s >> u.m_user_id >> u.m_fullname >> u.m_db_login >> u.m_login >> u.m_email >> u.m_custom_field1 >> u.m_custom_field2 >> u.m_custom_field3;
      list.append(u);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
  return list;
}

bool
user::create_db_user(const QString login, const QString password)
{
  db_cnx db;

  QString qlogin = db.escape_identifier(login);
  QString qpassword = db.escape_string_literal(password);
  QString query = QString("CREATE USER \"%1\" PASSWORD '%2'").arg(qlogin).arg(qpassword);

  try {
    sql_stream s(query, db);
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
user::check_db_role(const QString login, bool case_sensitive)
{
  db_cnx db;
  try {
    const char* query;
    if (case_sensitive) {
      query = "SELECT 1 FROM pg_roles WHERE rolname=:p1";
    }
    else {
      query = "SELECT 1 FROM pg_roles WHERE lower(rolname)=lower(:p1)";
    }
    sql_stream s("SELECT 1 FROM pg_roles WHERE rolname=:p1", db);
    s << login;
    if (!s.eos()) {
      return true;
    }
    return false;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
}
