/* Copyright (C) 2004-2016 Daniel Verite

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
#include "app_config.h"
#include "identities.h"
#include "db.h"
#include "sqlstream.h"

mail_identity::mail_identity() : m_identity_id(0),
				 m_root_tag_id(0),
				 m_is_restricted(false),
				 m_dirty(false),
				 m_fetched(false)
{
}

mail_identity::~mail_identity()
{
}

bool
mail_identity::compare_fields(const mail_identity& other)
{
  return (
    this->m_email_addr == other.m_email_addr &&
    this->m_name == other.m_name &&
    this->m_signature == other.m_signature &&
    this->m_root_tag_id == other.m_root_tag_id &&
    this->m_is_restricted == other.m_is_restricted &&
    this->m_xface == other.m_xface);
}

bool
identities::fetch(bool force/*=false*/)
{
  if (!force && m_fetched)
    return true;

  db_cnx db;
  const QString default_email = get_config().get_string("default_identity");
  try {
    sql_stream s("SELECT identity_id, email_addr,username,xface,signature,root_tag,restricted FROM identities ORDER BY email_addr", db);
    while (!s.eos()) {
      mail_identity id;
      s >> id.m_identity_id;
      s >> id.m_email_addr >> id.m_name >> id.m_xface >> id.m_signature
	>> id.m_root_tag_id >> id.m_is_restricted;
      if (id.m_email_addr.isEmpty()) {
	// Ignore identities with no email to be safe. This shouldn't happen.
	continue;
      }
      id.m_fetched=true;
      id.m_orig_email_addr = id.m_email_addr;
      id.m_is_default = (id.m_email_addr == default_email);	
      (*this)[id.m_email_addr]=id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    clear();
    return false;
  }
  return true;
}


mail_identity*
identities::get_by_id(int id)
{
  identities::iterator iter = begin();
  for (iter=begin(); iter!=end(); ++iter) {
    if (iter->second.m_identity_id == id)
      return &(iter->second);
  }
  return NULL;
}

bool
mail_identity::update_db()
{
  db_cnx db;
  try {
    sql_stream ss("SELECT 1 FROM identities WHERE email_addr=:p1", db);
    ss << m_orig_email_addr;
    if (!ss.eos()) {
      sql_stream su("UPDATE identities SET email_addr=:p1, username=:p2, xface=:p3, signature=:p4, restricted=:r, root_tag=nullif(:t,0) WHERE email_addr=:p5", db);
      su << m_email_addr << m_name << m_xface << m_signature << m_is_restricted << m_root_tag_id;
      su << m_orig_email_addr;
    }
    else {
      sql_stream si("INSERT INTO identities(email_addr,username,xface,signature,restricted,root_tag) VALUES (:p1,:p2,:p3,:p4,:p5,nullif(:p6,0))", db);
      si << m_email_addr << m_name << m_xface << m_signature << m_is_restricted << m_root_tag_id;
    }
    m_orig_email_addr = m_email_addr;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

//static
QString
identities::email_from_id(int id)
{
  db_cnx db;
  try {
    QString res;
    sql_stream s("SELECT email_addr FROM identities WHERE identity_id=:p1", db);
    s << id;
    if (!s.eos()) {
      s >> res;
      return res;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
  return QString();
}
