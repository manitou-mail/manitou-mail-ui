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

#include "msgs_page_list.h"
#include "msg_list_window.h"
#include <QSplitter>
#include <QStackedWidget>
#include <algorithm>

std::list<msgs_page*> msgs_page_list::m_all_pages_list;

msgs_page_list::msgs_page_list(QWidget* parent)
{
  m_current_page=NULL;
  qstackw = new QStackedWidget(parent);
}

void
msgs_page_list::raise_page(msgs_page* p)
{
  qstackw->setCurrentWidget(p->m_page_splitter);
  m_current_page=p;
}

msgs_page*
msgs_page_list::next_page()
{
  page_iter it = std::find(plist.begin(), plist.end(), m_current_page);
  if (it==plist.end())
    return NULL;
  ++it;
  return (it!=plist.end() ? (*it) : NULL);
}

msgs_page*
msgs_page_list::previous_page()
{
  std::list<msgs_page*>::reverse_iterator it = std::find(plist.rbegin(), plist.rend(), m_current_page);
  if (it==plist.rend())
    return NULL;
  ++it;
  return (it!=plist.rend() ? (*it) : NULL);
}

msgs_page*
msgs_page_list::first_page()
{
  page_iter it = plist.begin();
  return (it!=plist.end() ? *it : NULL);
}

msgs_page*
msgs_page_list::last_page()
{
  return plist.size()>0 ? plist.back() : NULL;
}

msgs_page*
msgs_page_list::current_page()
{
  return m_current_page;
}

/* Retrieve 'm_current_page' based on which widget the QStackedWidget has made visible */
void
msgs_page_list::find_current_page()
{
  QWidget* w = qstackw->currentWidget();

  for (page_iter it=plist.begin(); it!=plist.end(); ++it) {
    if ((*it)->m_page_splitter == w) {
      m_current_page=*it;
      return;
    }
  }
  m_current_page=NULL;
}

void
msgs_page_list::remove_page(msgs_page* p)
{
  page_iter it = std::find(plist.begin(), plist.end(), p);
  if (it==plist.end()) {
    ERR_PRINTF("page not found (plist.size()=%d)", plist.size());
    return;
  }
  QWidget* w = p->m_page_splitter;
  if (w) {
    m_all_pages_list.remove(*it);
    /*it =*/ plist.erase(it);
    qstackw->removeWidget(w);
    delete w;
    find_current_page();
  }
  else {
    ERR_PRINTF("no splitter in page");
  }
}

void
msgs_page_list::add_page(msgs_page* p)
{
  plist.push_back(p);
  m_all_pages_list.push_back(p);
}

/* Remove the set of pages ranging from 'p' to the end of the list */
void
msgs_page_list::cut_pages(msgs_page* p)
{
  page_iter it = std::find(plist.begin(), plist.end(), p);
  while (it!=plist.end()) {
    page_iter it1 = it;
    it1++;
    remove_page(*it);
    it=it1;
  }
}
