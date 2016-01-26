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

#ifndef INC_FETCH_THREAD_H
#define INC_FETCH_THREAD_H

#include <QThread>
#include <QString>

#include "database.h"
#include "message.h"
#include "main.h"

class fetch_thread: public QThread
{
  Q_OBJECT
public:
  fetch_thread();
  virtual void run();
  void release();
  void cancel();
  db_cnx* m_cnx;
  int m_max_results;
  int m_step_count; // increase each time the thread is reused (fetch more)
  std::list<mail_result>* m_results;
  int store_results(sql_stream& stream, int max_nb);
  QString m_query;
  QString m_errstr;
  int m_exec_time;   // query exec time in milliseconds
  int m_tuples_count;

  // boundaries
  mail_id_t m_max_mail_id;
  mail_id_t m_min_mail_id;
  QString m_max_msg_date;
  QString m_min_msg_date;

  //  progressive_wordsearch m_psearch;
  bool m_fetch_more;
  bool m_cancelled;

  // Notice processing
  static void notice_receiver(void *arg, const PGresult *res);

signals:
  void progress(int);
};


#endif
