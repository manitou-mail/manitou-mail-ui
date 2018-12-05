/* Copyright (C) 2004-2018 Daniel Verite

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

#include <string>
#include <sstream>

#include <ctype.h>
#include <stdlib.h>

#include "main.h"
#include "sqlstream.h"
#include "db.h"

sql_stream::sql_stream(const QString query, db_cnx& db, bool auto_exec) :
  m_db(db), m_auto_exec(auto_exec)
{
  QByteArray qba = query.toUtf8();
  init(qba.constData());
}

sql_stream::sql_stream(const char *query, db_cnx& db, bool auto_exec) :
  m_db(db), m_auto_exec(auto_exec)
{
  init(query);
}

void
sql_stream::parse(const char* query)
{
  const char* ptr = query;
  const char* pstart = query;		// param start. Initialized to avoid a warning.
  const char* comment_start = NULL;
  const char* errparse = NULL;
  int nested_brackets = 0;
  char c;

  int state = 1;
  int nstate = 1;

  while (!errparse) {
    c = *ptr;
    switch(state) {
      // initial state
    case 1:
      if (c == ':')
	nstate = 2;
      else if (c == '\'')
	nstate = 3;
      else if (c == '"')
	nstate = 8;
      // else stay in state 1
      break;

      // colon :
    case 2:
      if (c == '\'') {
	pstart = ptr;
	nstate = 4;
      }
      else if (sql_bind_param::is_valid_char(c)) {
	pstart = ptr;
	nstate = 5;
      }
      else if (c == ':') {	// two colons
	nstate = 10;
      }
      else
	nstate = 1;
      break;

    case 10:			// after two colons, same transitions as state 1
      if (c == '\\')
	nstate = 3;
      else if (c == '"')
	nstate = 8;
      else
	nstate = 1;
      break;

      // string literals enclosed in single quotes
    case 3:
      if (c == '\'')
	nstate = 6;
      else
	nstate = 3;
      break;

      // in :'param'
    case 4:
      if (c == '\'') {
	if (ptr-pstart-1 == 0) {
	  errparse = "Empty parameter name";
	}
	else {
	  DBG_PRINTF(8, "Adding param '%s' from pos %d",
		     std::string(pstart+1, ptr-pstart-1).c_str(),
		     pstart-query-1);
	  sql_bind_param p(std::string(pstart+1, ptr-pstart-1),
			   pstart-query-1,
			   true);
	  m_vars.push_back(p);
	}
	nstate = 1;
      }
      else if (sql_bind_param::is_valid_char(c)) {
	nstate = 4;
      }
      else {
	errparse = "Illegal character in parameter name";
      }
      break;

      // in :param (at least one letter long)
    case 5:
      if (sql_bind_param::is_valid_char(c)) {
	nstate = 5;
      }
      else {
	DBG_PRINTF(8, "Adding param '%s' from pos %d",
		   std::string(pstart, ptr-pstart).c_str(),
		   pstart-query-1);
	sql_bind_param p(std::string(pstart, ptr-pstart),
			 pstart-query-1,
			 false);
	m_vars.push_back(p);
	if (c == '{') {
	  /* the param placeholder is followed by a comment
	     (:param{this is a comment}) */
	  nested_brackets = 1;
	  comment_start = ptr;
	  nstate = 11;
	}
	else
	  nstate = 1;
      }
      break;

    case 6:			// on a single quote inside single-quoted text
      if (c == '\'')
	nstate = 3;
      else if (c == ':')
	nstate = 2;
      else if (c == '"')
	nstate = 8;
      else
	nstate = 1;
      break;

      // in double quotes
    case 8:
      if (c == '"')
	nstate = 9;
      break;

    case 9:			// on a double quote inside double-quoted text
      if (c == '"')
	nstate = 8;
      else if (c == ':')
	nstate = 2;
      else if (c == '\'')
	nstate = 3;
      else
	nstate = 1;
      break;

    case 11:
      if (c == '}') {
	if (--nested_brackets == 0) {
	  sql_bind_param& p = m_vars.back();
	  p.set_comment(std::string(comment_start+1, ptr-comment_start-1));
	  DBG_PRINTF(4, "Setting comment '%s' to var '%s'", p.comment().c_str(),
		     p.name().c_str());
	  nstate = 1;
	}
	else
	  errparse = "Unbalanced curly brackets";
      }
      else if (c == '{') {
	nested_brackets++;
      }
      break;

    } // switch(state)

    if (state != nstate)
      DBG_PRINTF(8, "At char '%c', state %d => %d", c, state, nstate);

    if (*ptr != '\0')
      ptr++;
    else
      break;
    state = nstate;
  }

  if (errparse != NULL) {
    if (*ptr == '\0' && (state == 1 || state == 5 || state == 6 || state == 9)) {
      // reached a final state at the end of string, good.
    }
    else {
      errparse = "Unexpected end of string";
    }
  }

  if (errparse != NULL) {
    throw db_excpt(QString(query),
		   QString(errparse),
		   db_excpt::client_assertion);
  }
}

