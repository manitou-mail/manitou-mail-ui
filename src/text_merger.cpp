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

#include <QString>
#include <QApplication>
#include <QStringList>
#include <QTextCodec>
#include <QFile>

#include "text_merger.h"

//#define DEBUG

/* Usage:
  QFile f("data_file.txt");
  QString tmpl = "Template with {field1}, {field2}, ...";
  if (f.open(QIODevice::Text| QIODevice::ReadOnly)) {
    try {
      text_merger tm;
      tm.init(&f);
      while (!f.atEnd()) {
	QString res=tm.merge_record(&f, tmpl);
	// do something with res
      }
    }
    catch(QString error) {
      // ...
    }
  }
}
*/

text_merger::text_merger()
{
  // TODO: optionally use a different, per-file codec
  m_codec = QTextCodec::codecForLocale();
  if (!m_codec) {
    throw QObject::tr("No usable codec for the current locale");
  }
  m_separator = m_default_separator;
}

const QChar
text_merger::m_default_separator=',';

// The variable name will be captured in the parenthesized subexpression
const QString
text_merger::m_placeholder_regexp="\\{\\{(.+)\\}\\}";

int
text_merger::nb_lines() const
{
  return m_line;
}

int
text_merger::nb_columns() const
{
  return m_field_names.size();
}


// return the list of column names, sorted by their index
QStringList
text_merger::column_names() const
{
  QStringList cols;
  for (int i=0; i<m_field_names.size(); i++) {
    cols.append(m_field_names.key(i));
  }
  return cols;
}

QString
text_merger::csv_join(const QStringList list, QChar separator)
{
  QString result;
  QStringList::const_iterator it = list.constBegin();
  for (; it!=list.constEnd(); ++it) {
    QString field = *it;
    field.replace('"', "\"\""); // double the double quotes 
    if (!result.isEmpty())
      result.append(separator);
    result.append(QString("\"%1\"").arg(field));
  }
  return result;
}


QString
text_merger::column_name(int index) const
{
  QMap<QString,int>::const_iterator it = m_field_names.constBegin();
  for (; it!=m_field_names.constEnd(); ++it) {
    if (it.value()==index)
      return it.key();
  }
  return QString();
}

// can throw a QString
void
text_merger::init(QIODevice* io)
{
  QStringList columns = parse_header(io);
  for (int i=0; i<columns.size(); i++)
    m_field_names.insert(columns.at(i), i);

  m_line=1;
}

void
text_merger::init(const QStringList columns, QChar separator)
{
  m_line=1;
  for (int i=0; i<columns.size(); i++)
    m_field_names.insert(columns.at(i), i);
  m_separator = separator;
}

QString
text_merger::merge_record(QIODevice* io, const QString tmpl)
{
  if (m_field_names.isEmpty()) {
    throw QObject::tr("No merge fields in data file.");
  }
  QStringList values=collect_data(io);
  if (values.isEmpty())
    return QString();
  // discard empty lines (LF or CRLF alone). This is mostly useful if encountered at the end
  if (values.size()==1 && values.at(0).isEmpty())
    return QString();
  if (values.size()==m_field_names.size()) {
    return merge_template(tmpl, values);
  }
  else
    throw QObject::tr("%1 field(s) at line %2 when %3 were expected").arg(values.size()).arg(m_line).arg(m_field_names.size());
  
}

QString
text_merger::merge_template(const QString tmpl, const QStringList values)
{
  QString result=tmpl;
  QRegExp rx(m_placeholder_regexp);
  rx.setMinimal(true); // non-greedy
  /* Search for every occurrence of {something} in the template.
     For each occurrence, see if that's one of our fields.
     If yes, replace it with its value, if no then ignore it. */
  QMap<QString,int>::const_iterator it;
  int pos=0;
  while ((pos=rx.indexIn(result, pos)) >= 0) {
    //    QString name=result.mid(pos+1, rx.matchedLength()-2);
    it = m_field_names.constFind(rx.cap(1));
    if (it!=m_field_names.constEnd()) {
      int field_pos = it.value();
      if (field_pos < values.size()) {
	result.replace(pos, rx.matchedLength(), values.at(field_pos));
	pos += values.at(field_pos).length();
      }
      else
	pos += rx.matchedLength(); // actually an ERROR, since the field should have been in 'values'
    }
    else
      pos += rx.matchedLength();
  }
  return result;
}

