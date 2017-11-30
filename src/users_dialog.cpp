/* Copyright (C) 2016-2017 Daniel Verite

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

/* TODO:
   - users: allows sort/reverse sort on login and name ?
   - groups: add clickable list of users belonging to group
   - add warning msgbox on open if profile is not allowed to administrate
   - edit user: add "Suppress" button with warning to remove account
 */

#include "main.h"
#include "users_dialog.h"
#include "users.h"
#include "identities.h"

#ifdef SHOW_USERS_GRID
#include "pivot_table.h"
#endif

#include <QTreeWidget>
#include <QHeaderView>
#include <QLayout>
#include <QLabel>
#include <QFormLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QFontMetrics>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSizePolicy>
#include <QListWidget>
#include <QCheckBox>

//
// custom_user_field
//
custom_user_field::custom_user_field()
{
  QFontMetrics m(font()) ;
  setFixedHeight(m.lineSpacing()*4); // 4 visible lines
  setMinimumWidth(50*m.width("a")); // ~70 characters
  setLineWrapMode(QPlainTextEdit::NoWrap);
  setTabChangesFocus(true);
}

//
// field_follow_size
//
field_follow_size::field_follow_size()
{

}

void
field_follow_size::resizeEvent(QResizeEvent* event)
{
  emit resized(event->size());
}

void
field_follow_size::follow_resize(const QSize sz)
{
  this->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
  this->setMaximumSize(sz);
}



