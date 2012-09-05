/* Copyright (C) 2004-2012 Daniel Verite

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

/*
Partitioned wordsearch is currently not compatible with words
exclusion so it's #undefined. It's not clear how much speed
gain it provides, anyway. It may be reinstated in the future with
more sophisticated code to support it, if more testing shows that it's
worth the trouble.

#define PARTITIONED_WORDSEARCH 1
*/

#include "main.h"
#include "selectmail.h"
#include "msg_list_window.h"
#include "db.h"
#include "tags.h"
#include "msg_status_cache.h"
#include "sqlquery.h"
#include "sqlstream.h"
#include "addresses.h"
#include "helper.h"
#include "icons.h"
#include "sql_editor.h"
#include "words.h"

#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QButtonGroup>
#include <QSpinBox>
#include <QMessageBox>
#include <QApplication>
#include <QCursor>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>
#include <QFontMetrics>
#include <QRadioButton>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QHBoxLayout>

const int
msgs_filter::max_possible_prio=32767;

msgs_filter::msgs_filter()
{
  init();
}

void
msgs_filter::init()
{
  m_order=(get_config().get_msgs_sort_order()==Qt::AscendingOrder)?1:-1;
  m_mailId=0;
  m_tag_id=0;
  m_thread_id=0;
  m_status=-1;
  m_status_set=0;
  m_status_unset=0;
  m_include_trash = false;
  m_newer_than=0;
  m_max_results=get_config().get_number("max_msgs_per_selection");
  if (m_max_results==0)
    m_max_results=1000;
  m_addresses_count=0;
  m_sAddress=QString::null;
  m_subject=QString::null;
  m_body_substring=QString::null;
  m_addr_to=QString::null;
  m_sql_stmt=QString::null;
  m_tag_name=QString::null;
  m_date_min=QDate();
  m_date_max=QDate();
  //  m_word=QString::null;
  m_fts.m_words.clear();
  m_fts.m_exclude_words.clear();
  m_in_trash=false;
  m_auto_refresh=false;
  m_min_prio = max_possible_prio+1;
  m_nAddrType = 0;

  m_user_query=QString::null;

  // results
  m_fetched=false;
  m_fetch_results=NULL;
  m_date_bound=QString::null;
  m_mail_id_bound=0;
  m_has_more_results = false;
  
}

msgs_filter::~msgs_filter()
{
  if (m_fetch_results)
    delete m_fetch_results;
}

// Initialize a fetch_thread with conditions and state known by the filter
void
msgs_filter::preprocess_fetch(fetch_thread& thread)
{
  thread.m_psearch = m_psearch; // not necessary, also done by asynchronous_fetch
}


// Store post-fetch state to the filter from the fetch_thread
void
msgs_filter::postprocess_fetch(fetch_thread& thread)
{
  m_has_more_results = m_max_results>0 && (thread.m_tuples_count > m_max_results);
  DBG_PRINTF(6,"postprocess_fetch: m_has_more_results=%d", (int)m_has_more_results);
  m_boundary = thread.m_boundary;
  if (m_order>0) {
    m_mail_id_bound = thread.m_max_mail_id;
    m_date_bound = thread.m_max_msg_date;
  }
  else {
    m_mail_id_bound = thread.m_min_mail_id;
    m_date_bound = thread.m_min_msg_date;
    m_boundary = thread.m_boundary;
  }
  m_psearch = thread.m_psearch;
}

int
msgs_filter::add_address_selection (sql_query& q,
				    const QString email_addr,
				    int addr_type)
{
  mail_address addr;
  bool address_found;
  if (!addr.fetchByEmail(email_addr, &address_found) || !address_found) {
    // no address matches m_sAddress or an error has occurred when
    // trying to fetch: the result has to be empty
    return 0;
  }
  QString alias=QString("ma%1").arg(++m_addresses_count);
  q.add_table(QString("mail_addresses %1").arg(alias));
  q.add_clause(QString(alias) + ".addr_id", addr.id());
  q.add_clause(QString("m.mail_id=%1.mail_id").arg(alias));
  if (addr_type != 0) {
    q.add_clause(QString("%1.addr_type").arg(alias), addr_type);
  }
  return 1;
}


//static
int
msgs_filter::load_result_list(PGresult* res, std::list<mail_result>* l, int max_nb)
{
  if (res && PQresultStatus(res)==PGRES_TUPLES_OK) {
    DBG_PRINTF(5,"load_result_list %d results max=%d", PQntuples(res), max_nb);

    // if max_nb==-1, keep all the tuples
    // if max_nb>=0, keep at most max_nb tuples
    if (max_nb<0)
      max_nb=PQntuples(res);
    else if (max_nb > PQntuples(res))
      max_nb = PQntuples(res);

    for (int i=0; i < max_nb; i++) {
      mail_result r;
      r.m_id = atoi(PQgetvalue(res, i , 0));
      r.m_from = PQgetvalue(res, i, 1);
      r.m_subject = QString::fromUtf8(PQgetvalue(res, i, 2));
      r.m_date = PQgetvalue(res, i, 3);
      r.m_thread_id = atoi(PQgetvalue(res, i , 4));
      r.m_status = atoi(PQgetvalue(res, i, 5));
      msg_status_cache::update(r.m_id, r.m_status);
      r.m_in_replyto = atoi(PQgetvalue(res, i, 6));
      r.m_sender_name = QString::fromUtf8(PQgetvalue(res, i, 7));
      r.m_pri = atoi(PQgetvalue(res, i, 8));
      r.m_flags = (uint)atoi(PQgetvalue(res, i, 9));
      r.m_recipients = QString::fromUtf8(PQgetvalue(res, i, 10));
      l->push_back(r);
    }
    return max_nb;
  }
  else
    return 0;
}

