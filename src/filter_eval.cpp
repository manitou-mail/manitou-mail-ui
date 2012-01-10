/* Copyright (C) 2011 Daniel Verite

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

#include <QString>
#include <QVariant>
#include <QDebug>
#include <QCoreApplication>
#include <QStringList>

#include "main.h"
#include "filter_eval.h"
#include "addresses.h"
#include "identities.h"

filter_eval_value::filter_eval_value() : vtype(type_null)
{
}

filter_eval_value
filter_evaluator::and_operator(const filter_eval_value v1,
			       const filter_eval_value v2,
			       filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value v;
  v.val = v1.val.toBool() && v2.val.toBool();
  v.vtype = filter_eval_value::type_bool;
  return v;
}

filter_eval_value
filter_evaluator::or_operator(const filter_eval_value v1,
			      const filter_eval_value v2,
			      filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value v;
  v.vtype = filter_eval_value::type_bool;
  v.val = v1.val.toBool() || v2.val.toBool();
  return v;
}

filter_eval_value
filter_evaluator::contains_operator(const filter_eval_value v1,
				    const filter_eval_value v2,
				    filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value v;
  QString s1 = v1.val.toString();
  QString s2 = v2.val.toString();
  v.vtype = filter_eval_value::type_bool;
  v.val = (s1.indexOf(s2, 0, Qt::CaseInsensitive) >= 0);
  v.vtype = filter_eval_value::type_number;
  return v;
}

filter_eval_value
filter_evaluator::is_operator(const filter_eval_value v1,
			      const filter_eval_value v2,
			      filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value v;
  QString s1 = v1.val.toString();
  QString s2 = v2.val.toString();
  v.vtype = filter_eval_value::type_bool;
  v.val = (s1.compare(s2, Qt::CaseInsensitive) == 0) ? 1 : 0;
  return v;
}

filter_eval_value
filter_evaluator::isnot_operator(const filter_eval_value v1,
				 const filter_eval_value v2,
				 filter_eval_context* ctxt)
{
  filter_eval_value v = is_operator(v1, v2, ctxt);
  v.vtype = filter_eval_value::type_bool;
  v.val = !v.val.toBool();
  return v;
}

filter_eval_value
filter_evaluator::not_equals_operator(const filter_eval_value v1,
				      const filter_eval_value v2,
				      filter_eval_context* ctxt)
{
  filter_eval_value v = equals_operator(v1, v2, ctxt);
  v.val = !v.val.toBool();
  return v;
}

filter_eval_value
filter_evaluator::equals_operator(const filter_eval_value v1,
				  const filter_eval_value v2,
				  filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value v;
  QString s1 = v1.val.toString();
  QString s2 = v2.val.toString();
  v.val = (s1.compare(s2, Qt::CaseSensitive) == 0) ? 1 : 0;
  v.vtype = filter_eval_value::type_bool;
  return v;
}

filter_eval_value
filter_evaluator::regmatches_operator(const filter_eval_value v1,
				      const filter_eval_value v2,
				      filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value v;
  QRegExp rx(v2.val.toString());
  v.val = (rx.indexIn(v1.val.toString())) ? 1 : 0;
  v.vtype = filter_eval_value::type_bool;
  return v;
}

filter_eval_value
filter_evaluator::not_operator(const filter_eval_value v,
			       filter_eval_context* ctxt)
{
  Q_UNUSED(ctxt);
  filter_eval_value vr;
  vr.val = !v.val.toBool();
  return vr;

}

/* Return the value of the first field corresponding to 'fld',
   or an null QString if not found */
QString
filter_evaluator::db_lookup_header(const QString fld, filter_eval_context* ctxt)
{
  const QString& h = db_get_header(ctxt);
  QString field=fld;
  field.append(':');
  int flen = field.length();
  int npos=0;
  int nend=h.length();
  QString result;

  while (npos < nend) {
    int fpos = h.indexOf(field, npos, Qt::CaseInsensitive);
    if (fpos == -1) // not found
      break;
    if (fpos==0 || h.at(fpos-1)=='\n') {
      // found at the beginning of a line
      int nposlf = h.indexOf('\n', fpos+flen);
      if (nposlf==-1)
	result = h.mid(fpos+flen).trimmed();
      else
	result = h.mid(fpos+flen, nposlf-(fpos+flen)).trimmed();
      break;
    }
    else
      npos = fpos+flen;
  }
  return result;
}

