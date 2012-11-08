/* Copyright (C) 2004-2012 Daniel Verite

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

#include "db.h"
#include <stdio.h>
#include <iostream>

#include <QByteArray>
#include <QMessageBox>
#include <QSocketNotifier>

#include "database.h"
#include "sqlstream.h"
#include "sqlquery.h"
#include "addresses.h"
#include "db_listener.h"

//static PGconn *pgconn;
pgConnection pgDb;

PGconn* GETDB()
{
  //  return pgconn;
  return pgDb.connection();
}

void DBEXCPT(PGconn* c)
{
  //  std::cerr << PQerrorMessage(c);
  QString err=PQerrorMessage(c);
  QMessageBox::warning(NULL, QObject::tr("Database error"), err);
}

void DBEXCPT(db_excpt& p)
{
  //  std::cerr << p.query() << ":" << p.errmsg() << std::endl;
  QString err=p.query();
  err+=":\n";
  err+=p.errmsg();
  QMessageBox::warning(NULL, QObject::tr("Database error"), err);
}

db_excpt::db_excpt(const QString query,
		   const QString msg,
		   QString code/*=QString::null*/)
{
  m_query=query;
  m_err_msg=msg;
  m_err_code=code;
  DBG_PRINTF(3, "db_excpt: query='%s', err='%s'", m_query.toLocal8Bit().constData(), m_err_msg.toLocal8Bit().constData());
}

db_excpt::db_excpt(const QString query, db_cnx& d)
{
  m_query=query;
  char* pg_msg = PQerrorMessage(d.connection());
  if (pg_msg!=NULL)
    m_err_msg = QString::fromUtf8(pg_msg);
}

int
ConnectDb(const char* cnx_string, QString* errstr)
{
  try {
    pgDb.logon(cnx_string);
    db_cnx::set_connect_string(cnx_string);
    db_cnx db;
    sql_stream s("SELECT current_database()", db);
    if (!s.eos()) {
      QString dbname;
      s >> dbname;
      db_cnx::set_dbname(dbname);
    }
  }
  catch(db_excpt& p) {
    QByteArray errmsg_bytes = p.errmsg().toLocal8Bit();    
    std::cerr << errmsg_bytes.constData() << std::endl;
    if (errstr)
      *errstr = p.errmsg();
    return 0;
  }
  return 1;
}

void // static
db_cnx::disconnect_all()
{
  // close secondary connections
  std::list<db_cnx_elt*>::iterator it=m_cnx_list.begin();
  for (; it!=m_cnx_list.end(); it++) {
    if ((*it)->m_connected) {
      (*it)->m_db->logoff();
      (*it)->m_connected=false;
    }
  }
}

void
DisconnectDb()
{
  // close primary connection
  pgDb.logoff();
  db_cnx::disconnect_all();
}

database::database() : m_open_trans_count(0)
{
}

database::~database()
{
}

void database::begin_transaction()
{
  m_open_trans_count++;
}

void database::commit_transaction()
{
}

void database::rollback_transaction()
{
}

int database::open_transactions_count() const
{
  return m_open_trans_count;
}

void
pgConnection::add_listener(db_listener* listener)
{
  m_listeners.append(listener);
  QString s = "LISTEN " + listener->notification_name();
  QByteArray ba = s.toUtf8();
  PQexec(m_pgConn, ba.constData());
}

void
pgConnection::remove_listener(db_listener* listener)
{
  int i=m_listeners.indexOf(listener);
  if (i>=0) {
    QString s = "UNLISTEN " + listener->notification_name();
    QByteArray ba = s.toUtf8();
    PQexec(m_pgConn, ba.constData());
    m_listeners.removeAt(i);
  }
}

// static data members
bool db_cnx::m_initialized;
std::list<db_cnx_elt*> db_cnx::m_cnx_list;
QMutex db_cnx::m_mutex;
QString db_cnx::m_connect_string;
QString db_cnx::m_dbname;

// static
void
db_cnx::set_connect_string(const char* cnx)
{
  m_connect_string = cnx;
}

// static
void
db_cnx::set_dbname(const QString dbname)
{
  m_dbname = dbname;
}

// static
const QString&
db_cnx::dbname()
{
  return m_dbname;
}


/* idle(): Return false if at least one non-primary connection is in
   use, meaning that we're probably running a query in a sub-thread.
   Should be called to avoid hitting the db with multiple simultaneous
   queries whenever possible */
// static
bool
db_cnx::idle()
{
  // TODO: do we need to use m_mutex here?
  std::list<db_cnx_elt*>::iterator it=m_cnx_list.begin();
  for (; it!=m_cnx_list.end(); it++) {
    if (!(*it)->m_available)
      return false;
  }
  return true;
}


db_cnx::~db_cnx()
{
  if (m_cnx) {
    std::list<db_cnx_elt*>::iterator it=m_cnx_list.begin();
    for (; it!=m_cnx_list.end(); it++) {
      if ((*it)->m_db == m_cnx) {
	(*it)->m_available=true;
      }
    }
  }
}