int
fetch_thread::store_results(sql_stream& s, int max_nb)
{
  int i=0;
  QString date_stamp;
  mail_result r;

  while (!s.eos() && (max_nb==-1 || i<max_nb)) {
    s >> r.m_id >> r.m_from >> r.m_subject >> r.m_date >> r.m_thread_id
      >> r.m_status >> r.m_in_replyto >> r.m_sender_name >> r.m_pri >> r.m_flags
      >> r.m_recipients;
    msg_status_cache::update(r.m_id, r.m_status);
    m_results->push_back(r);

    date_stamp = r.m_date.isEmpty() ? QString("00000000000000%1").arg(r.m_id) :
      QString("%1%2").arg(r.m_date).arg(r.m_id);
    if (m_boundary.isEmpty() || m_boundary < date_stamp)
      m_boundary = date_stamp;

    if (m_min_msg_date.isEmpty() && !r.m_date.isEmpty())
      m_min_msg_date=r.m_date;
    else if (r.m_date < m_min_msg_date)
      m_min_msg_date=r.m_date;

    if (m_max_msg_date.isEmpty() && !r.m_date.isEmpty())
      m_max_msg_date=r.m_date;
    else if (r.m_date > m_max_msg_date)
      m_max_msg_date=r.m_date;

    if (m_min_mail_id==0)
      m_min_mail_id=r.m_id;
    else if (r.m_id < m_min_mail_id)
      m_min_mail_id=r.m_id;

    if (r.m_id > m_max_mail_id)
      m_max_mail_id=r.m_id;

    i++;
  }
  return i;
}

fetch_thread::fetch_thread()
{
  m_cnx=NULL;
  m_fetch_more=false;
}

// Launch the query and fetch results fetch. Overrides QThread::run()
void
fetch_thread::run()
{
  if (!m_cnx) return;
  DBG_PRINTF(5,"fetch_thread::run(), max_results=%d", m_max_results);
  m_errstr=QString::null;
  QTime start = QTime::currentTime();

  m_tuples_count=0;
  m_min_mail_id = m_max_mail_id = 0;
  m_max_msg_date = QString::null;
  m_min_msg_date = QString::null;
  m_boundary = QString::null;

  // special case repeated executions of the query for piecemeal fetch of
  // IWI results
  if (!m_psearch.m_parts.isEmpty()) {
    /* if it's a "fetch more" kind of search, m_nb_fetched_parts is the index of
       the last IWI part that was joined against at the previous step,
       otherwise it's 0 */
    int parts_idx = m_psearch.m_nb_fetched_parts;
    do {
      int part_no = m_psearch.m_parts.at(parts_idx++);
      QString s=m_query;
      try {
	sql_stream sq(m_query, *m_cnx);
	sq << part_no;
	sq.execute();
	store_results(sq, m_max_results-m_tuples_count);
	m_tuples_count += sq.row_count();
      }
      catch(db_excpt& x) {
	m_errstr = x.errmsg();
	break;
      }
    } while (m_tuples_count<m_max_results && parts_idx<m_psearch.m_parts.size());
    m_psearch.m_nb_fetched_parts = parts_idx-1;
  }
  else {
    // search not involving the word indexes
    try {
      sql_stream sq(m_query, *m_cnx);
      sq.execute();
      store_results(sq, m_max_results>0?m_max_results:-1);
      m_tuples_count = sq.row_count();
    }
    catch(db_excpt& x) {
      m_errstr = x.errmsg();
    }
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
    PQrequestCancel(c);
  }
  m_fetch_more=false;
}

void
fetch_thread::release()
{
  DBG_PRINTF(4, "fetch_thread::release()");
  if (m_cnx) {
    delete m_cnx;
    m_cnx=NULL;
  }
  m_fetch_more=false;
}

int
msgs_filter::parse_search_string(QString s, fts_options& opt)
{
  int state=10;
  QString curr_word;
  QString curr_substr;
  QString curr_op, curr_opval;
  uint len=s.length();
  for (uint i=0; i<len; i++) {
    QChar c=s.at(i);
    DBG_PRINTF(7, "p i=%u, char=%c, state=%d", i, c.toLatin1(), state);
    if (c==QChar('"')) {
      if (state==10) state=40;
      else if (state==40) {
	if (!curr_substr.isEmpty())
	  opt.m_substrs.append(curr_substr);
	state=10;
      }
      else if (state==50) {
	curr_word.append(c);
	curr_substr.append(c);	
      }
    }
    else if (state==10 && c==QChar(':')) {
      state=20; // search option expressed as optname:optvalue
      curr_op=curr_word;
      curr_opval.truncate(0);
      curr_word.truncate(0);
    }
    else if (state==20) {
      if (c==' ') {
	opt.m_operators.insert(curr_op, curr_opval);
	state=10;
      }
      else
	curr_opval.append(c); // TODO: better fail if c is not LetterOrNumber
    }
    else if (state==40 && c==QChar('\\')) {
      // next character is quoted
      state=50;
    }
    else if (!(c.isLetterOrNumber() || c=='_' || c=='.' || c=='@' || c=='-')) {
      // delimiter
      if (state==10 || state==40 || state==50) {
	if (!curr_word.isEmpty()) {
	  if (curr_word.at(0)=='-')
	    opt.m_exclude_words.append(curr_word.mid(1));
	  else
	    opt.m_words.append(curr_word);
	  curr_word.truncate(0);
	}
      }
      if (state==40 || state==50) {
	curr_substr.append(c);
      }
    }
    else {
      // non-delimiter (potential word constituant)
      curr_word.append(c);	// A4
      if (state==40 || state==50)
	curr_substr.append(c); // A6
    }
  }
  if (state!=10 && state!=20) {
    DBG_PRINTF(3, "parse error: state=%d", state);
  }
  if (state==20)
    opt.m_operators.insert(curr_op, curr_opval);

  if (!curr_word.isEmpty()) {
    if (curr_word.at(0)=='-')
      opt.m_exclude_words.append(curr_word.mid(1));
    else
      opt.m_words.append(curr_word);
  }

  return 0;
}

QString
msgs_filter::quote_like_arg(const QString& s)
{
  QString r=s;
  r.replace('\\', "\\\\");
  r.replace('_', "\\_");
  r.replace('\'', "\\'");
  r.replace('%', "\\%");
  r.append('%');
  r.prepend('%');
  return r;
}

