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

#include <math.h>
#include <QWidget>
#include <QLayout>
#include <QTreeWidget>
#include <QStringList>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>

#include "db.h"
#include "main.h"
#include "sqlstream.h"
#include "headers_groupview.h"

void analyze_headers(const std::list<unsigned int>& l)
{
  headers_groupview* w = new headers_groupview(NULL);
  w->init(l);
  w->show();
}

headers_groupview::headers_groupview(QWidget* parent) : QWidget(parent)
{
  m_threshold=0;
  QVBoxLayout* top_layout = new QVBoxLayout(this);

  m_trview = new hd_treeview(this);
  CHECK_PTR(m_trview);
  m_trview->setRootIsDecorated(true);
  m_trview->setAllColumnsShowFocus(true);
  top_layout->addWidget(m_trview);

  QHBoxLayout* hl = new QHBoxLayout();
  top_layout->addLayout(hl);
  QPushButton* ok = new QPushButton(tr("Close"), this);
  connect(ok, SIGNAL(clicked()), this, SLOT(close()));
  hl->addStretch(1);
  hl->addWidget(ok);
  hl->addStretch(1);
}

headers_groupview::~headers_groupview()
{
}

void
headers_groupview::set_threshold(int v)
{
  m_threshold=v;
}

void
headers_groupview::closeEvent(QCloseEvent* e)
{
  emit close();
  e->accept();
}

void
headers_groupview::init(const std::list<unsigned int>& id_list)
{
  int headers_count=0;
  db_cnx db;
  sql_stream s("SELECT lines FROM header WHERE mail_id=:p1", db);
  std::list<unsigned int>::const_iterator iter=id_list.begin();
  QString h;
  m_mv_t::iterator v_it;
  for (; iter!=id_list.end(); ++iter) {
    s << *iter;
    if (!s.eos()) {
      s >> h;
    }
    headers_count++;
    mail_id_t mail_id=*iter;
    QStringList sl = h.split('\n', QString::SkipEmptyParts);
    QString s;

    std::set<QString> hset;
    m_mid_t::iterator mit;
    m_map_id[mail_id] = hset;
    mit = m_map_id.find(mail_id);

    for (QStringList::iterator isl = sl.begin(); isl!=sl.end(); ++isl) {
      s = (*isl).trimmed();
      //DBG_PRINTF(3, "%s\n", s.latin1());
      v_it = m_map_val.find(s);
      if (v_it==m_map_val.end()) {
	std::set<mail_id_t> mset;
	mset.insert(*iter);
	m_map_val[s] = mset;
      }
      else {
	v_it->second.insert(*iter); // update m_map_val
      }
      mit->second.insert(s);	// update m_map_id
    }
  }
  
  header_item* qitem;
  for (v_it=m_map_val.begin(); v_it!=m_map_val.end(); ++v_it) {
    int count=v_it->second.size();
    if (count>1 && ceil((100.0*count)/headers_count) >= m_threshold) {
      qitem = new header_item(m_trview, count, v_it->first, headers_count);
      //      qitem->setExpandable(true);
      qitem->m_id_set = v_it->second;
      qitem->m_pmap_id = &m_map_id;
    }
  }
  
}

hd_treeview::hd_treeview(QWidget* parent) : QTreeWidget(parent)
{
  sortByColumn(0, Qt::DescendingOrder);
  QStringList headers;
  headers << tr("Count") << tr("Header: value");
  setHeaderLabels(headers);
}

hd_treeview::~hd_treeview()
{
}

header_item::header_item(hd_treeview* parent, int count, 
			 const QString& hval, int total):
  QTreeWidgetItem(parent), m_cnt(count)
{
  char buf[10]="";
  if (total!=0)
    sprintf(buf, "%2.2f%%", (100.0*count)/total);
  setText(col_for_count, QString("%1 (%2)").arg(count).arg(buf));
  setText(col_for_value, hval);
}

header_item::header_item(header_item* parent, int count, 
			 const QString& hval, int total):
  QTreeWidgetItem(parent), m_cnt(count)
{
  char buf[10]="";
  if (total!=0)
    sprintf(buf, "%2.2f%%", (100.0*count)/total);
  setText(col_for_count, QString("%1 (%2)").arg(count).arg(buf));
  setText(col_for_value, hval);
}

#if 0
// maybe reactivate that code when we'll support tree-based navigation
// through headers. It's still Qt3 code because of setOpen() and setExpandable()
void
header_item::setOpen(bool b)
{
  if (b && childCount()==0) {
    QString hs=text(col_for_value);
    std::map<QString,int> lmap; // header value => count
    std::map<QString,int>::iterator lmap_it;
    if (!m_id_set.empty()) {
      std::set<mail_id_t>::iterator ml_it = m_id_set.begin();
      // for each mail that contains the current header line
      for (; ml_it!=m_id_set.end(); ++ml_it) {
	mail_id_t mail_id = *ml_it;
	// for each header line contained by the message numbered 'id'
	m_mid_t::iterator mit = m_pmap_id->find(mail_id);
	if (mit == m_pmap_id->end())
	  continue;
	std::set<QString>& s = mit->second;
	std::set<QString>::iterator sit;
	// store in lmap the number of occurrences of the header line
	for (sit = s.begin(); sit!=s.end(); ++sit) {
	  lmap_it = lmap.find(*sit);
	  if (lmap_it==lmap.end())
	    lmap[*sit]=1;
	  else
	    lmap_it->second++;
	}
      }
      // Here we have lmap filled with each different header line associated
      // with its number of occurrences
      // Now create the child entries
      header_item* qitem;
      for (lmap_it=lmap.begin(); lmap_it!=lmap.end(); ++lmap_it) {
	if (lmap_it->second >= 2 && lmap_it->first!=hs) {
	  qitem = new header_item(this, lmap_it->second, lmap_it->first,
				  m_id_set.size());
	  qitem->setExpandable(true);
	  qitem->m_pmap_id = m_pmap_id;
	}
      }
    }
  }
  QTreeWidgetItem::setOpen(b);
}
#endif

