/* Copyright (C) 2004-2007 Daniel Vérité

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

#ifndef INC_MSGS_PAGE_LIST_H
#define INC_MSGS_PAGE_LIST_H

#include <list>

class QWidget;
class QStackedWidget;
class msgs_page;

class msgs_page_list
{
public:
  msgs_page_list(QWidget* parent);
  int count() const {
    return plist.size();
  }
  void add_page(msgs_page* p);
  void raise_page(msgs_page* p);
  void remove_page(msgs_page* p);
  void cut_pages(msgs_page* start);
  msgs_page* current_page();
  msgs_page* next_page();
  msgs_page* previous_page();
  msgs_page* first_page();
  msgs_page* last_page();

  // remove these when the callers have been refactored to use functors
  std::list<msgs_page*>::iterator begin() {
    return plist.begin();
  }
  std::list<msgs_page*>::iterator end() {
    return plist.end();
  }
  bool empty() const {
    return plist.empty();
  }

  QStackedWidget* stacked_widget() {
    return qstackw;
  }

  // list of mail pages that are currently instantiated
  // used to propagate mail changes to all windows
  static std::list<msgs_page*> m_all_pages_list;

private:
  void find_current_page();
  std::list<msgs_page*> plist;
  msgs_page* m_current_page;
  /* The widgets we stack into this variable are QSplitter
     that separate the listview at the top from the message body and attachments
     below */
  QStackedWidget* qstackw;
  typedef std::list<msgs_page*>::iterator page_iter;
};

#endif // INC_MSGS_PAGE_LIST_H