/*
  Return values:
  0: database error
  1: OK
  2: the query would return no result (currently, it happens when a condition
   is set on an email address that doesn't exist in the ADDRESSES table,
   or a tag that's not in the TAGS table)

  'fetch_more' can be set on subsequent invocations to fetch more results
  (think "next page") based on the lower/higher (msg_date,mail_id) previously
  retrieved
*/
int
msgs_filter::build_query(sql_query& q, bool fetch_more/*=false*/)
{
  Q_UNUSED(fetch_more);
  db_cnx db;
  try {
    bool done_with_status=false;	// true when tests on status have been short-circuited
    QString sWhere;
    QString main_table;
    int status_set = m_status_set;
    int status_unset = m_status_unset;
    if (!m_include_trash) {
      if (m_in_trash) {
	status_set |= mail_msg::statusTrashed;
      }
      else {
	status_unset |= mail_msg::statusTrashed;
      }
    }

    main_table="mail m";
    if (m_tag_name == "(No tag set)") {
      /* optimize the cases of "no tag set" condition AND'ed with
	 some particular values for the status */
      if (m_status == -1 && !m_in_trash && m_status_set==0 && m_status_unset == mail_msg::statusArchived) {
	// current and not tagged
	q.add_clause(QString("m.mail_id in (SELECT ms.mail_id FROM mail_status ms"
			     " LEFT OUTER JOIN mail_tags mt ON mt.mail_id=ms.mail_id"
			     " WHERE  mt.mail_id is null AND ms.status&%1=0)")
		     .arg(mail_msg::statusArchived|mail_msg::statusTrashed));
	done_with_status=true;
      }
      else if (m_status==0) {
	// new and not tagged
	q.add_clause(QString("m.mail_id in (SELECT ms.mail_id FROM mail_status ms"
			     " LEFT OUTER JOIN mail_tags mt ON mt.mail_id=ms.mail_id"
			     " WHERE  mt.mail_id is null AND ms.status=0)"));
	done_with_status=true;
      }
      else {
	main_table += " LEFT OUTER JOIN mail_tags mt ON mt.mail_id=m.mail_id";
	q.add_clause(" mt.mail_id is null");
      }
    }
    q.add_table(main_table);

    // bounds. m_mail_id_bound and m_date_bound should be either both set or both unset
#if 0
    if (fetch_more && m_mail_id_bound>0) {
      if (m_order>0) {
	q.add_clause(QString("m.mail_id>%1").arg(m_mail_id_bound));
      }
      else
	q.add_clause(QString("m.mail_id<%1").arg(m_mail_id_bound));
    }
    if (fetch_more && !m_date_bound.isEmpty()) {
      if (m_order>0) {
	// msgs fetched from oldest to newest
	q.add_clause(QString("msg_date>=to_date('%1','YYYYMMDDHH24MISS')").arg(m_date_bound));
      }
      else {
	q.add_clause(QString("msg_date<=to_date('%1','YYYYMMDDHH24MISS')").arg(m_date_bound));  
      }
    }
#else
    if (!m_boundary.isEmpty() && !m_date_bound.isEmpty() && m_order<0) {
      q.add_clause(QString("msg_date<=to_date('%1','YYYYMMDDHH24MISS') AND to_char(msg_date,'YYYYMMDDHH24MISS')||m.mail_id::text<'%2'").arg(m_date_bound).arg(m_boundary));
    }
#endif

    if (!m_sql_stmt.isEmpty()) {
      q.add_clause(QString("m.mail_id in (") + m_sql_stmt + QString(")"));
    }

    if (m_min_prio <= max_possible_prio) {
      q.add_clause(QString("m.priority>=%1").arg(m_min_prio));
    }
    if (!m_sAddress.isEmpty()) {
      int addr_type;
      switch(m_nAddrType) {
	// is it really necessary to translate here? TODO: find a way
	// to get the caller to pass an addr_type from the mail_address enum
      case rFrom:
	addr_type = mail_address::addrFrom;
	break;
      case rTo:
	addr_type = mail_address::addrTo;
	break;
      case rCc:
	addr_type = mail_address::addrCc;
	break;
      default:
	addr_type = 0; // any
	break;
      }
      if (!add_address_selection(q, m_sAddress, addr_type))
	return 2;
    }
    if (!m_addr_to.isEmpty()) {
      if (!add_address_selection(q, m_addr_to, mail_address::addrTo))
	return 2;
    }
    //    if (!m_tag_name.isEmpty() && m_tag_name!="(No tag set)") {
    if (m_tag_id!=0) {
      q.add_table("mail_tags mq");
      q.add_clause(QString("mq.mail_id=m.mail_id and mq.tag=%1").arg(m_tag_id));
    }
    if (!m_subject.isEmpty()) {
      q.add_clause("subject", quote_like_arg(m_subject), " ILIKE ");
    }
    if (m_thread_id) {
      q.add_clause("thread_id", (int)m_thread_id);
    }
    if (!m_date_min.isNull() && m_date_min.isValid()) {
      QString date_clause;
      if (!m_date_max.isNull() && m_date_max.isValid()) {
	date_clause = QString("msg_date >= '%1'::date and msg_date<'%2'::date+1::int").arg(m_date_min.toString("yyyy/M/d")).arg(m_date_max.toString("yyyy/M/d"));
      }
      else {
	date_clause = QString("msg_date>='%1'::date").arg(m_date_min.toString("yyyy/M/d"));
      }
      q.add_clause(date_clause);
    }
    else if (!m_date_max.isNull() && m_date_max.isValid()) {
      q.add_clause(QString("msg_date<'%1'::date+1::int").arg(m_date_max.toString("yyyy/M/d")));
    }
    if (!m_body_substring.isEmpty()) {
      q.add_table("body b");
      q.add_clause(QString("strpos(b.bodytext,'") + m_body_substring + QString("')>0 and m.mail_id=b.mail_id"));
    }

    if (!m_fts.m_words.empty() || !m_fts.m_exclude_words.empty()) {
#ifdef PARTITIONED_WORDSEARCH
      m_psearch.get_index_parts(m_fts.m_words);
      if (!m_psearch.m_parts.isEmpty()) {
	QString words_incl = db_word::format_db_string_array(m_fts.m_words, db);
	q.add_clause(QString("m.mail_id in (select * from wordsearch_part(%1,:part_no))").arg(words_incl));
      }
      else
	q.add_clause("m.mail_id=0");
#else
      if (get_config().get_string("search/accents")=="unaccented" ||
	  m_fts.m_operators["accents"]=="i" ||
	  m_fts.m_operators["accents"]=="insensitive")
      {
	db_word::unaccent(m_fts.m_words);
	db_word::unaccent(m_fts.m_exclude_words);
      }
      QString words_incl = db_word::format_db_string_array(m_fts.m_words, db);
      QString words_excl = db_word::format_db_string_array(m_fts.m_exclude_words, db);
      q.add_clause(QString("m.mail_id in (select * from wordsearch(%1,%2))").arg(words_incl).arg(words_excl));
#endif
    }

    if (!m_fts.m_substrs.empty()) {
      QStringList::Iterator it = m_fts.m_substrs.begin();
      if (it!=m_fts.m_substrs.end())
	q.add_table("body b");
      for (; it!=m_fts.m_substrs.end(); ++it) {
	q.add_clause("bodytext ilike '" + quote_like_arg(*it) + "' AND m.mail_id=b.mail_id");
      }
    }

    if (m_mailId) {
      //sWhere.sprintf(" WHERE mail_id=%d", m_mailId);
      q.add_clause("mail_id", (int)m_mailId);
    }
    if (m_newer_than) {
      QString s;
      s.sprintf("(msg_date>=date_trunc('days',now()-interval '%d days'))", m_newer_than);
      //      s.sprintf("msg_day>=%d", days_to_now-(m_newer_than-1));
      q.add_clause(s);
    }
    // Selection on status bitmasks
    if ((status_set || status_unset) && !done_with_status && m_status==-1) {
      QString s;
      if (status_set) {
	if (!status_unset) {
	  s.sprintf("status&%d=%d", status_set, status_set);
	}
	else
	  s.sprintf("(status&%d=%d AND status&%d=0)", status_set, status_set, status_unset);
      }
      else {
	if (status_unset == mail_msg::statusArchived ||
	    status_unset == mail_msg::statusArchived+mail_msg::statusTrashed) {
	  // unprocessed messages: optimize by joining with mail_status
	  q.add_table("mail_status ms");
	  s.sprintf("ms.mail_id=m.mail_id AND ms.status&%d=0", status_unset);
	}
	else {
	  s.sprintf("status&%d=0", status_unset);
	}
      }
      q.add_clause(s);
    }
    if (m_status!=-1 && !done_with_status) {
      if (m_status==0) {
	// new messages: optimize by joining with mail_status
	q.add_table("mail_status ms");
	q.add_clause("ms.mail_id=m.mail_id AND ms.status=0");
      }
      else {
	QString s;
	s.sprintf("status=%d", m_status);
	q.add_clause(s);
      }
    }

    if (m_fts.m_words.isEmpty()) {
      QString sFinal="ORDER BY msg_date";
      if (m_order<0)
	sFinal+=" DESC";
      sFinal+=",mail_id";
      if (m_order<0)
	sFinal+=" DESC";
      if (m_max_results>0) {
	QString s;
	s.sprintf(" LIMIT %u", m_max_results+1);
	sFinal+=s;
      }
      q.add_final(sFinal);
    }
    else {
      // wordsearch has a different limit and sort
      // currently: none
#ifdef PARTITIONED_WORDSEARCH
      QString sFinal="ORDER BY to_char(msg_date,'YYYYMMDDHH24MISS')||m.mail_id::text DESC";
#else
      QString sFinal="ORDER BY to_char(msg_date,'YYYYMMDDHH24MISS') DESC, m.mail_id DESC";
#endif
      q.add_final(sFinal);
    }

    QString select = "SELECT m.mail_id,sender,subject,to_char(msg_date,'YYYYMMDDHH24MISS'),thread_id,m.status,in_reply_to,sender_fullname,priority,flags,recipients";
    q.start(select);
    if (m_sql_stmt.isEmpty())
      m_user_query = q.subquery("m.mail_id");
    else
      m_user_query=m_sql_stmt;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return 0;
  }
  return 1;
}

