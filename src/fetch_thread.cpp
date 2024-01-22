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

#include "fetch_thread.h"
#include "selectmail.h"
#include "msg_status_cache.h"

fetch_thread::fetch_thread()
{
  m_cnx=NULL;
  m_cancelled=false;
}

void
fetch_thread::store_results(sql_stream& s, int max_nb)
{
  mail_result r;
  int i = 0;

  while (!s.eos() && (max_nb==-1 || i<max_nb)) {
    s >> r.m_id >> r.m_from >> r.m_subject >> r.m_date >> r.m_thread_id
      >> r.m_status >> r.m_in_replyto >> r.m_sender_name >> r.m_pri >> r.m_flags
      >> r.m_recipients;
    msg_status_cache::update(r.m_id, r.m_status);
    m_results->push_back(r);

    /* Find the min/max of (msg_date,mail_id) */
    if (m_min_msg_date.isEmpty()) {
      m_min_msg_date = r.m_date;
      m_min_mail_id = r.m_id;
    }
    else if (r.m_date < m_min_msg_date) {
      m_min_msg_date = r.m_date;
      m_min_mail_id = r.m_id;
    }
    else if (r.m_date == m_min_msg_date) {
      if (r.m_id < m_min_mail_id)
	m_min_mail_id = r.m_id;
    }

    if (m_max_msg_date.isEmpty()) {
      m_max_msg_date = r.m_date;
      m_max_mail_id = r.m_id;
    }
    else if (r.m_date > m_max_msg_date) {
      m_max_msg_date = r.m_date;
      m_max_mail_id = r.m_id;
    }
    else if (r.m_date == m_max_msg_date) {
      if (r.m_id > m_max_mail_id)
	m_max_mail_id = r.m_id;
    }
    ++i;
  }
}


void
fetch_thread::notice_receiver(void *arg, const PGresult *res)
{
  fetch_thread* t = (fetch_thread*)arg;
  printf("NOTICE received in thread message=%s", PQresultErrorMessage(res));
  QString txt = PQresultErrorMessage(res);
  QRegExp rx("done partition group (\\d+)/(\\d+), hits=(\\d+)");
  rx.setMinimal(true); // non-greedy
  if (rx.indexIn(txt, 0)) {
    QStringList list = rx.capturedTexts();
    int grp = list.at(1).toInt();
    int total = list.at(2).toInt();
    int hits = list.at(3).toInt();
    if (total<0 || grp<0 || hits<0) {
      // error in conversion, bogus input in the message
      return;
    }
    if (grp==0)
      emit t->progress(-total);
    emit t->progress(grp+1);
  }
  else {
    DBG_PRINTF(3, "unrecognized notice");
  }

}

// Launch the query and fetch results fetch. Overrides QThread::run()
void
fetch_thread::run()
{
  if (!m_cnx) return;
  DBG_PRINTF(5,"fetch_thread::run(), max_results=%d", m_max_results);
  m_errstr=QString();
  QTime start = QTime::currentTime();

  m_tuples_count=0;
  m_min_mail_id = m_max_mail_id = 0;
  m_max_msg_date = QString();
  m_min_msg_date = QString();

  // install a notice receiver to handle progress report for certain queries
  // (wordsearch)
  PGconn* c = m_cnx->connection();
  PQsetNoticeReceiver(c, notice_receiver, (void*)this);

  db_transaction trans(*m_cnx);
  try {
    sql_stream st("SET TRANSACTION READ ONLY", *m_cnx);
    sql_stream sq(m_query, *m_cnx);
    sq.execute();
    trans.commit();
    store_results(sq, m_max_results>0?m_max_results:-1);
    m_tuples_count = sq.row_count();
  }
  catch(db_excpt& x) {
    DBG_PRINTF(4, "db_excpt on m_cnx=%p", m_cnx);
    m_errstr = x.errmsg();
    trans.rollback();
  }

  m_exec_time = start.elapsed();

}

// stop the fetch
void
fetch_thread::cancel()
{
  if (m_cnx) {
    DBG_PRINTF(5, "fetch_thread::cancel()");
    PGconn* c = m_cnx->connection();
    char errbuf[256];
    PGcancel* cancel = PQgetCancel(c);
    if (cancel) {
      PQcancel(cancel, errbuf, sizeof(errbuf));
      PQfreeCancel(cancel);
      m_cancelled=true;
    }
  }
}

void
fetch_thread::release()
{
  DBG_PRINTF(4, "fetch_thread::release()");
  if (m_cnx) {
    delete m_cnx;
    m_cnx=NULL;
  }
}