//
// user_edit_dialog
//
user_edit_dialog::user_edit_dialog(int role_oid)
{
  m_role_oid = role_oid;
  setWindowTitle(role_oid==0?tr("Add a database user"):tr("Edit a database user"));
  m_mode = (role_oid==0)?new_user:existing_user;

  user u;
  if (m_mode == existing_user)
    u.fetch_by_oid(role_oid);

  QVBoxLayout* top_layout = new QVBoxLayout;

  db_cnx db;
  QString dbname=QString(tr("Current database: <b>%1</b>")).arg(db.dbname().toHtmlEscaped());
  QLabel* ldb = new QLabel(dbname);
  ldb->setTextFormat(Qt::RichText);
  top_layout->addWidget(ldb, 0, Qt::AlignHCenter);

  QFormLayout* layout = new QFormLayout;
  m_fullname = new QLineEdit;
#ifdef PERM_LOGIN
  m_perm_login = new QCheckBox;
#endif
  m_perm_connect = new QCheckBox;
  m_registered = new QCheckBox;

  m_email = new QLineEdit;

  /* m_login, m_password and m_password2 should keep the same width
     even though m_password is part of a QHBoxLayout and the others
     are not (as they're tied to the outer QFormLayout). This is why
     they're implemented as instance of the "field_follow_size" class */

  m_login = new field_follow_size; //QLineEdit;
  m_login->setMaxLength(name_maxlength);
  m_password = new field_follow_size;
  m_password2 = new field_follow_size;
  connect(m_password, SIGNAL(resized(const QSize)),
	  m_password2, SLOT(follow_resize(const QSize)));
  connect(m_password, SIGNAL(resized(const QSize)),
	  m_login, SLOT(follow_resize(const QSize)));
  m_change_password_btn = new QCheckBox(tr("Change"));
  m_password->setEchoMode(QLineEdit::Password);
  m_password2->setEchoMode(QLineEdit::Password);
  m_password->setMaxLength(name_maxlength);
  m_password2->setMaxLength(name_maxlength);

  QHBoxLayout* vlpassw = new QHBoxLayout;
  vlpassw->addWidget(m_password);
  vlpassw->addWidget(m_change_password_btn);
  if (m_mode==existing_user) {
    m_password->setEnabled(false);
    m_password2->setEnabled(false);
    m_change_password_btn->setEnabled(true);
    connect(m_change_password_btn, SIGNAL(clicked()),
	    this, SLOT(enable_alter_user()));
  }
  else {
    m_change_password_btn->setEnabled(false);
  }

  m_qlist_roles = new QListWidget;

  QList<user> roles_list = users_repository::get_list();

  QList<QString> assigned_roles = user::granted_roles(m_role_oid);

  for (int ri=0; ri<roles_list.size(); ri++) {
    const user& u = roles_list.at(ri);
    if (!u.m_can_login && u.m_role_oid>0) {
      // keep only entries from pg_roles without the LOGIN privilege
      // they're supposed to be roles to assign rather than users
      QListWidgetItem* item = new QListWidgetItem(u.m_db_login, m_qlist_roles);
      item->setFlags(Qt::ItemIsUserCheckable|/*Qt::ItemIsSelectable|*/Qt::ItemIsEnabled);
      item->setCheckState(assigned_roles.indexOf(u.m_db_login) >= 0 ?
			  Qt::Checked : Qt::Unchecked);
    }
  }
  //  m_case_sensitive = new QCheckBox;
  m_custom1 = new custom_user_field;
  m_custom2 = new custom_user_field;
  m_custom3 = new custom_user_field;

  if (u.m_is_superuser) {
    QLabel* label = new QLabel(tr("<b>Superuser account: unrestricted permissions.</b>"));
    layout->addRow(QString(), label);
  }

  layout->addRow(tr("Login <sup>(*)</sup>:"), m_login);

  layout->addRow(tr("Password <sup>(*)</sup>:"), /*m_password*/ vlpassw);
  layout->addRow(tr("Retype password <sup>(*)</sup>:"), m_password2);
  /*
  m_password2->resize(QSize(m_password->width(), m_password2->height()));
  m_password2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
  */
#ifdef PERM_LOGIN
  add_field_with_help(layout,
		       tr("Active <sup>(*)</sup>:"),
		       m_perm_login,
		       tr("The database role has the LOGIN capability."));
#endif

  add_field_with_help(layout,
		      tr("Can connect <sup>(**)</sup>:"),
		      m_perm_connect,
		      tr("The login has CONNECT permission on this database."));

  add_field_with_help(layout,
		      tr("Registered <sup>(**)</sup>:"),
		      m_registered,
		      tr("The user account corresponds to an operator in this database."));

  layout->addRow(tr("Operator name <sup>(*)</sup>:"), m_fullname);

  layout->addRow(tr("Groups <sup>(*)</sup>:"), m_qlist_roles);

  layout->addRow(tr("Custom field #1 <sup>(**)</sup>:"), m_custom1);
  layout->addRow(tr("Custom field #2 <sup>(**)</sup>:"), m_custom2);
  layout->addRow(tr("Custom field #3 <sup>(**)</sup>:"), m_custom3);

  top_layout->addLayout(layout);

  QLabel* lremark = new QLabel(tr("(*)  Fields marked with <sup>(*)</sup> apply across all databases of this server.<br>(**) Fields marked with <sup>(**)</sup> apply only to the current database: <b>%1</b>.").arg(db.dbname().toHtmlEscaped()));
  QFont smf = lremark->font();
  smf.setPointSize((smf.pointSize()*8)/10);
  lremark->setFont(smf);
  top_layout->addWidget(lremark);

  m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
				   QDialogButtonBox::Cancel);
  top_layout->addWidget(m_buttons);
  connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));

  setLayout(top_layout);

  m_user_id = 0;

  if (role_oid!=0) {
    /* set readOnly but not disabled because disabled is hard to read
       and the login is the major information */
    m_login->setReadOnly(true); //m_login->setEnabled(false);

    // u is loaded (for an existing role) at the top of the function
    m_user_id = u.m_user_id;
    m_fullname->setText(u.m_fullname);
    m_login->setText(u.m_login);
    m_initial_login = u.m_login;
    m_custom1->setPlainText(u.m_custom_field1);
    m_custom2->setPlainText(u.m_custom_field2);
    m_custom3->setPlainText(u.m_custom_field3);
#ifdef PERM_LOGIN
    m_perm_login->setChecked(u.m_can_login);
#endif
    m_perm_connect->setChecked(u.m_can_connect);
    m_registered->setChecked(u.m_user_id!=0);
  }

  connect(m_registered, SIGNAL(clicked()), this, SLOT(enable_fields()));
  enable_fields();
}

void
user_edit_dialog::enable_fields()
{
  bool b = m_registered->isChecked();
  m_fullname->setEnabled(b);
  m_custom1->setEnabled(b);
  m_custom2->setEnabled(b);
  m_custom3->setEnabled(b);
}

