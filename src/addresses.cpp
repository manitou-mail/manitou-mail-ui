/* Copyright (C) 2004-2017 Daniel Verite

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
#include "addresses.h"
#include "db.h"
#include "sqlstream.h"

//static
QString
mail_address::parse_extract_email(const QString line)
{
  QList<QString> emails;
  QList<QString> names;
  ExtractAddresses(line, emails, names);
  if (emails.size()==1)
    return emails.front();
  else
    return QString();
}

/*
  Hand made parser
  Currently recognizes:

  foo@example.org
  foo@example.org (Foo J. Bar)
  <foo@example.org>
  Foo Bar <foo@example.org>
  'Foo Bar' <foo@example.org>
  "Foo Bar" <foo@example.org>
*/

/*static*/
int
mail_address::ExtractAddresses(const QString& line,
			       QList<QString>& addrs,
			       QList<QString>& names)
{
  int len = line.length();
  int idx = 0;
  QChar ch;
  QChar enclose_char;

  int err = 0;

  while (idx < len) {
    ch = line.at(idx);
    bool done=true;
    if (ch.unicode() < 128) {
      switch(ch.toLatin1()) {
      default:
	done=false;
	break;
      case ' ':
      case '\t':
      case '\n':
      case '\r':
      case ',':
	idx++;
	break;			// address separator
      case '\0':
	break;
      case '<':
	// <foo@example.org> expected
	{
	  int start = ++idx;
	  QChar end_ch('>');
	  do {
	    if (idx >= len)
	      return 1;		// err = 1;
	    ch = line.at(idx++);
	  } while (ch != end_ch);
	  addrs.append(line.mid(start, idx-start-1));
	  names.push_back(QString(""));
	}
	break;
    case '\'':
    case '"':
      // "Foo Bar" <foo@example.org>
      {
	enclose_char = ch;
	int start = ++idx;
	do {
	  if (idx >= len)
	    return 1;		// err = 1;
	  ch = line.at(idx++);
	} while (ch != enclose_char);
	names.append(line.mid(start, idx-start-1));

	do {
	  if (idx >= len)
	    return 1;		// err = 1;
	  ch = line.at(idx++);
	} while (ch != '<');

	start = idx;
	do {
	  if (idx >= len)
	    return 1;		// err = 1;
	  ch = line.at(idx++);
	} while (ch != '>');
	addrs.append(line.mid(start, idx-start-1));
      }
      break;
      }
      if (done)
	continue;		// the while loop
    }

    // this path is both for ch.unicode()>=128, or ch.unicode()<128
    // and not dealt with by the switch() above

    int start = idx, start1;

    do {
      if (idx >= len)
	break;
      ch = line.at(idx++);
    } while (ch != ' ' && ch != '\t' && ch != ',' && ch != '\n');

    if (idx == len) {
      // email alone and eol
      addrs.append(line.mid(start, idx-start));
      names.append(QString(""));
    }
    else if (ch == ',' || ch == '\n') {
      addrs.append(line.mid(start, idx-start-1));
      names.append(QString(""));
    }
    else {
      //  {Name firstname <email>} or {email (name firstname)}
      // addr
      int end_addr = idx;
      // skip blanks
      do {
	if (idx >= len)
	  return 1; // err=1
	ch = line.at(idx++);
      } while (ch == ' ' || ch == '\t');

      if (ch == '<') {
	QString name1 = line.mid(start);
	name1.truncate(idx-start-1);
	// strip trailing white spaces from the name
	names.append(name1.trimmed());

	start1 = idx;
	do {
	  if (idx >= len)
	    return 1; // err=1
	  ch = line.at(idx++);
	} while (ch != '>');
	addrs.append(line.mid(start1, idx-start1-1));
      }
      else if (ch == '(') {
	start1 = idx; // points to char just after left parenthesis
	do {
	  if (idx >= len)
	    return 1; // err=1
	  ch = line.at(idx++);
	} while (ch != ')');
	addrs.append(line.mid(start, end_addr-start-1));
	names.append(line.mid(start1, idx-start1-1));
      }
      else {
	// Name firstname <email>
	do {
	  if (idx >= len)
	    return 1; // err=1
	  ch = line.at(idx++);
	} while (ch != '<');

	start1 = idx;		// start of email

	do {
	  if (idx >= len)
	    return 1; // err=1
	  ch = line.at(idx++);
	} while (ch != '>');
	names.append(line.mid(start, start1-start-1).trimmed());
	addrs.append(line.mid(start1, idx-start1-1));
      }
    }

  }
  return err;
}