void
sql_stream::init(const char *query)
{
  m_nArgPos = 0;
  m_queryBuf = m_localQueryBuf;
  m_queryBufSize = sizeof(m_localQueryBuf)-1;
  m_queryFmt = query;
  m_chunk_size = 1024;
  m_bExecuted = 0;
  m_pgRes = NULL;

  int len=strlen(query);
  if (len>m_queryBufSize)
    query_make_space(len);

  m_queryLen=m_initialQueryLen=len;
  strcpy(m_queryBuf, query);

  parse(query);

  if (m_vars.size()==0 && m_auto_exec)
    execute();
}

sql_stream::~sql_stream()
{
#if 0
  if (m_nArgPos<(int)m_vars.size()) {
    QString q(m_queryBuf);
    if (q.length()>40) { // cut the query to an reasonable size for debug output
      q.truncate(37);
      q.append("...");
    }
    DBG_PRINTF(2, "WRN: m_nArgPos=%d while m_vars.size()=%d for query '%s'", m_nArgPos, (int)m_vars.size(), q.toLocal8Bit().constData());
  }
#endif
  if (m_pgRes)
    PQclear(m_pgRes);
  if (m_queryBuf!=m_localQueryBuf)
    free(m_queryBuf);
}

void
sql_stream::reset_results()
{
  m_bExecuted=false;
  m_nArgPos=0;
  if (m_pgRes) {
    PQclear(m_pgRes);
    m_pgRes=NULL;
  }
  strcpy(m_queryBuf, m_queryFmt.c_str());
  for (unsigned int i=0; i<m_vars.size(); i++) {
    m_vars[i].resetOffset();
  }
  m_queryLen=m_initialQueryLen;
}

/*
 * Check if there is at least enough free space to add @len bytes to
 * the buffer.  If not, make it grow to fit
 */
void
sql_stream::query_make_space(int len)
{
  if (m_queryLen+len < m_queryBufSize)
    return;			// m_queryBuf is large enough
  if (m_queryBuf==m_localQueryBuf) {
    char* p=(char*)malloc(1+m_queryBufSize+len+m_chunk_size);
    if (p) {
      strcpy (p, m_queryBuf);
      m_queryBuf = p;
    }
  }
  else {
    m_queryBuf=(char*)realloc(m_queryBuf, 1+m_queryBufSize+len+m_chunk_size);
  }
  if (!m_queryBuf)
    throw std::bad_alloc();
  m_queryBufSize+=len+m_chunk_size;
  m_chunk_size<<=1;
}

/*
 * Replace the placeholder with the value.
 * @size does not include a NULL terminator.
 */
void
sql_stream::replace_placeholder(int argPos, const char* buf, int size)
{
  query_make_space(size);
  sql_bind_param& p = m_vars[argPos];
  int placeholder_len = p.total_length();
  if (p.comment().size() > 0)
    placeholder_len += p.comment().size() + 2; // {comment text}

  DBG_PRINTF(8, "placeholder_len=%d for arg=%d", placeholder_len, argPos);
  // shift the substring at the right of the placeholder
  memmove(m_queryBuf+p.pos()+size,
	  m_queryBuf+p.pos()+placeholder_len,
	  m_queryLen-(p.pos()+placeholder_len));
  // insert the value where the placeholder was
  memcpy(m_queryBuf+p.pos(), buf, size);
  m_queryLen += (size-placeholder_len);
  m_queryBuf[m_queryLen] = '\0';
  // change the offsets of the remaining placeholders
  for (unsigned int i = argPos+1; i < m_vars.size(); i++) {
    m_vars[i].add_offset(size-placeholder_len);
  }
}