void
user_edit_dialog::enable_alter_user()
{
  bool b = m_change_password_btn->isChecked();
  m_login->setReadOnly(!b); //   m_login->setEnabled(b);
  m_password->setEnabled(b);
  m_password2->setEnabled(b);
  if (b)
    m_password->setFocus();
}

void
user_edit_dialog::add_field_with_help(QFormLayout* layout,
				      const QString title,
				      QWidget* field,
				      const QString help_text)
{
  QLabel* label = new QLabel(help_text);
  QFont smf = label->font();
  smf.setPointSize((smf.pointSize()*8)/10);
  label->setFont(smf);
  label->setAlignment(Qt::AlignLeft);
  QHBoxLayout* hb = new QHBoxLayout();
  hb->addWidget(field);
  hb->addWidget(label, 1, Qt::AlignLeft|Qt::AlignVCenter);

  layout->addRow(title, hb);
}


user_edit_dialog::~user_edit_dialog()
{
}

bool
user_edit_dialog::check_passwords(QString& errstr)
{
  errstr="";
  QString p1 = m_password->text().trimmed();
  QString p2 = m_password2->text().trimmed();
  if (!p1.isEmpty() || !p2.isEmpty()) {
    if (p1 != p2) {
      errstr = tr("The passwords don't match.");
      return false;
    }
  }
  if (m_change_password_btn->isChecked()) {
    if (p1.isEmpty()) {
      errstr = tr("The new password cannot be empty. Uncheck the Change button to keep the old password.");
    }
  }
  return true;
}

bool
user_edit_dialog::check_valid()
{
  QString already_used = tr("The login is already used for another user or group.");
  QString errstr;
  if (m_mode == new_user) {
    if (user::oid_db_role(m_login->text(), false) > 0) {
      errstr = already_used;
    }
    else
      check_passwords(errstr);
  }
  else {
    if (m_login->text() != m_initial_login) {
      if (m_login->text().isEmpty()) {
	errstr = tr("The login cannot be empty.");
      }
      else if (m_password->text().isEmpty()) {
	errstr = tr("When changing the login, a password must be set simultaneously.");
      }
      else if (user::oid_db_role(m_login->text(), false) > 0) {
	errstr = already_used;
      }
      else
	check_passwords(errstr);
    }
    else
      check_passwords(errstr);

    if (m_registered->isChecked() && m_fullname->text().trimmed().isEmpty()) {
      errstr = tr("A registered account may not have an empty operator name.");
    }
  }

  if (!errstr.isEmpty()) {
    QMessageBox::critical(this, tr("Error"), errstr);
    return false;
  }

  return true;
}

void
user_edit_dialog::get_user_fields(user& u)
{
  u.m_user_id=m_user_id;
  u.m_role_oid=m_role_oid;
  u.m_login=m_login->text().trimmed();
  u.m_email=m_email->text();
  u.m_fullname=m_fullname->text();
  u.m_custom_field1=m_custom1->toPlainText();
  u.m_custom_field2=m_custom2->toPlainText();
  u.m_custom_field3=m_custom3->toPlainText();
#ifdef PERM_LOGIN
  u.m_can_login=m_perm_login->isChecked();
#else
  u.m_can_login=true;
#endif
  u.m_can_connect=m_perm_connect->isChecked();
}

bool
user_edit_dialog::create_new_user(user& u, db_ctxt* pdbc)
{
  if (user::create_db_user(m_login->text().trimmed(),
			   m_password->text().trimmed(),
			   u.m_can_login,
			   pdbc))
    {
      if (u.m_can_connect) {
	user::grant_connect(u.m_login, pdbc->m_db->dbname(), pdbc);
      }
      if (m_registered->isChecked())
	u.insert(pdbc);
    }
  return true;
}

