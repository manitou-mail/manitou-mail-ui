/* Copyright (C) 2004-2010 Daniel Verite

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
    }
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
users_repository::name (int id)
{
  std::map<int,QString>::const_iterator i;
  i = m_users_map.find(id);
  if (i!=m_users_map.end())
    return i->second;
  else
    return QString();
}
