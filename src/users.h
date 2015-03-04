/* Copyright (C) 2004-2014 Daniel Verite

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

class user 
{
public:
  user() {}
  virtual ~user() {}
  QString name(int user_id);
  static int current_user_id();
  static bool create_if_missing(const QString fullname);
  bool update_fields();
  bool fetch(int user_id);

  static bool create_db_user(const QString login, const QString password);
  static bool check_db_role(const QString login, bool case_sensitive);

  QString m_fullname;
  int m_user_id;
  QString m_login;
  QString m_db_login;
  QString m_email;
  QString m_custom_field1;
  QString m_custom_field2;
  QString m_custom_field3;
private:
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

#endif // INC_USERS_H