bool
user_edit_dialog::update_user(user& u, db_ctxt* dbc)
{
  // expected: dbc->propagate_exceptions==true
  DBG_PRINTF(4, "update_user(u.user_id=%d, u.login=%s", u.m_user_id, u.m_login.toLocal8Bit().constData());
  bool success = true;

  if (u.m_login != m_initial_login) {
    success = user::change_login(m_initial_login, u.m_login, m_password->text(), dbc);
    if (!success)
      return false;
  }
  if (m_registered->isChecked()) {
    if (u.m_user_id==0)
      success = u.insert(dbc);
    else
      success = u.update_fields(dbc); // changed fields include the login
  }
  else {
    if (m_user_id!=0) {
      // checked before then unchecked, clear it in users
      success = u.unregister(dbc);
    }
  }

  if (success) {
    if (m_change_password_btn->isChecked() && !m_password->text().isEmpty()) {
      success = user::change_password(u.m_login, m_password->text(), dbc);
    }
    const QString dbname = dbc->m_db->dbname();
    if (u.m_can_connect) {
      DBG_PRINTF(4, "grant_connect");
      success = (success && user::grant_connect(u.m_login, dbname, dbc));
    }
    else {
      DBG_PRINTF(4, "revoke_connect");
      success = (success && user::revoke_connect(u.m_login, dbname, dbc));
    }
  }
  return success;
}

bool
user_edit_dialog::update_granted_roles(user& u, db_ctxt* dbc)
{
  // Re-read the roles from database, compare to our list and
  // run GRANT or REVOKE accordingly
  bool ok=true;
  QList<QString> assigned_roles = user::granted_roles(m_role_oid);
  const QString username = u.m_login;

  for (int i=0; i<m_qlist_roles->count(); i++) {
    QListWidgetItem* item = m_qlist_roles->item(i);
    if (item->checkState()==Qt::Checked) {
      if (assigned_roles.indexOf(item->text()) < 0) {
	// checked but not granted => grant it
	ok=user::grant_role(username, item->text(), dbc);
	if (!ok) break;
	assigned_roles.append(item->text());
      }
    }
    else {
      if (assigned_roles.indexOf(item->text()) >= 0) {
	// not checked but granted => revoke it
	ok = user::revoke_role(username, item->text(), dbc);
	if (!ok) break;
      }
    }
  }

  return ok;
}

void
user_edit_dialog::done(int r)
{
  bool success = false;
  if(QDialog::Accepted==r) {
    // ok was pressed
    if (check_valid()) {
      user u;
      get_user_fields(u);
      db_ctxt dbc;
      dbc.propagate_exceptions = true;
      try {
	dbc.m_db->begin_transaction();
	if (m_mode==new_user) {
	  success = create_new_user(u, &dbc);
	}
	else
	  success = update_user(u, &dbc);
	if (success)
	  success = update_granted_roles(u, &dbc);
	dbc.m_db->commit_transaction();
      }
      catch(db_excpt& p) {
	DBG_PRINTF(3, "exception caugth");
	dbc.m_db->rollback_transaction();
	success = false;
	DBEXCPT (p);
      }
      if (success)
	QDialog::done(r);
    }
  }
  else {
    QDialog::done(r);
  }
}