/*
  Return values: same as build_query()
*/
int
msgs_filter::asynchronous_fetch(fetch_thread* t, bool fetch_more/*=false*/)
{
  m_fetched = true;
  sql_query q;
  int r = build_query(q, fetch_more);
  if (r==1) {
    // start the query only if it might return results
    if (m_fetch_results)
      delete m_fetch_results;
    m_fetch_results = new std::list<mail_result>;
    t->m_results = m_fetch_results;
    t->m_fetch_more = fetch_more;
    t->m_max_results = m_max_results;
    t->m_psearch = m_psearch;
    if (!t->m_cnx) {
      t->m_cnx = new db_cnx(true);
      if (!t->m_cnx->ping()) {
	DBG_PRINTF(3, "Connection to database lost. Trying to reconnect");
	if (!t->m_cnx->datab()->reconnect()) {
	  DBG_PRINTF(3, "Failed to reconnect to the database");
	  m_errmsg = QObject::tr("No connection to the database.");
	  return 0;
	}
      }
    }
    t->m_query = q.get();
    m_start_time = QTime::currentTime();
    t->start();
  }
  return r;
}

/*
  Fetch the selection into a mail list widget
  Return values:
   0. fetch error. m_errmsg may contain an error message
   1. OK
   2. A condition doesn't match even before running the main query
     (example: non-existing email address)

  'fetch_more' can be set on subsequent invocations to fetch more results
  (think "next page") based on the lower/higher (msg_date,mail_id) previously
  retrieved
*/
int
msgs_filter::fetch(mail_listview* qlv, bool fetch_more/*=false*/)
{
  DBG_PRINTF(8, "msgs_filter::fetch()");
  m_errmsg=QString::null;
  m_fetched = true;
  int r=1;
  try {
    sql_query q;
    r=build_query(q, fetch_more);
    if (r==1) {
      db_cnx db;
      QString s=q.get();
      const char* query = s.toUtf8().constData();
      DBG_PRINTF(5,"%s", query);
      m_exec_time=0;
      m_start_time = QTime::currentTime();
#ifdef WITH_PGSQL
      PGconn* c=GETDB();
      PGresult* res = PQexec(c, query);
      if (res && PQresultStatus(res)==PGRES_TUPLES_OK) {
	m_exec_time = m_start_time.elapsed();
	m_fetch_results = new std::list<mail_result>;
	if (m_fetch_results) {
	  load_result_list(res, m_fetch_results, m_max_results-1);
	  make_list(qlv);
	  delete m_fetch_results;
	  m_fetch_results=NULL;
	}
      }
      else {
	DBG_PRINTF(2, "PQexec error");
	m_exec_time=-1;
	m_errmsg = PQerrorMessage(c);
	QMessageBox::warning(NULL, APP_NAME, QObject::tr("Unable to execute query.") + QString("\n")+ m_errmsg);
      }
      if (res)
	PQclear(res);
#endif
    }
    else if (r==0) {
      QMessageBox::information(NULL, APP_NAME, QObject::tr("Fetch error"));
    }
    else if (r==2) {
      QMessageBox::information(NULL, APP_NAME, QObject::tr("No results"));
    }
  }
  catch (int e) {
    r=e;
  }
  return r;
}

int
msgs_filter::exec_time() const
{
  return m_exec_time;
}

