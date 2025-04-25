/* Copyright (C) 2004-2025 Daniel Verite

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

#ifndef INC_HEADERS_GROUPVIEW_H
#define INC_HEADERS_GROUPVIEW_H

#include "dbtypes.h"
#include <list>
#include <map>
#include <set>
#include <QString>
#include <QCloseEvent>
#include <QTreeWidget>

class QCloseEvent;

typedef std::map<QString, std::set<mail_id_t> > m_mv_t;
typedef std::map<mail_id_t, std::set<QString> > m_mid_t;

// headers treeview
class hd_treeview: public QTreeWidget
{
  Q_OBJECT
public:
  hd_treeview(QWidget* parent);
  virtual ~hd_treeview();
};

class headers_groupview : public QWidget
{
  Q_OBJECT
public:
  headers_groupview(QWidget* parent=0);
  virtual ~headers_groupview();
  void init(const std::list<unsigned int>& id_list); // list of mail_id
  hd_treeview* m_trview;
  m_mv_t m_map_val;
  m_mid_t m_map_id;
  void closeEvent(QCloseEvent*);
  void set_threshold(int);
signals:
  void close();
private:
  // the minimum percentage of identical headers values required for a header
  // to be displayed. Defaults to 0.
  int m_threshold;
};

class header_item: public QTreeWidgetItem
{
public:
  header_item(hd_treeview* parent, int count, const QString& hval, int total);
  header_item(header_item* parent, int count, const QString& hval, int total);
  virtual ~header_item() {}
#if 0
  int compare(QTreeWidgetItem *i, int col, bool asc) const {
    if (col==col_for_count) {
      header_item* hi = static_cast<header_item*>(i);
      return (m_cnt - hi->m_cnt);
    }
    else
      return QTreeWidgetItem::compare(i,col,asc);
  }
#endif
  int m_cnt;
  enum {
	col_for_count=0,
    col_for_value=1
  };
  std::set<mail_id_t> m_id_set;
  m_mid_t* m_pmap_id;
};

#endif