// role_oid => list of identities that this role can access
QMap<int,bool>
role_perms_edit_dialog::accessible_identities(Oid role_oid, db_ctxt* dbc)
{
  db_cnx* db = dbc->m_db;
  QMap<int,bool> list;
  QString query = "SELECT identity_id FROM identities_permissions WHERE role_oid=:p1";
  try {
    sql_stream s(query, *db);
    s << role_oid;
    while (!s.eos()) {
      int id;
      s >> id;
      list.insert(id,true);
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


role_perms_edit_dialog::role_perms_edit_dialog(int role_oid)
{
  m_role_oid = role_oid;
  db_ctxt dbc;

  setWindowTitle(role_oid==0?tr("Add a group"):tr("Edit group permissions"));

  QVBoxLayout* top_layout = new QVBoxLayout(this);

  db_role role(m_role_oid);
  if (role_oid > 0)
    role.fetch_properties(&dbc);

  top_layout->addWidget(new QLabel(tr("Name of role:")));

  m_role_name = new QLineEdit(role.name());
  m_role_name->setMaxLength(name_maxlength);
  top_layout->addWidget(m_role_name);

  top_layout->addWidget(new QLabel(tr("Access to restricted identities:")));
  m_list_idents = new QListWidget();
  top_layout->addWidget(m_list_idents);
  identities idents;
  idents.fetch();
  m_a_ids = accessible_identities(m_role_oid, &dbc);
  identities::const_iterator iter1;
  if (!dbc.m_db->datab()->has_row_level_security()) {
    m_list_idents->addItem(tr("Row level security is unavailable on this server."));
    m_list_idents->setEnabled(false);
  }
  else for (iter1 = idents.begin(); iter1 != idents.end(); ++iter1) {
    /* Only the restricted identities are shown in the list, since
       access control through RLS policies applies only with them.
       That may disconcert users, but showing non-restricted identities
       without the possibility of unchecking them would probably
       be worst. */
    if (!iter1->second.m_is_restricted)
      continue;
    QListWidgetItem* item = new QListWidgetItem(iter1->second.m_email_addr);
    m_list_idents->addItem(item);
    item->setCheckState(m_a_ids.contains(iter1->second.m_identity_id) ?
			Qt::Checked : Qt::Unchecked);
    item->setData(Qt::UserRole, iter1->second.m_identity_id);
  }

  if (m_role_oid > 0 && role.is_superuser()) {
    QLabel* label = new QLabel(tr("<b>This role is superuser, implying all permissions on all database objects.</b>"));
    top_layout->addWidget(label);
  }
#if 0
  m_description = new QPlainTextEdit;
  top_layout->addWidget(m_description);
#endif

  top_layout->addWidget(new QLabel(tr("Permissions:")));

  m_perm_read = new QCheckBox(tr("Read messages"));
  m_perm_update = new QCheckBox(tr("Modify messages (status)"));
  m_perm_trash = new QCheckBox(tr("Trash messages"));
  m_perm_delete = new QCheckBox(tr("Delete messages"));
  m_perm_compose = new QCheckBox(tr("Write new messages"));
  m_perm_basic_management = new QCheckBox(tr("Define tags and filters"));

  QCheckBox* tab[] = {m_perm_read, m_perm_update, m_perm_trash, m_perm_delete, m_perm_compose, m_perm_basic_management};
  for (uint i=0; i<sizeof(tab)/sizeof(tab[0]); i++) {
    top_layout->addWidget(tab[i]);
    if (role.is_superuser())
      tab[i]->setEnabled(false);
  }

  m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
				   QDialogButtonBox::Cancel);
  top_layout->addStretch(1);
  top_layout->addWidget(m_buttons);
  connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));

  set_checkboxes();
}

role_perms_edit_dialog::~role_perms_edit_dialog()
{
}

void
role_perms_edit_dialog::done(int r)
{
  if (r == QDialog::Accepted) {
    if (!m_role_oid && m_role_name->text().trimmed().isEmpty()) {
      QMessageBox::critical(this, tr("Error"), tr("The role name must not be empty."));
      return;
    }
    else
      set_grants();
  }
  QDialog::done(r);
}

