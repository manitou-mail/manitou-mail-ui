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

#ifndef INC_DATABASE_H
#define INC_DATABASE_H


#include <QString>
#include <QThread>
#include <QMutex>
#include <QList>
#include <list>

// PostgreSQL implementation
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

class db_cnx;
class db_excpt;
class db_listener;

class QSocketNotifier;

/*
  Top-level database class.
  Implementations for different RDBMS inherit from this class and
  override the appropriate functions depending on the database
  capabilities and API
*/

class database
{
public:
  database();
  virtual ~database();
  virtual void begin_transaction();
  virtual void commit_transaction();
  virtual void rollback_transaction();
  virtual int logon(const char* conninfo)=0;
  virtual void logoff()=0;
  virtual bool reconnect()=0;
  virtual QString escape_string_literal(const QString)=0;
  virtual QString escape_identifier(const QString)=0;
  void end_transaction();
  int open_transactions_count() const;
  const QString& encoding() const {
    return m_encoding;
  }
  virtual void add_listener(db_listener*)=0;
  virtual void remove_listener(db_listener*)=0;
protected:
  int set_encoding(const QString encoding) {
    m_encoding=encoding;
    return 0;
  }
private:
  /* Number of open transactions.

     This is used to allow for nested beginTransaction/endTransactions
     pairs in a RDBMS where commit blocks are not nested (i.e. commit
     or rollback is applied to all changes that occurred after the
     last commit or rollback, without savepoints like in Oracle).
     commit or rollback issued in non-top level transactions blocks
     will be ignored; only the top level transaction commit or
     rollback will be sent to the db backend, thus commiting or
     rolling back all the changes made in the nested transactions as
     well as in the top level block.  */
  int m_open_trans_count;
  // currently SQL_ASCII or UNICODE
  QString m_encoding;
};


/// Connection class

class db_cnx_elt
{
public:
  db_cnx_elt() {
    m_available=true;
    m_connected=false;
  }
  database* m_db;
  bool m_available;
  bool m_connected;
};

class pgConnection;

class pg_notifier: public QObject
{
  Q_OBJECT
public:
  pg_notifier(pgConnection* cnx);
  ~pg_notifier();
private slots:
  void process_notification();
private:
  QSocketNotifier* m_socket_notifier;
  pgConnection* m_pgcnx;
};

class pgConnection : public database
{
public:
  pgConnection(): m_pgConn(NULL), m_notifier(NULL) {}
  virtual ~pgConnection() {
    logoff();
  }
  int logon(const char* conninfo);
  void logoff();
  bool reconnect();
  bool ping();
  QString escape_string_literal(const QString);
  QString escape_identifier(const QString);
  PGconn* connection() {
    return m_pgConn;
  }
  QList<db_listener*> m_listeners;
  void add_listener(db_listener*);
  void remove_listener(db_listener*);
private:
  PGconn* m_pgConn;
  pg_notifier* m_notifier;
};


/*
  db_cnx is instantiated when there is a need to query the database
  and destroyed when it's finished. It does not reconnect each time
  but rather reuse connections previously that are maintained opened.
*/
class db_cnx
{
public:
  db_cnx(bool other_thread=false);
  virtual ~db_cnx();
  PGconn* connection() {
    return m_cnx->connection();
  }
  database* datab() {
    return m_cnx;
  }
  const database* cdatab() const {
    return m_cnx;
  }
  bool next_seq_val(const char*, int*);
  bool next_seq_val(const char*, unsigned int*);
  // overrides database's methods
  void begin_transaction();
  void commit_transaction();
  void rollback_transaction();
  void end_transaction() {
    m_cnx->end_transaction();
  }
  static void set_connect_string(const char* cnx);
  static void set_dbname(const QString dbname);
  static void disconnect_all();
  static bool idle();

  void enable_user_alerts(bool); // return previous state

  bool ping();
  bool table_exists(const QString);
  void handle_exception(db_excpt& e);

  static const QString& dbname();
  QString escape_string_literal(const QString);
  QString escape_identifier(const QString);
private:
  pgConnection* m_cnx;
  bool m_alerts_enabled;

  static std::list<db_cnx_elt*> m_cnx_list;
  static QMutex m_mutex;
  static bool m_initialized;
  static QString m_connect_string;
  static QString m_dbname;
};

// Transaction object. The destructor issues a rollback if commit has
// not been called and we're a top level transaction within our
// connection
class db_transaction
{
public:
//  db_transaction(database& db);
  db_transaction(db_cnx& d);
  virtual ~db_transaction();
  void commit();
  void rollback();
private:
  db_cnx* m_pDb;
  bool m_commit_done;
};

//#define db_cnx pgConnect

/// sql Exception class
class db_excpt
{
public:
  db_excpt() {}
  db_excpt(const QString query, db_cnx& d);
  // defined in db.cpp
  db_excpt(const QString query, const QString msg, QString code=QString::null);
  virtual ~db_excpt() {}
  QString query() const { return m_query; }
  QString errmsg() const { return m_err_msg; }
  QString errcode() const { return m_err_code; }
  // replace a substring in an error message, typically to hide a password
  void replace(const QString substring, const QString replacement);
  bool unique_constraint_violation() const {
    return m_err_code=="23505";
  }
  // error_code fixed to "UU001" for client-side errors
  static const char* client_assertion;
private:
  QString m_query;
  QString m_err_msg;
  QString m_err_code;
};

void DBEXCPT(db_excpt& p);	// db.cpp

class db_ctxt
{
public:
  db_ctxt(bool pexc=false);
  ~db_ctxt();
  db_cnx* m_db;
  bool propagate_exceptions;
};

// Manitou-Mail schema information
class db_schema
{
public:
  static QString m_version_string;	// cached from table runtime_info
  static int m_version[3];
  static int compare_version(const QString);
  static int* version();
};

#endif // INC_DATABASE_H
