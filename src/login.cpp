/* Copyright (C) 2004-2017 Daniel Verite

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

#include "login.h"
#include "main.h"
#include "db.h"
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStringList>
#include <QSet>
#include <QSettings>

#include "helper.h"
#include "icons.h"

login_dialog::login_dialog() : QDialog(0)
{
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  QGridLayout* grid = new QGridLayout();
  top_layout->addLayout(grid);

  int row=0;
  grid->addWidget(new QLabel(tr("Database name:"), this), row, 0);
  m_dbname = new QComboBox(this);
  m_dbname->setEditable(true);
  grid->addWidget(m_dbname, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Login:"), this), row, 0);
  m_login = new QLineEdit(this);
  grid->addWidget(m_login, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Password:"), this), row, 0);
  m_password = new QLineEdit(this);
  m_password->setEchoMode(QLineEdit::Password);
  grid->addWidget(m_password, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Host:"), this), row, 0);
  m_host = new QLineEdit(this);
  grid->addWidget(m_host, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("More parameters:"), this), row, 0);
  m_params = new QLineEdit(this);
  grid->addWidget(m_params, row, 1);
  row++;

  m_tls = new QCheckBox(tr("Encrypted session"));
  m_tls->setTristate();
  m_tls->setCheckState(Qt::Checked);
  grid->addWidget(m_tls, row, 1);

  QHBoxLayout* hbox = new QHBoxLayout();
  top_layout->addLayout(hbox);

  hbox->setMargin(10);
  hbox->setSpacing(20);
  QPushButton* wok = new QPushButton(tr("OK"), this);
  QPushButton* whelp = new QPushButton(tr("Help"), this);
  QPushButton* wcancel = new QPushButton(tr("Cancel"), this);
  hbox->addWidget(wok);
  hbox->addWidget(whelp);
  hbox->addWidget(wcancel);
  connect(wok, SIGNAL(clicked()), this, SLOT(ok()));
  connect(wcancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(whelp, SIGNAL(clicked()), this, SLOT(help()));

  setWindowTitle(tr("Connect to a manitou database"));
  setWindowIcon(UI_ICON(ICON16_DISCONNECT));

  init_settings();
  set_focus();
}

login_dialog::~login_dialog()
{
}

void
login_dialog::init_settings()
{
  QSettings settings("Manitou-Mail", "manitou-ui");
  set_login(settings.value("login").toString());
  set_dbname(settings.value("dbname").toString());
  set_host(settings.value("host").toString());
  set_params(settings.value("params").toString());
  if (settings.contains("tls")) {
    bool ok=false;
    // serialized tri-state value corresponding to Qt::CheckState
    int tls_state = settings.value("tls").toInt(&ok);
    if (ok && tls_state>=0 && tls_state<=2)
      set_tls((Qt::CheckState)tls_state);
  }
}

void
login_dialog::ok()
{
  db_connect();
}

void
login_dialog::help()
{
  helper::show_help("connecting");
}


QString
login_dialog::connect_string()
{
  QString res;
  if (!m_dbname->currentText().isEmpty()) {
    res.append("dbname=" + quote_connect_param(m_dbname->currentText()));
  }
  if (!m_login->text().isEmpty()) {
    res.append(" user=" + quote_connect_param(m_login->text()));
  }
  if (!m_password->text().isEmpty()) {
    res.append(" password=" + quote_connect_param(m_password->text()));
  }
  if (!m_host->text().isEmpty()) {
    res.append(" host=" + quote_connect_param(m_host->text()));
  }
  res.append(" application_name=manitou");
  if (!m_params->text().isEmpty()) {
    res.append(" " + m_params->text());
  }
  if (m_tls->checkState()==Qt::Checked)
    res.append(" sslmode=require");
  else if (m_tls->checkState()==Qt::Unchecked)
    res.append(" sslmode=disable");
  /* if Qt::PartiallyChecked, do not specify sslmode. The user can
     direct it through m_params or PGSSLMODE */
  return res.trimmed();
}

QString
login_dialog::quote_connect_param(QString param)
{
  /*
    From postgres doc: To write an empty value, or a value containing
    spaces, surround it with single quotes, e.g., keyword = 'a
    value'. Single quotes and backslashes within the value must be
    escaped with a backslash, i.e., \' and \\
  */
  if (param.isEmpty() || param.contains(' ') || param.contains('\'')) {
    return QString("'%1'").arg(param.replace("\\", "\\\\").replace("'", "\\'"));
  }
  else
    return param;
}

void
login_dialog::set_login(const QString login)
{
  m_login->setText(login);
}

// list of values separated by ';'
void
login_dialog::set_dbname(const QString dbname)
{
  if (!dbname.isEmpty()) {
    QStringList list = dbname.split(";", QString::SkipEmptyParts);
    for (QStringList::const_iterator i=list.begin(); i!=list.end(); ++i) {
      m_dbname->addItem(*i);
    }
  }
}

void
login_dialog::set_params(const QString params)
{
  m_params->setText(params);
}

void
login_dialog::set_host(const QString host)
{
  m_host->setText(host);
}

void
login_dialog::set_tls(Qt::CheckState state)
{
  m_tls->setCheckState(state);
}

// focus on the password if login is set
void
login_dialog::set_focus()
{
  if (!m_login->text().isEmpty()) {
    m_password->setFocus();
  }
}

QString
login_dialog::host() const
{
  return m_host->text();
}

// returns a list of unique values separated by ';', with the currently
// selected being the first of the list
QString
login_dialog::dbnames() const
{
  QString n = m_dbname->currentText();
  QSet<QString> set;
  set.insert(n);
  for (int i=0; i<m_dbname->count(); i++) {
    if (!set.contains(m_dbname->itemText(i))) {
      n.append(';');
      n.append(m_dbname->itemText(i));
      set.insert(m_dbname->itemText(i));
    }
  }    
  return n;
}

QString
login_dialog::login() const
{
  return m_login->text();
}
QString
login_dialog::params() const
{
  return m_params->text();
}

void
login_dialog::db_connect()
{
  QString errstr;
  QString qcs=connect_string();
  if (!ConnectDb(qcs.toLocal8Bit(), &errstr)) {
    QMessageBox::critical(NULL, QObject::tr("Fatal database error"), QObject::tr("Error while connecting to the database:\n")+errstr);
  }
  else {
    QSettings settings("Manitou-Mail", "manitou-ui");
    settings.setValue("login", login());
    settings.setValue("dbname", dbnames()); // stringlist
    settings.setValue("host", host());
    settings.setValue("params", params());
    settings.setValue("tls", m_tls->checkState());
    accept();
  }
}