const QString&
filter_evaluator::db_get_header(filter_eval_context* ctxt)
{
  return ctxt->message.get_headers();
}

QString
filter_evaluator::db_get_body(filter_eval_context* ctxt)
{
  return ctxt->message.get_body_text();
}

QString
filter_evaluator::db_get_identity(filter_eval_context* ctxt)
{
  int id= ctxt->message.identity_id();
  QString e;
  if (id)
    e = identities::email_from_id(id);
  return e;
}

time_t
filter_evaluator::db_get_sender_timestamp(filter_eval_context* ctxt)
{
  time_t t;
  if (ctxt->message.get_sender_timestamp(&t))
    return t;
  else
    return (time_t)0;
}

int
filter_evaluator::db_get_age(filter_eval_context* ctxt, const QString unit)
{
  int a;
  if (ctxt->message.get_msg_age(unit, &a)) {
    return a;
  }
  else {
    ctxt->errstr = QObject::tr("Unable to get the age of the message from the database");
    return 0;
  }
}

int
filter_evaluator::db_get_rawsize(filter_eval_context* ctxt)
{
  int size;
  bool res = ctxt->message.get_rawsize(&size);
  if (res)
    return size;
  else {
    ctxt->errstr = QObject::tr("Unable to get the size of the message from the database");
    return 0;
  }
}


filter_eval_value
filter_evaluator::func_now(const filter_eval_value v,
			   filter_eval_context* ctxt)
{
  return eval_date(v, ctxt, ctxt->start_time, 1);
}

filter_eval_value
filter_evaluator::func_now_utc(const filter_eval_value v,
			       filter_eval_context* ctxt)
{
  return eval_date(v, ctxt, ctxt->start_time, 2);
}


// variant: 1=>localtime, 2=>utc
filter_eval_value
filter_evaluator::eval_date(const filter_eval_value v,
			    filter_eval_context* ctxt,
			    time_t timestamp,
			    int variant)
{
  struct tm* t;
  filter_eval_value res;
  if (timestamp==(time_t)0)
    return res;

  if (variant==1)
    t = localtime(&timestamp);
  else /* if (variant==2) */
    t = gmtime(&timestamp);

  char buf[4+1+2+1+2+1];
  const char* format;

  QString arg = v.val.toString().toLower();
  if (arg=="hour") format="%H";
  else if (arg=="minute") format="%M";
  else if (arg=="second") format="%S";
  else if (arg=="date") format="%Y-%m-%d";
  else if (arg=="day") format="%d";
  else if (arg=="weekday") format="%w";
  else if (arg=="month") format="%m";
  else if (arg=="year") format="%Y";
  else if (arg=="time") format="%H:%M:%S";
  else {
    ctxt->errstr = QObject::tr("Invalid date/time field '%1'").arg(v.val.toString());
    return res;
  }

  strftime(buf, sizeof(buf), format, t);
  res.vtype = filter_eval_value::type_string;
  res.val = QString(buf);
  
  DBG_PRINTF(5, "eval_date timestamp=%d,resultat=%s", (int)timestamp, buf);

  return res;
}


filter_eval_value
filter_evaluator::func_subject(const filter_eval_value v,
			       filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  filter_eval_value v1;
  v1.val = "subject";
  v1.vtype = filter_eval_value::type_string;
  return func_header(v1, ctxt);
}

filter_eval_value
filter_evaluator::func_to(const filter_eval_value v,
			  filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  return func_list_header_addresses("To", ctxt);
}

filter_eval_value
filter_evaluator::func_list_header_addresses(const QString field,
					     filter_eval_context* ctxt)
{
  filter_eval_value res;
  res.vtype = filter_eval_value::type_string;

  filter_eval_value v1;
  v1.val = field;
  v1.vtype = filter_eval_value::type_string;
  filter_eval_value v = func_header(v1, ctxt);

  if (v.vtype == filter_eval_value::type_string && !v.val.toString().isEmpty()) {
    std::list<QString> emails;
    std::list<QString> names;
    int r = mail_address::ExtractAddresses(v.val.toString().toLatin1().constData(),
					   emails, names);
    if (r==0) { // success
      QStringList l = QStringList::fromStdList(emails);
      res.val = l.join(",");
    }
  }

  return res;
}

