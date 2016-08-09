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

#include "users.h"
#include "db.h"
#include "sqlstream.h"
#include "main.h"

/* cached value of USERS.user_id for the user connected to the database
   -1 => user unknown yet
   0 => unable to find it in USERS
   >0 => real user_id found by matching current_user WITH users.login
*/
//static
int
user::m_current_user_id=-1;

user::user(): m_user_id(0), m_can_login(false), m_can_connect(false),
	     m_is_superuser(false), m_role_oid(0)
{
}

user::~user()
{
}

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

//static
bool
user::has_permission(const QString ability)
{
  // application-side permission checks are not available before 1.5.1
  if (db_schema::compare_version("1.5.1") < 0)
    return true;

  static QMap<QString,bool> permission_cache;

  QMap<QString,bool>::const_iterator it = permission_cache.constFind(ability);
  if (it != permission_cache.constEnd())
    return it.value();

  db_ctxt dbc(true);

  QList<db_obj_privilege> privs = db_obj_privilege::ability_privileges(ability, &dbc);
  if (privs.isEmpty())
    return true;
  try {
    db_role role = db_role::current_role(&dbc);
    bool perm = role.has_multiple_privileges(privs, &dbc);
    permission_cache[ability] = perm;
    return perm;
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
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

/* Returns true if the currently logged user has the database
   permission to create other users, false otherwise or if we can't check */
// static
bool
user::can_create_user()
{
  db_cnx db;
  try {
    sql_stream s("SELECT rolcreaterole FROM pg_roles WHERE rolname=current_user", db);
    if (!s.eos()) {
      QString t_or_f;
      s >> t_or_f;
      return t_or_f=="t";
    }
    else
      return false;
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
}

bool
user::unregister(db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::unregister");
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  try {
    sql_stream si("UPDATE users SET login=NULL WHERE user_id=:id", *db);
    si << m_user_id;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
user::insert(db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::insert");
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  try {
    sql_stream si("INSERT INTO users(user_id,fullname,login,email,custom_field1,custom_field2,custom_field3) SELECT 1+coalesce(max(user_id),0), :name,:login,:email,:c1,:c2,:c3 FROM users RETURNING user_id", *db);
    stream_fields(si);
    si >> m_user_id;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
user::fetch(int user_id)
{
  db_cnx db;
  try {
    sql_stream s("SELECT fullname,login,email,custom_field1,custom_field2,custom_field3,pu.rolcanlogin,rolsuper,pu.oid FROM users u JOIN pg_roles pu ON (pu.rolname=u.login) WHERE user_id=:p1", db);
    s << user_id;
    if (!s.eos()) {
      s >> m_fullname >> m_login >> m_email >> m_custom_field1 >> m_custom_field2 >> m_custom_field3 >> m_can_login >> m_is_superuser >> m_role_oid;
      m_user_id = user_id;
    }
    else {
      m_user_id=0;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
  return true;
}

bool
user::fetch_by_oid(int role_oid)
{
  db_cnx db;
  try {
    m_user_id=0;
    sql_stream s
      ("SELECT coalesce(u.user_id,0) as user_id,"
       "fullname,"
       "pu.rolname,"
       "email,"
       "custom_field1,custom_field2,custom_field3,"
       "pu.rolcanlogin,"
       "pu.rolsuper,"
       "pg_catalog.has_database_privilege(pu.rolname,current_database(),'connect'),"
       "pu.oid"
       " FROM users u RIGHT JOIN pg_roles pu"
       " ON (pu.rolname=u.login)"
       " WHERE pu.oid=:p1", db);
    s << role_oid;
    if (!s.eos()) {
      s >> m_user_id >> m_fullname >> m_login >> m_email >> m_custom_field1 >> m_custom_field2 >> m_custom_field3 >> m_can_login >> m_is_superuser >> m_can_connect >> m_role_oid;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    return false;
  }
  return true;
}

sql_stream&
user::stream_fields(sql_stream& s)
{
  QString* fields[] = {&m_fullname, &m_login, &m_email, &m_custom_field1, &m_custom_field2, &m_custom_field3 };
  for (uint i=0; i<sizeof(fields)/sizeof(fields[0]); i++) {
    if (fields[i]->isEmpty())
      s << sql_null();
    else
      s << *fields[i];
  }
  return s;
}

bool
user::update_fields(db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::update_fields");
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  try {
    sql_stream s("UPDATE users SET fullname=:p1,login=:p2,email=:p3,custom_field1=:p4,custom_field2=:p5,custom_field3=:p6 WHERE user_id=:id", *db);
    stream_fields(s);
    s << m_user_id;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
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

// Fetch all roles sorted by name
QList<user>
users_repository::get_list()
{
  QList<user> list;
  db_cnx db;
  try {
    sql_stream s("SELECT coalesce(u.user_id,0),fullname,pu.rolname,login,email,custom_field1,custom_field2,custom_field3,pu.rolcanlogin,pu.rolsuper,pg_catalog.has_database_privilege(pu.rolname,current_database(),'connect'),pu.oid FROM users u RIGHT JOIN pg_roles pu ON (pu.rolname=u.login) ORDER BY pu.rolname", db);
    while (!s.eos()) {
      user u;
      s >> u.m_user_id >> u.m_fullname >> u.m_db_login >> u.m_login >> u.m_email >> u.m_custom_field1 >> u.m_custom_field2 >> u.m_custom_field3 >> u.m_can_login >> u.m_is_superuser >> u.m_can_connect >> u.m_role_oid;
      list.append(u);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
  return list;
}


//static
QList<QString>
user::granted_roles(int oid)
{
  QList<QString> list;
  db_cnx db;
  try {
    sql_stream s("SELECT r.rolname FROM pg_catalog.pg_auth_members m"
        " JOIN pg_catalog.pg_roles r ON (m.roleid = r.oid)"
        " WHERE m.member = :oid ORDER BY 1", db);
    s << oid;
    while (!s.eos()) {
      QString role;
      s >> role;
      list.append(role);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
  return list;
}


// Enable or disable LOGIN privilege based on 'can_login'.
bool //static
user::allow_login(const QString login, bool can_login, db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::allow_login(%s,%d)", login.toLocal8Bit().constData(), can_login);
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  QString qlogin = db->escape_identifier(login);
  try {
    sql_stream s("SELECT rolcanlogin FROM pg_roles WHERE rolname=:r", *db);
    s << login;
    bool rolcanlogin;
    if (!s.eos()) {
      s >> rolcanlogin;
      if (can_login != rolcanlogin) {
	sql_stream s(QString("ALTER ROLE %1 %2").
		     arg(qlogin).
		     arg(can_login?"LOGIN":"NOLOGIN"), *db);
      }
      return true;
    }
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT (p);
  }
  return false;
}

bool // static
user::change_password(const QString login,
		      const QString password,
		      db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::change_password(%s)", login.toLocal8Bit().constData());
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  QString qlogin = db->escape_identifier(login);
  QString qpassword = db->escape_string_literal(password);
  try {
    sql_stream s(QString("ALTER ROLE %1 PASSWORD '%2'")
		 .arg(qlogin)
		 .arg(qpassword), *db);
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    DBEXCPT (p);
    return false;
  }
  return true;
}

bool // static
user::change_login(const QString old_login,
		   const QString new_login,
		   const QString password,
		   db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::change_login(%s,%s)",
	     old_login.toLocal8Bit().constData(),
	     new_login.toLocal8Bit().constData());
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  QString q_old_login = db->escape_identifier(old_login);
  QString q_new_login = db->escape_identifier(new_login);
  try {
    sql_stream s(QString("ALTER ROLE %1 RENAME TO %2")
		 .arg(q_old_login)
		 .arg(q_new_login), *db);
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    DBEXCPT (p);
    return false;
  }
  return true;
}

bool
user::grant_role(const QString username,
		 const QString rolname,
		 db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::grant_role(%s,%s)", username.toLocal8Bit().constData(),
	     rolname.toLocal8Bit().constData());
  return req2id("GRANT %1 TO %2", rolname, username, dbc);
}

bool
user::grant_connect(const QString username,
		    const QString dbname,
		    db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::grant_connect(%s,%s)", username.toLocal8Bit().constData(),
	     dbname.toLocal8Bit().constData());

  return req2id("GRANT CONNECT ON DATABASE %1 TO %2",
		dbname,
		username,
		dbc);
}

/* Factored code for any query that takes 2 identifiers as params
   and a database context */
bool
user::req2id(const QString query,
	     const QString id1,
	     const QString id2,
	     db_ctxt* dbc)
{
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  QString qid1 = db->escape_identifier(id1);
  QString qid2 = db->escape_identifier(id2);
  try {
    sql_stream s(query.arg(qid1).arg(qid2), *db);
    return true;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return false;
}

bool
user::revoke_connect(const QString username,
		     const QString dbname,
		     db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::revoke_connect(%s,%s)", username.toLocal8Bit().constData(),
	     dbname.toLocal8Bit().constData());

  return req2id("REVOKE CONNECT ON DATABASE %1 FROM %2",
		dbname,
		username,
		dbc);
}


bool
user::revoke_role(const QString username,
		  const QString rolname,
		  db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::revoke_role(%s,%s)", username.toLocal8Bit().constData(),
	     rolname.toLocal8Bit().constData());
  return req2id("REVOKE %1 FROM %2", rolname, username, dbc);
}


/*
 Create a new database role.
 No password if password.isNull() is true
*/
bool
user::create_db_user(const QString login,
		     const QString password,
		     bool can_login,
		     db_ctxt* dbc)
{
  DBG_PRINTF(4, "user::create_db_user(%s,%d)", login.toLocal8Bit().constData(),
	     can_login);
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  QString qlogin = db->escape_identifier(login);
  QString qpassword = db->escape_string_literal(password);
  QString query = QString("CREATE ROLE %1").arg(qlogin);
  if (!password.isNull())
    query.append(QString(" PASSWORD '%1'").arg(qpassword));
  if (can_login) {
    query.append(" LOGIN");
  }

  try {
    sql_stream s(query, *db);
  }
  catch(db_excpt& p) {
    if (!password.isNull()) {
      /* we want to mask the password in the error message */
      p.replace(QString("PASSWORD '%1'").arg(qpassword),
		"PASSWORD '******'");
    }
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
    return false;
  }
  return true;
}

Oid
user::oid_db_role(const QString login, bool case_sensitive)
{
  Q_UNUSED(case_sensitive);
  db_cnx db;
  try {
#if 0
    const char* query;
    if (case_sensitive) {
      query = "SELECT oid FROM pg_roles WHERE rolname=:p1";
    }
    else {
      query = "SELECT oid FROM pg_roles WHERE lower(rolname)=lower(:p1)";
    }
#endif
    sql_stream s("SELECT oid FROM pg_roles WHERE rolname=:p1", db);
    s << login;
    if (!s.eos()) {
      Oid oid;
      s >> oid;
      return oid;
    }
    return 0;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return 0;
  }
}

db_role::db_role(int oid) : m_oid(oid), m_is_superuser(false)
{
}

db_role::~db_role()
{
}



/* returns false if privilege on object_name is not granted to role.
   warning: returns false also on db problem and no exception raised
   (unless dbc->propagate_exceptions is true)
*/
bool
db_role::has_privilege(QString object_type,
		       QString object_name,
		       QString privilege,
		       db_ctxt* dbc)
{
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;
  QString funcname;
  if (object_type=="table") {
    funcname = "has_table_privilege";
  }
  else if (object_type=="function")
    funcname = "has_function_privilege";
  else {
    DBG_PRINTF(1, "wrong object_type for has_privilege");
    Q_ASSERT(false);
    return false;
  }

  try {
    QString query = QString("SELECT %1(:o, :objname, :priv)").arg(funcname);

    sql_stream s(query, *db);
    s << m_oid << object_name << privilege;
    bool has_priv = false;
    if (!s.eos()) {
      s >> has_priv;
    }
    DBG_PRINTF(6, "%s oid=%d objname=%s priv=%s => %d",
	       query.toLocal8Bit().constData(),
	       m_oid,
	       object_name.toLocal8Bit().constData(),
	       privilege.toLocal8Bit().constData(),
	       has_priv);
    return has_priv;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return false;
}

/*
   Returns true if the role has all the privileges in 'list'.
   Implemented as to get all the answers through a single query
   (as opposed to issuing multiple calls to has_privilege())
*/
bool
db_role::has_multiple_privileges(QList<db_obj_privilege> list,
				db_ctxt* dbc)
{
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  if (list.size()==0)
    return true;

  try {
    QStringList table_values;
    QStringList func_values;

    for (int i=0; i<list.size(); i++) {
      db_obj_privilege p = list.at(i);
      if (p.m_objtype == "table") {
	QString qtable = db->escape_string_literal(p.m_objname);
	table_values.append(QString("('%1','%2')").arg(qtable).arg(p.m_privtype));
      }
      else if (p.m_objtype == "function") {
	QString qfunc = db->escape_string_literal(p.m_objname);
	func_values.append(QString("('%1')").arg(qfunc));
      }
      else {
	throw db_excpt(QString::null,
		       QString("Unhandled object type: %1").arg(p.m_objtype),
		       db_excpt::client_assertion);

      }
    }

    bool has_priv_tables = true;
    bool has_priv_funcs = true;

    if (!table_values.isEmpty()) {
      QString values_clause = table_values.join(",");
      QString query = QString
	("SELECT bool_and( has_table_privilege(:o, p.tablename, p.privtype) )"
	 " FROM (VALUES %1) AS p(tablename,privtype)")
	.arg(values_clause);

      sql_stream s(query, *db);
      s << m_oid;

      if (!s.eos()) {
	s >> has_priv_tables;
      }
    }

    if (!func_values.isEmpty()) {
      QString values_clause = func_values.join(",");
      QString query = QString
	("SELECT bool_and( has_function_privilege(:o, p.funcname, 'execute') )"
	 " FROM (VALUES %1) AS p(funcname)")
	.arg(values_clause);

      sql_stream s(query, *db);
      s << m_oid;

      if (!s.eos()) {
	s >> has_priv_funcs;
      }
    }

    return has_priv_tables && has_priv_funcs;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return false;
}



bool
db_role::can_execute(QString func_proto, db_ctxt* dbc)
{
  return has_privilege("function", func_proto, "execute", dbc);
}

bool
db_role::can_update_table(QString tablename, db_ctxt* dbc)
{
  return has_privilege("table", tablename, "update", dbc);
}

bool
db_role::grant_table_privilege(QString tablename, QString privilege, db_ctxt* dbc)
{
  return set_table_priv(true, tablename, privilege, dbc);
}

bool
db_role::revoke_table_privilege(QString tablename, QString privilege, db_ctxt* dbc)
{
  return set_table_priv(false, tablename, privilege, dbc);
}

bool
db_role::set_execute_priv_function(bool set, QString func_name, db_ctxt* dbc)
{
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  try {
    QString qname = db->escape_identifier(this->name());
    /* We don't quote function names; they are not user-definable and
       our function names don't need quoting. If they did, we should
       parse them to quote argument types separately */
    QString query;
    if (set)
      query = QString("GRANT EXECUTE ON FUNCTION %1 TO %2").arg(func_name).arg(qname);
    else
      query = QString("REVOKE EXECUTE ON FUNCTION %1 FROM %2").arg(func_name).arg(qname);
    sql_stream s(query, *db);
    return true;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return false;

}

bool
db_role::grant_execute_function(QString func_name, db_ctxt* dbc)
{
  return set_execute_priv_function(true, func_name, dbc);
}

bool
db_role::revoke_execute_function(QString func_name, db_ctxt* dbc)
{
  return set_execute_priv_function(false, func_name, dbc);
}

bool
db_role::grant(const db_obj_privilege priv, db_ctxt* dbc)
{
  if (priv.m_objtype == "function")
    return this->grant_execute_function(priv.m_objname, dbc);
  else
    return this->set_table_priv(true, priv.m_objname, priv.m_privtype, dbc);
}

bool
db_role::revoke(const db_obj_privilege priv, db_ctxt* dbc)
{
  if (priv.m_objtype == "function")
    return this->revoke_execute_function(priv.m_objname, dbc);
  else
    return this->set_table_priv(false, priv.m_objname, priv.m_privtype, dbc);
}

/*
  The special value "crud" for 'privilege' means select/insert/update/delete
  combined.
 */
bool
db_role::set_table_priv(bool set, QString tablename, QString privilege, db_ctxt* dbc)
{
  db_cnx db0;
  db_cnx* db = dbc ? dbc->m_db : &db0;

  try {
    QString base_query;
    if (set) {
      base_query = "GRANT %1 ON TABLE %2 TO %3";
    }
    else {
      base_query = "REVOKE %1 ON TABLE %2 FROM %3";
    }
    QString qtable = db->escape_identifier(tablename);
    QString qname = db->escape_identifier(this->name());
    DBG_PRINTF(4, "tablename=%s qtable=%s", tablename.toLocal8Bit().constData(),
	       qtable.toLocal8Bit().constData());
    if (privilege != "crud") {
      QString query = base_query.arg(privilege).arg(qtable).arg(qname);
      sql_stream s(query, *db);
    }
    else {
#if 0
      const char* privs[] = {"select", "insert", "update", "delete"};
      for (uint i=0; i<sizeof(privs)/sizeof(privs[0]); i++) {
	sql_stream s(base_query.arg(privs[i]).arg(qtable).arg(qname), *db);
      }
#else
      sql_stream s1(base_query.arg("select,insert,update,delete").arg(qtable).arg(qname), *db);
#endif
    }
    return true;
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return false;

}

bool
db_role::rename(QString new_name, db_ctxt* dbc)
{
  try {
    QString query = QString("ALTER ROLE %1 RENAME TO %2").
      arg(dbc->m_db->escape_identifier(name())).
      arg(dbc->m_db->escape_identifier(new_name));
    sql_stream s(query, *dbc->m_db);
    m_name = new_name;
    return true;
  }
  catch(db_excpt& p) {
    if (dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return false;
}

bool
db_role::fetch_properties(db_ctxt* dbc)
{
  try {
    QString query = "SELECT rolname,rolsuper FROM pg_roles WHERE oid=:o";
    sql_stream s(query, *dbc->m_db);
    s  << m_oid;
    if (!s.eos()) {
      s >> m_name >> m_is_superuser;
    }
  }
  catch(db_excpt& p) {
    if (dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
    return false;
  }
  return true;
}

QString
db_role::name()
{
  return m_name;
}

//static
db_role
db_role::current_role(db_ctxt* dbc)
{
  QString query = "SELECT oid FROM pg_roles WHERE rolname=current_user";
  sql_stream s(query, *dbc->m_db);
  if (!s.eos()) {
    int oid;
    s >> oid;
    db_role r(oid);
    r.fetch_properties(dbc);
    return r;
  }
  else
    return db_role(0);
}

//////////////////////////////////////////////////////
db_obj_privilege::db_obj_privilege()
{
}

db_obj_privilege::db_obj_privilege(QString objtype, QString objname, QString privtype)
  : m_objtype(objtype), m_objname(objname), m_privtype(privtype)
{

}

/*
  Returns the list of database privileges required to have an ability.
  The ability corresponds to 'enum privileges' from users_dialog (to be
  refactored in users or db_obj_privilege or a new class.
  Values currently handled are: read,update,trash,delete,compose,basic_management
  More to come as the permission system gets refined in the future.
*/
QList<db_obj_privilege>
db_obj_privilege::ability_privileges(const QString ability, db_ctxt* dbc)
{
  QList<db_obj_privilege> list;
  try {
    sql_stream s("SELECT * FROM object_permissions(:p)", *dbc->m_db);
    s << ability;
    while (!s.eos()) {
      db_obj_privilege o;
      s >> o.m_objname >> o.m_objtype >> o.m_privtype;
      list.append(o);
    }
  }
  catch(db_excpt& p) {
    if (dbc && dbc->propagate_exceptions)
      throw p;
    else
      DBEXCPT(p);
  }
  return list;
}

#if 0
/* Grant the privilege */
void
db_obj_privilege::grant(const QString rolename, db_ctxt* dbc) const
{
  db_cnx* db = dbc->m_db;
  QString query = QString("GRANT %1 ON %2 TO %3").arg(m_privtype).
    arg(db->escape_identifier(m_objname)).arg(db->escape_identifier(rolename));
  sql_stream s(query, *db);
}

/* Revoke the privilege */
void
db_obj_privilege::revoke(const QString rolename, db_ctxt* dbc) const
{
  db_cnx* db = dbc->m_db;
  QString query = QString("REVOKE %1 ON %2 FROM %3").arg(m_privtype).
    arg(db->escape_identifier(m_objname)).arg(db->escape_identifier(rolename));
  sql_stream s(query, *db);
}
#endif