db_cnx::db_cnx(bool other_thread)
{
  if (!other_thread) {
    // just use the main connection for the main thread
    m_cnx=&pgDb;
    return;
  }

  const int max_cnx=5;
  m_mutex.lock();
  if (!m_initialized) {
    for (int i=0; i<max_cnx; i++) {
      db_cnx_elt* p = new db_cnx_elt;
      m_cnx_list.push_back(p);
    }
    m_initialized=true;
  }

  std::list<db_cnx_elt*>::iterator it = m_cnx_list.begin();
  for (; it!=m_cnx_list.end(); it++) {
    if ((*it)->m_available) {
      pgConnection* p;
      if (!(*it)->m_connected) {
	p = new pgConnection;
	DBG_PRINTF(4, "Opening a new database connection");
	(*it)->m_db = p;
	p->logon(m_connect_string.toLocal8Bit().constData());
	(*it)->m_connected=true;
      }
      (*it)->m_available=false;
      m_cnx = dynamic_cast<pgConnection*>((*it)->m_db); // FIXME??
      break;
    }
  }
  m_mutex.unlock();

  m_alerts_enabled=true;

#if 0
  {
    // DEBUG
    QString state;
    int i=0;
    std::list<db_cnx_elt*>::iterator it2 = m_cnx_list.begin();
    for (; it2!=m_cnx_list.end(); ++it2,++i) {
      state.append(QString("\nconnection %1 connected:%2 available:%3").arg(i).arg((*it2)->m_connected).arg((*it2)->m_available));
    }
    DBG_PRINTF(3, "Connections: %s", state.toLocal8Bit().constData());
  }
#endif

  if (it==m_cnx_list.end()) {
    DBG_PRINTF(2, "No database connection found");
    throw db_excpt(NULL, QObject::tr("The %1 database connections are already in use.").arg(max_cnx));
  }
}


int
pgConnection::logon(const char* conninfo)
{
  m_pgConn = PQconnectdb(conninfo);
  if (!m_pgConn) {
    throw db_excpt("connect", "not enough memory");
  }
  DBG_PRINTF(5,"logon m_pgConn=0x%p", m_pgConn);
  if (PQstatus(m_pgConn) == CONNECTION_BAD) {
    throw db_excpt("connect", PQerrorMessage(m_pgConn));
  }

  /* If the user has set PGCLIENTENCODING in its environment, then we decide
     to do no translation behind the postgresql client layer, since we
     assume that the user knows what he's doing. In the future, we may
     decide for a fixed encoding (that would be unicode/utf8, most
     probably) and override PGCLIENTENCODING. */
  if (!getenv("PGCLIENTENCODING")) {
    PGresult* res=PQexec(m_pgConn, "SELECT pg_encoding_to_char(encoding) FROM pg_database WHERE datname=current_database()");
    if (res && PQresultStatus(res)==PGRES_TUPLES_OK) {
      const char* enc=(const char*)PQgetvalue(res,0,0);
      // pgsql versions before 8.1 return 'UNICODE', >=8.1 return 'UTF8'
      // we keep UTF8
      if (!strcmp(enc,"UNICODE"))
	enc="UTF8";
      set_encoding(enc);
    }
    if (res)
      PQclear(res);
  }
  PQexec(m_pgConn, "SET standard_conforming_strings=on");

  if (this==&pgDb)
    m_notifier = new pg_notifier(this);
 
  return 1;
}

void
pgConnection::logoff()
{
  if (m_pgConn) {
    if (m_notifier)
      delete m_notifier;
    
    PQfinish(m_pgConn);
    m_pgConn=NULL;
  }
}

bool
pgConnection::reconnect()
{
  DBG_PRINTF(3, "pgConnection::reconnect()");
  if (m_pgConn) {
    PQreset(m_pgConn);
    if (PQstatus(m_pgConn)!=CONNECTION_OK)
      return false;
  }
  for (int i=0; i<m_listeners.size(); i++) {
    /* Reinitialize listeners. It is necessary if the db backend
       process went down, and if the socket changed */
    db_listener* l = m_listeners.at(i);
    QString stmt = "LISTEN " + l->notification_name();
    PQexec(m_pgConn, stmt.toUtf8().constData());
  }
  return true;
}

bool
pgConnection::ping()
{
  if (!m_pgConn)
    return false;

  if (open_transactions_count() > 0) {
    /* We don't test the connection when inside a transaction because
       if the transaction is in a unusable state, any SQL will fail */
    return true;
  }

  PGresult* res = PQexec(m_pgConn, "SELECT 1");
  if (res) {
    bool ret = (PQresultStatus(res)==PGRES_TUPLES_OK);
    PQclear(res);
    return ret;
  }
  else
    return false;
}

