/* Copyright (C) 2004-2023 Daniel Verite

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

#ifndef INC_USERS_H
#define INC_USERS_H

#include <QString>
#include <QList>
#include <map>
#include "sqlstream.h"

class user 
{
public:
  user();
  virtual ~user();
  QString name(int user_id);

  static int current_user_id();
  static bool has_permission(const QString ability);
  static bool create_if_missing(const QString fullname);

  bool update_fields(db_ctxt* dbc=NULL);
  bool unregister(db_ctxt* dbc=NULL);
  bool insert(db_ctxt* dbc=NULL);
  bool fetch(int user_id);
  bool fetch_by_oid(int role_oid);

  static bool create_db_user(const QString login,
			     const QString password,
			     bool can_login=true,
			     db_ctxt* dbc=NULL);
  static Oid oid_db_role(const QString login, bool case_sensitive);
  static bool can_create_user();
  static bool allow_login(const QString login,
			  bool enable,
			  db_ctxt* dbc=NULL);

  static bool change_login(const QString old_login,
			   const QString new_login,
			   const QString password,
			   db_ctxt* dbc=NULL);

  static bool change_password(const QString login,
			      const QString password,
			      db_ctxt* dbc=NULL);
  static bool grant_role(const QString username,
			 const QString rolname,
			 db_ctxt* dbc=NULL);
  static bool revoke_role(const QString username,
			  const QString rolname,
			  db_ctxt* dbc=NULL);
  static bool grant_connect(const QString username,
			    const QString dbname,
			    db_ctxt* dbc=NULL);
  static bool revoke_connect(const QString username,
			     const QString dbname,
			     db_ctxt* dbc=NULL);
  static bool req2id(const QString query,
		     const QString id1,
		     const QString id2,
		     db_ctxt* dbc=NULL);
  static QList<QString> granted_roles(int oid);

  QString m_fullname;
  int m_user_id;
  QString m_login;
  QString m_db_login;
  QString m_email;
  QString m_custom_field1;
  QString m_custom_field2;
  QString m_custom_field3;
  bool m_can_login;		// LOGIN privilege for the account (instance level)
  bool m_can_connect;		// CONNECT privilege (database level)
  bool m_is_superuser;		// pg_roles.rolsuper (instance level)
  int m_role_oid;
private:
  sql_stream& stream_fields(sql_stream&);
  static int m_current_user_id;
};


class users_repository
{
public:
  static std::map<int,QString> m_users_map;
  static int m_users_map_fetched;
  static void fetch();
  static void reset();
  static QString name(int id);
  static QList<user> get_list();
};

/* Privilege to do a database action (select,update,delete,execute...)
   on a database object. */
class db_obj_privilege
{
public:
  db_obj_privilege();
  db_obj_privilege(QString objtype, QString objname, QString privtype);
  /* object types: table, function */
  QString m_objtype;
  QString m_objname;
  /* privilege types: select, update, delete, execute */
  QString m_privtype;
  static QList<db_obj_privilege> ability_privileges(const QString ability,
						    db_ctxt* dbc);
};

class db_role
{
public:
  db_role(int oid);
  ~db_role();

  static db_role current_role(db_ctxt* dbc);
  QString name();
  bool is_superuser() const { return m_is_superuser; }

  bool has_privilege(QString object_type,
		     QString object_name,
		     QString privilege,
		     db_ctxt* dbc=NULL);
  bool has_multiple_privileges(QList<db_obj_privilege>, db_ctxt* dbc=NULL);
  bool can_execute(QString func_proto, db_ctxt* dbc=NULL);
  bool can_update_table(QString table_name, db_ctxt* dbc=NULL);
  bool grant_execute_function(QString func_name, db_ctxt* dbc=NULL);
  bool revoke_execute_function(QString func_name, db_ctxt* dbc=NULL);
  bool set_execute_priv_function(bool set, QString func_name, db_ctxt* dbc=NULL);
  bool grant_table_privilege(QString table_name, QString privilege, db_ctxt* dbc=NULL);
  bool revoke_table_privilege(QString table_name, QString privilege, db_ctxt* dbc=NULL);

  bool rename(QString new_name, db_ctxt* dbc);
  bool fetch_properties(db_ctxt* dbc);
  bool grant(const db_obj_privilege, db_ctxt*);
  bool revoke(const db_obj_privilege, db_ctxt*);
private:
  // grant or revoke depending on 'set'
  bool set_table_priv(bool set, QString tablename, QString privilege, db_ctxt* dbc=NULL);

  int m_oid;
  bool m_is_superuser;
  QString m_name;
};


#endif // INC_USERS_H
