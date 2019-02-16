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

// Implements a QTableWidget showing pivoted results from a SQL query

#include "pivot_table.h"
#include "sqlstream.h"
#include <QTableWidgetItem>

pivot_table::pivot_table(QWidget* parent) : QTableWidget(parent)
{
}

pivot_table::~pivot_table()
{
}

/*
  sql_query must return 3 columns: ROWNAME,COLNAME,VALUE
*/
void
pivot_table::init(QString sql_query, enum pivot_order order)
{
  db_cnx db;
  QMap<QString,int> col_set; // column name=>position
  QMap<QString,int> row_set; // row name=>position

  QStringList col_order; // columns in the order of the resultset
  QStringList row_order; // rows in the order of the resultset

  try {
    QString row, colname, val;
    sql_stream s(sql_query, db);
    int col_cnt=0;
    int row_cnt=0;
    while (!s.eos()) {
      s >> row >> colname >> val;
      if (!colname.isEmpty() && !col_set.contains(colname)) {
	col_order.append(colname);
	col_set.insert(colname, col_cnt++);
      }
      if (!row.isEmpty() && !row_set.contains(row)) {
	row_order.append(row);
	row_set.insert(row, row_cnt++);
      }
    }

    if (order==order_columns || order==order_both) {
      col_order.sort();
      // reverse order if necessary
      // for(int k=0, s=list.size(), max=(s/2); k<max; k++) list.swap(k,s-(1+k));
      for (int i=0; i<col_order.count(); i++) {
	int old_pos = col_set.value(col_order.at(i), -1);
	Q_ASSERT_X(old_pos<0, "pivot_table", "column not found while reordering");
	if (old_pos >=0) {
	  col_set.insert(col_order.at(i), i); // new position
	}
      }
    }

    if (order==order_rows || order==order_both) {
      row_order.sort();
      // reverse order if necessary
      // for(int k=0, s=list.size(), max=(s/2); k<max; k++) list.swap(k,s-(1+k));
      for (int i=0; i<row_order.count(); i++) {
	int old_pos = row_set.value(row_order.at(i), -1);
	Q_ASSERT_X(old_pos<0, "pivot_table", "row not found while reordering");
	if (old_pos >=0) {
	  row_set.insert(row_order.at(i), i); // new position
	}
      }
    }

    setRowCount(row_cnt);
    setColumnCount(col_cnt);
    setHorizontalHeaderLabels(col_order);
    setVerticalHeaderLabels(row_order);

    s.rewind();

    while (!s.eos()) {
      s >> row >> colname >> val;
      if (colname.isEmpty() || row.isEmpty())
	continue;
      int col_n = col_set[colname];
      int row_n = row_set[row];
      setItem(row_n, col_n, new QTableWidgetItem(val));
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}
