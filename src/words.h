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

#ifndef INC_WORDS_H
#define INC_WORDS_H

#include <map>
#include <qstring.h>
#include "bitvector.h"
#include "dbtypes.h"

class db_word
{
public:
  db_word();
  virtual ~db_word();
  static bool is_non_indexable(const QString w);
  void set_text(const QString& text);

  const QString text() const {
    return m_text;
  }
  // fetch the word_id
  bool fetch_id();

  // fetch the inverted index entries related to the word
  bool fetch_vectors();

  const bit_vector* vector_part(uint part_no) const;

  const std::map<uint,bit_vector*>* vectors() const {
    return &m_vectors;
  }

  static QString format_db_string_array(const QStringList& words, db_cnx& db);

private:
  // the word
  QString m_text;

  /* vector of mails containing this word, fetchable from the inverted index
     the map index is the partno column (number of partition) of
     inverted_word_index */
  std::map<uint,bit_vector*> m_vectors;

  // wordtext.word_id
  uint m_word_id;
};

//
// Results of word searches combined by logical operators
//
class wordsearch_resultset
{
public:
  wordsearch_resultset();
  ~wordsearch_resultset();
  void get_result_bits(std::list<uint>& l,
		       mail_id_t limit,
		       int direction, // -1,+1 or 0
		       uint max_results);
  void and_word(const db_word& dbw);
  void insert_word(const db_word& dbw);
private:
  void clear();
  std::map<uint,bit_vector*> m_vect;
  static int m_partsize;
};

class progressive_wordsearch
{
public:
  progressive_wordsearch() : m_nb_fetched_parts(0) {}
  bool get_index_parts(const QStringList& words);
  QList<int> m_parts;
  int m_nb_fetched_parts;
};

#endif