filter_eval_value
filter_evaluator::func_cc(const filter_eval_value v,
			  filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  return func_list_header_addresses("Cc", ctxt);
}

filter_eval_value
filter_evaluator::func_from(const filter_eval_value v,
			  filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  return func_list_header_addresses("From", ctxt);
}


filter_eval_value
filter_evaluator::func_recipients(const filter_eval_value v,
				  filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  QStringList rcpts;
  rcpts << db_lookup_header("To", ctxt) << db_lookup_header("Cc", ctxt)
	<< db_lookup_header("Bcc", ctxt);
  filter_eval_value vr;
  vr.vtype = filter_eval_value::type_string;
  vr.val = rcpts.join(",");
  return vr;
}

filter_eval_value
filter_evaluator::func_header(const filter_eval_value v,
			      filter_eval_context* ctxt)
{
  QString h = db_get_header(ctxt);
  filter_eval_value vr;
  QString field = v.val.toString();
  field.append(':');
  int flen = field.length();
  int npos=0;
  int nend=h.length();

  while (npos < nend) {
    int fpos = h.indexOf(field, npos, Qt::CaseInsensitive);
    if (fpos == -1) // not found
      break;
    if (fpos==0 || h.at(fpos-1)=='\n') {
      // found at the beginning of a line
      int nposlf = h.indexOf('\n', fpos+flen);
      if (nposlf==-1)
	vr.val = h.mid(fpos+flen).trimmed();
      else
	vr.val = h.mid(fpos+flen, nposlf-(fpos+flen)).trimmed();
      vr.vtype = filter_eval_value::type_string;
      break;
    }
    else
      npos = fpos+flen;
  }
  return vr;
}

filter_eval_value
filter_evaluator::func_headers(const filter_eval_value v,
			       filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  filter_eval_value vr;
  vr.val = db_get_header(ctxt);
  vr.vtype = filter_eval_value::type_string;
  return vr;
}

filter_eval_value
filter_evaluator::func_rawsize(const filter_eval_value v,
			       filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  filter_eval_value vr;
  vr.val = db_get_rawsize(ctxt);
  vr.vtype = filter_eval_value::type_number;
  return vr;
}

filter_eval_value
filter_evaluator::func_body(const filter_eval_value v,
			    filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  filter_eval_value vr;
  vr.val = db_get_body(ctxt);
  vr.vtype = filter_eval_value::type_string;
  return vr;
}

filter_eval_value
filter_evaluator::func_identity(const filter_eval_value v,
				filter_eval_context* ctxt)
{
  Q_UNUSED(v);
  filter_eval_value vr;
  vr.val = db_get_identity(ctxt);
  vr.vtype = filter_eval_value::type_string;
  return vr;
}

filter_eval_value
filter_evaluator::func_condition(const filter_eval_value v,
				 filter_eval_context* ctxt)
{
  return eval_subexpr(ctxt, v.val.toString().trimmed());
}

filter_eval_value
filter_evaluator::func_date(const filter_eval_value v,
			    filter_eval_context* ctxt)
{
  time_t t = db_get_sender_timestamp(ctxt);
  return eval_date(v, ctxt, t, 1);
}

filter_eval_value
filter_evaluator::func_date_utc(const filter_eval_value v,
				filter_eval_context* ctxt)
{
  time_t t = db_get_sender_timestamp(ctxt);
  return eval_date(v, ctxt, t, 2);
}

filter_eval_value
filter_evaluator::func_age(const filter_eval_value v,
			   filter_eval_context* ctxt)
{
  filter_eval_value vr;
  vr.vtype = filter_eval_value::type_number;
  QString unit = v.val.toString();
  if (unit!="days" && unit!="hours" && unit!="minutes") {
    ctxt->errstr = QObject::tr("age() argument must be \"days\", \"hours\", or \"minutes\"");    
  }
  else {
    vr.val = db_get_age(ctxt, unit);
  }
  return vr;  
}