void
role_perms_edit_dialog::set_grants()
{
  db_ctxt dbc(true);
  struct { QCheckBox* checkbox; const char* ability; }
  mail_perms[] = {
    {m_perm_read, "read"},
    {m_perm_update, "update"},
    {m_perm_trash, "trash"},
    {m_perm_delete, "delete"},
    {m_perm_compose, "compose"},
    {m_perm_basic_management, "admin-level1"},
  };

  QString rolname = m_role_name->text().trimmed();

  try {
    dbc.m_db->begin_transaction();
    if (!m_role_oid) {
      // Create the new role
      user::create_db_user(rolname, QString(), false, &dbc);
      m_role_oid = user::oid_db_role(rolname, true);
      if (!m_role_oid) {
	QMessageBox::critical(this, tr("Error"), tr("The role could not be created."));
	return;
      }
    }

    db_role role(m_role_oid);
    role.fetch_properties(&dbc);

    if (rolname != role.name()) {
      // Rename it in the db
      role.rename(rolname, &dbc);
    }

    for (uint iperm=0; iperm<sizeof(mail_perms)/sizeof(mail_perms[0]); iperm++) {
      // Set or unset permissions to exercise ability on messages
      DBG_PRINTF(5, "processing permissions for ability: %s", mail_perms[iperm].ability);
      QList<db_obj_privilege> privs = db_obj_privilege::ability_privileges(mail_perms[iperm].ability, &dbc);
      for (int pi=0; pi < privs.size(); pi++) {
	if (mail_perms[iperm].checkbox->isChecked())
	  role.grant(privs.at(pi), &dbc);
	else
	  role.revoke(privs.at(pi), &dbc);
      }
    }

    if (dbc.m_db->datab()->has_row_level_security()) {
      QList<int> list_ids;  // list of checked private identities
      bool ids_changed = false;	// did any checkbox switch state?
      for (int row=0; row < m_list_idents->count(); row++) {
	QListWidgetItem* item = m_list_idents->item(row);
	int iid = item->data(Qt::UserRole).toInt(); // identity_id
	if ((item->checkState() == Qt::Checked) != m_a_ids.contains(iid)) // compare 2 states
	  ids_changed = true;
	if (item->checkState() == Qt::Checked)
	  list_ids.append(iid);
      }
      if (ids_changed) {
	// call set_identity_permissions(in_oid, in_identities int[], in_perms char[] = ['A']);
	sql_stream s("select set_identity_permissions(:o, :t, null)", *dbc.m_db);
	s << m_role_oid << list_ids;
      }
    }
    dbc.m_db->commit_transaction();
  }
  catch(db_excpt& p) {
    dbc.m_db->rollback_transaction();
    DBEXCPT (p);
  }
}

