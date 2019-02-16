/* Copyright (C) 2004-2019 Daniel Verite

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

#ifndef INC_MAILHEADER_H
#define INC_MAILHEADER_H

#include <QString>

class mail_header
{
public:
  mail_header():m_mail_id(0) {}
  virtual ~mail_header() {}
  void setMailId(mail_id_t mail_id) {
    m_mail_id=mail_id;
  }
  void setMessageId(const QString& msg_id) {
    m_messageId=msg_id;
  }
  bool fetch();
  bool store();
  /* create m_lines from header variables */
  void make();
  bool store_addresses_list(const QString& addr_list,int addr_type);
  QString recipients_list();

  QString m_from;		// full (address and optional name)
  QString m_subject;
  QString m_to;
  QString m_cc;
  QString m_replyTo;
  QString m_bcc;
  QString m_sender;		// email only
  QString m_sender_fullname;	// name only
  QString m_inReplyTo;
  QString m_messageId;
  QString m_xface;
  QString m_lines;
  mail_id_t m_mail_id;
  QString m_envelope_from;
  static void decode_rfc822(const QString src, QString& dest);
  static uint header_length(const char* rawmsg);
  static void decode_line(QString&);
  bool fetch_raw();

  void set_raw(const char* raw_msg, int length);

  const QString& raw_headers();

  /* Return values of matching fields, in the order of the header */
  QStringList get_header(const QString header_name);

  /* Contents extracted from the RAW_MAIL table or from an attachment
     and converted to unicode (done in fetch_raw()), but not
     MIME-decoded */
  QString m_raw;

  /* format the raw header 'src' into 'dest' (as an html string) */
  void format(QString& dest, const QString& src);
private:
  bool store_addresses();
};

#endif // INC_MAILHEADER_H
