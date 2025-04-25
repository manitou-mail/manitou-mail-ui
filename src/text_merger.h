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

#ifndef INC_TEXT_MERGER_H
#define INC_TEXT_MERGER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>

class QTextCodec;
class QIODevice;

/* This class implements merging a template with a datafile in CSV format */
class text_merger
{
public:
  text_merger();
  void init(QIODevice* f);
  void init(const QStringList cols, QChar sep);
  QString merge_record(QIODevice* f, const QString tmpl);
  // DEBUG
  void print_fields();
  int nb_lines() const; // number of parsed lines, including header
  int nb_columns() const; // number of columns as given by the first line (header)
  QString column_name(int index) const;
  QStringList column_names() const;
  QStringList column_names(const QString header);
  QStringList collect_data(QIODevice*);
  static QString csv_join(const QStringList list, QChar separator);
  static void extract_variables(const QString tmpl, QSet<QString>& vars);
  QString merge_template(const QString tmpl, const QStringList values);
private:
  QStringList parse_header(QIODevice*);
  QChar guess_separator(const QString header);
  static const QChar m_default_separator;
  bool is_separator(QChar c);
  static const QString m_placeholder_regexp;
private:
  QTextCodec* m_codec;
  int m_line;
  QChar m_separator;
  QMap<QString,int> m_field_names; // name => 0-based index in the column list
};

#endif
