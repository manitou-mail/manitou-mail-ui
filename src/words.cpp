/* Copyright (C) 2004-2011 Daniel Verite

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

#include "main.h"
#include "words.h"
#include "db.h"
#include "sqlstream.h"

#ifdef Q_OS_WIN
#include <winsock2.h>
#endif
#if defined(Q_OS_UNIX) || defined(Q_OS_MAC)
#include <netinet/in.h>
#endif

//static
int wordsearch_resultset::m_partsize=16384;

db_word::db_word() : m_word_id(0)
{
}

db_word::~db_word()
{
  std::map<uint,bit_vector*>::iterator it;
  for (it = m_vectors.begin(); it!=m_vectors.end(); ++it) {
    delete it->second;
  }
}

//static
bool
db_word::is_non_indexable(const QString w)
{
//  DBG_PRINTF(4, "is_non_indexable(%s)\n", w.latin1());
  if (w.length()<3)
    return true;

  db_cnx db;
  bool result;
  try {
    sql_stream s("SELECT 1 FROM non_indexable_words WHERE wordtext=:p1", db);
    s << w;
    result=!s.eos();
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

void
db_word::set_text(const QString& txt)
{
  m_text=txt;
}

bool
db_word::fetch_id()
{
  db_cnx db;
  bool result;
  try {
    sql_stream s("SELECT word_id FROM words WHERE wordtext=:p1", db);
    s << m_text;
    if (!s.eos())
      s >> m_word_id;
    else
      m_word_id=0;
    return true;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

/* Return the vector of bits related to 'part_no' partition, or NULL
 if that part of the vector if empty */
const bit_vector*
db_word::vector_part(uint part_no) const
{
  std::map<uint,bit_vector*>::const_iterator it;
  it = m_vectors.find(part_no);
  return (it!=m_vectors.end() ? it->second : NULL);
}


// Fetch the inverted index entries related to the word
bool
db_word::fetch_vectors()
{
  db_cnx db;
  PGconn* pgconn = db.connection();

  if (!m_word_id && !fetch_id())
    return false;

  QString query= QString("SELECT part_no,mailvec,nz_offset FROM inverted_word_index WHERE word_id=%1").arg(m_word_id);

  PGresult* res = PQexecParams(pgconn,
			       query.toLatin1().constData(),
			       0, // number of params
			       NULL, // param types,
			       NULL, // param values,
			       NULL, // param lengths
			       NULL, // param formats
			       1 // result format=binary
			       );
  if (res && PQresultStatus(res)==PGRES_TUPLES_OK) {
    for (int row=0; row<PQntuples(res); row++) {
      //int f=PQfformat(res,1);
      //Oid o=PQftype(res, 1);
      unsigned long partno = ntohl(*(unsigned long*)PQgetvalue(res, row, 0));
      uint part=(uint)partno;
      unsigned long nzo = ntohl(*(unsigned long*)PQgetvalue(res, row, 2));
      uint nz_offset=(uint)nzo;
//      DBG_PRINTF(5, "word='%s' word_id=%d, partno=%d, nz_offset=%d, mailvec.length=%d\n",
//		 m_text.latin1(), m_word_id, part, nz_offset, PQgetlength(res, row, 1));
      bit_vector* v = new bit_vector();
      v->set_buf((const uchar*)PQgetvalue(res, row, 1),
		 PQgetlength(res, row, 1),
		 nz_offset);
      /* (word_id,partno) tuples should be unique. TODO: check
	 the existence of a m_vectors[part] entry before assigning
	 it as a consistency test */
      m_vectors[part] = v;
    }
  }
  else if (res) {
    DBEXCPT(pgconn);
    return false;
  }
  if (res)
    PQclear(res);
  return true;
}

// static
QString
db_word::format_db_string_array(const QStringList& words, db_cnx& db)
{
  QString words_pg_array = "ARRAY[";
  for (QStringList::const_iterator it = words.begin(); it!=words.end(); ++it) {
    if (it!=words.begin())
      words_pg_array.append(',');
    words_pg_array.append("'" + db.escape_string_literal(*it) + "'");
  }
  words_pg_array.append(']');
  return words_pg_array;
}


wordsearch_resultset::wordsearch_resultset()
{
}

wordsearch_resultset::~wordsearch_resultset()
{
  clear();
}

void
wordsearch_resultset::and_word(const db_word& dbw)
{
//  DBG_PRINTF(7, "and_word('%s')\n", dbw.text().latin1());
  std::map<uint,bit_vector*>::iterator it;

  for (it = m_vect.begin(); it!=m_vect.end(); ++it) {
    const bit_vector* v = dbw.vector_part(it->first);
    if (!v) {
      it->second->clear();
    }
    else {
      it->second->and_op(*v);
    }
  }  
}

void
wordsearch_resultset::insert_word(const db_word& dbw)
{
//  DBG_PRINTF(7, "insert_word('%s')\n", dbw.text().latin1());
  // copy the vectors from dbw
  const std::map<uint,bit_vector*>* w_vecs = dbw.vectors();
  std::map<uint,bit_vector*>::const_iterator it;
  for (it=w_vecs->begin(); it!=w_vecs->end(); ++it) {
    bit_vector* v = new bit_vector();
    v->set_buf(it->second->buf(), it->second->size());
    m_vect[it->first] = v;
  }
}

void
wordsearch_resultset::clear()
{
  std::map<uint,bit_vector*>::iterator it;
  for (it = m_vect.begin(); it!=m_vect.end(); ++it) {
    delete it->second;
  }
  m_vect.clear();
}

/*
 get a list of mail_id from the word vectors
 limit is a mail_id 
 if direction==-1, include only messages for which mail_id < limit
 if direction==1, include only messages for which mail_id > limit
 if direction==0, limit is ignored
*/
void
wordsearch_resultset::get_result_bits(std::list<mail_id_t>& l,
				      mail_id_t limit,
				      int direction,
				      uint max_results)
{
  DBG_PRINTF(7, "get_result_bits(limit=%d,direction=%d,max_results=%u)", limit, direction, max_results);
  std::map<uint,bit_vector*>::iterator it;
  uint cnt_results=0;
/*  if (max_results==0)
    return;*/
  for (it = m_vect.begin(); it!=m_vect.end(); ++it) {
    int part_offset = it->first*m_partsize;
    uint sz=it->second->size();
    const uchar* buf = it->second->buf();

    for (uint o=0; o<sz; o++) {
      uchar mask=1;
      uchar c=buf[o];
      if (!c) continue;		// shortcut
      for (uint i=0; i<8; i++) {
	if (c&mask) {
	  mail_id_t id = part_offset+(o*8)+i+1;
	  if (direction==0) {
	    l.push_back(id);
	    cnt_results++;
	  }
	  else if (direction==-1) {
	    if (id<limit) {
	      l.push_back(id);
	      cnt_results++;
	    }
	  }
	  else if (direction==1) {
	    if (id>limit) {
	      l.push_back(id);
	      cnt_results++;
	    }
	  }
	  if (max_results>0 && cnt_results >= max_results)
	    return;		// shortcut to exit
	}
	mask = mask << 1;
      }
    }
  }
}

//static
bool
progressive_wordsearch::get_index_parts(const QStringList& words)
{
  db_cnx db;
  bool result=true;
  try {
    QString query = QString("SELECT * FROM wordsearch_get_parts(%1) ORDER BY 1 desc")
      .arg(db_word::format_db_string_array(words, db));
    sql_stream s(query, db);
    m_parts.clear();
    while (!s.eos()) {
      int part_no;
      s >> part_no;
      m_parts.append(part_no);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

