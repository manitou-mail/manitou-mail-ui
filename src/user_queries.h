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

#ifndef INC_USER_QUERIES_H
#define INC_USER_QUERIES_H

#include <QDialog>
#include <QString>
#include <QTextEdit>

#include <map>

class msgs_filter;
class QLineEdit;
class QTextEdit;

class save_query_box : public QDialog
{
  Q_OBJECT
public:
  save_query_box(QWidget* parent, msgs_filter* f, int mode, const QString title);
  virtual ~save_query_box();
  QLineEdit* m_name;
  QString m_sql;
private slots:
  void check_accept();
private:
  QString m_initial_title;
  QTextEdit* m_sql_edit;
};

class user_queries_repository
{
public:
/*
  user_queries_repository() {}
  virtual ~user_queries_repository() {}
*/
  static bool fetch();			// fetch the user_queries table into m_map
  static void reset();
  static bool name_exists(const QString name);
  static bool remove_query(const QString name);

  static std::map<QString,QString> m_map;	// name => sql statement
  static bool m_uq_map_fetched;
};

#endif // INC_USER_QUERIES_H
