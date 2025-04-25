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

#include "db.h"
#include "db_listener.h"
#include "sqlstream.h"
#include "main.h"


db_listener::db_listener(db_cnx& db, const QString notif_name)
{
  m_db = db.datab();
  sql_stream s1("SELECT quote_ident(:p1)", db);
  s1 << notif_name;
  if (!s1.eos()) {
    s1 >> m_notif_name;
  }
  if (!m_notif_name.isEmpty()) {
    m_db->add_listener(this);
    DBG_PRINTF(4, "add listener this=%p", this);
  }
}

db_listener::~db_listener()
{
  DBG_PRINTF(4, "delete this=%p", this);
  m_db->remove_listener(this);
}

// slot
void
db_listener::process_notification()
{
  DBG_PRINTF(4, "process notification this=%p", this);
  emit notified();
}