filter_eval_func
filter_evaluator::eval_funcs[] = {
  { "age", func_age, 1, filter_eval_value::type_number },
  { "body", func_body, 0, filter_eval_value::type_string },
  { "cc", func_cc, 0, filter_eval_value::type_string },
  { "condition", func_condition, 1, filter_eval_value::type_string },
  { "date", func_date, 1, filter_eval_value::type_string },
  { "date_utc", func_date_utc, 1, filter_eval_value::type_string },
  { "from", func_from, 0, filter_eval_value::type_string },
  { "header", func_header, 1, filter_eval_value::type_string },
  { "headers", func_headers, 0, filter_eval_value::type_string },
  { "identity", func_identity, 0, filter_eval_value::type_string },
  { "now", func_now, 1, filter_eval_value::type_number },
  { "now_utc", func_now_utc, 1, filter_eval_value::type_number },
  { "rawsize", func_rawsize, 0, filter_eval_value::type_number },
  { "recipients", func_recipients, 0, filter_eval_value::type_string },
  { "subject", func_subject, 0, filter_eval_value::type_string },
  { "to", func_to, 0, filter_eval_value::type_string }
};

binary_op_t
filter_evaluator::binary_ops[] = {
  {"and", PRI_AND, and_operator },
  {"contains", PRI_CONTAINS, contains_operator },
  {"contain", PRI_CONTAINS, contains_operator},
  {"eq", PRI_CMP, equals_operator},
  {"is", PRI_CMP, is_operator},
  {"isnot", PRI_CMP, isnot_operator},
  {"ne", PRI_CMP, not_equals_operator},
  {"or", PRI_OR, or_operator },
  {"regmatches", PRI_REGEXP, regmatches_operator}
};

unary_op_t
filter_evaluator::unary_ops[] = {
  {"not", PRI_UNARY_NOT, not_operator }
};


void
filter_evaluator::skip_blanks(filter_eval_context* ctxt)
{
  QChar c;
  int p = ctxt->evp;
  if (p < ctxt->expr.length()) {
    do {
      c = ctxt->expr.at(p++);
    } while (c==' ' || c=='\t' || c=='\n' || c=='\r');
    ctxt->evp = p-1;
  }
}

QString
filter_evaluator::getsym(filter_eval_context* ctxt)
{
  int p=ctxt->evp;
  QChar c;
  QString v;
  bool end=false;
  do {
    c = ctxt->expr.at(p++).toLower();
    // [a-z_0-9]
    if ((c.unicode()>=(int)'a' && c.unicode()<=(int)'z') || c=='_' || c.isNumber())
      v.append(c);
    else
      end=true;
  } while (!end);
  ctxt->evp=p-1;
  return v;
}

QString
filter_evaluator::get_op_name(filter_eval_context* ctxt)
{
  int p=ctxt->evp;
  QChar c;
  QString v;
  bool end=false;
  do {
    c = ctxt->expr.at(p++).toLower();
    // [A-Za-z_0-9]
    if ((c.unicode()>=(int)'a' && c.unicode()<=(int)'z') || c=='_' || c.isNumber())
      v.append(c);
    else
      end=true;
  } while (!end);
  ctxt->evp=p-1;
  return v;
}

bool
filter_evaluator::eval_string(filter_eval_context* ctxt, QChar endc, bool quote)
{
  bool end=false;
  int p=ctxt->evp;
  QChar c;
  QString v;
  int l=ctxt->expr.length();
  while (!end) {
    if (p==l) {
      ctxt->errstr = QObject::tr("Premature end of string");
      ctxt->evp=p-1;
      return false;
    }
    c = ctxt->expr.at(p++);
    if (quote && c=='\\') {
      if (p<l) {
	c = ctxt->expr.at(p++);
	v.append(c);
      }
      else
	continue;
    }
    else if (c == endc)
      end = true;
    else
      v.append(c);
  }
  ctxt->evp=p;
  filter_eval_value ev;
  ev.val = v;
  ev.vtype = filter_eval_value::type_string;
  ctxt->evstack.push(ev);
  return true;
}

QChar
filter_evaluator::nxchar(filter_eval_context* ctxt)
{
  if (ctxt->evp < ctxt->expr.length()) {
    return ctxt->expr.at(ctxt->evp);
  }
  return QChar();
}

QChar
filter_evaluator::nxchar1(filter_eval_context* ctxt)
{
  if (ctxt->evp+1 < ctxt->expr.length()) {
    return ctxt->expr.at(ctxt->evp+1);
  }
  return QChar();
}

