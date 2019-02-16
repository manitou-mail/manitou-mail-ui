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

#ifndef INC_FILTER_LOG_H
#define INC_FILTER_LOG_H

#include <QString>
#include <QList>
#include "date.h"
#include "db.h"
#include "message.h"

// Represents entries from the filter_log table
class filter_log_entry
{
public:
  filter_log_entry() {};
  ~filter_log_entry() {};
  QString filter_name() const { return m_filter_name; }
  QString filter_expression() const { return m_filter_expression; }
  int filter_expr_id() const { return m_expr_id; }
  QString m_filter_name;
  QString m_filter_expression;
  int m_expr_id;
  date m_hit_date;
  bool m_deleted;	     // the filter has been deleted since the entry's creation
  bool m_modified;     // the filter has been modified since the entry's creation
};

class filter_log_list : public QList<filter_log_entry>
{
public:
  filter_log_list() : m_mail_id(0) {};
  ~filter_log_list() {};
  mail_id_t m_mail_id;
  bool fetch(mail_id_t);
};


#endif // INC_FILTER_LOG_H
