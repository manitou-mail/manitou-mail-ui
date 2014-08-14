/* Copyright (C) 2004-2014 Daniel Verite

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
#include "db.h"
#include "sqlstream.h"
#include "sqlquery.h"
#include "users.h"
#include "msg_status_cache.h"
#include "identities.h"
#include "app_config.h"
#include "mail_displayer.h"
#include "ui_feedback.h"

#include <QUuid>
#include <QObject>

mail_msg::mail_msg() :
  m_nMailId(0),
  m_thread_id(0),
  m_pri(0),
  m_identity_id(0),
  m_body_fetched(false),
  m_body_html_fetched(false),
  m_body_fetched_length(0),
  m_body_length(0),
  m_bHeaderFetched(false),
  m_tags_fetched(false),
  m_mailnote_in_db(false),
  m_nInReplyTo(0),
  m_rawsize(0)
{
}

mail_msg::mail_msg(mail_id_t id,const QString& from,
		   const QString& subject, const date date):
  m_nMailId(id),
  m_thread_id(0),
  m_pri(0),
  m_identity_id(0),
  m_body_fetched(false),
  m_body_html_fetched(false),
  m_body_fetched_length(0),
  m_body_length(0),
  m_bHeaderFetched(false),
  m_tags_fetched(false),
  m_mailnote_in_db(false),
  m_nInReplyTo(0),
  m_rawsize(0)
{
  m_sFrom=from;
  m_sSubject=subject;
  m_cDate=date;
  m_Attachments.setMailId(id);
  m_header.setMailId(id);
}

mail_msg::mail_msg(const mail_result& r):
  m_nMailId(r.m_id),
  m_thread_id(r.m_thread_id),
  m_pri(r.m_pri),
  m_identity_id(0),
  m_body_fetched(false),
  m_body_html_fetched(false),
  m_body_fetched_length(0),
  m_body_length(0),
  m_bHeaderFetched(false),
  m_tags_fetched(false),
  m_mailnote_in_db(false),
  m_nInReplyTo(r.m_in_replyto),
  m_rawsize(0)

{
  m_sFrom = r.m_from;
  m_sSubject = r.m_subject;
  m_cDate = date(r.m_date);
  m_Attachments.setMailId(r.m_id);
  m_header.setMailId(r.m_id);

  set_orig_status(r.m_status);
  setStatus(r.m_status);
  set_sender_name(r.m_sender_name);
  set_flags(r.m_flags);
}

void
mail_msg::set_identity_id(int id)
{
  m_identity_id = id;
}

bool
mail_msg::fetch_body_text(bool partial)
{
  if (!GetId())
    return false;

  // don't fetch from db when m_sBody already contains the requested contents
  if (!m_body_fetched || (!partial && m_body_fetched_length<m_body_length))
  {
    db_cnx db;
    try {
      const int maxsz=30000;
      QString part;
      if (partial)
	part = QString("substr(bodytext,1,%1)").arg(maxsz);
      else
	part="bodytext";
      sql_stream s(QString("SELECT %1 FROM body WHERE mail_id=:p1").arg(part), db);
      s << get_id();
      if (!s.eos()) {
	s >> m_sBody;
	m_body_fetched_length = m_sBody.length();
	if (partial) {
	  if (m_body_fetched_length==maxsz) {
	    sql_stream s1("SELECT length(bodytext) FROM body WHERE mail_id=:p1", db);
	    s1 << get_id();
	    if (!s1.eos()) {
	      s1 >> m_body_length;
	    }
	    else {
	      // should not happen: the entry in the body table has
	      // disappeared between the 2 statements. in this case,
	      // let's consider that the text has been fetched entirely
	      m_body_length = m_body_fetched_length;
	    }
	  }
	  else {
	    m_body_length = m_body_fetched_length;
	  }
	}
	else {
	  m_body_length = m_body_fetched_length;
	}
      }
      else {
	// no entry in body table
	m_sBody.truncate(0);
	m_body_fetched_length = m_body_length = 0;
      }
      m_body_fetched=true;
    }
    catch (db_excpt p) {
      DBEXCPT(p);
      m_sBody=QString("");
      return false;
    }
  }
  return true;
}


QString&
mail_msg::get_body_text(bool partial)
{
  fetch_body_text(partial);
  return m_sBody;
}

bool
mail_msg::update_body(const QString& txt)
{
  if (!get_id()) return false;
  db_cnx db;
  try {
    sql_stream s("UPDATE body SET bodytext=:p1 WHERE mail_id=:p2", db);
    s << txt << get_id();
    if (s.affected_rows()==0) {
      sql_stream s1("INSERT INTO body(mail_id,bodytext) VALUES(:p1,:p2)", db);
      s1 << get_id() << txt;
    }
    if (m_body_fetched)
      m_sBody = txt;
  }
  catch (db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mail_msg::bounce()
{
  return true;			// FIXME TODO
}


void
mail_msg::set_quoted_body(const QString& squote, const QString& sprepend,
			  body_format format)
{
  if (format == format_html) {
    QString html = "<html><body>";
    html.append(mail_displayer::htmlize(sprepend));
    html.append("<p><blockquote style=\"border-left: 1px solid rgb(204, 204, 204); margin: 0pt 0pt 0pt 0.8ex; padding-left: 1ex;\">");
    html.append(squote);
    html.append("</blockquote></body></html>");
    set_body_html(html);
    return;
  }

  // Format = format_plain_text
  QString& body = get_body_text();
  uint npos = 0;
  uint nend = squote.length();
  body = sprepend;
  const QChar lf = QChar('\n');
  QString prefix="> ";
  if (get_config().exists("reply_quote_prefix"))
    prefix = get_config().get_string("reply_quote_prefix");
  // if the prefix is insanely long, truncate it
  if (prefix.length()>76) {
    prefix.truncate(76);
  }
  // Reformat the text to have lines shorter than 78 characters when possible
  const int width = 78-prefix.length();

  while (npos < nend) {
    int nposlf = squote.indexOf('\n',npos);
    if (nposlf == -1)
      nposlf = nend;
    QString para = squote.mid(npos, nposlf-npos);
    // Reformat only non-quoted text and paragraphs longer than :width
    if (para.length() > width && para.at(0) != '>') {
      QChar sep = ' ';
      QStringList list = para.split(sep);
      QString line;

      for (int i=0; i < list.size(); i++) {
	QString word = list.at(i);
	if (line.length() + word.length() < width) {
	  if (!line.isEmpty())
	    line.append(sep);
	}
	else {
	  // flush current line before adding current word
	  if (line.length() > 0) {
	    body.append(prefix);
	    body.append(line);
	    body.append(lf);
	    line.truncate(0);
	  }
	}
	// append the word even if it's longer than :width
	line.append(word);
      } // end of for each word

      if (!line.isEmpty()) {
	// flush remaining line
	body.append(prefix);
	body.append(line);
	body.append(lf);
      }  
    } // end of paragraph reformating
    else {
      body.append(prefix);
      body.append(para);
      body.append(lf);
    }
    npos = nposlf + 1;
  }
  body.append(lf);
}

const QString&
mail_msg::get_headers()
{
  if (!m_bHeaderFetched && GetId()) {
    if (header().fetch()) {
      m_sHeaders=header().m_lines;
      m_bHeaderFetched=true;
    }
  }
  return m_sHeaders;
}

bool
mail_msg::store_tags()
{
  std::list<uint>::const_iterator iter;
  db_cnx db;
  try {
    sql_stream s1("INSERT INTO mail_tags(mail_id,tag,agent) VALUES (:p1,:p2,:p3)", db);
    for (iter=m_tags.begin(); iter!=m_tags.end(); iter++) {
      s1 << GetId() << *iter << user::current_user_id();
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

// returns true if the <tag_id> tag is currently on
bool
mail_msg::hasTag(int tag_id) const
{
  std::list<uint>::const_iterator iter;
  for (iter = m_tags.begin(); iter != m_tags.end(); iter++) {
    if (*iter == (uint)tag_id)
      return true;
  }
  return false;
}

// scan the headers and return a particular line
// starting with <headerType>
QString
mail_msg::get_header(const QString headerType)
{
  const QString& h=get_headers();
  uint nPos=0;
  uint nEnd=h.length();
  uint nHlen=headerType.length();

  QByteArray qba = headerType.toLatin1();
  const char* hType=qba.constData();
  while (nPos<nEnd) {
    // end of current line
    int nPosLf=h.indexOf('\n',nPos);
    if (nPosLf==-1)
      nPosLf=nEnd;
    QString t=h.mid(nPos,nHlen);
    if (!strcasecmp(t.toLatin1().constData()/*thisheader*/, hType)) {
      return h.mid(nPos+nHlen, nPosLf-(nPos+nHlen));
    }
    nPos=nPosLf+1;
  }
  return "";
}

