/* Copyright (C) 2004-2015 Daniel Verite

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

#ifndef INC_MESSAGE_H
#define INC_MESSAGE_H

#include "dbtypes.h"
#include "time.h"
#include <QString>

#include "addresses.h"
#include "mailheader.h"
#include "attachment.h"
//#include "ui_feedback.h"

class ui_feedback;

#include <vector>

class mail_result
{
public:
  mail_id_t m_id;
  QString m_from;
  QString m_subject;
  QString m_date;
  int m_thread_id;
  int m_status;
  mail_id_t m_in_replyto;
  int m_pri;
  QString m_sender_name;
  uint m_flags;
  QString m_recipients;
};

class mail_msg
{
public:
  mail_msg();
  mail_msg(mail_id_t id,const QString& from, const QString& subject,
	   const date date);
  mail_msg(const mail_result&);
  enum body_format {
    format_plain_text,
    format_html
  };

  virtual ~mail_msg() {}
  mail_id_t GetId() const { return m_nMailId; }
  mail_id_t getId() const { return m_nMailId; }
  mail_id_t get_id() const { return m_nMailId; }
  static mail_id_t id_from_string(const QString s, bool* ok=0) {
#if defined(DB_MAIL_ID_32)
    return s.toUInt(ok);
#elif defined(DB_MAIL_ID_64)
    return s.toULongLong(ok);
#else
    #error "no suitable DB_MAIL_ID_xx definition"
#endif
  }
  void SetId(mail_id_t id) {
    m_nMailId=id;
    m_Attachments.setMailId(id);
    m_header.setMailId(id);
  }
  void set_mail_id(mail_id_t id) { SetId(id); }
  //static QString SubjectMakeReply(const QString& subject);
  void make_from_raw(const char* rawMessage, uint size);

  // attach msg to this message as a message/rfc822 attachment
  void attach_message (mail_msg* msg);

  void find_text_to_quote(QString& text_to_quote);

  bool update_body(const QString& txt);
  bool has_attachments() const {
    return (m_flags & flag_has_attachments)!=0;
  }
  bool has_note() const {
    return (m_flags & flag_has_note)!=0;
  }
  bool get_rawsize(int*);
  mail_header& header() { return m_header; }
  int identity_id();
  void set_identity_id(int);
  const QString& get_headers();
  QString get_header(const QString);
  void set_header(const mail_header& header) {
    m_header=header;
  }
  void set_priority(int pri) {
    m_pri=pri;
  }
  int priority() const {
    return m_pri;
  }
  void set_recipients(const QString r) {
    m_recipients=r;
  }
  unsigned int threadId() const { return m_thread_id; }
  mail_id_t inReplyTo() const { return m_nInReplyTo; }
  void setInReplyTo(mail_id_t i) { m_nInReplyTo=i; }

  const std::vector<mail_id_t>& forwardOf() const { return m_forwarded_mail_vect; }
  void set_fwded_mail_id (mail_id_t i) { m_forwarded_mail_vect.push_back(i); }

  void setThread(int id) { m_thread_id=id; }

  // body: TODO: move into its own class
  QString& get_body_text(bool partial=false);
  QString& get_body_html();

  void set_body_text(const QString& body) { m_sBody = body; }
  void set_body_html(const QString& html) { m_body_html = html; }
  bool body_in_cache() const {
    return m_body_fetched;
  }
  bool fetch_body_text(bool partial=false);
  bool fetch_body_html();

  int body_fetched_length() const {
    return m_body_fetched_length;
  }
  int body_length() const {
    return m_body_length;
  }
  void set_quoted_body(const QString&, const QString&, body_format);
  QString forwarded_header_excerpt();

  QString From() const { return m_sFrom; }
  QString sender_name() const { return m_sender_name; }
  void set_sender_name (const QString& s) { m_sender_name=s; }
  void set_flags(uint flags) { m_flags=flags; }
  uint flags() const { return m_flags; }
  QString recipients() const { return m_recipients; }
  QString Subject() const { return m_sSubject; }
  const QString subject() const { return m_sSubject; }
  const date& msg_date() const { return m_cDate; }
  bool get_msg_age(const QString unit, int*); // unit="days" or "hours" or "minutes"
  bool get_sender_timestamp(time_t*);
  void set_note(const QString& s) {
    m_mail_note=s;
  }
  const QString& getNote() const { return m_mail_note; }

  bool set_tag (uint id, bool set=true);
  void set_tags (const std::list<uint>& l);
  std::list<uint>& get_tags();
  const std::list<uint>& get_cached_tags() const;

  uint status() const { return m_status; }
  uint thread_id() const { return m_thread_id; }
  void setStatus(uint status) { m_status=status; }
  void set_status(uint status) { m_status=status; }
  void set_orig_status(uint status) { m_db_status=m_status=status; }
  uint status_last_user () { return m_user_id_status; }
  bool is_current() const {
    return (m_status&(statusArchived|statusTrashed))==0;
  }

  // fetch the status. Use this to know if it has changed
  void fetch_status ();

  // get from the database all the data that is subject to change
  // (status, thread_id, operator, ...)
  void refresh();

  void setDate(const date& d) { m_cDate=d; }

  mail_msg setup_reply(const QString& quoteText, int whom_to, body_format format);
  mail_msg setup_forward();

  // Write the status to the database. If force is true, write it
  // even if the value in memory is the same as the new
  // value, overwriting the change another user may have done
  // in the meantime in the database
  bool update_status(bool force=false);

  bool update_priority();

  bool store(ui_feedback* ui=NULL);
  bool mdelete();
  bool trash();
  bool untrash();
  bool bounce();
  bool store_note();
  bool fetchNote();
  /* At the end of the composition of a new HTML mail, paths of files
     that are to be converted into database attachments are stored here */
  QStringList m_attached_local_files;

  attachments_list& attachments() { return m_Attachments; }
  attachment* body_html_attached_part();

  bool hasTag(int tag_id) const;

  QString lookup_dest_identity();

  void streamout_body (std::ofstream&);
  //  void streamout_mbox (std::ofstream&);

  typedef const enum {
    statusNew=0,
    statusRead=1,
    statusReplied=4,
    statusFwded=8,
    statusTrashed=16,
    statusArchived=32,
    statusSelected=64,
    statusOutgoing=128,
    statusSent=256,
    statusReplying=512,
    statusComposed=1024,
    statusAttached=2048,		// internal
    statusMax=2048
  } status_t;

  typedef const enum {
    flag_has_attachments=1,
    flag_has_note=2
  } msg_flags_t;

  static bool set_or_with_status(std::set<mail_msg*>& s, uint or_mask);
  static bool trash_set(std::set<mail_msg*>& mset);
  static int toggle_tags_set(std::set<mail_msg*>& mset,
			     uint tag_id, bool on);

