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

#include "db.h"
#include "db_listener.h"
#include "sqlstream.h"
#include "main.h"

#include <QSocketNotifier>

db_listener::db_listener(db_cnx& db, const QString notif_name)
{
  m_socket_notifier = NULL;
  m_db = db.datab();
  sql_stream s1("SELECT quote_ident(:p1)", db);
  s1 << notif_name;
  if (!s1.eos()) {
    s1 >> m_notif_name;
  }
  if (!m_notif_name.isEmpty()) {
    m_db->add_listener(this);
    setup_notification();
    setup_db();
  }
}

db_listener::~db_listener()
{
  if (m_socket_notifier)
    delete m_socket_notifier;
}

bool
db_listener::setup_db()
{
  PGconn* c = (dynamic_cast<pgConnection*>(m_db))->connection();
  QString stmt = QString("LISTEN %1").arg(m_notif_name);
  PGresult* res = PQexec(c, stmt.toUtf8().constData());
  DBG_PRINTF(3, "statement '%s' executed", stmt.toUtf8().constData());
  if (!res) {
    DBG_PRINTF(3, "LISTEN failed");
    return false;
  }
  PQfreemem(res);
  return true;
}

bool
db_listener::setup_notification()
{
  if (m_socket_notifier) {
    delete m_socket_notifier;
    m_socket_notifier=NULL;
  }

  PGconn* c = (dynamic_cast<pgConnection*>(m_db))->connection();
  int socket = PQsocket(c);
  if (socket != -1) {
    m_socket_notifier = new QSocketNotifier(socket, QSocketNotifier::Read);
    connect(m_socket_notifier, SIGNAL(activated(int)), this, SLOT(process_notification()));
  }
  else {
    DBG_PRINTF(1, "could not get db socket: PQsocket returned -1");
  }  
  
  return true;
}

// slot
void
db_listener::process_notification()
{
  DBG_PRINTF(3, "process_notification()");
  pgConnection* cnx = dynamic_cast<pgConnection*>(m_db);
  PGconn* c = cnx->connection();
  int r=PQconsumeInput(c);
  if (r==0) {
    DBG_PRINTF(3, "PQconsumeInput returns 0");
    
    if (!cnx->ping() && cnx->reconnect()) {
      DBG_PRINTF(3, "database reconnect");      
      setup_notification();
    }
  }
  else {
    PGnotify* n;
    while ((n=PQnotifies(c))!=NULL) {
      if (n->relname == m_notif_name) {
	emit notified();
	PQfreemem(n);
      }
    }
  }
}
