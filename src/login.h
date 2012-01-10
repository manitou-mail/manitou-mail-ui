/* Copyright (C) 2004-2008 Daniel Vérité

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

#ifndef INC_LOGIN_H
#define INC_LOGIN_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QComboBox;

class login_dialog: public QDialog
{
  Q_OBJECT
public:
  login_dialog();
  virtual ~login_dialog();
  QString connect_string();
  void set_login(const QString login);
  void set_dbname(const QString dbname); // list of values separated by ';'
  void set_params(const QString params);
  void set_host(const QString host);
  void set_focus(); // focus on the password if login is set
  QString login() const;
  QString dbnames() const;
  QString host() const;
  QString params() const;
private:
  QComboBox* m_dbname;
  QLineEdit* m_login;
  QLineEdit* m_password;
  QLineEdit* m_host;
  QLineEdit* m_params;
private slots:
  void ok();
  void help();
};

#endif