bool
filter_evaluator::is_unary_op(const QString sym)
{
  for (uint i=0; i<sizeof(unary_ops)/sizeof(unary_ops[0]); i++) {
    if (unary_ops[i].name == sym)
      return true;
  }
  return false;
}

unary_op_t*
filter_evaluator::get_unary_op(const QString sym)
{
  QString lsym = sym.toLower();
  for (uint i=0; i<sizeof(unary_ops)/sizeof(unary_ops[0]); i++) {
    if (unary_ops[i].name == lsym)
      return &unary_ops[i];
  }
  return NULL;
}

binary_op_t*
filter_evaluator::get_binary_op(const QString sym)
{
  QString lsym = sym.toLower();
  for (uint i=0; i<sizeof(binary_ops)/sizeof(binary_ops[0]); i++) {
    if (binary_ops[i].name == lsym)
      return &binary_ops[i];
  }
  return NULL;
}

filter_eval_func*
filter_evaluator::get_func(const QString sym)
{
  QString lsym = sym.toLower();
  for (uint i=0; i<sizeof(eval_funcs)/sizeof(eval_funcs[0]); i++) {
    if (eval_funcs[i].name == lsym)
      return &eval_funcs[i];
  }
  return NULL;
}

#if 0
bool
filter_evaluator::is_function(const QString sym)
{
  QString lsym = sym.toLower();
  for (uint i=0; i<sizeof(eval_funcs)/sizeof(eval_funcs[0]); i++) {
    if (eval_funcs[i].name == lsym)
      return true;
  }
  return false;
}
#endif

bool
filter_evaluator::process_binary_op(filter_eval_context* ctxt, QString op, int prio)
{
  ctxt->evp++;
  if (inner_eval(prio, ctxt)) {
    filter_eval_value ev2 = ctxt->evstack.pop();
    filter_eval_value ev1 = ctxt->evstack.pop();
    QVariant v2 = ev2.val;
    QVariant v1 = ev1.val;
    filter_eval_value res;
    res.vtype = filter_eval_value::type_bool;
    DBG_PRINTF(6, "binary op %d %s %d", (int)ev1.vtype, op.toLocal8Bit().constData(), (int) ev2.vtype);
    if (op == "=") {
      if (ctxt->execute)
	res.val = v1.toString().toLower()==v2.toString().toLower();
    }
    else if (op == "==") {
      if (ctxt->execute)
	res.val = v1.toInt()==v2.toInt();
    }
    else if (op == "!=") {
      if (ctxt->execute)
	res.val = v1.toInt()!=v2.toInt();
    }
    else if (op == "=~") {
      QRegExp rx(v2.toString(), Qt::CaseInsensitive, QRegExp::RegExp2);
      if (ctxt->execute)
	res.val = (rx.indexIn(v1.toString()) >= 0);
    }
    else if (op == "!~") {
      QRegExp rx(v2.toString(), Qt::CaseInsensitive, QRegExp::RegExp2);
      if (ctxt->execute)
	res.val = (rx.indexIn(v1.toString()) < 0);
    }
    else if (op == "<") {
      if (ctxt->execute)
	res.val = (v1.toInt() < v2.toInt());
    }
    else if (op == ">") {
      if (ctxt->execute)
	res.val = (v1.toInt() > v2.toInt());
    }
    else if (op == ">=") {
      if (ctxt->execute)
	res.val = (v1.toInt() >= v2.toInt());
    }
    else if (op == "<=") {
      if (ctxt->execute)
	res.val = (v1.toInt() <= v2.toInt());
    }
    else {
      ctxt->errstr = QObject::tr("Unknown binary operator: %1").arg(op);
      return false;
    }
    ctxt->evstack.push(res);
  }
  return true;
}

bool
filter_evaluator::eval_number(filter_eval_context* ctxt)
{
  qlonglong v=0;
  bool end=false;
  int p=ctxt->evp;
  QChar c;
  while (!end && p<ctxt->expr.length()) {
    c = ctxt->expr.at(p++);
    if (c.isNumber()) {
      qlonglong ov = v;
      v = v*10 + c.digitValue();
      if (v < ov) { // overflow
	ctxt->errstr = QObject::tr("Integer number is too large");
	return false;
      }
    }
    else {
      QChar c1=c.toLower();
      // k,m or g after a number means kbytes,mbytes or gbytes
      if (c1=='k') { v *= 1024; }
      else if (c1=='m') { v *= 1024*1024; }
      else if (c1=='g') { v *= 1024*1024*1024; }
      else p--;
      end=true;
    }
  }
  ctxt->evp = p;
  filter_eval_value ev;
  ev.vtype = filter_eval_value::type_number;
  ev.val = QVariant(v);
  ctxt->evstack.push(ev);
  return true;
}

