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

#include "filter_log.h"

bool
filter_log_list::fetch(mail_id_t mail_id)
{
  db_cnx db;
  m_mail_id=mail_id;
  try {
    /* If the filter has been deleted after the filter_log entry
       creation, then coalesce(e.expr_id,0) will be 0.
       Also if the filter has been modified since, hit_date-e.last_update will be <0
     */
    sql_stream s("SELECT f.expr_id, e.name, e.expression, to_char(hit_date,'YYYYMMDDHHMI'),coalesce(e.expr_id,0), SIGN(EXTRACT(epoch FROM (hit_date-e.last_update))) FROM filter_log f LEFT JOIN filter_expr e ON e.expr_id=f.expr_id WHERE f.mail_id=:id ORDER BY hit_date", db);
    s << mail_id;
    while (!s.eof()) {
      filter_log_entry e;
      QString hit_date;
      int e_id, sign;
      s >> e.m_expr_id >> e.m_filter_name >> e.m_filter_expression >> hit_date >> e_id >> sign;
      e.m_deleted = (e_id==0);
      e.m_modified = (sign<0);
      e.m_hit_date = date(hit_date);
      this->append(e);
    }
  }
  catch (db_excpt p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}