void
role_perms_edit_dialog::set_checkboxes()
{
  db_ctxt dbc(true);
  try {
    if (m_role_oid > 0) {
      db_role role(m_role_oid);

      QList<db_obj_privilege> privs_for_read = db_obj_privilege::ability_privileges("read", &dbc);
      m_initial_privs[priv_read] = role.has_multiple_privileges(privs_for_read, &dbc);


      QList<db_obj_privilege> privs_for_update = db_obj_privilege::ability_privileges("update", &dbc);
      m_initial_privs[priv_update] = role.has_multiple_privileges(privs_for_update, &dbc);


      QList<db_obj_privilege> privs_for_trash = db_obj_privilege::ability_privileges("trash", &dbc);
      m_initial_privs[priv_trash] = role.has_multiple_privileges(privs_for_trash, &dbc);

      QList<db_obj_privilege> privs_for_delete = db_obj_privilege::ability_privileges("delete", &dbc);
      m_initial_privs[priv_delete] = role.has_multiple_privileges(privs_for_delete, &dbc);

      QList<db_obj_privilege> privs_for_compose = db_obj_privilege::ability_privileges("compose", &dbc);
      m_initial_privs[priv_compose] = role.has_multiple_privileges(privs_for_compose, &dbc);


      QList<db_obj_privilege> privs_for_basic_management = db_obj_privilege::ability_privileges("admin-level1", &dbc);
      m_initial_privs[priv_basic_management] = role.has_multiple_privileges(privs_for_basic_management, &dbc);
    }
    else {
      memset((void*)m_initial_privs, 0, sizeof(m_initial_privs));
    }

    m_perm_read->setChecked(m_initial_privs[priv_read]);
    m_perm_update->setChecked(m_initial_privs[priv_update]);
    m_perm_trash->setChecked(m_initial_privs[priv_trash]);
    m_perm_delete->setChecked(m_initial_privs[priv_delete]);
    m_perm_compose->setChecked(m_initial_privs[priv_compose]);
    m_perm_basic_management->setChecked(m_initial_privs[priv_basic_management]);

  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
}



///////////////////////////////////////////////////////////////
users_dialog::users_dialog(QWidget* parent): QWidget(parent)
{
  setWindowTitle(tr("Database users"));
  QVBoxLayout* layout = new QVBoxLayout(this);

  db_cnx db;
  QString dbname = QString(tr("Current database: <b>%1</b>")).arg(db.dbname().toHtmlEscaped());
  QLabel* ldb = new QLabel(dbname);
  ldb->setTextFormat(Qt::RichText);
  layout->addWidget(ldb, 0, Qt::AlignHCenter);

  m_tab = new QTabWidget();
  layout->addWidget(m_tab);

  m_ulist = new QTreeWidget;
  m_ulist->setMinimumHeight(150);

  connect(m_ulist, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	  this, SLOT(edit_user()));


  m_tab->addTab(m_ulist, tr("Users"));

  m_glist = new QTreeWidget;
  m_tab->addTab(m_glist, tr("Groups"));
  connect(m_glist, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	  this, SLOT(edit_user()));

#ifdef SHOW_USERS_GRID
  m_synth = new pivot_table;
  m_synth->init("SELECT r1.rolname AS username, r.rolname AS groupname, chr(10004)"
		" FROM pg_auth_members m"
		" RIGHT JOIN pg_roles r ON m.roleid = r.oid"
		" RIGHT JOIN pg_roles r1 ON m.member = r1.oid"
		" WHERE r1.rolcanlogin",
		pivot_table::order_both);
  m_tab->addTab(m_synth, tr("Tabular view"));
#endif

  QHBoxLayout* btnl = new QHBoxLayout;
  layout->addLayout(btnl);

  QPushButton* btn_add = new QPushButton(tr("&Add"));
  QPushButton* btn_edit = new QPushButton(tr("&Edit"));
  QPushButton* btn_close = new QPushButton(tr("&Close"));
  /*  QPushButton* btn_remove = new QPushButton(tr("&Disable"));*/

  connect(btn_add, SIGNAL(clicked()), this, SLOT(add_user()));
  connect(btn_edit, SIGNAL(clicked()), this, SLOT(edit_user()));
  /*  connect(btn_remove, SIGNAL(clicked()), this, SLOT(remove_user()));*/
  connect(btn_close, SIGNAL(clicked()), this, SLOT(close()));

  if (!user::can_create_user())
    btn_add->setEnabled(false);

  btnl->addWidget(btn_add);
  btnl->addWidget(btn_edit);
  /*  btnl->addWidget(btn_remove);*/
  btnl->addStretch(3);
  btnl->addWidget(btn_close);

  init();
}

users_dialog::~users_dialog()
{
}

void
users_dialog::init()
{
  m_user_list = users_repository::get_list();
  m_ulist->setHeaderLabels(QStringList() << tr("Login") << tr("Can connect") << tr("Operator name"));
  QTreeWidgetItem* h = m_ulist->headerItem();
  h->setToolTip(0, tr("Database login"));
  h->setToolTip(1, tr("Checked if allowed to connect to this database"));
  h->setToolTip(2, tr("Name as a Manitou-Mail operator in this database"));
  QFontMetrics m(m_ulist->font());
  m_ulist->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  setMinimumWidth(m.width("a")*60);

  refresh_list();
  refresh_role_list();
  m_ulist->adjustSize();

  m_glist->setHeaderLabels(QStringList() << tr("Role"));
}

int
users_dialog::selected_role_id(QTreeWidget* widget)
{
  QList<QTreeWidgetItem*> sel = widget->selectedItems();
  if (sel.size()==1) {
    return sel.at(0)->data(0, UserIdRole).toInt();
  }
  return 0;
}

/* Redraw the groups list and re-select the previously selected entry if any */
void
users_dialog::refresh_role_list()
{
  int id = selected_role_id(m_ulist);
  m_glist->clear();
  m_user_list = users_repository::get_list();

  for (QList<user>::iterator it = m_user_list.begin();
       it != m_user_list.end(); ++it)
    {
      if (it->m_can_login)
	continue;
      QTreeWidgetItem* item = new QTreeWidgetItem(m_glist);
      item->setData(0, UserIdRole, it->m_role_oid);
      item->setText(0, it->m_db_login);
    }

  if (id) {
    QTreeWidgetItem* item = find_user_item(id, m_glist);
    if (item)
      m_ulist->setCurrentItem(item);
  }
}

/* Redraw the users list and re-select the previously selected entry if any */
void
users_dialog::refresh_list()
{
  int id = selected_role_id(m_ulist);
  m_ulist->clear();

  m_user_list = users_repository::get_list();

  for (QList<user>::iterator it = m_user_list.begin();
       it != m_user_list.end(); ++it)
    {
      if (!it->m_can_login)
	continue;
      QString perms, tip;
      QTreeWidgetItem* item = new QTreeWidgetItem(m_ulist);
      item->setText(0, it->m_db_login);

      QString yes = QString(QChar(0x2611))+" ";
      QString no = QString(QChar(0x2610))+" ";
#ifdef PERM_LOGIN
      perms.append(it->m_can_login?yes:no);
      tip.append(it->m_can_login?
		 yes+tr("Has the LOGIN capability"):
		 no+tr("Does not have the LOGIN capability"));
      tip.append("\n");
#endif
      perms.append(it->m_can_connect?yes:no);
      tip.append(it->m_can_connect?
		 yes+tr("Allowed to connect to this database"):
		 no+tr("Not allowed to connect to this database"));
      tip.append("\n");
#if 0
      perms.append(!it->m_login.isEmpty()?yes:no);
      tip.append(!it->m_login.isEmpty()?
		 yes+tr("Registered as operator in this database"):
		 no+tr("Not registered as operator in this database"));
      // it->m_can_login ? QString(QChar(0x2713)):"");
#endif
      item->setText(1, perms);
      item->setToolTip(1, tip);

      item->setText(2, it->m_fullname);
      //      item->setText(2, it->m_can_login ? tr("Yes"):tr("No"));

      item->setData(0, UserIdRole, it->m_role_oid);
    }

  if (id) {
    QTreeWidgetItem* item = find_user_item(id, m_ulist);
    if (item)
      m_ulist->setCurrentItem(item);
  }
}

QTreeWidgetItem*
users_dialog::find_user_item(int user_id, QTreeWidget* tree)
{
  QTreeWidgetItemIterator it(tree);
  while (*it) {
    if ((*it)->data(0, UserIdRole).toInt()==user_id)
      return *it;
    ++it;
  }
  return NULL;
}

void
users_dialog::add_user()
{
  switch (m_tab->currentIndex()) {
  case 0:
    {
      user_edit_dialog dlg;
      if (dlg.exec() == QDialog::Accepted) {
	refresh_list();
	refresh_role_list();
      }
    }
    break;
  case 1:
  default:
    {
      role_perms_edit_dialog dlg;
      if (dlg.exec() == QDialog::Accepted) {
	refresh_list();
	refresh_role_list();
      }
    }
    break;
  }
}

void
users_dialog::edit_user()
{
  switch (m_tab->currentIndex()) {
  case 0:
    {
      int id = selected_role_id(m_ulist);
      if (id) {
	user_edit_dialog dlg(id);
	if (dlg.exec() == QDialog::Accepted) {
	  refresh_list();
	  refresh_role_list();
	}
      }
    }
    break;
  case 1:
  default:
    {
      int id = selected_role_id(m_glist);
      if (id) {
	role_perms_edit_dialog dlg(id);
	if (dlg.exec() == QDialog::Accepted) {
	  refresh_list();
	  refresh_role_list();
	}
      }
    }
    break;
  }
}

#if 0
void
users_dialog::remove_user()
{
  // Toggle the user's login/nologin capability
}
#endif

void
open_users_dialog()
{
  users_dialog* dlg = new users_dialog(NULL);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}

/*
Users are uniquely identified by their database login.
Logins and their associated passwords are handled by PostgreSQL and shared across all databases. In parallel, each Manitou-Mail database contains a table with the names and internal identifiers of the users known to this database. Operator names are only informative, they don't have to be unique and they don't have to be deleted when a person's account is removed.

Scenarios:

- single user

- one administrator for both the databases and the application, and one group of users

- separate database and application administrators.

Removing a user:

- step 1: the association between the user account and the databases should be suppressed: uncheck the "Registered" checkbox in each database.

- step 2: Remove the permission to connect to each database: uncheck the "Can connect" checkbox in each database.

- step 3 (optional): suppress the PostgreSQL account at the instance level with DROP ROLE in SQL.

Step 1 will clear the login name from the database's users table so that another account may later refer to that user. The entry and name of the user are not deleted, as mail history may refer to it.

In postgresql, users and groups have been unified under "roles", and
"role" is now the recommended term. However in this context, we still
refer to users and groups as it's easier to understand. The
relationship between roles, users and groups is that "groups" are
roles that can't log in to postgres (they lack the LOGIN capability),
whereas "users" are roles that can log in.

*/