//static 
filter_eval_value
filter_evaluator::eval_subexpr(filter_eval_context* ctxt, const QString sym)
{
  QMap<QString,filter_eval_value>::const_iterator ei;
  ei = ctxt->expr_cache.constFind(sym);
  if (ei != ctxt->expr_cache.constEnd()) {
    return (*ei);
  }

  filter_eval_value vr;

  if (!ctxt->filter_list) {
    ctxt->errstr = QObject::tr("Condition not found: '%1'").arg(sym);
    return vr;
  }

  const filter_expr* exp = ctxt->filter_list->find_name(sym);
  if (!exp) {
    ctxt->errstr = QObject::tr("Condition not found: '%1'").arg(sym);
    return vr;
  }

  // prevent infinite recursion
  DBG_PRINTF(6, "eval_subexpr of %s, call_stack.size()=%d",
	     sym.toLocal8Bit().constData(), ctxt->call_stack.size());
  for (int i=ctxt->call_stack.size()-1; i>=0 ; i--) {
    DBG_PRINTF(6, "call_stack[%d]=%s", i, ctxt->call_stack.at(i).toLocal8Bit().constData());
    if (ctxt->call_stack.at(i) == sym) {
      ctxt->errstr = QObject::tr("Infinite recursion prevented while evaluating %1").arg(sym);
      return vr;
    }
  }

  QString subexpr = exp->m_expr_text;
  filter_eval_context subctxt = *ctxt;
  subctxt.evp = 0;
  subctxt.expr = subexpr;
  subctxt.call_stack.push(sym);
  if (inner_eval(0, &subctxt)) {
    vr = subctxt.evstack.pop();
    ctxt->expr_cache[sym] = vr;
  }
  else {
    ctxt->errstr = QObject::tr("Error while evaluating %1: %2").arg(sym).arg(subctxt.errstr);
  }
  return vr;
}