void
msgs_filter::add_result(mail_result& r, mail_listview* qlv)
{
  mail_result* iter = &r;
  if (qlv->find(iter->m_id)!=NULL)
    return;		// avoid duplicates
  {
    mail_msg* msg = new mail_msg(r);
    m_list_msgs.push_back(msg); // FIXME: check if we still need that list
    std::list<mail_msg*> list;
    list.push_back(msg);
    qlv->insert_list(list);
  }
}

void
msgs_filter::make_list(mail_listview* qlv)
{
  bool all_outgoing=false;
  if (!m_fetch_results)
    return;       // No result

  bool refetch=false;
  if (!qlv->empty()) {
    /* the list already contains some messages. That means
       that we'll test the messages coming from the db against
       those already in the list to avoid duplicates */
    refetch=true;
  }


  std::list<mail_result>::const_iterator iter = m_fetch_results->begin();
  if (iter!=m_fetch_results->end())
    all_outgoing = (iter->m_status & mail_msg::statusOutgoing) == mail_msg::statusOutgoing;

  for (; iter != m_fetch_results->end(); ++iter) {
    if (refetch) {
      if (qlv->find(iter->m_id)!=NULL)
	continue;		// avoid duplicates
    }

    mail_msg* msg = new mail_msg(iter->m_id, iter->m_from, iter->m_subject,
				 date(iter->m_date));
    msg->setThread(iter->m_thread_id);
    msg->set_orig_status(iter->m_status);
    msg->setStatus(iter->m_status);
    all_outgoing &= (iter->m_status & mail_msg::statusOutgoing) == mail_msg::statusOutgoing;
    msg->setInReplyTo((mail_id_t)iter->m_in_replyto);
    msg->set_sender_name(iter->m_sender_name);
    msg->set_flags(iter->m_flags);
    msg->set_priority(iter->m_pri);
    msg->set_recipients(iter->m_recipients);

    m_list_msgs.push_back(msg);
  }
  
  if (all_outgoing && qlv->empty() && get_config().get_bool("display/auto_sender_column"))
    qlv->swap_sender_recipient(true);

  qlv->insert_list(m_list_msgs);
}