void
sql_stream::next_bind()
{
  if (++m_nArgPos>=(int)m_vars.size() && m_auto_exec)
    execute();
}

sql_stream&
sql_stream::operator<<(const char* p)
{
  check_binds();
  size_t len = p ? strlen(p) : 0;
  char local_buf[1024+1];
  char* buf;
  if (len < (sizeof(local_buf)-1)/2)
    buf = local_buf;
  else
    buf = (char*)malloc(2*len+2+1);

  buf[0] = '\'';
  int escaped_size = PQescapeString(buf+1, p, len);
  buf[escaped_size+1]='\'';
  buf[escaped_size+2]='\0';

  if (p != NULL) {
    replace_placeholder(m_nArgPos, buf, escaped_size+2);
  }
  else {
    // null pointer => null sql value
    replace_placeholder(m_nArgPos, "null", 4);
  }

  if (buf!=local_buf)
    free(buf);

  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(const QString& s)
{
  QByteArray q;
  if (m_db.datab()->encoding() == "UTF8")
    q=s.toUtf8();
  else
    q=s.toLocal8Bit();
  return operator<<((const char*)q);
}

sql_stream&
sql_stream::operator<<(int i)
{
  check_binds();
  char buf[15];
  sprintf(buf,"%d", i);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(char c)
{
  check_binds();
  char buf[2];
  buf[0] = c;
  buf[1] = '\0';
  /* apply the same quoting rules for characters as for strings */
  return operator<<(buf);
}

sql_stream&
sql_stream::operator<<(unsigned int i)
{
  check_binds();
  char buf[15];
  sprintf(buf,"%u", i);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(long l)
{
  check_binds();
  char buf[15];
  sprintf(buf,"%ld", l);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(bool value)
{
  check_binds();
  replace_placeholder(m_nArgPos, (value ? "true" : "false"), (value ? 4 : 5));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(unsigned long l)
{
  check_binds();
  char buf[15];
  sprintf(buf,"%lu", l);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(quint64 l)
{
  check_binds();
  char buf[21];
  sprintf(buf,"%llu", l);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(short s)
{
  check_binds();
  char buf[15];
  sprintf(buf,"%hd", s);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(unsigned short s)
{
  check_binds();
  char buf[15];
  sprintf(buf,"%hu", s);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(float f)
{
  check_binds();
  char buf[100];
  sprintf(buf,"%g", f);
  replace_placeholder(m_nArgPos, buf, strlen(buf));
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(sql_null n _UNUSED_)
{
  check_binds();
  replace_placeholder(m_nArgPos, "null", 4);
  next_bind();
  return *this;
}

sql_stream&
sql_stream::operator<<(const QList<int>& list)
{
  check_binds();

  QString txt = "ARRAY[";
  for (QList<int>::const_iterator it = list.begin(); it != list.end(); ++it) {
    if (it != list.begin())
      txt.append(',');
    txt.append(QString("%1").arg(*it));
  }
  txt.append("]::int[]");
  if (list.isEmpty())
    txt = "'{}'::int[]";

  replace_placeholder(m_nArgPos, txt.toLocal8Bit().constData(), txt.length());
  next_bind();
  return *this;
}

void
sql_stream::check_binds()
{
  if (m_bExecuted && m_nArgPos==(int)m_vars.size()) {
    // reset the query for another execution
    reset_results();
  }
  if (m_nArgPos>=(int)m_vars.size())
    throw db_excpt(m_queryBuf, "Mismatch between bound variables and query");
}

void
sql_stream::print()
{
#if 0
  std::cout << "buf=" << m_queryBuf << std::endl;
  std::cout << "len=" << m_queryLen << ", bufsize=" << m_queryBufSize << std::endl;
  std::cout << "params\n";
  for (int i=0; i<(int)m_vars.size(); i++) {
    const sql_bind_param& v=m_vars[i];
    std::cout << v.name() << " => " << v.pos() << std::endl;
  }
#endif
}

void
sql_stream::execute()
{
  /* in the general case execute() will be called implicitly as soon
     as all the variables are bound. However the user code might want
     to call it explicitly to have a better control over the execution
     workflow. That's why in the case of a second call to execute() we
     choose not to throw any error and instead just return, except if
     m_auto_exec is false in which case execute() is always
     explicit */

  if (m_bExecuted && m_auto_exec)
    return;

  if (m_nArgPos<(int)m_vars.size())
    throw db_excpt(m_queryBuf, QString("Not all variables are bound (%1 out of %2)").arg(m_nArgPos).arg(m_vars.size()));

  DBG_PRINTF(5,"execute: %s", m_queryBuf);
  m_pgRes=PQexec(m_db.connection(), m_queryBuf);
  if (!m_pgRes)
    throw db_excpt(m_queryBuf, PQerrorMessage(m_db.connection()));
  if (PQresultStatus(m_pgRes)!=PGRES_TUPLES_OK && PQresultStatus(m_pgRes)!=PGRES_COMMAND_OK) {
    throw db_excpt(m_queryBuf, PQresultErrorMessage(m_pgRes),
		   QString(PQresultErrorField(m_pgRes, PG_DIAG_SQLSTATE)));
  }
  const char* t=PQcmdTuples(m_pgRes);
  if (t && *t) {
    m_affected_rows=atoi(t);
  }
  else
    m_affected_rows=0;
    
  m_rowNumber=0;
  m_colNumber=0;
  m_bExecuted=1;
}

int
sql_stream::row_count() const
{
  return (m_pgRes && PQresultStatus(m_pgRes)==PGRES_TUPLES_OK) ? PQntuples(m_pgRes) : 0;
}

int
sql_stream::eof()
{
  if (!m_bExecuted)
    execute();
  return m_rowNumber>=PQntuples(m_pgRes);
}

void
sql_stream::next_result()
{
  m_colNumber=((m_colNumber+1)%PQnfields(m_pgRes));
  if (m_colNumber==0)
    m_rowNumber++;
}

void
sql_stream::rewind()
{
  m_colNumber = 0;
  m_rowNumber = 0;
}

void
sql_stream::check_eof()
{
  if (eof())
    throw db_excpt(m_queryBuf, "End of stream reached");
}

sql_stream&
sql_stream::operator>>(int& i)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  if (!m_val_null)
    i=atoi(PQgetvalue(m_pgRes, m_rowNumber, m_colNumber));
  else
    i=0;
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(unsigned int& i)
{
  check_eof();
  unsigned long ul=strtoul(PQgetvalue(m_pgRes, m_rowNumber, m_colNumber),
			   NULL, 10);
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  if (!m_val_null)
    i=(unsigned int)ul;
  else
    i=0;
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(quint64& i)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  i = (m_val_null ? 0 : strtoull(PQgetvalue(m_pgRes, m_rowNumber, m_colNumber),
				 NULL, 10));
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(char& c)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  char* p=PQgetvalue(m_pgRes, m_rowNumber, m_colNumber);
  c = p?*p:'\0';
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(bool& c)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  char* p=PQgetvalue(m_pgRes, m_rowNumber, m_colNumber);
  c=(p!=NULL && *p=='t');
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(char* p)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  if (m_val_null)
    *p='\0';
  else {
    // pretty dangerous if the buffer is not big enough, but
    // we have no way of knowing. Better use C++ strings
    strcpy(p, PQgetvalue(m_pgRes, m_rowNumber, m_colNumber));
  }
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(QString& s)
{
  check_eof();
  if (m_db.datab()->encoding() == "UTF8")
    s=QString::fromUtf8(PQgetvalue(m_pgRes, m_rowNumber, m_colNumber));
  else
    s=PQgetvalue(m_pgRes, m_rowNumber, m_colNumber);
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(float& f)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  if (!m_val_null) {
    std::istringstream istr(PQgetvalue(m_pgRes, m_rowNumber, m_colNumber));
    istr.imbue(std::locale("C"));
    istr >> f;
  }
  else
    f=0;
  next_result();
  return *this;
}

sql_stream&
sql_stream::operator>>(double& f)
{
  check_eof();
  m_val_null = PQgetisnull(m_pgRes, m_rowNumber, m_colNumber);
  if (!m_val_null) {
    std::istringstream istr(PQgetvalue(m_pgRes, m_rowNumber, m_colNumber));
    istr.imbue(std::locale("C"));
    istr >> f;
  }
  else
    f=0;
  next_result();
  return *this;
}
