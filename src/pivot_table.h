/* Copyright (C) 2004-2024 Daniel Verite

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

#ifndef PIVOT_TABLE_H
#define PIVOT_TABLE_H

#include <QTableWidget>

class pivot_table : public QTableWidget
{
public:
  pivot_table(QWidget* parent=NULL);
  virtual ~pivot_table();
  enum pivot_order { order_none, order_rows, order_columns, order_both };
  void init(QString sql_query, enum pivot_order order=order_none);

};


#endif
