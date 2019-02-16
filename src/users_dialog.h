/* Copyright (C) 2004-2019 Daniel Verite

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

#ifndef USERS_DIALOG_H
#define USERS_DIALOG_H

#include <QWidget>
#include <QDialog>
#include <QMap>
#include <QLineEdit>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPlainTextEdit>

#include "users.h"

// #define PERM_LOGIN
// #define SHOW_USERS_GRID

class QTreeWidget;
class QTreeWidgetItem;
class QFormLayout;
class QListWidget;
class QTabWidget;
class QResizeEvent;
class QListWidget;
class QCheckBox;

#ifdef SHOW_USERS_GRID
class pivot_table;
#endif

class custom_user_field: public QPlainTextEdit
{
public:
  custom_user_field();
};

/* Password field in a pair of passwords that
   keep the same size */
class field_follow_size: public QLineEdit
{
  Q_OBJECT
public:
  field_follow_size();
public slots:
  void follow_resize(const QSize sz);
protected:
  void resizeEvent(QResizeEvent*);
signals:
  void resized(const QSize);
};

class role_perms_edit_dialog: public QDialog
{
  Q_OBJECT
public:
  role_perms_edit_dialog(int role_oid=0); // OID from pg_roles
  ~role_perms_edit_dialog();

  enum privileges {
    priv_read,
    priv_update,
    priv_trash,
    priv_delete,
    priv_compose,
    priv_basic_management,
    priv_max
  };
private:
  bool m_initial_privs[priv_max];

  int m_role_oid;
  QMap<int,bool> m_a_ids;

  QLineEdit* m_role_name;
  QListWidget* m_list_idents;

  static const int name_maxlength=64;

  QPlainTextEdit* m_description;

  // SELECT on mail
  QCheckBox* m_perm_read;
  // UPDATE on mail
  QCheckBox* m_perm_update;
  // trash  mail (not delete)
  QCheckBox* m_perm_trash;
  // DELETE on mail
  QCheckBox* m_perm_delete;
  // INSERT ON mail
  QCheckBox* m_perm_compose;
  // INSERT ON tags
  QCheckBox* m_perm_basic_management;

  QDialogButtonBox* m_buttons;
  void set_grants();
  QMap<int,bool> accessible_identities(Oid, db_ctxt*);

private slots:
  void set_checkboxes();
public slots:
  void done(int);
};

class user_edit_dialog: public QDialog
{
  Q_OBJECT
public:
  user_edit_dialog(int role_oid=0); // OID from pg_roles
  ~user_edit_dialog();
private:
  QLineEdit* m_fullname;
  field_follow_size* m_login; //  QLineEdit
  field_follow_size* m_password;
  field_follow_size* m_password2;
  QCheckBox* m_change_password_btn;
  QLineEdit* m_email;
  QCheckBox* m_case_sensitive;

#ifdef PERM_LOGIN
  QCheckBox* m_perm_login;	// db account has LOGIN permission
#endif
  QCheckBox* m_perm_connect;	// db account has the CONNECT grant to the db
  QCheckBox* m_registered;	// login exists in users tables

  custom_user_field* m_custom1;
  custom_user_field* m_custom2;
  custom_user_field* m_custom3;
  QDialogButtonBox* m_buttons;
  QListWidget* m_qlist_roles;

  int m_user_id;		// users.user_id, per database
  int m_role_oid;		// pg_roles.oid, per PG instance
  QString m_initial_login;
private:
  void add_field_with_help(QFormLayout* layout,
			   const QString title,
			   QWidget* field,
			   const QString help_text);
  static const int name_maxlength=64;
  bool check_valid();
  bool check_passwords(QString&);

  enum {
    new_user,
    existing_user
  } m_mode;

  void get_user_fields(user& u);
  bool create_new_user(user&, db_ctxt*);
  bool update_user(user&, db_ctxt*);
  bool update_granted_roles(user&, db_ctxt*);
public slots:
  void done(int);
  void enable_fields();
  void enable_alter_user();
};



class users_dialog: public QWidget
{
  Q_OBJECT
public:
  users_dialog(QWidget* parent=NULL);
  ~users_dialog();
  static const int UserIdRole = Qt::UserRole;
private slots:
  void add_user();
  void edit_user();
#if 0
  void remove_user();
#endif
private:
  QTreeWidgetItem* find_user_item(int,QTreeWidget*);
  int selected_role_id(QTreeWidget*); // OID from pg_roles
  void init();
  void refresh_list();
  void refresh_role_list();
  QTabWidget* m_tab;
  QTreeWidget* m_ulist;
  QTreeWidget* m_glist; // group permissions
#ifdef SHOW_USERS_GRID
  pivot_table* m_synth;
#endif
  QList<user> m_user_list;
};

#endif
