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

#ifndef INC_ADDRESSES_H
#define INC_ADDRESSES_H

#include <list>
#include "dbtypes.h"
#include "date.h"

class mail_address
{
public:
  mail_address(): m_invalid(0), m_id(0) {}
  ~mail_address() {}

  static int ExtractAddresses(const QString& addr,QList<QString>&, QList<QString>&);

  static bool basic_syntax_check(const QString email);

  /* replace new lines in the addresses line by commas.
     replace multiple new lines by a comma and spaces. */
  static void join_address_lines(QString&);
  /* input: an address possibly containing a name part
     return: the email part only, or empty if not parsable */
  static QString parse_extract_email(const QString email_and_name);

  void set(const QString s) { m_address=s; }
  void set_email (const QString s) { m_address=s; }
  void normalize();
  bool fetchByEmail (const QString& s, bool* found);
  bool fetch_by_nickname (const QString& s, bool* found);
  void fetch_details (const QString& email);
  // fetch the list of addresses that are aliased to this->m_address
  void fetch_aliases (std::list<mail_address>*);
  void add_alias (const QString email);
  void remove_alias (const QString email);
  int recv_pri() const { return m_recv_pri; }
  bool store();
  bool update_last_used(const QString);
  void setName(const QString s) { m_name=s; }
  void set_name(const QString s) { m_name=s; }
  void setId(const int i) { m_id=i; }
  int id() const { return m_id; }
  void set_nb_msgs(int nfrom,int nto) {
    m_nb_from=nfrom;
    m_nb_to=nto;
  }
  int nb_from() const { return m_nb_from; }
  int nb_to() const { return m_nb_to; }
  const QString get() const { return m_address; }
  const QString email() const { return m_address; }
  const QString name() const { return m_name; }
  const QString nickname() const { return m_nickname; }
  void set_last_sent(const date s) { m_date_last_sent=s; }
  void set_last_recv(const date s) { m_date_last_recv=s; }
  const date last_sent() const { return m_date_last_sent; }
  const date last_recv() const { return m_date_last_recv; }
  // return a string with the email and name formatted depending
  // on the contents of the name
  QString email_and_name();

  // update in the database the fields to the values of 'to_update'
  bool diff_update(const mail_address& to_update);

  // id of the context in which the address is used
  // =mail_addresses.addr_type
  typedef const enum {
    addrFrom=1,
    addrTo=2,
    addrCc=3,
    addrReplyTo=4,
    addrBcc=5
  } t_addrType;
  QString m_address;		// addresses.email_addr
  QString m_name;		// addresses.name
  QString m_nickname;		// addresses.nickname
  QString m_notes;
  int m_invalid;
  int m_recv_pri;
private:
  unsigned int m_id;		// addresses.addr_id
  int m_nb_from;
  int m_nb_to;
  date m_date_last_sent;
  date m_date_last_recv;

  static QString part_string(const char* start, int len);
};

class mail_address_list : public std::list<mail_address>
{
public:
  mail_address_list() {}
  virtual ~mail_address_list() {}
  bool fetchLike (const QString pattern, const QString order,
		  bool at_start=false);
  // search name parts and addresses that start with a substring
  bool fetch_completions(const QString substring);

  // search for substring anywhere in email or name (not just at start of word)
  bool fetch_from_substring(const QString substring);

  bool fetchFromMail (mail_id_t mail_id, int /*mail_address::t_addrType*/ addr_type);
  bool fetch();
  bool fetch_recent(int type, int count=10, int offset=0);

  class search_criteria {
  public:
    QString m_email;
    QString m_name;
    QString m_nickname;
  } m_search;
};

#endif // INC_ADDRESSES_H