QStringList
text_merger::column_names(const QString header)
{
  QStringList list = header.split(m_separator);
  return list;
}

/* Parse the first line of the CSV file. The line should contain the column names.
   Parse rules for the header differ from the data by the fact that:
   - one line only, no newline allowed inside a column name
   - we don't know the field separator before parsing, the parser has to find it
   - no double-quote is permitted inside a column name
*/

bool
text_merger::is_separator(QChar c)
{
  return (c==',' || c==';' || c=='\t');
}

QStringList
text_merger::parse_header(QIODevice* io)
{
  int state=1; // parse state
  int trans; // transition to next parse state
  QString field_value; // current field name
  QStringList fields;
  int column=0;
  QChar c;
  QString errmsg;
  bool separator_found=false;

  m_separator = QChar();

  QByteArray bytes = io->readLine();
  QString data_line = m_codec->toUnicode(bytes).trimmed();

  for (column=1; column<=data_line.length(); column++) {
    c = data_line.at(column-1);
    trans=0;
    switch(state) {
      // state 1: initial, at the beginning or end of a field
    case 1:
      if ((separator_found && c==m_separator) || is_separator(c)) {
	// empty field
	trans = 7;
	errmsg = QObject::tr("Empty field");
      }
      else if (c=='"')
	trans=2;
      else {
	trans=4;
	field_value.append(c);
      }
      break;

      // state 2: inside a section beginning with a quote
    case 2:
      if (c=='"')
	trans=3; // end of quoted section or quoted quote
      else {
	trans=2;
	field_value.append(c);
      }
      break;

      // state 3: after a quote encountered inside a quoted section
    case 3:
      if (!separator_found && is_separator(c)) {
	m_separator=c;
	separator_found=true;
	if (field_value.isEmpty()) {
	  trans=7;
	  errmsg = QObject::tr("Empty field");
	}
	else {
	  trans=1;
	  fields.append(field_value);
	  field_value.truncate(0);
	}
      }
      else if (c==m_separator) {
	if (field_value.isEmpty()) {
	  trans=7;
	  errmsg = QObject::tr("Empty field");
	}
	else {
	  trans=1;
	  fields.append(field_value);
	  field_value.truncate(0);
	}
      }
      else if (c=='"') {
	trans=7;
	errmsg = QObject::tr("Double-quote not allowed inside a quoted section in CSV header");
      }
      else {
	trans=7;
	errmsg = QObject::tr("A double quote at the end of a field must be followed by another field or an end of line");
      }
      break;

    // state 4: inside a field
    case 4:
      if (c=='"') {
	trans=7;
	errmsg = QObject::tr("A double quote is not allowed in a field name");
      }
      else if (is_separator(c) && !separator_found) {
	m_separator=c;
	separator_found=true;
	trans=1;
	fields.append(field_value);
	field_value.truncate(0);
      }
      else if (c==m_separator) {
	trans=1;
	fields.append(field_value);
	field_value.truncate(0);
      }
      else {
	trans=4;
	field_value.append(c);
      }
      break;

    case 7:
    default:
      throw QObject::tr("Unhandled parser state %1").arg(state);
    }

    if (trans>0) {
      state=trans;
      if (state==7) {
	QString exc_msg = QObject::tr("Syntax error line 1 column %1.").arg(column);
	if (!errmsg.isEmpty())
	  exc_msg += "\n" + errmsg;
	throw exc_msg;
      }
    }
  }

  if (state==4 || state==3)
    fields.append(field_value);

  return fields;
}