/*
  Dialog used to fill in the criteria for a database fetch

  if 'open_new' is true, open a new msg_list_window with the results.
  if false, the caller has to provide a slot for the 'fetch_done' signal
  that will be emitted when the results are available.
*/
msg_select_dialog::msg_select_dialog(bool open_new/*=true*/) : QDialog(0)
{
  setWindowTitle(tr("Query selection"));
  setWindowIcon(UI_ICON(FT_ICON16_NEW_QUERY));
  m_waiting_for_results = false;
  m_new_selection=open_new;

  QVBoxLayout* topLayout = new QVBoxLayout(this);
  //  QVBox* box=new QVBox(this,"vbox");
  QLabel* lb=new QLabel(tr("Fill in one or more selection criteria:"), this);
  topLayout->addWidget(lb);

  QGridLayout* gridLayout = new QGridLayout();
  topLayout->addLayout(gridLayout);

  int nRow=0;
  m_wAddrType=new QComboBox(this);
  // each index in the combobox must match an entry in the enum
  // msgs_filter::recipient_type
  m_wAddrType->addItem("From");
  m_wAddrType->addItem("To");
  m_wAddrType->addItem("Cc");
  m_wAddrType->addItem("Any");
  connect(m_wAddrType, SIGNAL(activated(int)), SLOT(addr_type_changed(int)));
  gridLayout->addWidget(m_wAddrType,nRow,0);
  m_wcontact = new edit_address_widget(this);
  gridLayout->addWidget(m_wcontact, nRow, 1);

  nRow++;
  QLabel* lDate2 = new QLabel(tr("Dates between:"), this);
  gridLayout->addWidget(lDate2, nRow, 0);
  QWidget* hbdate = new QWidget;
  m_chk_datemin = new QCheckBox(tr("min"));
  m_wmin_date = new QDateTimeEdit;
  m_chk_datemax = new QCheckBox(tr("max"));
  m_wmax_date = new QDateTimeEdit;
  QHBoxLayout* hldate = new QHBoxLayout;
  hldate->setSpacing(2);
  hldate->addWidget(m_chk_datemin);
  hldate->addWidget(m_wmin_date);
  hldate->addWidget(m_chk_datemax);
  hldate->addWidget(m_wmax_date);
  hbdate->setLayout(hldate);

  gridLayout->addWidget(hbdate, nRow, 1);

  QString df = get_config().get_string("date_format");
  QString dorder="yyyy.MM.dd";
  
  if (df.startsWith("DD/MM/YYYY")) {
    dorder="dd.MM.yyyy";		// european-style date
  }
  m_wmax_date->setDisplayFormat(dorder);
  m_wmin_date->setDisplayFormat(dorder);

  {
    nRow++;
    QLabel* lDate=new QLabel(tr("Not older than:"), this);
    gridLayout->addWidget(lDate,nRow,0);
    QHBoxLayout* hbd = new QHBoxLayout();
    m_wdate_spin = new QSpinBox(this);
    m_wdate_spin->setMinimum(0);
    m_wdate_spin->setSpecialValueText(tr("(Ignore)"));
    hbd->addWidget(m_wdate_spin);

    m_wdate_button_group = new QButtonGroup(this);
    QHBoxLayout* interv_unit_layout = new QHBoxLayout;
    QRadioButton* r_day = new QRadioButton(tr("days"));
    QRadioButton* r_week = new QRadioButton(tr("weeks"));
    QRadioButton* r_month = new QRadioButton(tr("months"));
    m_wdate_button_group->addButton(r_day, 0);
    m_wdate_button_group->addButton(r_week, 1);
    m_wdate_button_group->addButton(r_month, 2);
    interv_unit_layout->addWidget(r_day);
    interv_unit_layout->addWidget(r_week);
    interv_unit_layout->addWidget(r_month);
  
    hbd->addLayout(interv_unit_layout);
    hbd->setStretchFactor(interv_unit_layout, 0);
    hbd->addStretch(1);
    r_day->setChecked(true);
    gridLayout->addLayout(hbd, nRow, 1);
  }

  nRow++;
  QLabel* lTo = new QLabel(tr("To:"), this);
  gridLayout->addWidget(lTo,nRow,0);
  m_wto = new edit_address_widget(this);
  gridLayout->addWidget(m_wto, nRow, 1);

  nRow++;
  QLabel* lSubject=new QLabel(tr("Subject:"), this);
  gridLayout->addWidget(lSubject,nRow,0);
  m_wSubject=new QLineEdit(this);
  gridLayout->addWidget(m_wSubject, nRow, 1);

  nRow++;
  QLabel* lString=new QLabel(tr("Contains:"), this);
  gridLayout->addWidget(lString,nRow,0);
  m_wString=new QLineEdit(this);
  gridLayout->addWidget(m_wString, nRow, 1);

  nRow++;
  QLabel* lSql=new QLabel(tr("SQL statement:"), this);
  gridLayout->addWidget(lSql,nRow,0);
  QHBoxLayout* hbz = new QHBoxLayout();
  QIcon zoom_icon(FT_MAKE_ICON(FT_ICON16_ZOOM_PAGE));
  m_wSqlStmt=new QLineEdit();
  hbz->addWidget(m_wSqlStmt);
  m_zoom_button = new QToolButton();
  hbz->addWidget(m_zoom_button);
  m_zoom_button->setIcon(zoom_icon);
  m_zoom_button->setToolTip(tr("Zoom"));
  gridLayout->addLayout(hbz, nRow, 1);
  connect(m_zoom_button, SIGNAL(clicked()), this, SLOT(zoom_on_sql()));

  // Selection by tag
  nRow++;
  QLabel* lTag = new QLabel(tr("Contains tag:"), this);
  gridLayout->addWidget(lTag,nRow,0);

  m_qtag_sel = new tag_selector(this);
  m_qtag_sel->init(true);
  gridLayout->addWidget(m_qtag_sel, nRow, 1);

  nRow++;
  gridLayout->addWidget(new QLabel(tr("Status:"), this), nRow,0);
  {
    QHBoxLayout* hb=new QHBoxLayout();
    hb->setSpacing(5);
    m_wStatus=new QLineEdit("status");
    hb->addWidget(m_wStatus);
    m_wStatus->setReadOnly(true);
    m_wStatus->setText(tr("Any"));
    m_wStatusMoreButton = new QPushButton(tr("Edit..."));
    hb->addWidget(m_wStatusMoreButton);
    gridLayout->addLayout(hb, nRow, 1);
    connect(m_wStatusMoreButton, SIGNAL(clicked()), this, SLOT(more_status()));
    // the default choice for the status is "either" for both set
    // and unset bits (=no filtering against status)
    m_status_set_mask = 0;
    m_status_unset_mask = 0; //mail_msg::statusArchived;
  }

  nRow++;
  gridLayout->addWidget(new QLabel(tr("Limit to:"),this), nRow, 0);
  // Limit to: [max results] messages (3 widgets)
  QHBoxLayout* hbox_lt=new QHBoxLayout();
  hbox_lt->setSpacing(10);
  m_wMaxResults=new QLineEdit();
  hbox_lt->addWidget(m_wMaxResults);
  QFontMetrics fm(m_wMaxResults->font());
  m_wMaxResults->setMaximumWidth(fm.width("000000")+15);

  m_wMaxResults->setText(get_config().get_string("max_msgs_per_selection"));
  hbox_lt->addWidget(new QLabel(tr("messages")));
  hbox_lt->addStretch(10);
  gridLayout->addLayout(hbox_lt, nRow, 1);

  nRow++;
  m_trash_checkbox = new QCheckBox(tr("In trash"),this);
  gridLayout->addWidget(m_trash_checkbox, nRow, 1);

  nRow++;
  QHBoxLayout* hbox=new QHBoxLayout;
  hbox->setMargin(10);
  hbox->setSpacing(20);

  m_wOkButton = new QPushButton(tr("OK"));
  hbox->addWidget(m_wOkButton);
  m_wOkButton->setDefault(true);

  m_wCancelButton = new QPushButton(tr("Cancel"));
  hbox->addWidget(m_wCancelButton);
  QPushButton* help_btn = new QPushButton(tr("Help"));
  hbox->addWidget(help_btn);
  connect(help_btn, SIGNAL(clicked()), this, SLOT(help()));

  gridLayout->addLayout(hbox, nRow, 0, 1, -1);

  connect(m_wOkButton, SIGNAL(clicked()), this, SLOT(ok()));
  connect(m_wCancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
}

QString
msg_select_dialog::str_status_mask()
{
  struct st_status {
    const char* name;
    int value;
  };
  static const st_status status_tab[] = {
    {QT_TR_NOOP("Rd"), mail_msg::statusRead},
    {QT_TR_NOOP("Rp"), mail_msg::statusReplied},
    {QT_TR_NOOP("F"), mail_msg::statusFwded},
    {QT_TR_NOOP("T"), mail_msg::statusTrashed},
    {QT_TR_NOOP("A"), mail_msg::statusArchived},
    {QT_TR_NOOP("O"), mail_msg::statusOutgoing},
    {QT_TR_NOOP("C"), mail_msg::statusComposed},
    {QT_TR_NOOP("S"), mail_msg::statusSent}
  };

  QString s;
  const QChar sep='+';
  for (uint i=0; i<sizeof(status_tab)/sizeof(status_tab[0]); i++) {
    if (m_status_set_mask & status_tab[i].value) {
      if (!s.isEmpty())
	s.append(sep);
      s.append(tr(status_tab[i].name));
    }
    if (m_status_unset_mask & status_tab[i].value) {
      if (!s.isEmpty())
	s.append(sep);
      s.append("!" + tr(status_tab[i].name));
    }
  }
  if (s.isEmpty())
    s="Any";
  return s;
}

status_dialog::status_dialog(QWidget* parent): QDialog(parent)
{
  QVBoxLayout* topLayout = new QVBoxLayout(this);
  m_statusBox = new select_status_box(true, this);
  topLayout->addWidget(m_statusBox);

  QHBoxLayout* hbox=new QHBoxLayout;
  hbox->setMargin(10);
  hbox->setSpacing(20);
  QPushButton* wOk=new QPushButton(tr("OK"));
  QPushButton* wCancel=new QPushButton(tr("Cancel"));
  hbox->addWidget(wOk);
  hbox->addWidget(wCancel);
  connect(wOk,SIGNAL(clicked()), this, SLOT(accept()));
  connect(wCancel,SIGNAL(clicked()), this, SLOT(reject()));
  topLayout->addLayout(hbox);
}

// Pops up the full status selection panel
void
msg_select_dialog::more_status()
{
  status_dialog* statusDlg=new status_dialog(this);

  statusDlg->m_statusBox->set_mask(m_status_set_mask, m_status_unset_mask);
  int r=statusDlg->exec();
  if (r==QDialog::Accepted) {
    m_status_set_mask=statusDlg->m_statusBox->mask_yes();
    m_status_unset_mask=statusDlg->m_statusBox->mask_no();
    m_wStatus->setText(str_status_mask());
  }
  delete statusDlg;
}

// Pops up a dialog for full text SQL editing
void
msg_select_dialog::zoom_on_sql()
{
  sql_editor* w=new sql_editor(this);
  QString initial_txt = m_wSqlStmt->text();
  w->set_text(initial_txt);
  int ret=w->exec();
  if (ret && w->get_text() != initial_txt) {
    m_wSqlStmt->setText(w->get_text());
  }
  w->close();
}

void
msg_select_dialog::to_filter(msgs_filter* filter)
{
  filter->m_body_substring = m_wString->text();
  filter->m_subject = m_wSubject->text();
  filter->m_sql_stmt = m_wSqlStmt->text();
  if (!m_wcontact->text().trimmed().isEmpty())
    filter->m_sAddress = mail_address::parse_extract_email(m_wcontact->text().trimmed());
  else
    filter->m_sAddress = QString::null;
  filter->m_nAddrType = m_wAddrType->currentIndex();

  if (!m_wto->text().trimmed().isEmpty())
    filter->m_addr_to = mail_address::parse_extract_email(m_wto->text().trimmed());
  else
    filter->m_addr_to = QString::null;
    
  filter->m_tag_id = m_qtag_sel->current_tag_id();

  filter->m_in_trash = m_trash_checkbox->isChecked();
  if (m_chk_datemax->isChecked())
    filter->m_date_max = m_wmax_date->date();
  else
    filter->m_date_max = QDate();
  if (m_chk_datemin->isChecked())
    filter->m_date_min = m_wmin_date->date();
  else
    filter->m_date_min = QDate();
  bool ok;
  int idate = m_wdate_spin->text().toInt(&ok);
  if (ok) {
    QAbstractButton* b = m_wdate_button_group->checkedButton();
    if (b && idate>0) {
      int d=0;
      filter->m_newer_details.days=0;
      filter->m_newer_details.weeks=0;
      filter->m_newer_details.months=0;
      switch(m_wdate_button_group->id(b)) {
      case 0:
	d=idate;
	filter->m_newer_details.days=idate;
	break;
      case 1:
	d=idate*7;
	filter->m_newer_details.weeks=idate;
	break;
      case 2:
	d=idate*30;
	filter->m_newer_details.months=idate;
	break;
      }
      filter->m_newer_than=d;
    }
  }
  if (m_wMaxResults->text().isEmpty())
    filter->m_max_results=0;	// no limit
  else {
    filter->m_max_results=m_wMaxResults->text().toUInt(&ok);
    if (!ok) {
      QMessageBox::information(this, "Error", tr("Error: non-numeric value for the maximum number of messages"));
      return;
    }
  }
  filter->m_status_set=m_status_set_mask;
  filter->m_status_unset=m_status_unset_mask;
}

void
msg_select_dialog::filter_to_dialog(const msgs_filter* filter)
{
  m_wmin_date->setDate(filter->m_date_min);
  if (!filter->m_date_min.isNull())
    m_chk_datemin->setChecked(true);
  m_wmax_date->setDate(filter->m_date_max);
  if (!filter->m_date_max.isNull())
    m_chk_datemax->setChecked(true);
  m_wString->setText(filter->m_body_substring);
  m_wSubject->setText(filter->m_subject);
  m_wSqlStmt->setText(filter->m_sql_stmt);
  m_wcontact->setText(filter->m_sAddress);
  m_wAddrType->setCurrentIndex(filter->m_nAddrType);
  m_trash_checkbox->setChecked(filter->m_in_trash);

  // tags
  if (filter->m_tag_id!=0) {
    m_qtag_sel->set_current_tag_id(filter->m_tag_id);
  }

  m_wto->setText(filter->m_addr_to);
  if (filter->m_max_results > 0) {
    m_wMaxResults->setText(QString("%1").arg(filter->m_max_results));
  }
  else
    m_wMaxResults->setText(QString::null);

  if (filter->m_newer_than>0) {
    int id=0,d=0;
    if (filter->m_newer_details.days) {
      id=0; d=filter->m_newer_details.days;
    }
    else if (filter->m_newer_details.weeks) {
      id=1; d=filter->m_newer_details.weeks;
    }
    else if (filter->m_newer_details.months) {
      id=2; d=filter->m_newer_details.months;
    }
    QAbstractButton* b = m_wdate_button_group->button(id);
    if (b) {
      QRadioButton* rb = dynamic_cast<QRadioButton*>(b);
      rb->setChecked(true);
      m_wdate_spin->setValue(d);
    }
  }
  m_status_set_mask = filter->m_status_set;
  m_status_unset_mask = filter->m_status_unset;
  m_wStatus->setText(str_status_mask());
}

void
msg_select_dialog::timer_done()
{
  if (m_waiting_for_results && m_thread.isFinished()) {
    m_waiting_for_results=false;
    DBG_PRINTF(5,"msg_select_dialog::timer_done()");
    QApplication::restoreOverrideCursor();

    m_filter.postprocess_fetch(m_thread);

    if (m_filter.m_fetch_results && m_filter.m_fetch_results->size() > 0) {
      if (m_new_selection) {
	msg_list_window* w = new msg_list_window(&m_filter, 0);
	w->show();
      }
      else {
	emit fetch_done(&m_filter);
      }
      m_thread.release();
      close();
    }
    else {
      m_waiting_for_results = false;
      m_timer->stop();
      delete m_timer;
      if (!m_thread.m_errstr.isEmpty()) {
	QMessageBox::information(this, APP_NAME, m_thread.m_errstr);
      }
      else
	QMessageBox::information(this, APP_NAME, tr("No results"));
      m_wCancelButton->setText(tr("Cancel"));
      enable_inputs(true);
      m_thread.release();
    }
  }
}

void
msg_select_dialog::enable_inputs(bool enable)
{
  QWidget* widgets[] = {
    m_wcontact, m_wAddrType, m_qtag_sel, m_wto,
    m_wSubject, m_wString, m_wSqlStmt, m_wStatus, m_wMaxResults,
    m_wOkButton, m_wStatusMoreButton, m_wmin_date, m_wmax_date,
    m_wdate_spin, m_chk_datemax, m_chk_datemin,
    m_zoom_button
  };
  for (uint i=0; i<sizeof(widgets)/sizeof(widgets[0]); i++)
    widgets[i]->setEnabled(enable);
}

void
msg_select_dialog::ok()
{
  //  msgs_filter cFilter;
  to_filter(&m_filter);
  int r = m_filter.asynchronous_fetch(&m_thread);
  // at this point, the query is currently being run in m_thread,
  // and we'll check for its completion in timer_done()
  if (r==1) {
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timer_done()));
    m_waiting_for_results = true;
    m_timer->start(100);		// check results every 1/10s

    //    QIconSet ico_stop(FT_MAKE_ICON(FT_ICON16_STOP));
    m_wCancelButton->setText(tr("Abort"));
    //    m_wCancelButton->setIconSet(ico_stop);
    enable_inputs(false);
    const QCursor cursor(Qt::WaitCursor);
    QApplication::setOverrideCursor(cursor);
  }
  else if (r==0) {
    QMessageBox::information(this, APP_NAME, tr("Fetch error"));
  }
  else if (r==2) {
    QMessageBox::information(this, APP_NAME, tr("No results"));
  }
}