bool
filter_evaluator::inner_eval(int current_prio, filter_eval_context* ctxt)
{
  bool endexpr=false;

  skip_blanks(ctxt);
  QChar c = nxchar(ctxt);
  if (c=='(') {
    ctxt->npar++;
    ctxt->evp++;
    if (inner_eval(0, ctxt) && nxchar(ctxt)!=')') {
      ctxt->errstr = QObject::tr("Unmatched parenthesis");
    }
    ctxt->npar--;
    ctxt->evp++;
  }
  else if (c==')') {
    /* We accept parentheses around an empty content, but only
       in the context of evaluating function arguments */
    if (ctxt->evstack.size()>0 && ctxt->npar>0) {
      filter_eval_value& pv = ctxt->evstack.top();
      if (pv.vtype==filter_eval_value::type_func)
	endexpr=true;
    }
    if (!endexpr) {
      ctxt->errstr = QObject::tr("Unmatched closing parenthesis");
    }
  }
  else if (c=='\\') {
    QChar c1 = nxchar1(ctxt);
    if (c1=='"' || c1=='\'') {
      ctxt->evp+=2;
      if (!eval_string(ctxt, c1, true))
	return false;
    }
    else
      ctxt->errstr = QObject::tr("Unexpected character '\\'");
  }
  else if (c=='"' || c=='\'') {
    ctxt->evp++;
    if (!eval_string(ctxt, c, false))
      return false;
  }
  else if (c=='!') { // logical not
    if (current_prio <= PRI_UNARY_NOT) {
      ctxt->evp++;
      if (inner_eval(PRI_UNARY_NOT, ctxt)) {
	filter_eval_value& pv = ctxt->evstack.top();
	if (ctxt->execute)
	  pv.val = !pv.val.toBool();
      }
    }
    else
      endexpr=false;
  }
  else if (c.isNumber()) {
    if (!eval_number(ctxt))
      return false;
  }
  else if ((c.toLower().unicode()>=(int)'a' && c.toLower().unicode()<=(int)'z') || c=='_') {
    int startp = ctxt->evp;
    QString sym = getsym(ctxt);
    if (is_unary_op(sym)) {
      unary_op_t* uop = get_unary_op(sym);
      if (current_prio <= uop->prio) {
	if (inner_eval(uop->prio, ctxt)) {
	  filter_eval_value pv = ctxt->evstack.pop();
	  if (ctxt->execute)
	    pv = (*(uop->func))(pv, ctxt);
	  ctxt->evstack.push(pv);
	}
      }
      else {
	ctxt->evp = startp;
	endexpr = true;
      }
    }
    else {
      filter_eval_func* pfunc = get_func(sym);
      if (pfunc != NULL) {
	// function call
	skip_blanks(ctxt);
	if (nxchar(ctxt)=='(') {
	  ctxt->evp++;
	  ctxt->npar++;
	  skip_blanks(ctxt);	  
	  if (nxchar(ctxt)==')') {
	    ctxt->evp++;
	    ctxt->npar--;
	    if (pfunc->nb_args==0) {
	      filter_eval_value res;
	      if (ctxt->execute) {
		filter_eval_value v_arg; // null argument
		res = (*(pfunc->func))(v_arg, ctxt);
	      }
	      ctxt->evstack.push(res);
	    }
	    else {
	      ctxt->errstr = QObject::tr("Missing argument to function '%1'").arg(sym);	      
	    }
	  }
	  else {
	    if (pfunc->nb_args==0) {
	      ctxt->errstr = QObject::tr("The function '%1' does not accept any argument").arg(sym);
	    }
	    else {
	      int stack_depth = ctxt->evstack.size();

	      if (inner_eval(0, ctxt)) {
		if (nxchar(ctxt)!=')') {
		  ctxt->errstr = QObject::tr("Unmatched parenthesis");
		}
		else {
		  ctxt->npar--;
		  ctxt->evp++;
		  filter_eval_value v_arg;
		  if (stack_depth != ctxt->evstack.size())
		    v_arg = ctxt->evstack.pop();
		  filter_eval_value res;
		  if (ctxt->execute || sym=="condition") {
		    res = (*(pfunc->func))(v_arg, ctxt);
		  }
		  ctxt->evstack.push(res);
		}
	      }
	      else {
		endexpr=true; /* ctxt->errstr has supposedly been set
				 by the call to inner_eval() that
				 returned false */
	      }
	    }
	  }
	}
	else {
	  // no open parenthesis
	  if (pfunc->nb_args==0) {
	    filter_eval_value res;
	    if (ctxt->execute) {
	      filter_eval_value v_arg; // null argument
	      res = (*(pfunc->func))(v_arg, ctxt);
	    }
	    ctxt->evstack.push(res);
	  }
	  else {
	    ctxt->errstr = QObject::tr("Open parenthesis expected after function requiring arguments");
	  }
	}
      }
      else {
	// sub-expr, evaluate now
	filter_eval_value v;
	v= eval_subexpr(ctxt, sym);
	if (!ctxt->errstr.isEmpty()) {
	  ctxt->evp = startp;
	  return false;
	}
	ctxt->evstack.push(v);
      }
    }
  }
  else if (c.isNull()) {
    ctxt->errstr = QObject::tr("Unexpected end of expression");
  }
  else {
    // default
    DBG_PRINTF(4, "unexpected character codepoint=%d", c.unicode());
    ctxt->errstr = QObject::tr("Unexpected character");
  }
  
  if (!ctxt->errstr.isEmpty())
    return false;

  // ** binary operator and 2nd operand
  while (!endexpr && ctxt->errstr.isEmpty()) {
    skip_blanks(ctxt);
    if (ctxt->evp >= ctxt->expr.length()) {
      endexpr = true;
      break;
    }
    c = nxchar(ctxt);
    if (c == ')') {
      if (ctxt->npar == 0)
	ctxt->errstr = QObject::tr("Unmatched closing parenthesis");
      endexpr=true;
    }
    else if (c.isLetter()) {
      int p = ctxt->evp;
      QString o = get_op_name(ctxt);
      binary_op_t* bop = get_binary_op(o);
      if (bop!=NULL) {
	if (current_prio <= bop->prio) {
	  if (inner_eval(bop->prio, ctxt)) {
	    filter_eval_value pv2 = ctxt->evstack.pop();
	    filter_eval_value pv1 = ctxt->evstack.pop();
	    filter_eval_value res;
	    if (ctxt->execute)
	      res = (*(bop->func))(pv1, pv2, ctxt);
	    ctxt->evstack.push(res);
	  }
	}
	else {
	  ctxt->evp = p;
	  endexpr = true;
	}
      }
      else {
	ctxt->evp = p;
	ctxt->errstr = QObject::tr("Unknown operator");
      }
    }
    else if (c == '=') {
      QChar c1 = nxchar1(ctxt);
      if (c1 == '~') {
	if (current_prio <= filter_evaluator::PRI_REGEXP) {
	  ctxt->evp++;
	  process_binary_op(ctxt, "=~", filter_evaluator::PRI_REGEXP);
	}
	else
	  endexpr = true;
      }
      else if (c1 == '=') {
	if (current_prio <= filter_evaluator::PRI_CMP) {
	  ctxt->evp++;
	  process_binary_op(ctxt, "==", filter_evaluator::PRI_CMP);
	}
	else
	  endexpr=true;
      }
      else {
	if (current_prio <= filter_evaluator::PRI_CMP)
	  process_binary_op(ctxt, "=", filter_evaluator::PRI_CMP);
	else
	  endexpr = true;
      }
    }
    else if (c == '!') {
      QChar c1 = nxchar1(ctxt);
      if (c1 == '~') {
	if (current_prio <= filter_evaluator::PRI_REGEXP) {
	  ctxt->evp++;
	  process_binary_op(ctxt, "!~", filter_evaluator::PRI_CMP);
	}
	else
	  endexpr = true;
      }
      else if (c1 == '=') {
	if (current_prio <= filter_evaluator::PRI_CMP) {
	  ctxt->evp++;
	  process_binary_op(ctxt, "!=", filter_evaluator::PRI_CMP);
	}
	else
	  endexpr = true;
      }
      else {
	ctxt->errstr = QObject::tr("Unexpected operator");
      }
    }
    else if (c == '<') {
      if (nxchar1(ctxt) == '=') {
	if (current_prio <= filter_evaluator::PRI_CMP) {
	  ctxt->evp++;
	  process_binary_op(ctxt, "<=", filter_evaluator::PRI_CMP);
	}
	else
	  endexpr = true;
      }
      else {
	if (current_prio <= filter_evaluator::PRI_CMP)
	  process_binary_op(ctxt, "<", filter_evaluator::PRI_CMP);
	else
	  endexpr = true;
      }
    }
    else if (c == '>') {
      if (nxchar1(ctxt) == '=') {
	if (current_prio <= filter_evaluator::PRI_CMP) {
	  ctxt->evp++;
	  process_binary_op(ctxt, ">=", filter_evaluator::PRI_CMP);
	}
	else
	  endexpr = true;
      }
      else {
	if (current_prio <= filter_evaluator::PRI_CMP)
	  process_binary_op(ctxt, ">", filter_evaluator::PRI_CMP);
	else
	  endexpr = true;
      }
    }
    else if (c == '(') {
      ctxt->errstr = QObject::tr("Unexpected open parenthesis");
    }
    else {
      ctxt->errstr = QObject::tr("Unexpected operator");
    }
  }
  return ctxt->errstr.isEmpty();
}

filter_eval_result
filter_evaluator::evaluate(const filter_expr fe, const expr_list& elist, mail_id_t mail_id)
{
  filter_eval_context ctxt;
  ctxt.evp=0;
  ctxt.expr = fe.m_expr_text;
  ctxt.npar=0;
  ctxt.mail_id=mail_id;
  if (mail_id==0)
    ctxt.execute=false;
  ctxt.message.set_mail_id(mail_id);
  ctxt.filter_list = &elist;
  ctxt.start_time = time(NULL);

  filter_eval_result res;
  bool success;
  if (ctxt.expr.isEmpty()) {
    ctxt.errstr = QObject::tr("Empty expression");
    success=false;
  }
  else
    success = inner_eval(0, &ctxt);
  if (success) {
    filter_eval_value v = ctxt.evstack.pop();
    res.result = v.val.toBool();
  }
  else {
    res.result = false;
    res.errstr = ctxt.errstr;
    res.evp = ctxt.evp;
  }
  return res;
}
