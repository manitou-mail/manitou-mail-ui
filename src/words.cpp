/* Copyright (C) 2004-2025 Daniel Verite

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

db_word::db_word() : m_word_id(0)
{
}

db_word::~db_word()
{
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

//static
QString
db_word::unaccent(const QString s)
{
  QString s2 = s.normalized(QString::NormalizationForm_KD);
  QString out;
  for (int i=0, j=s2.length(); i<j; i++) {
    // strip diacritic marks
    if (s2.at(i).category()!=QChar::Mark_NonSpacing &&
	s2.at(i).category()!=QChar::Mark_SpacingCombining) {
      out.append(s2.at(i));
    }
  }
  return out;
}

//static
void
db_word::unaccent(QStringList& s)
{
  for (QStringList::iterator it = s.begin(); it!=s.end(); ++it) {
    *it = unaccent(*it);
  }
}

// static
QString
db_word::format_db_string_array(const QStringList& words, db_cnx& db)
{
  QString txt = "ARRAY[";
  for (QStringList::const_iterator it = words.begin(); it!=words.end(); ++it) {
    if (it!=words.begin())
      txt.append(',');
    txt.append("'" + db.escape_string_literal(*it) + "'");
  }
  txt.append(']');
  /* For an empty array, add a cast to avoid the postgres resolve
     error: "cannot determine type of empty array". Plus we use '{}'
     instead of array[] it's accepted only by PG>=8.4 */
  if (words.isEmpty())    
    txt = "'{}'::text[]";  //txt.append("::text[]");
  return txt;
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

