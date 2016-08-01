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
#include <QDateEdit>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QCalendarWidget>

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
  m_has_progress_bar = false;
  m_date_clause = QString::null;
  m_date_before_clause = QString::null;
  m_date_after_clause = QString::null;
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

  m_mail_id_bound = 0;
  m_date_bound = QString::null;

  // results
  m_fetched = false;
  m_fetch_results = NULL;

  m_results_date_bound = QString::null;
  m_results_mail_id_bound = 0;
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
  Q_UNUSED(thread);
}


// Store post-fetch state to the filter from the fetch_thread
void
msgs_filter::postprocess_fetch(fetch_thread& thread)
{
  m_has_more_results = m_max_results>0 && (thread.m_tuples_count > m_max_results);

  if (m_order>0) {
    m_results_mail_id_bound = thread.m_max_mail_id;
    m_results_date_bound = thread.m_max_msg_date;
  }
  else {
    m_results_mail_id_bound = thread.m_min_mail_id;
    m_results_date_bound = thread.m_min_msg_date;
  }
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

/*
  Parse a searchbar expression. No semantic analysis, just extraction of tokens.
*/
int
msgs_filter::parse_search_string(QString s, fts_options& opt)
{
  /* state:
     10: start or in-between tokens
     20: option value
     40: word inside enclosing double quotes
     45: option value inside enclosing double quotes
     50: next character is backslash-quoted (outside of enclosed text)
     55: next character is backslash-quoted (inside of enclosed text in option)
  */
  int state=10;

  QString curr_word;
  QString curr_substr;
  QString curr_op, curr_opval;
  uint len=s.length();
  for (uint i=0; i<len; i++) {
    QChar c=s.at(i);
    DBG_PRINTF(7, "p i=%u, char=%c, state=%d", i, c.toLatin1(), state);
    if (c==QChar('"')) {
      if (state==10)
	state=40;
      else if (state==20)
	state=45;
      else if (state==40) {
	if (!curr_substr.isEmpty())
	  opt.m_substrs.append(curr_substr);
	state=10;
      }
      else if (state==45) {
	opt.m_operators.insertMulti(curr_op, curr_opval);
	state=10;
      }
      else if (state==50) {
	curr_word.append(c);
	curr_substr.append(c);	
      }
      else if (state==55) {
	curr_opval.append(c);	// append double-quote as a normal character
	state=45;
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
	opt.m_operators.insertMulti(curr_op, curr_opval);
	state=10;
      }
      else
	curr_opval.append(c); // TODO: better fail if c is not LetterOrNumber
    }
    else if (state==40 && c==QChar('\\')) {
      // next character (in non-option enclosed section) is quoted
      state=50;
    }
    else if (state==45 && c==QChar('\\')) {
      // next character (in enclosed option value) is quote
      state=55;
    }
    else if (state==45) {
      curr_opval.append(c);
    }
    else if (state==55) {
      curr_opval.append(c);
      state=45;
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
    opt.m_operators.insertMulti(curr_op.toLower(), curr_opval);

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

// static
const char*
msgs_filter::select_list_mail_columns =
  "m.mail_id,"
  "sender,"
  "subject,"
  "to_char(msg_date,'YYYYMMDDHH24MISS'),"
  "thread_id,"
  "m.status,"
  "in_reply_to,"
  "sender_fullname,"
  "priority,"
  "flags,"
  "recipients";

/*
  Return values:
  0: database error or invalid input
  1: OK
  2: the query would return no result (currently, it happens when a condition
     is set on an email address that doesn't exist in the ADDRESSES table,
     or a tag that's not in the TAGS table)
  3: there are invalid parameters in the filter that imply a user-visible
     error message.

  'fetch_more' can be set on subsequent invocations to fetch more results
  (think "next page") based on the lower/higher (msg_date,mail_id) previously
  retrieved
*/
int
msgs_filter::build_query(sql_query& q)
{
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

    if (!m_date_bound.isEmpty()) {
      if (m_order < 0) {
	q.add_clause(QString("(m.msg_date,m.mail_id) < (to_timestamp('%1','YYYYMMDDHH24MISS'),%2)").arg(m_date_bound).arg(m_mail_id_bound));
      }
      else if (m_order > 0) {
	q.add_clause(QString("(m.msg_date,m.mail_id) > (to_timestamp('%1','YYYYMMDDHH24MISS'),%2)").arg(m_date_bound).arg(m_mail_id_bound));
      }
    }

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

    // <date clause>
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

    /* The date from the search bar is ignored if a date clause already exists.
       It shouldn't normally happen that both are specified. */
    if (m_date_clause.isEmpty()) {
      m_date_clause = m_fts.m_operators["date"];
      m_date_before_clause = m_fts.m_operators["before"];
      m_date_after_clause = m_fts.m_operators["after"];
    }

    if (!m_date_clause.isEmpty())
      process_date_clause(q, date_equal, m_date_clause);
    if (!m_date_before_clause.isEmpty())
      process_date_clause(q, date_before, m_date_before_clause);
    if (!m_date_after_clause.isEmpty())
      process_date_clause(q, date_after, m_date_after_clause);
      
    // </date clause>

    // status clause through is:status and isnot:status
    if (m_fts.m_operators.contains("is")) {
      process_status_clause(q, status_is, m_fts.m_operators.values("is"));
    }
    if (m_fts.m_operators.contains("isnot")) {
      process_status_clause(q, status_isnot, m_fts.m_operators.values("isnot"));
    }

    // from/to/cc clauses
    if (m_fts.m_operators.contains("from")) {
      process_address_clause(q, from_address, m_fts.m_operators.values("from"));
    }
    if (m_fts.m_operators.contains("to")) {
      process_address_clause(q, to_address, m_fts.m_operators.values("to"));
    }
    if (m_fts.m_operators.contains("cc")) {
      process_address_clause(q, cc_address, m_fts.m_operators.values("cc"));
    }

    // tag clause
    if (m_fts.m_operators.contains("tag")) {
      process_tag_clause(q, m_fts.m_operators.values("tag"));
    }

    if (!m_body_substring.isEmpty()) {
      q.add_table("body b");
      q.add_clause(QString("strpos(b.bodytext,'") + m_body_substring + QString("')>0 and m.mail_id=b.mail_id"));
    }

    if (!m_fts.m_words.isEmpty() || !m_fts.m_exclude_words.isEmpty()) {
      if (get_config().get_string("search/accents")=="unaccented" ||
	  m_fts.m_operators["accents"]=="i" ||
	  m_fts.m_operators["accents"]=="insensitive")
	{
	  db_word::unaccent(m_fts.m_words);
	  db_word::unaccent(m_fts.m_exclude_words);
	}
      QString words_incl = db_word::format_db_string_array(m_fts.m_words, db);
      QString words_excl = db_word::format_db_string_array(m_fts.m_exclude_words, db);
      if (!m_fts.m_words.isEmpty()) {
	q.add_clause(QString("m.mail_id in (select * from wordsearch(%1,%2))").arg(words_incl).arg(words_excl));
      }
      else {
	for (int i=0; i<m_fts.m_exclude_words.count(); i++) {
	  QString excl = db_word::format_db_string_array(QStringList(m_fts.m_exclude_words.at(i)), db);
	  q.add_clause(QString("m.mail_id not in (select * from wordsearch(%1))").arg(excl));
	}
      }
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

    {
      QString sFinal = QString("ORDER BY (m.msg_date,m.mail_id) %1").
	arg(m_order<0 ? "DESC" : "ASC");
      if (m_max_results>0) {
	sFinal.append(QString(" LIMIT %1").arg(m_max_results+1));
      }
      q.add_final(sFinal);
    }

    q.start(QString("SELECT ") + select_list_mail_columns);
    if (m_sql_stmt.isEmpty())
      m_user_query = q.subquery("m.mail_id");
    else
      m_user_query = m_sql_stmt;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return 0;
  }
  catch(QString errmsg) {
    QMessageBox::critical(NULL, QObject::tr("Error"), errmsg);
    // to be safe, leave a clean query, but the caller shouldn't use it
    q = msgs_filter::empty_list_query();
    return 3;
  }
  return 1;
}

// static
sql_query
msgs_filter::empty_list_query()
{
  sql_query q;
  q.add_table("mail");
  q.start(QString("SELECT ") + select_list_mail_columns);
  q.add_clause("false");
  return q;
}

/*
  Create SQL clauses from the given comparator (equal,before,after)
  to compare mail.msg_date with date_expr.
  Does not process m_date_min and m_date_max.
  For date_before and date_after, the boundary is included.
  May throw an exception with a QString.
*/
void
msgs_filter::process_date_clause(sql_query& q, date_comparator comp, QString date_expr)
{
  if (date_expr.isEmpty())
    return;

  QDate qd;

  if (date_expr == "today") {
    if (comp == date_equal)
      q.add_clause("msg_date>=date_trunc('day',now()) AND msg_date<date_trunc('day',now())+'1 day'::interval");
    else if (comp == date_before) {
      q.add_clause("msg_date<current_date");
    }
    else if (comp == date_after) {
      q.add_clause("msg_date>=current_date");
    }
  }
  else if (date_expr == "yesterday") {
    if (comp == date_equal)
      q.add_clause("msg_date>=current_date-1 AND msg_date<current_date");
    else if (comp == date_before)
      q.add_clause("msg_date<current_date");
    else if (comp == date_after)
      q.add_clause("msg_date>=current_date-1");
  }
  else if (date_expr.at(0) == '-') {
    // -[0-9]+ means "at most N days old". Implemented only for date_equal comparison
    if (comp == date_equal) {
      QRegExp rx("^-([0-9]+)$");
      if (date_expr.indexOf(rx) == 0) {
	int days = rx.capturedTexts().at(1).toInt();
	q.add_clause(QString("msg_date>=now()-'%1 days'::interval").arg(days));
      }
    }
  }
  else {
    QRegExp rx("^\\d{4}$");
    // exactly 4 digits: interpreted as a year (YYYY)
    if (rx.indexIn(date_expr) == 0) {
      int year = rx.cap(0).toInt();
      qd.setDate(year,1,1);
      if (!qd.isValid())
	throw QObject::tr("Invalid year in date parameter.");
      if (comp == date_equal)
	q.add_clause(QString("msg_date>='%1-01-01'::date AND msg_date<'%2-01-01'::date")
		     .arg(year).arg(year+1));
      else if (comp == date_before)
	q.add_clause(QString("msg_date<'%1-01-01'::date").arg(year+1));
      else if (comp == date_after)
	q.add_clause(QString("msg_date>='%1-01-01'::date").arg(year));
    }
    else {
      QRegExp rx1("^(\\d{4})-(\\d\\d?)$");
      // YYYY-MM
      if (rx1.indexIn(date_expr) == 0) {
	qd.setDate(rx1.cap(1).toInt(), rx1.cap(2).toInt(), 1);
	if (!qd.isValid())
	  throw QObject::tr("Invalid year-month in date parameter.");
	if (comp == date_equal) 
	  q.add_clause(QString("msg_date>='%1-%2-01'::date AND "
			       "msg_date<'%1-%2-01'::date+'1 month'::interval")
		       .arg(rx1.cap(1)).arg(rx1.cap(2)));
	else if (comp == date_before)
	  q.add_clause(QString("msg_date<'%1-%2-01'::date+'1 month'::interval")
		       .arg(rx1.cap(1)).arg(rx1.cap(2)));
	if (comp == date_after) 
	  q.add_clause(QString("msg_date>='%1-%2-01'::date")
		       .arg(rx1.cap(1)).arg(rx1.cap(2)));
      }
      else {
	QRegExp rx2("^(\\d{4})-(\\d\\d?)-(\\d\\d?)$");
	// YYYY-MM-DD
	if (rx2.indexIn(date_expr) == 0) {
	  qd.setDate(rx2.cap(1).toInt(), rx2.cap(2).toInt(), rx2.cap(3).toInt());
	  if (!qd.isValid())
	    throw QObject::tr("Invalid year-month-day in date parameter.");
	  if (comp == date_equal)
	    q.add_clause(QString("msg_date>='%1-%2-%3'::date AND "
				 "msg_date<'%1-%2-%3'::date+'1 day'::interval")
			 .arg(rx2.cap(1)).arg(rx2.cap(2)).arg(rx2.cap(3)));
	  else if (comp == date_before)
	    q.add_clause(QString("msg_date<'%1-%2-%3'::date+'1 day'::interval")
			 .arg(rx2.cap(1)).arg(rx2.cap(2)).arg(rx2.cap(3)));
	  else if (comp == date_after)
	    q.add_clause(QString("msg_date>='%1-%2-%3'::date")
			 .arg(rx2.cap(1)).arg(rx2.cap(2)).arg(rx2.cap(3)));
	}
	else
	  throw QObject::tr("Unable to parse date expression.");
      }
    }
  }
}

/*
  Create SQL clauses from the given comparator (status_is, status_isnot)
  to test if mail.status has or doesn't have bits corresponding
  to the statuses named in <vals>.
*/
void
msgs_filter::process_status_clause(sql_query& q,
				   status_comparator comp,
				   QList<QString> vals)
{
  static struct {
    const char* name;
    int bitmask;
  } statuses[] = {
    {"read", mail_msg::statusRead},
    {"replied", mail_msg::statusReplied},
    {"forwarded", mail_msg::statusFwded},
    {"archived", mail_msg::statusArchived},
    {"sent", mail_msg::statusSent}
  };

  for (int si=0; si < vals.size(); ++si) {
    const QString& s = vals.at(si);
    for (int i=0; i < (int)sizeof(statuses)/(int)sizeof(statuses[0]); i++) {
      if (QString::compare(s, statuses[i].name, Qt::CaseInsensitive)==0) {
	if (comp == status_is)
	  q.add_clause(QString("m.status&%1=%1").arg(statuses[i].bitmask));
	else if (comp == status_isnot)
	  q.add_clause(QString("m.status&%1=0").arg(statuses[i].bitmask));
      }
    }
  }
}

/*
  Create SQL clauses to filter on from/to/cc
  <vals> are interpreted as email addresses (no name)
*/
void
msgs_filter::process_address_clause(sql_query& q,
				    address_type atype,
				    QList<QString> vals)
{
  for (int si=0; si < vals.size(); ++si) {
    int cnt = ++m_addresses_count;
    int itype;
    switch(atype) {
    case from_address:
      itype = 1; break;
    case to_address:
      itype = 2; break;
    case cc_address:
      itype = 3; break;
    default:
      throw QString("Unexpected address type %1").arg((int)atype);
    }
    q.add_table(QString("mail_addresses ma%1").arg(cnt));
    q.add_table(QString("addresses a%1").arg(cnt));

    q.add_clause(QString("ma%1.addr_type=%2").arg(cnt).arg(itype));
    q.add_clause(QString("m.mail_id=ma%1.mail_id").arg(cnt));
    q.add_clause(QString("ma%1.addr_id=a%1.addr_id").arg(cnt));
    q.add_clause(QString("a%1.email_addr").arg(cnt), vals.at(si));
  }
}

/*
  Create SQL clauses to filter on tags
*/
void
msgs_filter::process_tag_clause(sql_query& q, QList<QString> vals)
{
  for (int si=0; si < vals.size(); ++si) {
    int tag_id = tags_repository::hierarchy_lookup(vals.at(si));
    DBG_PRINTF(7, "tag looked up: %s, found=%d", vals.at(si).toLocal8Bit().constData(),
	       tag_id);
    if (tag_id) {
      q.add_table(QString("mail_tags mt%1").arg(++m_addresses_count));
      q.add_clause(QString("mt%1.mail_id=m.mail_id AND mt%1.tag=%2").
		   arg(m_addresses_count).arg(tag_id));
    }
    else {
      q.add_clause("false");
    }
  }
}

/*
  Return values: same as build_query()

  'fetch_more' can be set on subsequent invocations to fetch more results
  (think "next page") based on the lower/higher (msg_date,mail_id) previously
  retrieved
*/
int
msgs_filter::asynchronous_fetch(fetch_thread* t, bool fetch_more/*=false*/)
{
  m_fetched = true;
  sql_query q;
  if (fetch_more) {
    m_date_bound = m_results_date_bound;
    m_mail_id_bound = m_results_mail_id_bound;
  }
  int r = build_query(q);
  if (r==1) {
    // start the query only if it might return results
    if (m_fetch_results)
      delete m_fetch_results;
    m_fetch_results = new std::list<mail_result>;
    t->m_results = m_fetch_results;
    t->m_fetch_more = fetch_more;
    t->m_max_results = m_max_results;

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
    DBG_PRINTF(4, "m_query=%s\n", t->m_query.toLocal8Bit().constData());
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
msgs_filter::fetch(mail_listview* qlv, bool fetch_more)
{
  DBG_PRINTF(8, "msgs_filter::fetch()");
  m_errmsg=QString::null;
  m_fetched = true;
  int r=1;
  try {
    sql_query q;
    if (fetch_more) {
      m_date_bound = m_results_date_bound;
      m_mail_id_bound = m_results_mail_id_bound;
    }
    r = build_query(q);
    if (r==1) {
      db_cnx db;
      QString s=q.get();
      QByteArray qb_query = s.toUtf8();
      const char* query = qb_query.constData();
      m_exec_time=0;
      m_start_time = QTime::currentTime();
#ifdef WITH_PGSQL
      PGconn* c=GETDB();
      PGresult* res=NULL;
      PGresult* res1=NULL;
      PGresult* res2=NULL;

      res1 = PQexec(c, "BEGIN; SET TRANSACTION READ ONLY");
      if (!res1 || PQresultStatus(res1)!=PGRES_COMMAND_OK) {
	QMessageBox::warning(NULL, APP_NAME, QObject::tr("Unable to execute query.") + QString("\n")+ PQerrorMessage(c));
      }
      else {
	DBG_PRINTF(5,"query=%s", query);
	res = PQexec(c, query);
	if (res && PQresultStatus(res)==PGRES_TUPLES_OK) {
	  m_exec_time = m_start_time.elapsed();
	  m_fetch_results = new std::list<mail_result>;
	  if (m_fetch_results) {
	    load_result_list(res, m_fetch_results, m_max_results-1);
	    make_list(qlv);
	    delete m_fetch_results;
	    m_fetch_results=NULL;
	  }
	  PGresult* res4=PQexec(c, "END");
	  if (res4)
	    PQclear(res4);
	}
	else {
	  DBG_PRINTF(2, "PQexec error: %s", PQerrorMessage(c));
	  m_exec_time=-1;
	  m_errmsg = PQerrorMessage(c);
	  PGresult* res3 = PQexec(c, "ROLLBACK");
	  QMessageBox::warning(NULL, APP_NAME, QObject::tr("Unable to execute query.") + QString("\n")+ m_errmsg);
	  if (!res3 || PQresultStatus(res3)!=PGRES_COMMAND_OK) {
	    QMessageBox::warning(NULL, QObject::tr("Database error"), QObject::tr("In addition, could not execute ROLLBACK.") + QString("\n")+ PQerrorMessage(c));
	  }
	}
      }
      if (res)
	PQclear(res);
      if (res1)
	PQclear(res1);
      if (res2)
	PQclear(res2);
#endif
    }
    else if (r==0) {
      QMessageBox::information(NULL, APP_NAME, QObject::tr("Fetch error"));
    }
    else if (r==2) {
      QMessageBox::information(NULL, APP_NAME, QObject::tr("No results"));
    }
    //  if (r==3) do nothing
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
  topLayout->addWidget(new QLabel(tr("Fill in the selection criteria:")));

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
  gridLayout->addWidget(m_wAddrType,nRow,0);
  m_wcontact = new edit_address_widget(this);

  gridLayout->addWidget(m_wcontact, nRow, 1);

  nRow++;
  gridLayout->addWidget(new QLabel(tr("Date:")), nRow, 0);
  QHBoxLayout* hldate = new QHBoxLayout;
  hldate->setSpacing(2);

  m_date_cb = new QComboBox;
  m_date_cb->addItem(tr("Any date"), QVariant("")); // index for "Any date" must be 0
  m_date_cb->addItem(tr("Today"), QVariant("today"));
  m_date_cb->addItem(tr("Yesterday"), QVariant("yesterday"));
  m_date_cb->addItem(tr("Last 7 days"), QVariant("-7"));
  m_date_cb->addItem(tr("Last 30 days"), QVariant("-30"));
  m_date_cb->addItem(tr("Last 60 days"), QVariant("-60"));
  m_date_cb->addItem(tr("Last 90 days"), QVariant("-90"));
  m_date_cb->addItem(tr("Last 365 days"), QVariant("-365"));
  m_date_cb->addItem(tr("Range..."), QVariant("range"));
  connect(m_date_cb, SIGNAL(currentIndexChanged(int)), this, SLOT(date_cb_changed(int)));
  hldate->addWidget(m_date_cb);

  m_chk_datemin = new QCheckBox(tr("Start"));
  //  m_chk_datemin->setStyleSheet("QCheckBox{spacing:0px}");
  m_wmin_date = new QDateEdit;
  m_wmin_date->setCalendarPopup(true);
  m_wmin_date->setDate(QDate::currentDate());

  m_chk_datemax = new QCheckBox(tr("End"));
  //  m_chk_datemax->setStyleSheet("QCheckBox{spacing:0px}");
  m_wmax_date = new QDateEdit;
  m_wmax_date->setCalendarPopup(true);
  m_wmax_date->setDate(QDate::currentDate());

  set_date_style();

  hldate->addWidget(m_chk_datemin);
  hldate->addWidget(m_wmin_date);
  hldate->addWidget(m_chk_datemax);
  hldate->addWidget(m_wmax_date);

  gridLayout->addLayout(hldate, nRow, 1);

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
  QLabel* lTag = new QLabel(tr("Tag:"), this);
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
  m_trash_checkbox = new QCheckBox(tr("In trashcan"),this);
  gridLayout->addWidget(m_trash_checkbox, nRow, 1);

  nRow++;
  QHBoxLayout* hbox=new QHBoxLayout;
  hbox->setMargin(10);
  hbox->setSpacing(20);

  m_btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
  gridLayout->addWidget(m_btn_box, nRow, 0, 1, -1);

  connect(m_btn_box, SIGNAL(helpRequested()), this, SLOT(help()));
  connect(m_btn_box, SIGNAL(accepted()), this, SLOT(ok()));
  connect(m_btn_box, SIGNAL(rejected()), this, SLOT(cancel()));
  enable_date_range();

  if (!get_config().get_bool("query_dialog/address_autocompleter")) {
    m_wcontact->enable_completer(false);
    m_wto->enable_completer(false);
  }

}

void
msg_select_dialog::set_date_style()
{
  QString df = get_config().get_string("date_format");
  QString fmt;
  if (df.startsWith("DD/MM/YYYY")) {
    fmt="dd/MM/yyyy";		// european-style date
  }
  else if (df.startsWith("MM/DD/YYYY")) {
    fmt="MM/dd/yyyy";		// american-style date
  }
  else {
    // non fixed date_format
    // check if the day comes after the month (american style) or the opposite
    QString ddf=m_wmax_date->displayFormat();
    QRegExp rx("M+\\WD+\\WY+", Qt::CaseInsensitive);
    if (rx.exactMatch(ddf)) {
      fmt="MM/dd/yyyy";
    }
    else {
      fmt="dd/MM/yyyy";      
    }
  }
  m_wmax_date->setDisplayFormat(fmt);
  m_wmin_date->setDisplayFormat(fmt);

  // QCalendarWidget does not automatically set the first day of week
  // based on locale, so do it explicitly.
  QCalendarWidget* calmin = m_wmin_date->calendarWidget();
  calmin->setFirstDayOfWeek(QLocale::system().firstDayOfWeek());

  QCalendarWidget* calmax = m_wmax_date->calendarWidget();
  calmax->setFirstDayOfWeek(QLocale::system().firstDayOfWeek());
}

void
msg_select_dialog::date_cb_changed(int index)
{
  Q_UNUSED(index);
  enable_date_range();
  if (m_date_cb->itemData(index).toString()=="range") {
    m_chk_datemax->setEnabled(true);
    m_chk_datemin->setEnabled(true);
    if (!m_chk_datemax->isChecked() && !m_chk_datemin->isChecked()) {
      /* if the user chooses the range option and no bound is checked yet,
	 check one automatically to avoid no-range-at-all results */
      m_chk_datemin->setChecked(true);
    }
  }
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
  if (!m_wString->text().isEmpty() && get_config().get_bool("query_dialog/iwi_search")) {
    // transform substring into words to query through the inverted word index
    filter->parse_search_string(m_wString->text(), filter->m_fts);
  }
  else {
    filter->m_body_substring = m_wString->text();
  }
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

  if (m_date_cb->currentIndex()>0) {
    QString str=m_date_cb->itemData(m_date_cb->currentIndex()).toString();
    filter->m_date_clause=str;
    if (str=="range") {
      if (m_chk_datemax->isChecked())
	filter->m_date_max = m_wmax_date->date();
      else
	filter->m_date_max = QDate();
      if (m_chk_datemin->isChecked())
	filter->m_date_min = m_wmin_date->date();
      else
	filter->m_date_min = QDate();
    }
  }
  else {
    filter->m_date_clause = QString::null;
    filter->m_date_before_clause = QString::null;
    filter->m_date_after_clause = QString::null;
    filter->m_date_min = QDate();
    filter->m_date_max = QDate();
  }

  if (m_wMaxResults->text().isEmpty())
    filter->m_max_results=0;	// no limit
  else {
    bool ok;
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

  if (!filter->m_date_clause.isEmpty()) {
    for (int i=1; i<m_date_cb->count(); i++) {
      if (m_date_cb->itemData(i)==filter->m_date_clause) {
	m_date_cb->setCurrentIndex(i);
	break;
      }
    }
  }
  else
    m_date_cb->setCurrentIndex(0);

  m_status_set_mask = filter->m_status_set;
  m_status_unset_mask = filter->m_status_unset;
  m_wStatus->setText(str_status_mask());
  enable_date_range();
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
      m_btn_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
      enable_inputs(true);
      enable_date_range();
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
    m_wStatusMoreButton, m_wmin_date, m_wmax_date,
    m_chk_datemax, m_chk_datemin,
    m_zoom_button
  };
  for (uint i=0; i<sizeof(widgets)/sizeof(widgets[0]); i++)
    widgets[i]->setEnabled(enable);
  m_btn_box->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

/* Enable the controls for the date range, but only if Range is selected
   in the Date combobox */
void
msg_select_dialog::enable_date_range()
{
  int idx=m_date_cb->currentIndex();
  bool en=(idx>=0 && m_date_cb->itemData(idx)=="range");
  m_chk_datemax->setEnabled(en);
  m_wmax_date->setEnabled(en);
  m_chk_datemin->setEnabled(en);
  m_wmin_date->setEnabled(en);
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

    m_btn_box->button(QDialogButtonBox::Cancel)->setText(tr("Abort"));
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
  // r==3 ignored, error message already displayed
}

void
msg_select_dialog::cancel()
{
  if (m_waiting_for_results) {
    // stop the query but don't close the dialog
    m_waiting_for_results = false;
    m_thread.cancel();
    //    m_thread.release();
    m_btn_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    QApplication::restoreOverrideCursor();
    enable_inputs(true);
    enable_date_range();
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
    g->setExclusive(true);
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

