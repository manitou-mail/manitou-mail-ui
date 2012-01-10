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

#include "db.h"
#include "sqlstream.h"
#include "date.h"

#include "main.h"
#include "mailing.h"
#include "text_merger.h"

#include <QList>
#include <QProgressBar>
#include <QBuffer>
#include <QByteArray>

bool
mailings::load()
{
  db_cnx db;
  try {
    sql_stream s("SELECT md.mailing_id,title,to_char(creation_date,'YYYYMMDDHH24MISS'),status,nb_sent,nb_total FROM mailing_definition md JOIN mailing_run USING(mailing_id) ORDER BY 1 DESC", db);
    while (!s.eof()) {
      mailing_db m;
      m.init_from_db(s);
      append(m);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

void
mailing_db::init_from_db(sql_stream& s)
{
  QString sdate;
  s >> m_id >> m_title >> sdate >> m_status >> m_nb_sent >> m_nb_total;
  m_creation_date = date(sdate);
}

bool
mailing_db::create(const mailing_options& options)
{
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s("INSERT INTO mailing_definition(title,sender_email,header_template, text_template, html_template, csv_columns) VALUES(:ti,:se,:hh,:tt,:ht,:cs) RETURNING mailing_id, to_char(creation_date,'YYYYMMDDHH24MISS')",  db);
    QString str_date;
    s << options.title << options.sender_email << options.header;
    s << options.text_part << options.html_part;
    if (options.field_names.size() > 0) {
      // CSV stuff
      QStringList csv_columns = options.field_names;
      csv_columns.removeFirst(); // remove the email column, we store in a dedicated field
      QString csv_header = csv_columns.join(QString("%1").arg(options.csv_separator));

      s << csv_header;
    }
    else
      s << sql_null();
    s >> m_id >> str_date;
    m_title = options.title;
    m_creation_date = date(str_date);

    sql_stream s1("INSERT INTO mailing_run(mailing_id,status,throughput,nb_total,nb_sent) VALUES(:p1,:st,1.0,:tot,0)", db);
    s1 << m_id << m_status << options.mail_addresses.size();
    db.commit_transaction();
    return true;
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  return true;    
}

bool
mailing_db::load_data(mailing_options* opt)
{
  db_cnx db;
  try {
    sql_stream s("SELECT recipient_email, csv_data FROM mailing_data WHERE mailing_id=:id ORDER BY mailing_data_id", db);
    s << m_id;
    QString email;
    QString csv_line;
    text_merger tm;
    tm.init(opt->field_names, ',');
    
    while (!s.eos()) {
      s >> email >> csv_line;
      opt->mail_addresses.append(email);
      if (!csv_line.isEmpty()) {
	QByteArray arr = csv_line.toUtf8();
	QBuffer buffer(&arr);
	buffer.open(QBuffer::ReadWrite);
	QStringList csv_values = tm.collect_data(&buffer);

	opt->csv_data.append(csv_values);
      }
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;    
}


bool
mailing_db::load_definition(mailing_options* options)
{
  db_cnx db;
  try {
    m_status=0;
    sql_stream s("SELECT title,sender_email,header_template, text_template, html_template, csv_columns FROM mailing_definition WHERE mailing_id=:id",  db);
    QString csv_columns;
    s << m_id;
    s >> options->title >> options->sender_email >> options->header;
    s >> options->text_part >> options->html_part >> csv_columns;

    if (!csv_columns.isEmpty()) {
      text_merger tm;
      /* we don't set the separator since text_merger has by default the same separator
	 than the database contents (comma) */
      options->field_names = tm.column_names(csv_columns);
      options->field_names.insert(0, "email");
    }
    else
      options->field_names.clear();

    return true;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;    
}


/* Store all the data for a mailing in a single operation.
   The caller is assumed to have opened a transaction.
   TODO: use COPY for improved speed
*/
bool
mailing_db::store_data(const QStringList& recipients, const QList<QStringList>& data_lines,
		       QProgressBar* bar)
{
  db_cnx db;
  try {
    int i=0;
    int step=recipients.size()/100;
    if (bar)
      bar->setRange(0, recipients.size());
    sql_stream s("INSERT INTO mailing_data(mailing_id, recipient_email, csv_data) VALUES(:id,:s, :data)", db);
    QList<QStringList>::const_iterator itd = data_lines.constBegin();
    for (QStringList::const_iterator iter = recipients.constBegin();
	 iter!=recipients.constEnd();
	 ++iter)
      {
	if (bar && (step==0 || (i++%step)==0)) {
	  bar->setValue(i);
	  bar->update();
	}
	s << m_id << *iter;
	if (itd != data_lines.constEnd()) {
	  QString csv_line = text_merger::csv_join(*itd, ',');
	  s << csv_line;
	  ++itd;
	}
	else
	  s << sql_null();
      }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;    
}

bool
mailing_db::stop()
{
  db_cnx db;
  try {
    sql_stream s1("UPDATE mailing_run SET status=:s WHERE mailing_id=:id", db);
    s1 << status_stopped << m_id;
    m_status = status_stopped;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mailing_db::instantiate_job()
{
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s1("UPDATE mailing_run SET status=:s WHERE mailing_id=:id", db);
    s1 << status_running << m_id;
    sql_stream s("INSERT INTO jobs_queue(job_type, job_args) SELECT :t,:a WHERE NOT EXISTS (SELECT 1 FROM jobs_queue WHERE job_type=:t1 AND job_args=:a1)", db);
    s << "mailing" << QString("%1").arg(m_id);
    s << "mailing" << QString("%1").arg(m_id);
    db.commit_transaction();
    m_status=status_running; // 1
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  return true;    
}

QString
mailing_db::status_text() const
{
  static const char* txt[] = {
    QT_TR_NOOP("Not started"),
    QT_TR_NOOP("Running"),
    QT_TR_NOOP("Stopped"),
    QT_TR_NOOP("Done")
  };
  if (m_status>=0 && m_status<(int)(sizeof(txt)/sizeof(txt[0]))) {
    return QObject::tr(txt[m_status]);
  }
  else
    return QString::null;
}

bool
mailing_db::purge()
{
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s1("UPDATE mailing_run SET status=:s WHERE mailing_id=:id", db);
    s1 << status_stopped << m_id;

    sql_stream s2("DELETE FROM mailing_data WHERE mailing_id=:id", db);
    s2 << m_id;

    sql_stream s3("DELETE FROM mailing_run WHERE mailing_id=:id", db);
    s3 << m_id;

    sql_stream s4("DELETE FROM mailing_definition WHERE mailing_id=:id", db);
    s4 << m_id;

    db.commit_transaction();
    m_status = status_stopped;
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  return true;    
}

/* Filter out characters that would be invalid for the
   mailing_db.header_template column */
//static
QString
mailing_db::filter_header_line(QString line)
{
  // maybe filter some other characters?
  line.replace("\n", " ");
  return line;
}