/*
  replace new lines in the addresses line by commas.
  replace multiple new lines by a comma and spaces.
*/
void  // static
mail_address::join_address_lines(QString& line)
{
  int pos=0;
  int len = line.length();
  QString d; // destination

  // skip starting \n+
  while (pos<len && line.at(pos)=='\n') {
    pos++;
  }
  // if the line is only \n+, null it
  if (pos == len) {
    line.truncate(0);
    return;
  }

  while (pos<len) {
    if (line.at(pos)=='\n' || line.at(pos)==',') {
      while (pos<len && (line.at(pos)=='\n' ||line.at(pos)==','))
	pos++;
      if (pos<len) {
	d.append(", ");
      }
    }
    else {
      d.append(line.at(pos++));
    }
  }
  line = d;
}

void
mail_address::normalize()
{
  m_address = m_address.toLower();
}

QString
mail_address::email_and_name()
{
  QString result;
  if (m_name.isEmpty()) {
    result = m_address;
  }
  else if (m_name.indexOf ('"') == -1) {
    // preferred format: "john doe" <john.doe@sample.com>
    result = QString("\"") + m_name + QString("\" <") + m_address +
      QString(">");
  }
  else if (m_name.indexOf("()") == -1) {
    // second preferred format: john.doe@sample.com (John Doe)
    result = m_address + QString(" (") + m_name + QString(")");
  }
  else {
    int pos = 0;
    // remove the parenthesis from m_name
    while ( (pos = m_name.indexOf("()", pos)) != -1) {
      m_name.remove (pos, 1);
    }
    // and return the second preferred form
    result = m_address + QString(" (") + m_name + QString(")");
  }
  return result;
}