void
msg_select_dialog::cancel()
{
  if (m_waiting_for_results) {
    // stop the query but don't close the dialog
    m_waiting_for_results = false;
    m_thread.cancel();
    m_thread.release();
    m_wCancelButton->setText(tr("Cancel"));
//    m_wCancelButton->setIconSet(QIconSet()); // empty pixmap
    QApplication::restoreOverrideCursor();
    enable_inputs(true);
  }
  else {
    // close the dialog
    close();
  }
}

void
msg_select_dialog::help()
{
  helper::show_help("query selection");
}

void
msg_select_dialog::addr_type_changed(int index)
{
  DBG_PRINTF(5,"addr_type_changed index=%d, curitem=%d",
	     index, m_wAddrType->currentIndex());
  int type;
  switch (index) {
  case 0:
    type=mail_address::addrFrom;
    break;
  case 1:
    type=mail_address::addrTo;
    break;
  case 2:
    type=mail_address::addrCc;
    break;
  default:
    type=0;
    break;
  }
#if 0
  m_wAddress->set_address_type(type);
#endif
}

//static
select_status_box::st_status select_status_box::m_status_tab[] = {
  {QT_TR_NOOP("Read"), mail_msg::statusRead},
  {QT_TR_NOOP("Replied"), mail_msg::statusReplied},
  {QT_TR_NOOP("Forwarded"), mail_msg::statusFwded},
  {QT_TR_NOOP("Trashed"), mail_msg::statusTrashed},
  {QT_TR_NOOP("Archived"), mail_msg::statusArchived},
  {QT_TR_NOOP("Outgoing"), mail_msg::statusOutgoing},
  {QT_TR_NOOP("Sent"), mail_msg::statusSent},
  {QT_TR_NOOP("Composed"), mail_msg::statusComposed}
};

