/* Copyright (C) 2004-2024 Daniel Verite

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

#ifndef INC_MAILING_H
#define INC_MAILING_H

#include <QList>
#include <QStringList>
#include "sqlstream.h"
#include "date.h"

class QProgressBar;

class mailing_options
{
public:
  mailing_options() {
    csv_separator=',';
  }
  QString title;
  QString sender_email;
  QStringList mail_addresses;
  QStringList field_names;
  QList<QStringList> csv_data;
  char csv_separator;
  int mail_format;
  //  double throughput; // LATER
  QString header;
  QString text_part;
  QString html_part;
};

class mailing_db
{
public:
  mailing_db() {m_id=0;}
  mailing_db(int id) {
    m_id=id;
  }
  void init_from_db(sql_stream&);
  QString title() const {
    return m_title;
  }
  int status() const {
    return m_status;
  }
  void set_status(int s) {
    m_status=s;
  }
  int id() const {
    return m_id;
  }
  date creation_date() const {
    return m_creation_date;
  }

  static QString filter_header_line(QString line);

  QString status_text() const;
  bool create(const mailing_options& options);

  // remove the mailing and all its data from the database
  bool purge();

  // fetch entries from the mailing_definition table into an instance of mailing_options
  bool load_definition(mailing_options* options);

  // fetch mail addresses and CSV contents from the mailing_data table
  bool load_data(mailing_options* options);

  bool store_data(const QStringList& recipients,
		  const QList<QStringList>& data_lines,
		  QProgressBar* bar=NULL);
  bool instantiate_job();
  bool stop();
  enum {
    // must match mailing.status in the db
    status_not_started=0,
    status_running=1,
    status_stopped=2,
    status_finished=3
  };
  typedef enum {
    format_text_plain=1,
    format_html_and_text,
    format_html_only,
  } template_format;
  int m_nb_sent;
  int m_nb_total;
private:
  int m_id;
  int m_status;
  QString m_title;
  date m_creation_date;
};

class mailings : public QList<mailing_db>
{
public:
  bool load();
};


#endif