QStringList
text_merger::collect_data(QIODevice* io)
{
  int state=1; // parse state
  int trans; // transition to next parse state
  QString field_value;
  QStringList fields;
  int column=0;
  bool parse_end=false;
  QChar c;
  bool line_complete=false;
  QString errmsg;

  do {
    if (io->atEnd()) { // OK if the parser's state is final (1 or 4)
      if (state==4 || state==3) {
	fields.append(field_value);
      }
      else if (state!=1) {
	throw QObject::tr("Unexpected end of file.");
      }
      parse_end=true;
    }
    else {
      QByteArray bytes = io->readLine();
      m_line++;
      QString data_line = m_codec->toUnicode(bytes);
      for (column=1; column<=data_line.length(); column++) {
	c = data_line.at(column-1);
	trans=0;
	switch(state) {
	  // state 1: initial, at the beginning or end of a field
	case 1:
	  if (c=='\r')
	    trans=10;
	  else if (c=='\n') { // empty line?
	    line_complete=true;
	    trans=1;
	  }
	  else if (c==m_separator) {
	    // empty field
	    fields.append("");
	    trans=1;
	  }
	  else if (c=='"')
	    trans=2;
	  else {
	    trans=4;
	    field_value.append(c);
	  }
	  break;

	  // state 2: inside a section beginning with a quote
	case 2:
	  if (c=='"')
	    trans=3; // end of quoted section or quoted quote
	  else {
	    trans=2;
	    field_value.append(c);
	  }
	  break;

	  // state 3: after a quote encountered inside a quoted section
	case 3:
	  if (c==m_separator) {
	    trans=1;
	    fields.append(field_value);
	    field_value.truncate(0);
	  }
	  else if (c=='"') {
	    trans=2;
	    field_value.append(c);
	  }
	  else if (c=='\r')
	    trans=10;
	  else if (c=='\n') {
	    trans=1;
	    line_complete=true;
	  }
	  else {
	    trans=7;
	    errmsg = QObject::tr("A double quote at the end of a field must be followed by another field or an end of line");
	  }
	  break;
	  // state 4: inside a field
	case 4:
	  if (c=='"') {
	    trans=7;
	    errmsg = QObject::tr("A double quote is allowed in a field only if that field is enclosed in double quotes");
	  }
	  else if (c=='\r')
	    trans=10;
	  else if (c=='\n') {
	    trans=1;
	    line_complete=true;
	  }
	  else if (c==m_separator) {
	    trans=1;
	    fields.append(field_value);
	    field_value.truncate(0);
	  }
	  else {
	    trans=4;
	    field_value.append(c);
	  }
	  break;

	  // state 10: Go there on finding CR, to handle CR,LF. Not used if the underlying IO has already
	  // converted CR,LF to LF
	case 10:
	  if (c=='\n') {
	    trans=1;
	    line_complete=true;
	  }
	  else {
	    trans=7;
	    errmsg = QObject::tr("Carriage return followed by unexpected character (a newline was expected)");
	  }
	  break;
	
	case 7:
	default:
	  throw QObject::tr("Unhandled parser state %1").arg(state);
	}

	if (trans>0) {
	  state=trans;
	  if (state==7) {
	    QString exc_msg = QObject::tr("Syntax error line %1 column %2.").arg(m_line).arg(column);
	    if (!errmsg.isEmpty())
	      exc_msg += "\n" + errmsg;
	    throw exc_msg;
	  }
	}
	if (line_complete) {
	  line_complete=false;
	  fields.append(field_value);
	  //	  field_value.truncate(0);
	  
	  //	  result.append(fields);
	  // fields.clear();
	  parse_end=true;
	}
      }
    }
  } while (!parse_end);

  return fields;
}

QChar
text_merger::guess_separator(const QString header)
{
  for (int i=0; i<header.length(); i++) {
    QChar c = header.at(i);
    if (is_separator(c) && i>0)
      return c;
  }
  // if no separator was found, maybe there's only one column.
  return m_default_separator;
}

//static
void
text_merger::extract_variables(const QString tmpl, QSet<QString>& vars)
{
  QRegExp rx(m_placeholder_regexp);
  rx.setMinimal(true); // non-greedy
  /* Search for every occurrence of {something} in the template. */
  int pos=0;
  while ((pos=rx.indexIn(tmpl, pos)) >= 0) {
    vars.insert(rx.cap(1));
    pos += rx.matchedLength();
  }
}