// insert or update the address
bool
mail_address::store()
{
  db_cnx db;
  try {
    if (m_id) {
      sql_stream s("UPDATE addresses SET email_addr=:p1,name=:p2,notes=:p3,nickname=:p4,invalid=:p5 WHERE addr_id=:id", db);
      s << m_address << m_name << m_notes << m_nickname << m_invalid;
/*
      if (!m_date_last_sent.isEmpty())
	s << m_date_last_sent;
      else
	s << sql_null();
*/
      s << m_id;
    }
    else {
      if (!db.next_seq_val("seq_addr_id", &m_id))
	return false;
      sql_stream s("INSERT INTO addresses(addr_id,email_addr,name,notes,nickname,invalid) VALUES (:p1,:p2,:p3,:p4,:p5, :p6)", db);
      s << m_id << m_address << m_name << m_notes << m_nickname << m_invalid;
/*
      if (!m_date_last_sent.isEmpty())
	s << m_date_last_sent;
      else
	s << sql_null();
*/
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

/*
  type="sent" or "recv"
*/
bool
mail_address::update_last_used(const QString type)
{
  db_cnx db;
  try {
    if (!m_id) {
      DBG_PRINTF(5,"warning: m_id=0\n");
      return false;
    }
    if (type=="sent") {
      sql_stream s("UPDATE addresses SET last_sent_to=now(), nb_sent_to=1+coalesce(nb_sent_to,0) WHERE addr_id=:id", db);
      s << m_id;
    }
    else if (type=="recv") {
      sql_stream s("UPDATE addresses SET last_recv_from=now() WHERE addr_id=:id", db);
      s << m_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

// lookup the email address in the addresses table
bool
mail_address::fetchByEmail(const QString& email, bool* found)
{
  // TODO: make a cache
  db_cnx db;
  try {
    sql_stream s("SELECT addr_id,name FROM addresses WHERE email_addr=lower(:p1)", db);
    s << email;
    if (!s.eof()) {
      s >> m_id >> m_name;
      m_address=email;
      *found=true;
    }
    else
      *found=false;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}


// lookup the email address in the addresses table
bool
mail_address::fetch_by_nickname(const QString& nick, bool* found)
{
  // TODO: make a cache
  db_cnx db;
  try {
    sql_stream s("SELECT addr_id,name,email_addr FROM addresses WHERE nickname=lower(:p1)", db);
    s << nick;
    if (!s.eof()) {
      s >> m_id >> m_name >> m_address;
      *found=true;
    }
    else
      *found=false;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

// make <email> be an alias to this.
void
mail_address::add_alias (const QString email)
{
  db_cnx db;
  try {
    sql_stream s1("SELECT addr_id FROM addresses where email_addr=:p1", db);
    s1 << email;
    if (!s1.eof()) {
      int addr_id;
      s1 >> addr_id;
      sql_stream s("UPDATE addresses SET owner_id=:p1 WHERE addr_id=:p2", db);
      s << m_id << addr_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}

// make <email> to no longer be an alias to this.
void
mail_address::remove_alias (const QString email)
{
  db_cnx db;
  try {
    sql_stream s1("SELECT addr_id FROM addresses where email_addr=:p1", db);
    s1 << email;
    if (!s1.eof()) {
      int addr_id;
      s1 >> addr_id;
      sql_stream s("UPDATE addresses SET owner_id=null WHERE addr_id=:p2", db);
      s << addr_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}

void
mail_address::fetch_details(const QString& email)
{
  db_cnx db;
  try {
    sql_stream s("SELECT addr_id,name,nickname,nb_recv_from,notes,TO_CHAR(last_recv_from,'YYYYMMDDHH24MISS'),TO_CHAR(last_sent_to,'YYYYMMDDHH24MISS'),invalid,recv_pri,nb_sent_to FROM addresses WHERE email_addr=lower(:p1)", db);
    s << email;
    if (!s.eof()) {
      QString sdate1, sdate2;
      s >> m_id >> m_name >> m_nickname >> m_nb_from >> m_notes >> sdate1 >> sdate2 >> m_invalid >> m_recv_pri >> m_nb_to;
      m_date_last_recv = date(sdate1);
      m_date_last_sent = date(sdate2);
      m_address=email;
    }
    else
      m_id=0;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}

void
mail_address::fetch_aliases (std::list<mail_address>* alist)
{
  db_cnx db;
  alist->clear();
  try {
    sql_stream s1("SELECT addr_id FROM addresses WHERE email_addr=:p1", db);
    s1 << m_address;
    if (!s1.eof()) {
      int id;
      s1 >> id;
      sql_stream s("SELECT addr_id,email_addr FROM addresses WHERE owner_id=:p1", db);
      s << id;
      while (!s.eof()) {
	int id;
	QString a_email;
	s >> id >> a_email;
	mail_address a;
	a.set (a_email);
	a.setId (id);
	alist->push_back (a);
      }
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}

bool
mail_address_list::fetchLike(const QString pattern,
			     const QString order,
			     bool at_start)
{
  QString query;
  QString percent_pattern;
  if (at_start)
    percent_pattern = pattern + "%%";
  else
    percent_pattern = "%%" + pattern + "%%";

  if (order == "last_recv_from") {
    query = "SELECT addr_id,email_addr,nb_recv_from,nb_sent_to FROM addresses WHERE email_addr ilike :p1 AND last_recv_from IS NOT NULL ORDER BY last_recv_from desc";
  }
  else if (order == "last_sent_to") {
    query = "SELECT a.addr_id,a.email_addr,a.nb_recv_from,a.nb_sent_to FROM addresses a WHERE email_addr ilike :p1 AND addr_id in (select addr_id from mail_addresses where addr_type=2) ORDER BY last_sent_to desc";
  }
  else {
    query = "SELECT addr_id,email_addr,nb_recv_from,nb_sent_to FROM addresses WHERE email_addr ilike :p1";
  }

  db_cnx db;
  try {
    sql_stream s(query, db);
    s << percent_pattern;
    while (!s.eos()) {
      int id;
      QString email;
      int nb_from,nb_to;
      s >> id >> email >> nb_from >> nb_to;

      mail_address addr;
      addr.setId(id);
      addr.set(email);
      addr.set_nb_msgs(nb_from, nb_to);
      push_back(addr);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

/* Check if an email address has only one @ sign and us-ascii characters */
// static
bool
mail_address::basic_syntax_check(const QString email)
{
  int cnt_at=0;
  int l=email.length();
  for (int i=0; i<l; i++) {
    QChar c=email.at(i);
    if (c=='@') {
      if (++cnt_at > 1)
	return false;
      // regexp for a domain name
      QRegExp re("^([A-Za-z0-9]([-a-zA-Z0-9]*[A-Za-z0-9])?\\.)+[A-Za-z][A-Za-z0-9]+$");
      return (re.indexIn(email, i+1, QRegExp::CaretAtOffset)==i+1);
    }
    else {
      if (c.unicode()<=0x20 || c.unicode()>=0x7f)
	return false;
    }
  }
  if (cnt_at==0)
    return false;

  return true;
}

/* Retrieve the contacts for which the email or one word from the
   comments (name, firstname...) starts with 'substring' */
bool
mail_address_list::fetch_completions(const QString substring)
{
  QString query=QString("SELECT addr_id,email_addr,name FROM addresses WHERE (last_sent_to is not null or last_recv_from is not null) AND (lower(name) ~ ('[[:<:]]'||lower(:p1)) OR email_addr ~ ('^'||lower(:p2))) AND coalesce(invalid,0)=0 ORDER BY greatest(last_sent_to,last_recv_from) DESC");
  db_cnx db;
  try {
    sql_stream s(query, db);
    s << substring << substring;
    while (!s.eos()) {
      int id;
      QString email, name;
      s >> id >> email >> name;
      mail_address addr;
      addr.setId(id);
      addr.set(email);
      addr.set_name(name);
      push_back(addr);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mail_address_list::fetch_from_substring(const QString substring)
{
  QString query;
  QString percent_pattern = "%" + substring + "%";

  query = "SELECT addr_id,email_addr,name,TO_CHAR(last_recv_from,'YYYYMMDDHH24MISS'),TO_CHAR(last_sent_to,'YYYYMMDDHH24MISS') FROM addresses WHERE email_addr ilike :p1 OR name ilike :p2";

  db_cnx db;
  try {
    sql_stream s(query, db);
    s << percent_pattern << percent_pattern;
    while (!s.eos()) {
      int id;
      QString email, name;
      QString date1, date2;
      s >> id >> email >> name >> date1 >> date2;

      mail_address addr;
      addr.setId(id);
      addr.set(email);
      addr.set_name(name);
      addr.set_last_recv(date(date1));
      addr.set_last_sent(date(date2));
      push_back(addr);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mail_address_list::fetchFromMail(mail_id_t mail_id, int/*mail_address::t_addrType */ addr_type)
{
  bool result=true;
  db_cnx db;
  try {
    sql_stream s("SELECT a.addr_id,a.email_addr,a.name"
		" FROM addresses a, mail_addresses ma"
		" WHERE ma.mail_id=:p1"
		" AND a.addr_id=ma.addr_id"
		" AND ma.addr_type=:p2", db);
    s << mail_id << addr_type;
    while (!s.eof()) {
      mail_address addr;
      QString email_addr;
      QString name;
      int addr_id;
      s >> addr_id >> email_addr >> name;
      addr.setId(addr_id);
      addr.set(email_addr);
      addr.set_name(name);
      push_back (addr);
    }
  }
  catch (db_excpt& p) {
    DBEXCPT (p);
    result = false;
  }
  return result;
}

bool
mail_address_list::fetch()
{
  QString query;
  QString percent_pattern;
  percent_pattern = QString("%%") + m_search.m_email + "%%";

    query = "SELECT addr_id,email_addr,nb_recv_from,nb_sent_to FROM addresses WHERE email_addr ilike :p1";

    db_cnx db;
  try {
    sql_stream s(query, db);
    s << percent_pattern;
    while (!s.eos()) {
      int id;
      QString email;
      int nb_from,nb_to;
      s >> id >> email >> nb_from >> nb_to;
      mail_address addr;
      addr.setId(id);
      addr.set(email);
      addr.set_nb_msgs(nb_from, nb_to);
      push_back(addr);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
mail_address::diff_update(const mail_address& to_update)
{
  std::list<QString> fields;
  std::list<QString> values;
  if (to_update.m_nickname != m_nickname) {
    fields.push_back("nickname");
    values.push_back(to_update.m_nickname);
  }
  if (to_update.m_name != m_name) {
    fields.push_back("name");
    values.push_back(to_update.m_name);
  }
  if (to_update.m_notes != m_notes) {
    fields.push_back("notes");
    values.push_back(to_update.m_notes);
  }
  if (to_update.m_recv_pri != m_recv_pri) {
    fields.push_back("recv_pri");
    values.push_back(QString("%1").arg(to_update.m_recv_pri));
  }
  if (!fields.empty()) {
    QString query = "UPDATE addresses SET ";
    std::list<QString>::iterator it1 = fields.begin();
    std::list<QString>::iterator it2 = values.begin();
    int nvar=1;
    for (; it1 != fields.end(); ++it1) {
      if (nvar>1)
	query.append(",");
      query.append(*it1);
      query.append("=");
      query.append(QString(":p%1").arg(nvar++));
    }
    db_cnx db;
    try {
      query.append(" WHERE email_addr=:email");
      sql_stream s(query, db);
      for (; it2 != values.end(); ++it2) {
	s << *it2;
      }
      s << m_address;
    }
    catch(db_excpt& p) {
      DBEXCPT(p);
      return false;
    }
  }
  return true;
}


/*
  Fetch <count> email addresses recently used in from or to recipients
  if <count>=0, don't limit the number of rows fetched
  <type>: 1=to, 2=from, 3=positive prio
  ignore addresses before <offset> if offset is given
*/
bool
mail_address_list::fetch_recent(int type, int count/*=10*/, int offset/*=0*/)
{
  QString query;
  QString percent_pattern;

  if (type == 3) {
    query = "SELECT addr_id,email_addr,name,to_char(last_sent_to,'YYYYMMDDHH24MISS'),recv_pri FROM addresses WHERE recv_pri>0 ORDER BY recv_pri desc";
  }
  else if (type == 2) {
    query = "SELECT addr_id,email_addr,name,to_char(last_recv_from,'YYYYMMDDHH24MISS'),recv_pri FROM addresses WHERE last_recv_from IS NOT null ORDER BY last_recv_from desc";
  }
  else if (type == 1) {
    query = "SELECT addr_id,email_addr,name,to_char(last_sent_to,'YYYYMMDDHH24MISS'),recv_pri FROM addresses WHERE last_sent_to IS NOT NULL ORDER BY last_sent_to desc";
  }
  else {
    throw "unknown type";
  }

  if (count) {
    query += QString(" LIMIT %1").arg(count);
  }
  if (offset) {
    query += QString(" OFFSET %1").arg(offset);
  }

  db_cnx db;
  try {
    sql_stream s(query, db);
    while (!s.eos()) {
      QString email;
      QString name;
      QString sdate;
      int pri;
      int id;
      s >> id >> email >> name >> sdate >> pri;
      mail_address addr;
      addr.setId(id);
      addr.set(email);
      addr.set_name(name);
      addr.m_recv_pri=pri;
      date d(sdate);
      if (type==2)
	addr.set_last_recv(d);
      else
	addr.set_last_sent(d);
      push_back(addr);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}