private:
  static void mail_id_to_select_in(const std::set<mail_msg*>& s,
				   QString& dest);
  static void mail_id_to_sql_array(const std::set<mail_msg*>& s,
				   QString& dest);
  bool store_tags();
  bool storeHeader();
  bool store_addresses();
  bool store_addresses_list(const QString&, int/*mail_address::t_addrType*/);
  void build_message_id();
  // build header lines for a new message
  void make_header();
  // returns header as a string (empty if header is missing)
private:
  mail_header m_header;
  mail_id_t m_nMailId;
  uint m_flags;
  QString m_sBody;
  QString m_body_html;
  QString m_sHeaders;
  QString m_sFrom;
  QString m_sSubject;
  QString m_recipients;
  QString m_sender_name;
  unsigned int m_thread_id;
  int m_pri;			// priority
  int m_identity_id;
  date m_cDate;
  bool m_body_fetched;
  bool m_body_html_fetched;
  int m_body_fetched_length;
  int m_body_length;
  bool m_bHeaderFetched;
  bool m_tags_fetched;
  std::list<uint> m_tags;
  uint m_status;
  uint m_db_status;

  // id of the user that last updated the status
  uint m_user_id_status;

  attachments_list m_Attachments;
  QString m_mail_note;
  bool m_mailnote_in_db;
  mail_id_t m_nInReplyTo;
  int m_rawsize;
  std::vector<mail_id_t> m_forwarded_mail_vect;
};

Q_DECLARE_METATYPE(mail_msg*)

#endif // INC_MESSAGE_H