/*
  Try to find to what identity the message (presumably incoming) was sent.
*/
QString
mail_msg::lookup_dest_identity()
{
  QString result;
  static identities m_ids;	// NON MT-SAFE
  if (!m_ids.fetch())
    return result;
  int ident = identity_id();

  if (ident==0) {
    QString to=get_header("To:").trimmed();
    QString cc=get_header("Cc:").trimmed();
    if (!cc.isEmpty()) {
      to += QString(",") + cc;
    }
    std::list<QString> emails_list;
    std::list<QString> names_list;
    mail_address::ExtractAddresses(to, emails_list, names_list);
    std::list<QString>::const_iterator iter1;
    for (iter1 = emails_list.begin(); iter1!=emails_list.end(); ++iter1) {
      identities::const_iterator iit = m_ids.find(*iter1);
      if (iit != m_ids.end()) {
	m_identity_id = iit->second.m_identity_id;
	result = *iter1;
	break;
      }
    }
  }
  else {
    mail_identity* pi = m_ids.get_by_id(ident);
    if (pi) {
      result = pi->m_email_addr;
    }
  }
  return result;
}


int
mail_msg::identity_id()
{
  if (m_identity_id)
    return m_identity_id;
  db_cnx db;
  try {
    sql_stream s("SELECT identity_id FROM mail WHERE mail_id=:p1", db);
    s << get_id();
    if (!s.eos()) {
      s >> m_identity_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
  return m_identity_id;
}

// Add or remove a tag in the database and in memory
bool
mail_msg::set_tag (uint id, bool set /*=true*/)
{
  db_cnx db;
  try {
    if (set) {
      sql_stream s1("INSERT INTO mail_tags(mail_id,tag,agent) VALUES (:p1,:p2,:p4)", db);
      s1 << GetId() << id << user::current_user_id();
    }
    else {
      sql_stream s2("DELETE FROM mail_tags WHERE mail_id=:p1 AND tag=:p2", db);
      s2 << GetId() << id;
    }
  }
  catch(db_excpt& p) {
    /* If there's an unique index violation when inserting, we want to
       ignore it (it means that the tag was already set for that
       message) */
    if (!p.unique_constraint_violation()) {
      DBEXCPT(p);
      return false;
    }
  }
  if (set)
    m_tags.push_back(id);
  else
    m_tags.remove(id);
  return true;
}

//static
int
mail_msg::toggle_tags_set(std::set<mail_msg*>& mset, uint tag_id, bool on)
{
  int result=0;
  if (mset.empty())
    return true;
  QString in_list;
  mail_id_to_select_in(mset, in_list);
  db_cnx db;
  try {
    if (on) {
      QString q1=QString("INSERT INTO mail_tags(mail_id,tag,agent) SELECT m.mail_id,:p1,:a FROM (SELECT mail_id FROM mail_tags WHERE tag=:p2) as mt RIGHT JOIN mail AS m ON mt.mail_id=m.mail_id WHERE mt.mail_id IS NULL AND m.mail_id IN %1").arg(in_list);
      sql_stream s1(q1, db);
      s1 << tag_id << user::current_user_id() << tag_id;
      result=s1.affected_rows();
    }
    else {
      QString q2=QString("DELETE FROM mail_tags WHERE tag=:p1 AND mail_id IN %1").arg(in_list);
      sql_stream s2(q2, db);
      s2 << tag_id;
      result=s2.affected_rows();
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
  return result;
}

bool
mail_msg::fetchNote()
{
  bool result=true;
  db_cnx db;
  try {
    sql_stream s("SELECT note FROM notes WHERE mail_id=:id", db);
    s << GetId();
    if (!s.eof()) {
      s >> m_mail_note;
      m_mailnote_in_db=true;
    }
    else
      m_mail_note=QString::null;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
mail_msg::trash()
{
  bool result=true;
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s("SELECT trash_msg(:id,:userid)", db);
    s << getId() << user::current_user_id();
    s >> m_status;
    m_db_status |= m_status;
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  return result;
}

//static
bool
mail_msg::trash_set(std::set<mail_msg*>& mset)
{
  if (mset.empty())
    return true;
  bool result=true;
  db_cnx db;
  try {
    db.begin_transaction();
    QString in_list;
    std::set<mail_msg*>::iterator it;
    mail_id_to_sql_array(mset, in_list);
    sql_stream sql(QString("SELECT trash_msg_set(%1, :ope)").arg(in_list), db);
    sql << user::current_user_id();
    for (it=mset.begin(); it!=mset.end(); ++it) {
      mail_msg* m = *it;
      m->set_orig_status(m->status() | statusTrashed);
    }
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
mail_msg::untrash()
{
  bool result=true;
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream sql("SELECT untrash_msg(:id, :ope)", db);
    sql << get_id() << user::current_user_id();
    sql >> m_status;
    m_db_status = m_status;
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
mail_msg::mdelete()
{
  bool result=true;
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream s("SELECT delete_msg(:p1)", db);
    s << getId();
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
mail_msg::store_note()
{
  bool result=true;
  const char *query=NULL;
  try {
    db_cnx db;
    if (m_mailnote_in_db) {
      if (m_mail_note.isEmpty()) {
	sql_stream s("DELETE FROM notes WHERE mail_id=:p1", db);
	s << GetId();
	m_flags &= ~flag_has_note;
      }
      else {
	query="UPDATE notes SET last_changed=now(), note=:p1 WHERE mail_id=:p2";
      }
    }
    else {
      query="INSERT INTO notes(note,mail_id,last_changed) VALUES(:p1,:p2,now())";
      m_flags |= flag_has_note;
    }
    if (query) {
      sql_stream s(query, db);
      s << m_mail_note << GetId();
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

const std::list<uint>&
mail_msg::get_cached_tags() const
{
  // FIXME: see what we could do if m_tags_fetched is false
  return m_tags;
}

std::list<uint>&
mail_msg::get_tags()
{
  if (m_tags_fetched) {
    return m_tags;
  }
  db_cnx db;
  try {
    sql_stream s ("SELECT tag FROM mail_tags WHERE mail_id=:p1", db);
    s << GetId();
    while (!s.eof()) {
      uint tid;
      s >> tid;
      m_tags.push_back(tid);
    }
  }
  catch (db_excpt& p) {
    DBEXCPT(p);
  }
  m_tags_fetched=true;
  return m_tags;
}

// update the message status in the database
bool
mail_msg::update_status(bool force/*=false*/)
{
  bool result = true;
  if (force || m_db_status != m_status) {
    db_cnx db;
    try {
      const char* query = "UPDATE mail SET status=:p1,mod_user_id=:o WHERE mail_id=:p2";
      sql_stream s(query, db);
      s << m_status << user::current_user_id() << getId();
      m_db_status = m_status;
      msg_status_cache::update(get_id(), m_status);
    }
    catch(db_excpt& p) {
      DBEXCPT (p);
      result = false;
    }
  }
  return result;
}

/*
  Set 'dest' to "(mail_id1, mail_id2, mail_id3,...)"
  where mail_idN come from the 's' set of mail_msg objects
*/
//static
void
mail_msg::mail_id_to_select_in(const std::set<mail_msg*>& s, QString& dest)
{
  std::set<mail_msg*>::iterator it = s.begin();
  dest.truncate(0);
  if (it==s.end())
    return;
  dest.append('(');
  for (; it!=s.end(); ++it) {
    if (it!=s.begin())
      dest.append(',');
    dest.append(QString("%1").arg((*it)->get_id()));
  }
  dest.append(')');
}

//static
void
mail_msg::mail_id_to_sql_array(const std::set<mail_msg*>& s, QString& dest)
{
  std::set<mail_msg*>::iterator it = s.begin();
  dest.truncate(0);
  if (it==s.end())
    dest= "'{}'::int[]";
  else {
    dest.append("'{");
    for (; it!=s.end(); ++it) {
      if (it!=s.begin())
	dest.append(',');
      dest.append(QString("%1").arg((*it)->get_id()));
    }
    dest.append("}'::int[]");
  }
}

/* Update the status of a set of mails */
// static
bool
mail_msg::set_or_with_status(std::set<mail_msg*>& s, uint or_mask)
{
  std::set<mail_msg*> mail_set;		// to update in MAIL
  std::set<mail_msg*> trashed_mail_set;	// to update in TRASHED_MAIL
  bool result = true;

  std::set<mail_msg*>::iterator it = s.begin();
  for (it=s.begin(); it!=s.end(); ++it) {
    mail_msg* m = *it;
    if (m->status() & statusTrashed)
      trashed_mail_set.insert(m);
    else
      mail_set.insert(m);
  }

  db_cnx db;
  try {
    QString in_list;
    // process MAIL
    if (!mail_set.empty()) {
      mail_id_to_select_in(mail_set, in_list);
      QString query = QString("UPDATE mail SET status=status|:p1, mod_user_id=:o WHERE mail_id IN %1").arg(in_list);
      sql_stream sql(query, db);
      sql << or_mask << user::current_user_id();
    }
    // process TRASHED_MAIL
    if (!trashed_mail_set.empty()) {
      mail_id_to_sql_array(trashed_mail_set, in_list);
      QString query = QString("SELECT trash_msg_set(%1, :p1)").arg(in_list);
      sql_stream sql(query, db);
      sql << user::current_user_id();
    }
    // do this only if no db exception has occurred
    for (it=s.begin(); it!=s.end(); ++it) {
      (*it)->set_orig_status((*it)->status() | or_mask);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    result = false;
  }
  return result;
}

bool
mail_msg::update_priority()
{
  bool result = true;
  db_cnx db;
  try {
    const char* query;
    query="UPDATE mail SET priority=:p1 WHERE mail_id=:p2";
    sql_stream s(query, db);
    s << m_pri << get_id();
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
    result = false;
  }
  return result;
}

void
mail_msg::fetch_status ()
{
  db_cnx db;
  try {
    sql_stream s ("SELECT status,mod_user_id FROM mail WHERE mail_id=:p2", db);
    s << get_id();
    if (!s.eos()) {
      s >> m_db_status >> m_user_id_status;
      m_status = m_db_status;
      msg_status_cache::update(get_id(), m_status);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
}

void
mail_msg::refresh()
{
  db_cnx db;
  try {
    sql_stream s ("SELECT status,mod_user_id,thread_id,flags FROM mail WHERE mail_id=:p2", db);
    s << get_id();
    if (!s.eos()) {
      s >> m_db_status >> m_user_id_status >> m_thread_id >> m_flags;
      m_status = m_db_status;
      msg_status_cache::update(get_id(), m_status);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT (p);
  }
}

void
mail_msg::build_message_id()
{
  QUuid uid = QUuid::createUuid();
  QString str=uid.toString();
  str.replace(QChar('{'), "").replace(QChar('}'), "");
  m_header.setMessageId(str+"@mm");
}

// Store the new message into the database
bool
mail_msg::store(ui_feedback* ui)
{
  bool result=false;
  PGresult* res;
  db_cnx db;
  PGconn* c=db.connection();
  try {
    db.begin_transaction();
    if (!m_nMailId) {
      sql_stream s("SELECT nextval('seq_mail_id')", db);
      s >> m_nMailId;
      build_message_id();
    }
    sql_write_fields fields(db);
    fields.add("mail_id", (int)GetId());
    fields.add("sender", m_header.m_sender);
    fields.add("sender_fullname", m_header.m_sender_fullname);
    fields.add("recipients", m_header.recipients_list());
    fields.add_if_not_empty("subject", m_header.m_subject, 1000);
    fields.add_if_not_empty("message_id", m_header.m_messageId, 100);
    fields.add_no_quote("msg_date", "now()");
    fields.add("mod_user_id", user::current_user_id());
    //    fields.add_no_quote("msg_day", "extract(days from now()-to_date('01/01/1970','DD/MM/YYYY'))");
    fields.add_no_quote("sender_date", "now()");
    fields.add_if_not_zero("in_reply_to", m_nInReplyTo);
    fields.add("flags", m_Attachments.size()>0?1:0);
    if (m_nInReplyTo) {
      /* if it's a reply, then get the thread_id in order to put it
	 into the thread */
      sql_stream st ("SELECT coalesce(thread_id,0) FROM mail WHERE mail_id=:p1", db);
      st << m_nInReplyTo;
      if (!st.eos()) {
	st >> m_thread_id;
	if (!m_thread_id) {
	  /* the message we're replying to has no thread: open one */
	  sql_stream st1 ("SELECT nextval('seq_thread_id')", db);
	  if (!st1.eos()) {
	    st1 >> m_thread_id;
	    // and link the original message to that thread
	    sql_stream sru ("UPDATE mail SET thread_id=:p1 WHERE mail_id=:p2", db);
	    sru << m_thread_id << m_nInReplyTo;
	  }
	}
	fields.add_if_not_zero ("thread_id", m_thread_id);
      }
      // update the status of the message we're replying to
      sql_stream sr ("UPDATE mail SET status=(status | :p1) WHERE mail_id=:p2", db);
      sr << statusReplied+statusArchived << m_nInReplyTo;
    }
    else if (!forwardOf().empty()) {
      const std::vector<mail_id_t>& v = forwardOf();
      sql_stream sr ("UPDATE mail SET status=(status | :p1) WHERE mail_id=:p2", db);
      for (uint ifwd=0; ifwd < v.size(); ifwd++) {
	// Update statuses of forwarded messages
	sr << statusFwded+statusArchived << v[ifwd];
      }
    }
    fields.add("status", statusRead + statusOutgoing);
    if (m_identity_id != 0)
      fields.add("identity_id", m_identity_id);

    QString sq = QString("INSERT INTO mail(%1) VALUES (%2)").arg(fields.fields()).arg(fields.values());
    const char* query;
    QByteArray qquery;
    if (db.datab()->encoding()=="UTF8")
      qquery=sq.toUtf8();
    else
      qquery=sq.toLocal8Bit();
    query=(const char*)qquery;
    DBG_PRINTF(5, "%s\n", query);
    res=PQexec(c,query);
    if (!res || PQresultStatus(res)!=PGRES_COMMAND_OK)
      throw 1;
    if (res)
      PQclear(res);

    msg_status_cache::update(get_id(), statusRead + statusOutgoing);

    if (m_body_html.isEmpty()) {
      // plain text only
      sql_stream sb("INSERT INTO body(mail_id,bodytext) VALUES (:p1,:p2)", db);
      sb << get_id() << m_sBody;
    }
    else {
      // plain text + html
      sql_stream sb("INSERT INTO body(mail_id,bodytext,bodyhtml) VALUES (:p1,:p2,:p3)", db);
      sb << get_id() << m_sBody << m_body_html;
    }

    result=store_tags();
    if (result) {
      mail_header& h=header();
      h.setMailId(GetId());
      result=h.store();
    }
    if (!m_mail_note.isEmpty()) {
      result = (result && store_note());
    }
    m_Attachments.setMailId(m_nMailId);
    result=(result && m_Attachments.store(ui));
    if (result)
      db.commit_transaction();
    else {
      DBG_PRINTF(2, "rollback");
      db.rollback_transaction();
    }
  }
  catch(db_excpt& p) {
    DBG_PRINTF(2, "rollback caused by db_excpt");
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  catch(int errcode) {
    DBG_PRINTF(2, "rollback caused by other reason (%d)", errcode);
    db.rollback_transaction();
    DBEXCPT(c);
    result=false;
  }
  return result;
}

/*
  Returns the attachment for the HTML part of the body, if there is
  one, otherwise returns NULL
 */
attachment*
mail_msg::body_html_attached_part()
{
  attachments_list& attchs = this->attachments();
  attchs.fetch();
  if (attchs.size() > 0) {
    attachments_list::iterator iter;
    for (iter=attchs.begin(); iter!=attchs.end(); iter++) {
      // we consider that the first unnamed html part is what we're looking for
      // later we'll rely on a better support for multipart/alternative in the db
      if (iter->filename().isEmpty() && iter->mime_type()=="text/html") {
	return &(*iter);
      }
    }
  }
  return NULL;
}

void
mail_msg::set_tags (const std::list<uint>& l)
{
  m_tags = l;
  m_tags_fetched=true;
}

/* Extract the text that can be quoted from a message when there is no
   selected text */
void
mail_msg::find_text_to_quote(QString& text_to_quote)
{
  if (get_body_text().isEmpty()) {
    /* The body to quote is empty.
       If there are unnamed text attachments, use them instead for quoting */
    text_to_quote.truncate(0);
    if (!attachments().empty()) {
      attachments_list::iterator iter=attachments().begin();
      for (; iter!=attachments().end(); ++iter)	{
	if ((iter->mime_type() == "text/plain" || iter->mime_type() == "text")
	    && iter->filename().isEmpty())  {
	  const char* contents=iter->get_contents();
	  if (contents)
	    text_to_quote.append(contents);
	}
      }
    }
  }
  else {
    text_to_quote = get_body_text();
  }
}

void
mail_msg::streamout_body(std::ofstream& o)
{
  QString& b=get_body_text();
  o.write (b.toLatin1().constData(), b.length()); 	// TODO: which encoding?
}


mail_msg
mail_msg::setup_reply(const QString& quoted_text, int whom_to, body_format format)
{
  mail_msg msg;
  mail_header replyHeader;

  QRegExp qRe("^re\\s*:\\s*", Qt::CaseInsensitive);
  // prepends "Re: " if it's not there already
  if (qRe.indexIn(Subject())==0)
    replyHeader.m_subject = Subject();
  else
    replyHeader.m_subject = QString("Re: ") + Subject();

  QString reply_to = get_header("Reply-To:");
  if (!reply_to.isEmpty()) {
    replyHeader.m_to = reply_to.trimmed();
  }
  else {
    // reply to sender
    replyHeader.m_to = get_header("From:").trimmed();
    if (replyHeader.m_to.isEmpty()) {
      replyHeader.m_to = From();
    }
  }

  if (whom_to==3) {
    // reply to mailing-list
    QString mailto=get_header("List-Post:").trimmed();
    QRegExp q("^<mailto:(.*)>$", Qt::CaseInsensitive);
    if (q.indexIn(mailto)==0) {
      mailto = q.cap(1);
    }
    replyHeader.m_to = mailto.isEmpty() ? From() : mailto;
  }

  // reply-to takes precedence over reply-to-all
  if (whom_to==2 && reply_to.isEmpty()) {
    // reply to all
    replyHeader.m_cc = get_header("To:").trimmed();
    QString old_cc = get_header ("Cc:").trimmed();
    if (!old_cc.isEmpty()) {
      if (!replyHeader.m_cc.isEmpty())
	replyHeader.m_cc += QString(",") + old_cc;
      else
	replyHeader.m_cc = old_cc;
    }

    // Break down the Cc fields into email+names components
    std::list<QString> emails_list;
    std::list<QString> names_list;
    mail_address::ExtractAddresses(replyHeader.m_cc.toLatin1().constData(),
				   emails_list, names_list);

    // Reassemble the list of addresses, skipping those that belong to us
    std::list<QString>::const_iterator iter1,iter2;
    QString expurged_cc;
    identities our_identities;
    our_identities.fetch();
    for (iter1 = emails_list.begin(), iter2 = names_list.begin();
	 iter1!=emails_list.end() && iter2!=names_list.end();
	 ++iter1, ++iter2) {
      identities::iterator iit = our_identities.find(*iter1);
      if (iit == our_identities.end()) { // not ours
	if (!expurged_cc.isEmpty())
	  expurged_cc += ", ";
	mail_address addr;
	addr.set_email(*iter1);
	addr.set_name(*iter2);
	expurged_cc += addr.email_and_name();
      }
    }
    DBG_PRINTF(5,"expurged_cc=%s\n", expurged_cc.toLatin1().constData());
    replyHeader.m_cc = expurged_cc;
  }
  QString us = lookup_dest_identity();
  replyHeader.m_envelope_from = us;
  replyHeader.m_inReplyTo = get_header("Message-Id:").trimmed();
  QString sender_name;	// to which the original message will be attributed
  QString sFrom=get_header("From:");

  if (!sFrom.isEmpty()) {
    // extract the sender's name from the headers
    std::list<QString> addrs;
    std::list<QString> names;
    int r=mail_address::ExtractAddresses(sFrom.toLatin1().constData(), addrs, names);
    if (!r && names.size()>0)
      sender_name=names.front();
  }
  if (sender_name.isEmpty()) {
    // else get it from the database
    mail_address_list from;
    if (from.fetchFromMail(GetId(), (int)mail_address::addrFrom) && from.size()>0)
      sender_name = from.begin()->name();
  }
  // else get it verbatim from the From: line
  if (sender_name.isEmpty())
    sender_name = sFrom;

  msg.set_header(replyHeader);

  // In-Reply-To (this message)
  msg.setInReplyTo(GetId());

  if (get_config().get_number("reply_copy_tags")) {
    /* assign to the reply the tags of the original mail */
    msg.set_tags (get_tags());
  }

  QString attrib;
  if (get_config().exists("reply_attribution_string")) {
    attrib = get_config().get_string("reply_attribution_string");
    attrib.replace("%F", sender_name);
    attrib.append("\n\n");
  }
  else {
    // default attribution string
    attrib = QString("\t ") + sender_name + " writes\n\n";
  }
  msg.set_quoted_body(quoted_text, attrib, format);
  return msg;
}

mail_msg
mail_msg::setup_forward()
{
  mail_msg msg;
  mail_header& fwd_header=msg.header();

  app_config& conf = get_config();
  QString subj_forw = conf.get_string("forward_subject_prefix");
  if (subj_forw.isEmpty()) {
    subj_forw = QObject::tr("[Forwarded]");
  }
  fwd_header.m_subject = subj_forw + QString(" ") + subject();

  // Auto-selection of the address to forward to, based on the initial To address
  if (conf.get_bool("use_forward_addresses_table")) {
    try {
      db_cnx db;
      sql_stream s(QString("SELECT forward_to FROM forward_addresses fa, mail_addresses ma, addresses a WHERE ma.mail_id=:p1 AND ma.addr_type=%1 AND ma.addr_id=a.addr_id AND a.email_addr=fa.to_email_addr").arg(mail_address::addrTo), db);
      s << get_id();
      if (!s.eos()) {
	s >> fwd_header.m_to;
      }
    }
    catch (db_excpt p) {
      DBEXCPT(p);
    }
  }

  QString sf_quote = conf.get_string("forward_start_quote");
  if (sf_quote.isEmpty()) {
    sf_quote=QObject::tr("------------------------- start of forwarded message ------------------------");
  }
  QString ef_quote = conf.get_string("forward_end_quote");
  if (ef_quote.isEmpty()) {
    ef_quote=QObject::tr("------------------------- end of forwarded message ---------------------------");
  }
  QString fwd_body;
  find_text_to_quote(fwd_body);

  QString quoted_header = this->forwarded_header_excerpt();
  fwd_body = sf_quote + "\n" + quoted_header + "\n" + fwd_body + "\n" + ef_quote + "\n";
  
  msg.set_body_text(fwd_body);
  msg.set_fwded_mail_id(this->get_id());

  return msg;
}

QString
mail_msg::forwarded_header_excerpt()
{
  static const char *hkeep[] = {
    "date:", "from:", "to:", "cc:", "reply-to:", "bcc:", "subject:"
  };

  const QString& h = this->get_headers();
  uint npos=0;
  uint nend=h.length();
  QString output;

  const int nh=sizeof(hkeep)/sizeof(hkeep[0]);
  QString slines[nh];

  while (npos<nend) {
    // position of end of current line
    int npos_lf=h.indexOf('\n',npos);
    if (npos_lf==-1)
      npos_lf=nend;

    for (int i=0; i<nh; i++) {
      uint nhlen=strlen(hkeep[i]);
      if (h.mid(npos,nhlen).toLower() == hkeep[i]) {
	slines[i]= h.mid(npos,nhlen) + h.mid(npos+nhlen, 1+npos_lf-(npos+nhlen));
	break;
      }
    }
    npos=npos_lf+1;
  }
  // order the headers as in hkeep and not as they appear in the
  // original message
  for (int j=0; j<nh; j++) {
    if (slines[j].length()>0)
      output+=slines[j];
  }
  return output;
}

QString&
mail_msg::get_body_html()
{
  fetch_body_html();
  return m_body_html;
}

bool
mail_msg::fetch_body_html()
{
  if (!GetId())
    return false;

  if (!m_body_html_fetched) {
    db_cnx db;
    try {
      sql_stream s("SELECT bodyhtml FROM body WHERE mail_id=:p1", db);
      s << get_id();
      if (!s.eos()) {
	s >> m_body_html;
      }
      m_body_html_fetched = true;
    }
    catch (db_excpt p) {
      DBEXCPT(p);
      m_body_html = QString("");
      return false;
    }
  }
  return true;

}


bool
mail_msg::get_rawsize(int* size)
{
  if (m_rawsize>0) {
    *size=m_rawsize;
    return true;
  }
  if (!get_id())
    return false;

  db_cnx db;
  try {
    sql_stream s("SELECT raw_size FROM mail WHERE mail_id=:p1", db);
    s << get_id();
    if (!s.eos()) {
      s >> m_rawsize;
      *size = m_rawsize;
    }
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}


/*
  unit in ("days", "hours", "minutes")
*/
bool
mail_msg::get_msg_age(const QString unit, int* age)
{
  if (!get_id())
    return false;

  db_cnx db;
  try {
    sql_stream s("SELECT cast(extract(epoch from now())-extract(epoch from sender_date) AS integer) FROM mail WHERE mail_id=:p1", db);
    s << get_id();
    if (!s.eos()) {
      int i;
      s >> i;
      if (unit=="minutes")
	*age = i/60;
      else if (unit=="hours")
	*age = i/3600;
      else if (unit=="days")
	*age = i/86400;
      else
	*age = 0;
    }
    else
      *age=0;
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;  
}

/*
  unit in ("days", "hours", "minutes")
*/
bool
mail_msg::get_sender_timestamp(time_t* t)
{
  if (!get_id())
    return false;

  db_cnx db;
  try {
    sql_stream s("SELECT extract(epoch from sender_date) FROM mail WHERE mail_id=:p1", db);
    s << get_id();
    if (!s.eos()) {
      float sd;
      s >> sd;
      *t = (time_t)sd;
    }
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;  
}
