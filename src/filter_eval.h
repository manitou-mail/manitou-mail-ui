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

#ifndef INC_FILTER_EVAL_H
#define INC_FILTER_EVAL_H

#include <QStack>
#include <QString>
#include <QVariant>
#include <QMap>

#include <time.h>
#include "dbtypes.h"
#include "db.h"
#include "message.h"
#include "filter_rules.h"

class filter_eval_value {
public:
  filter_eval_value();
  typedef enum {
    type_null,
    type_func,
    type_string,
    type_number,
    type_bool
  } val_type_t;
  val_type_t vtype;
  QVariant val;
};

class filter_eval_context {
public:
  filter_eval_context() : execute(true) {}
  QString expr;
  int evp;			// current position in expression string
  //  int len;			// expression length
  int npar;
  QStack<filter_eval_value> evstack;
  QString errstr;
  int mail_id;
  QString header_cache;
  QString body_cache;
  QString identity_cache;
  bool execute;
  QMap<QString,filter_eval_value> expr_cache;
  QStack<QString> call_stack;
  const expr_list* filter_list; // other expressions in the filtering system
  mail_msg message;
  time_t start_time;
};

class filter_eval_result {
public:
  int evp;
  QString errstr;
  bool result;
};

/*
class filter_eval_error {
public:
  QString message;
  int char_index;  
};
*/

typedef filter_eval_value (*eval_binop_ptr)(const filter_eval_value,
					    const filter_eval_value,
					    filter_eval_context*);

typedef filter_eval_value (*eval_unop_ptr)(const filter_eval_value,
					   filter_eval_context*);


typedef filter_eval_value (*eval_func_ptr)(const filter_eval_value,
					   filter_eval_context*);


struct filter_eval_func {
  const char* name;
  eval_func_ptr func;
  int nb_args;
  filter_eval_value::val_type_t return_type;
};


typedef struct {
  const char* name;
  int prio;
  eval_binop_ptr func;
} binary_op_t;

typedef struct {
  const char* name;
  int prio;
  eval_unop_ptr func;
} unary_op_t;


class filter_evaluator {
public:
  filter_eval_result evaluate(const filter_expr fe,
			      const expr_list& elist,
			      mail_id_t mail_id);
  static bool inner_eval(int current_prio, filter_eval_context* ctxt);
  static const int PRI_DOT=10;
  static const int PRI_AND=24;
  static const int PRI_OR=22;
  static const int PRI_UNARY_NOT=30;
  static const int PRI_CMP=40;
  static const int PRI_CONTAINS=40;
  static const int PRI_REGEXP=40;
  static void skip_blanks(filter_eval_context*);
  static QString getsym(filter_eval_context*);
  static QString get_op_name(filter_eval_context*);
  static bool eval_string(filter_eval_context*,QChar,bool);
  static bool eval_number(filter_eval_context*);
  //  static bool is_function(const QString);
  static bool is_unary_op(const QString);
  static bool process_binary_op(filter_eval_context* ctxt, QString op, int prio);
  static  QChar nxchar(filter_eval_context*);
  static QChar nxchar1(filter_eval_context*);
  static filter_eval_func* get_func(const QString funcname);
  static unary_op_t* get_unary_op(const QString);
  static binary_op_t* get_binary_op(const QString);
  static filter_eval_value eval_subexpr(filter_eval_context* ctxt, const QString sym);

  // binary operators
  static filter_eval_value contains_operator(const filter_eval_value v1,
					     const filter_eval_value v2,
					     filter_eval_context* ctxt);
  static filter_eval_value and_operator(const filter_eval_value v1,
					const filter_eval_value v2,
					filter_eval_context* ctxt);
  static filter_eval_value or_operator(const filter_eval_value v1,
				       const filter_eval_value v2,
				       filter_eval_context* ctxt);
  static filter_eval_value is_operator(const filter_eval_value v1,
				       const filter_eval_value v2,
				       filter_eval_context* ctxt);
  static filter_eval_value isnot_operator(const filter_eval_value v1,
					  const filter_eval_value v2,
					  filter_eval_context* ctxt);
  static filter_eval_value equals_operator(const filter_eval_value v1,
					   const filter_eval_value v2,
					   filter_eval_context* ctxt);
  static filter_eval_value not_equals_operator(const filter_eval_value v1,
					       const filter_eval_value v2,
					       filter_eval_context* ctxt);
  static filter_eval_value regmatches_operator(const filter_eval_value v1,
					       const filter_eval_value v2,
					       filter_eval_context* ctxt);

  // unary operators
  static filter_eval_value not_operator(const filter_eval_value v,
					filter_eval_context* ctxt);

  // functions
  static filter_eval_value func_age(const filter_eval_value,
				    filter_eval_context*);
  static filter_eval_value func_condition(const filter_eval_value,
					  filter_eval_context*);
  static filter_eval_value func_date(const filter_eval_value,
				     filter_eval_context*);
  static filter_eval_value func_date_utc(const filter_eval_value,
					 filter_eval_context*);
  static filter_eval_value func_header(const filter_eval_value,
				       filter_eval_context*);
  static filter_eval_value func_body(const filter_eval_value,
				       filter_eval_context*);
  static filter_eval_value func_headers(const filter_eval_value,
				       filter_eval_context*);
  static filter_eval_value func_identity(const filter_eval_value,
					 filter_eval_context*);
  static filter_eval_value func_rawsize(const filter_eval_value,
					filter_eval_context*);
  static filter_eval_value func_subject(const filter_eval_value,
					filter_eval_context*);
  static filter_eval_value func_from(const filter_eval_value,
				     filter_eval_context*);
  static filter_eval_value func_to(const filter_eval_value,
				   filter_eval_context*);
  static filter_eval_value func_cc(const filter_eval_value,
				   filter_eval_context*);
  static filter_eval_value func_recipients(const filter_eval_value,
					   filter_eval_context*);
  static filter_eval_value func_now(const filter_eval_value,
				    filter_eval_context*);
  static filter_eval_value func_now_utc(const filter_eval_value,
				       filter_eval_context*);


private:
  static binary_op_t binary_ops[];
  static unary_op_t unary_ops[];
  static filter_eval_func eval_funcs[];

  static filter_eval_value eval_date(const filter_eval_value,
				     filter_eval_context* ctxt, time_t, int);
  static filter_eval_value func_list_header_addresses(const QString field,
						      filter_eval_context*);
  // interface with database
  static const QString& db_get_header(filter_eval_context*);
  static QString db_lookup_header(const QString, filter_eval_context*);
  static QString db_get_body(filter_eval_context*);
  static QString db_get_identity(filter_eval_context*);
  static time_t db_get_sender_timestamp(filter_eval_context*);
  static int db_get_age(filter_eval_context*, const QString unit);
  static int db_get_rawsize(filter_eval_context*);
};

#endif
