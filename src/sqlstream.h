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

#ifndef INC_SQLSTREAM_H
#define INC_SQLSTREAM_H

#include <vector>

#include "dbtypes.h"
#include "database.h"
#include "sqlquery.h"
#include <QString>

/// sql_bind_param class. To be used for sql_stream internal purposes
class sql_bind_param
{
public:
  sql_bind_param() {}
  sql_bind_param(const std::string s, int pos) {
    m_name=s;
    m_initialOffsetInQuery=m_offsetInQuery=pos;
  }
  virtual ~sql_bind_param() {}
  void offset(int off) {
    m_offsetInQuery+=off;
  }
  void resetOffset() {
    m_offsetInQuery=m_initialOffsetInQuery;
  }
  const std::string name() const { return m_name; }
  int pos() const { return m_offsetInQuery; }
private:
  std::string m_name;
  std::string m_value;
  int m_offsetInQuery;		/* position of the ':' character in query */
  int m_initialOffsetInQuery;
};

/**
   sql_stream class. Allows the parametrized execution of a query
   and easy retrieval of results
*/
class sql_stream
{
public:
  /// constructor
  sql_stream(const char* query, db_cnx& db, bool auto_exec=true);
  sql_stream(const QString query, db_cnx& db, bool auto_exec=true);
  /// destructor
  virtual ~sql_stream();
  /// assign a char* parameter
  sql_stream& operator<<(const char*);
  sql_stream& operator<<(const QString&);
  sql_stream& operator<<(int);
  sql_stream& operator<<(char);
  sql_stream& operator<<(unsigned int);
  sql_stream& operator<<(long);
  sql_stream& operator<<(unsigned long);
  sql_stream& operator<<(short);
  sql_stream& operator<<(unsigned short);
  sql_stream& operator<<(float);
  sql_stream& operator<<(quint64);
  sql_stream& operator<<(bool);
  sql_stream& operator<<(sql_null);

  sql_stream& operator>>(int&);
  sql_stream& operator>>(unsigned int&);
  sql_stream& operator>>(bool&);
  sql_stream& operator>>(char*);
  sql_stream& operator>>(char&);
  sql_stream& operator>>(float&);
  sql_stream& operator>>(quint64&);
  sql_stream& operator>>(double&);
  sql_stream& operator>>(QString&);

  void print();

  /** send the query to the server */
  void execute();

  /** set results pointer to first row */
  void rewind();

  /** returns true if there are no more results to read from the stream,
      or false otherwise */
  int eof();
  int eos() { return eof(); }
  bool val_is_null() const {
    return m_val_null;
  }
  int affected_rows() const {
    return m_affected_rows;
  }
  int row_count() const;
private:
  void init(const char* query);
  void check_binds();
  void reset_results();
  void next_result();
  void check_eof();
  void query_make_space(int len);
  void replace_placeholder(int argPos, const char* buf, int size);
  void next_bind();

  db_cnx& m_db;
  int m_nArgPos;
  char* m_queryBuf;
  int m_queryBufSize;
  int m_queryLen;
  int m_initialQueryLen;
  char m_localQueryBuf[1024+1];
  std::string m_queryFmt;
  int m_chunk_size;
  std::vector<sql_bind_param> m_vars;
  // results
  int m_bExecuted;
  PGresult* m_pgRes;
  int m_rowNumber;		/* last row read from the stream */
  int m_colNumber;		/* last column read from the stream */
  bool m_val_null;
  int m_affected_rows;
  bool m_auto_exec;
};

#endif // INC_SQLSTREAM_H