select_status_box::select_status_box(bool either, QWidget* parent):
  QFrame (parent)
{
  //  setTitle("Status");
//    setFrameStyle( QFrame::Box | QFrame::Plain );
//    setLineWidth (2);
//    setMargin(10);

  m_either = either;
  QGridLayout* grid = new QGridLayout(this);
  grid->setMargin(4);
  grid->setSpacing(4);
  int button_id=0;
  for (uint i=0; i<sizeof(m_status_tab)/sizeof(m_status_tab[0]); i++) {
    grid->addWidget(new QLabel(tr(m_status_tab[i].name)), i, 0);
    QCheckBox* b1=new QCheckBox(tr("Yes"));
    QCheckBox* b2=new QCheckBox(tr("No"));
    QCheckBox* b3=new QCheckBox(tr("Either"));
    grid->addWidget(b1, i, 1);
    grid->addWidget(b2, i, 2);
    if (!either)
      b3->hide();
    else
      grid->addWidget(b3, i, 3);
    QButtonGroup* g=new QButtonGroup;
    g->setExclusive(TRUE);
    g->addButton(b1, button_id++);
    g->addButton(b2, button_id++);
    g->addButton(b3, button_id++);
    connect(g, SIGNAL(buttonClicked(int)),SLOT(status_changed(int)));
    m_button_groups.push_back(g);
  }
  m_mask_set=m_mask_unset=0;
}

void
select_status_box::set_mask (int mask_yes, int mask_no)
{
  DBG_PRINTF(3, "mask_yes=%x, mask_no=%x", mask_yes, mask_no);
  const int buttons_per_status = 3;
  for (uint i=0; i<sizeof(m_status_tab)/sizeof(m_status_tab[0]); i++) {
    QButtonGroup* g=m_button_groups[i];
    int v=m_status_tab[i].value;
    QAbstractButton* button;
    if (v & mask_yes) {
      button = g->button(i*buttons_per_status+0); // Yes
      button->setChecked(true);
    }
    else if (v & mask_no) {
      button = g->button(i*buttons_per_status+1); // No
      button->setChecked(true);
    }
    else if (m_either) {
      button = g->button(i*buttons_per_status+2); // Either
      button->setChecked(true);
    }
  }
  m_mask_set=mask_yes;
  m_mask_unset=mask_no;
}

// Ignore the "either" choice and just get the value of the status
int
select_status_box::status() const
{
  return m_mask_set;
}

int
select_status_box::mask_yes() const
{
  return m_mask_set;
}

int
select_status_box::mask_no() const
{
  return m_mask_unset;
}

void
select_status_box::status_changed(int id)
{
  const int buttons_per_status = 3;
  int status = id / buttons_per_status;		// 2 or 3 buttons per status
  QButtonGroup* g=m_button_groups[status];
  if (g) {
    QAbstractButton* b=g->button(id);
    if (b && b->isChecked()) {
      int v=m_status_tab[status].value;
      switch (id % buttons_per_status) {
      case 0:			// Yes
	m_mask_set |= v;
	m_mask_unset &= ~v;
	break;
      case 1:			// No
	m_mask_set &= ~v;
	m_mask_unset |= v;
	break;
      case 2:			// Either
	m_mask_set &= ~v;
	m_mask_unset &= ~v;
	break;
      default:
	DBG_PRINTF(5,"id out of (0,1,2)");
	break;
      }
    }
  }
}

