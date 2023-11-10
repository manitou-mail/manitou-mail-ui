/* Copyright (C) 2004-2023 Daniel Verite

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
#include <QString>
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


  static QString format_db_string_array(const QStringList& words, db_cnx& db);
  static QString unaccent(const QString);
  static void unaccent(QStringList&);
private:
  // the word
  QString m_text;

  // wordtext.word_id
  uint m_word_id;
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
