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

#include "db.h"
#include "main.h"
#include "app_config.h"
#include "icons.h"
#include "msg_list_window.h"
#include <QFile>

const char* app_config::m_default_values[] = {
  "autocheck_newmail", "3",
  "composer/address_check", "basic",
  "composer/format_for_new_mail", "text/plain",
  "composer/format_for_replies", "same_as_sender",
  "display/notifications/new_mail", "system",
  "display/wordsearch/progress_bar", "0",
  "display_threads", "1",
  "fetch_ahead_max_msgs", "10",
  "forward_includes_attachments", "1",
  "max_db_connections", "3",
  "max_msgs_per_selection", "500",
  "messages_order", "oldest_first",
  "msg_window_pages", "5",
  "preferred_display_format", "html",
  "query_dialog/address_autocompleter", "1",
  "sender_displayed_as", "name",
  "show_headers_level", "1",
  "show_tags", "1",
  NULL
};

bool
app_config::init()
{
  db_cnx db;
  try {
    // First, get the entries that are not bound to a specific configuration
    // (they apply to all configurations)
    sql_stream s("SELECT conf_key,value FROM config WHERE conf_name is null", db);
    QString key, value;
    while (!s.eos()) {
      s >> key >> value;
      m_mapconf[key] = value;
    }

    // Then, get the entries that are bound with our current configuration
    // (possibly overwriting some (key,value) pairs fetched just before)
    if (!m_name.isEmpty()) {
      sql_stream s2("SELECT conf_key,value FROM config WHERE conf_name=:p1", db);
      s2 << m_name;
      while (!s2.eos()) {
	s2 >> key >> value;
	m_mapconf[key] = value;
      }
    }

    const char** o=m_default_values;
    while (*o) {
      if (!exists(*o)) {
	m_mapconf[*o] = QString(*(o+1));
      }
      o+=2;
    }
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

// static
bool
app_config::get_all_conf_names(QStringList* l)
{
  db_cnx db;
  try {
    sql_stream s("SELECT distinct conf_name FROM config", db);
    QString confname;
    while (!s.eos()) {
      s >> confname;
      if (!confname.isEmpty())
	l->append(confname);
    }
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

void
app_config::dump()
{
  QString strconf = "<config>\n";
  const_iter iter;
  for (iter=m_mapconf.begin(); iter!=m_mapconf.end(); iter++) {
    strconf.append(QString(" <%1>%2</%1>\n").arg(iter->first).arg(iter->second));
  }
  strconf.append("</config>\n");
  QByteArray arr = strconf.toLocal8Bit();
  DBG_PRINTF(5,"%s\n", arr.constData());
}

/* Save to the database the set of key/values whose name start with
   'key_prefix' (or all keys if 'key_prefix' is null) */
bool
app_config::store(const QString key_prefix /* =QString::null */)
{
  const char* q_insert;
  const char* q_update;
  bool ret=true;
  if (m_name.isEmpty()) {
    q_update="UPDATE config SET value=:p1 WHERE conf_key=:p2 AND conf_name is null";
    q_insert="INSERT INTO config(value,conf_key) VALUES(:p1,:p2)";
  }
  else {
    q_update="UPDATE config SET value=:p1 WHERE conf_key=:p2 AND conf_name=:p3";
    q_insert="INSERT INTO config(value,conf_key,conf_name) VALUES(:p1,:p2,:p3)";
  }

  db_cnx db;
  try {
    //    db.begin_transaction();
    sql_stream su(q_update, db);
    sql_stream si(q_insert, db);
    const_iter iter;
    for (iter=m_mapconf.begin(); iter!=m_mapconf.end(); iter++) {
      if (key_prefix.isEmpty() || iter->first.indexOf(key_prefix)==0) {
	const QString value=iter->second;
	su << value << iter->first;
	if (!m_name.isEmpty())
	  su << m_name;
	// if nothing has been updated (no entry), then insert a new entry
	if (su.affected_rows()==0) {
	  si << value << iter->first;
	  if (!m_name.isEmpty())
	    si << m_name;
	}
      }
    }
    //    db.commit_transaction();
    ret=true;
  }
  catch (db_excpt p) {
    //    db.rollback_transaction();
    DBEXCPT(p);
    ret=false;
  }
  return ret;
}

int
app_config::get_number(const QString conf_key) const
{
  const_iter iter = m_mapconf.find(conf_key);
  if (iter!=m_mapconf.end()) {
    QString s = iter->second;
    return s.toInt();
  }
  else
    return 0;
}

bool
app_config::get_bool(const QString conf_key) const
{
  return (get_number(conf_key)!=0);
}

bool
app_config::get_bool(const QString conf_key, bool default_val) const
{
  const_iter iter = m_mapconf.find(conf_key);
  return iter==m_mapconf.end() ? default_val : (iter->second.toInt()!=0);
}

const QString
app_config::get_string(const QString conf_key) const
{
  static QString empty="";
  const_iter iter=m_mapconf.find(conf_key);
  if (iter!=m_mapconf.end()) {
    return iter->second;
  }
  else {
    //    DBG_PRINTF(5,"conf_key '%s' not found\n", conf_key.latin1());
    return empty;
  }
}

void
app_config::set_bool(const QString conf_key, bool value)
{
  m_mapconf[conf_key] = value ? "1" : "0";
}

void
app_config::set_number(const QString conf_key, int value)
{
  m_mapconf[conf_key]=QString("%1").arg(value);
}

void
app_config::set_string(const QString conf_key, const QString& value)
{
  m_mapconf[conf_key]=value;
}

void
app_config::remove(const QString conf_key)
{
  m_mapconf.erase(conf_key);
}

bool
app_config::exists(const QString conf_key)
{
  const_iter iter=m_mapconf.find(conf_key);
  return iter!=m_mapconf.end();
}

/*
  For each entry in 'newconf':
  If the entry exists in 'this' and is different, then update/insert it
  If the entry doesn't exist in 'this', then update/insert it
*/
bool
app_config::diff_update(const app_config& newconf)
{
  const char* q_insert;
  const char* q_update;
  const char* q_delete;
  if (m_name.isEmpty()) {
    q_update="UPDATE config SET value=:p1,date_update=now() WHERE conf_key=:p2 AND conf_name is null";
    q_insert="INSERT INTO config(value,conf_key,date_update) VALUES(:p1,:p2,now())";
    q_delete="DELETE FROM config WHERE conf_key=:p1 AND conf_name is null";
  }
  else {
    q_update="UPDATE config SET value=:p1,date_update=now() WHERE conf_key=:p2 AND conf_name=:p3";
    q_insert="INSERT INTO config(value,conf_key,conf_name,date_update) VALUES(:p1,:p2,:p3,now())";
    q_delete="DELETE FROM config WHERE conf_key=:p1 AND conf_name=:p2";
  }

  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream su(q_update, db);
    sql_stream si(q_insert, db);
    sql_stream sd(q_delete, db);
    const_iter iter;
    for (iter=newconf.map().begin(); iter!=newconf.map().end(); ++iter) {
      /* update or insert entries */
      const QString our_value=get_string(iter->first);
      if (iter->second != our_value) {
	if (iter->second.isEmpty()) {
	  // if the new value is empty then delete the entry
	  sd << iter->first;
	  if (!m_name.isEmpty())
	    sd << m_name;
	}
	else {
	  su << iter->second << iter->first;
	  if (!m_name.isEmpty())
	    su << m_name;
	  /* If nothing has been updated (no entry), then insert a new entry.
	     This will happen if the entry has been deleted after we fetched
	     it into 'this' */
	  if (su.affected_rows()==0) {
	    si << iter->second << iter->first;
	    if (!m_name.isEmpty())
	      si << m_name;
	  }
	}
      }
    }
    db.commit_transaction();
    return true;
  }
  catch (db_excpt p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
}

/*
  For each entry in 'newconf':
  If the entry exists in 'this' and is different, then update it in memory
*/
void
app_config::diff_replace(const app_config& newconf)
{
  const_iter iter;
  for (iter=newconf.map().begin(); iter!=newconf.map().end(); ++iter) {
    if (!exists(iter->first) || iter->second != get_string(iter->first)) {
      //DBG_PRINTF(5,"%s: %s\n", iter->first.latin1(), iter->second.latin1());
      set_string(iter->first, iter->second);
    }
  }
}

/*
  Update all internal settings that may depend on the configuration
*/
void
app_config::apply()
{
  if (exists("ui_style")) {
    // ui_style may be empty which means "back to the default style"
    gl_pApplication->set_style(get_string("ui_style"));
  }
  if (exists("xpm_path")) {
    extern QString gl_xpm_path;
    QString s=get_string("xpm_path");
    if (!s.isEmpty()) {
      if (QFile::exists(s+"/"+FT_ICON16_QUIT)) {
	gl_xpm_path = s;
      }
    }
  }
  msg_list_window::apply_conf_all_windows();
}

int
app_config::get_date_format_code() const
{
  QString d = get_config().get_string("date_format");
  if (d=="DD/MM/YYYY HH:MI")
    return 1;
  else if (d=="YYYY/MM/DD HH:MI")
    return 2;
  else if (d=="local")
    return 3;
  return 3;
}

Qt::SortOrder
app_config::get_msgs_sort_order() const
{
  QString d = get_config().get_string("messages_order");
  if (d=="oldest_first")
    return Qt::AscendingOrder;
  else
    return Qt::DescendingOrder;
}
