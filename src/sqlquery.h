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

#ifndef INC_SQLQUERY_H
#define INC_SQLQUERY_H

#include <QString>
#include <set>
#include "database.h"

class sql_null
{
 public:
  sql_null() {}
  ~sql_null() {}
private:
  int m_placeholder;		/* avoid a MSVC++ bug with empty classes */
};

class sql_query 
{
public:
  sql_query(): m_num_clauses(0),m_num_tables(0) {}
  virtual ~sql_query() {}
  void start(const QString sql);
  void add_clause(const QString field, const QString value);
  void add_clause(const QString field, const QString value, const char* cmp);
  void add_clause(const QString field, int value);
  void add_clause(const QString expr);
  int add_table(const QString tablename);
  void add_final(const QString sql);
  QString get();
  QString subquery(const QString select_list);
private:
  int m_num_clauses;
  int m_num_tables;
  QString m_where;
  QString m_from;
  QString m_end;
  QString m_start;
  QString m_tables;
  struct ltstr
  {
    bool operator()(const QString s1, const QString s2) const
    {
      return strcmp(s1.toLatin1(), s2.toLatin1()) < 0;
    }
  };
  std::set<QString,ltstr> m_tables_set;	// key=table's name
};

// Handle two strings: the list of fields and the list of their values
// Help to incrementally build a sql INSERT or UPDATE query with a variable
// number of fields assigned
class sql_write_fields
{
public:
  sql_write_fields(const db_cnx& db);
  ~sql_write_fields() {}
  void add(const char* field, int value);
  void add(const char* field, const QString value, int maxlength=0);
  void add(const char* field, sql_null n);
  void add_no_quote(const char* field, const QString value);
  void add_if_not_empty(const char* field, const QString value, int maxlength=0);
  void add_if_not_zero(const char* field, int value);
  const QString& values();
  const QString& fields();
private:
  QString m_values; // val1,val2,val3...
  QString m_fields; // field1,field2,field3...
  int m_fields_nb;
  bool m_utf8;
};

#endif // INC_SQLQUERY_H