QString
pgConnection::escape_string_literal(const QString src)
{
  char local_buf[2048]="\0";
  char* to=local_buf;
  int error;
  QByteArray arr = src.toUtf8();
  size_t length = arr.size();
  if (length*4 > sizeof(local_buf)) {
    to = (char*)malloc(length*4);
  }
  PQescapeStringConn(m_pgConn, to, arr.constData(), length, &error);
  // TODO: throw an exception if an error occurred
  QString out = QString::fromUtf8(to);
  if (to!=local_buf)
    free(to);
  return out;
}

#if 0
db_transaction::db_transaction(database& db): m_commit_done(false)
{
  m_pDb=&db;
  db.begin_transaction();
}
#endif

db_transaction::db_transaction(db_cnx& db): m_commit_done(false)
{
  m_pDb=&db;
  db.begin_transaction();
}

db_transaction::~db_transaction()
{
  if (m_pDb->datab()->open_transactions_count()==1 && !m_commit_done)
    m_pDb->rollback_transaction();
//  m_pDb->datab()->end_transaction();
}

void
db_transaction::commit()
{
  m_commit_done=true;
  if (m_pDb->datab()->open_transactions_count()==1) {
    m_pDb->commit_transaction();
  }
}

void
db_transaction::rollback()
{
  if (m_pDb->datab()->open_transactions_count()==1) {
    m_pDb->rollback_transaction();
  }
}

void
database::end_transaction()
{
  m_open_trans_count--;
  DBG_PRINTF(4, "new m_open_trans_count=%d", m_open_trans_count);
  if (m_open_trans_count<0) {
    fprintf(stderr, "Fatal error: m_open_trans_count<0\n");
    exit(1);
  }
}

bool
db_cnx::next_seq_val(const char* seqName, int* id)
{
  try {
    QString query = QString("SELECT nextval('%1')").arg(seqName);
    sql_stream s(query, *this);
    s >> *id;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
db_cnx::next_seq_val(const char* seqName, unsigned int* id)
{
  try {
    QString query = QString("SELECT nextval('%1')").arg(seqName);
    sql_stream s(query, *this);
    s >> *id;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

void
db_cnx::begin_transaction()
{
  m_cnx->begin_transaction();
  // don't use nested transactions
  DBG_PRINTF(5,"open_transactions_count()=%d", datab()->open_transactions_count());
  if (datab()->open_transactions_count()==1) {
    sql_stream s("BEGIN", *this);
  }
}

void
db_cnx::commit_transaction()
{
  end_transaction();
  if (datab()->open_transactions_count()==0) {
    sql_stream s("COMMIT", *this);
  }
}

void
db_cnx::rollback_transaction()
{
  end_transaction();
  if (datab()->open_transactions_count()==0) {
    sql_stream s("ROLLBACK", *this);
  }
}

void
db_cnx::handle_exception(db_excpt& e)
{
  if (m_alerts_enabled) {
    DBEXCPT(e);
  }
}

bool
db_cnx::ping()
{
  return m_cnx->ping();
}

QString
db_cnx::escape_string_literal(const QString str)
{
  return m_cnx->escape_string_literal(str);
}

pg_notifier::pg_notifier(pgConnection* cnx)
{
  m_pgcnx = cnx;
  PGconn* c = cnx->connection();
  int socket = PQsocket(c);
  if (socket != -1) {
    DBG_PRINTF(6, "Instantiate a new QSocketNotifier on fd=%d", socket);
    m_socket_notifier = new QSocketNotifier(socket, QSocketNotifier::Read);
    connect(m_socket_notifier, SIGNAL(activated(int)), this, SLOT(process_notification()));
  }
  else {
    DBG_PRINTF(1, "could not get db socket: PQsocket returned -1");
  }  
}

pg_notifier::~pg_notifier()
{
  if (m_socket_notifier)
    delete m_socket_notifier;
}

void
pg_notifier::process_notification()
{
  PGconn* c = m_pgcnx->connection();
  DBG_PRINTF(6, "process_notification() on socket %d", PQsocket(c));
  int r=PQconsumeInput(c);
  if (r==0) {
    DBG_PRINTF(6, "PQconsumeInput returns 0");
  }
  else {
    PGnotify* n;
    while ((n=PQnotifies(c))!=NULL) {
      DBG_PRINTF(6, "received db notify for %s", n->relname);
      for (int i=0; i<m_pgcnx->m_listeners.count(); i++) {
	db_listener* l = m_pgcnx->m_listeners.at(i);
	if (n->relname == l->notification_name()) {
	  l->process_notification();
	}
      }
      PQfreemem(n);
    }
  }
}

std::list<QString>
ReferencesList(const QString &s)
{
  std::list<QString> l;
  uint nLen=s.length();
  uint nPos=0;

  while (nPos<nLen) {
    int nStart=s.indexOf('<', nPos);
    if (nStart!=-1) {
      int endPos=s.indexOf('>',nStart);
      if (endPos!=-1) {
	l.push_back(s.mid(nStart+1,endPos-nStart-1));
	nPos=endPos+1;
      }
      else
	nPos=nLen;
    }
    else
      nPos=nLen;
  }
  return l;
}


