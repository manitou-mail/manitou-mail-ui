/* Copyright (C) 2004-2017 Daniel Verite

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

#include "main.h"
#include "msg_list_window.h"
#include "query_listview.h"
#include "message_view.h"
#include "attachment_listview.h"
#include "browser.h"

#include <QAction>
#include <QHeaderView>
#include <QList>
#include <QRegExp>
#include <QSplitter>
#include <QStackedWidget>

void
msg_list_window::change_page(msgs_page* p)
{
  // update the variables that have to point to the current page's data
  m_qlist = p->m_page_qlist;
  m_qAttch = p->m_page_attach;
  m_msgview = p->m_page_msgview;
  m_filter = p->m_page_filter;
  mail_msg* tmp_item = m_pCurrentItem;
  m_pCurrentItem = p->m_page_current_item;
  p->m_page_current_item = tmp_item;

  m_pages->raise_page(p);

  enable_forward_backward();
  set_title();
  m_tags_box->reset_tags();

  // highlight the choice in the fast selection listview if it makes sense
  m_query_lv->set_focus_on_id(p->m_query_lvitem_id);

  // redisplay body, attachments and all
  mails_selected();
}

void
msg_list_window::select_all_text()
{
  m_msgview->select_all_text();
}

void
msg_list_window::move_forward()
{
  msgs_page* next = m_pages->next_page();
  if (next)
    change_page(next);
  // else we're already at the end of the list, there's no going forward
}

void
msg_list_window::move_backward()
{
  msgs_page* prev = m_pages->previous_page();
  if (prev)
    change_page(prev);
  // else we're already at the beginning of the list, there's no going back
}

void
msg_list_window::enable_forward_backward()
{
  bool back = (m_pages->previous_page()!=NULL);
  bool forward = (m_pages->next_page()!=NULL);

  m_action_move_backward->setEnabled(back);
  m_action_move_forward->setEnabled(forward);
  m_msgview->enable_page_nav(back, forward);
}

/*
  Find and remove a page to get a place for a new one in the
  m_pages list
*/
void
msg_list_window::free_msgs_page()
{
  // when the current page is at the end of the list, remove the first page
  // of the list (first=deeper in the stack in this context)
  if (!m_pages->next_page()) {
    DBG_PRINTF(5, "remove first page");
    m_pages->remove_page(m_pages->first_page());
  }
  else {
    // when the current page is not at the end of the list, remove
    // the last page of the list
    DBG_PRINTF(5, "remove last page");
    m_pages->remove_page(m_pages->last_page());
  }
}


/*
  Change a widget's font for each of its instances (one per page)
  'element'==1 => widget=list of messages and attachments
  'element'==2 => widget=body
*/
void
msg_list_window::set_pages_font(int element, const QFont& font)
{
  std::list<msgs_page*>::iterator it1;
  for (it1=m_pages->begin(); it1!=m_pages->end(); ++it1) {
    if (element==1) {
      (*it1)->m_page_qlist->setFont(font);
      (*it1)->m_page_attach->setFont(font);
    }
    else if (element==2) {
      (*it1)->m_page_msgview->setFont(font);
    }
  } 
}

/*
  Refresh the status of a message in the listview. If code=-1,
  remove the mail from the list
*/
void
msgs_page::refresh_status(mail_msg* msg, int code)
{
  if (code==-1) {
    /* warning: the message is not removed from the msgs_filter's list,
       msg_list_window::remove_msg() should be used instead if that's
       what's needed */
    m_page_qlist->remove_msg(msg, false);
  }
  else {
    m_page_qlist->update_msg(msg);
  }
}

/*
  Instantiate and fetch a new page based on the selection contained in
  the filter 'f'.

  If 'if_results' is true and the fetch returns no result (0 row),
  then display an error instead of creating the page.
*/

void
msg_list_window::add_msgs_page(const msgs_filter* f, bool if_results _UNUSED_)
{
  m_filter = new msgs_filter(*f);
  QFont body_font;
  QFont list_font;
  QByteArray headerview_setup;

  msgs_page* current = m_pages->current_page();
  // current may be null if we're instantiating the first page
  if (current) {
    body_font = m_msgview->font();
    list_font = m_qlist->font();
    if (m_qlist->sender_recipient_swapped())
      m_qlist->swap_sender_recipient(false);    
    headerview_setup = m_qlist->header()->saveState();
  }

  // new splitter
  QStackedWidget* stackw = m_pages->stacked_widget();
  QSplitter* l=new QSplitter(Qt::Vertical, this);
  stackw->addWidget(l);

  m_qlist = new mail_listview(l);
  m_qlist->set_threaded(display_vars.m_threaded);
  m_qlist->m_msg_window=this;
  if (current)
    m_qlist->setFont(list_font);
  m_qlist->init_columns();
  if (current)
    m_qlist->header()->restoreState(headerview_setup);

  if (!m_filter->m_fetched)
    m_filter->fetch(m_qlist);
  else
    m_filter->make_list(m_qlist);
  msg_list_postprocess();

  m_msgview = new message_view(l, this);

  if (current)
    m_msgview->setFont(body_font);

  connect(m_msgview, SIGNAL(on_demand_show_request()), this, SLOT(display_msg_contents()));
  connect(m_msgview, SIGNAL(popup_body_context_menu()), this, SLOT(body_menu()));
  connect(m_msgview, SIGNAL(page_back()), this, SLOT(move_backward()));
  connect(m_msgview, SIGNAL(page_forward()), this, SLOT(move_forward()));

  m_qAttch = new attch_listview(l);
  connect(m_qAttch, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	  this, SLOT(attch_selected(QTreeWidgetItem*,int)));
  connect(m_qAttch, SIGNAL(init_progress(const QString)),
	  this, SLOT(install_progressbar(const QString)));
  connect(m_qAttch, SIGNAL(progress(int)), this, SLOT(show_progress(int)));
  connect(m_qAttch, SIGNAL(finish_progress()), this, SLOT(uninstall_progressbar()));
  connect(this, SIGNAL(abort_progress()), m_qAttch, SLOT(download_aborted()));

  // splitter for 3 panes: list of messages / body / attachments list
  static const int splitter_default_sizes[3] = {100,400,25};
  QList<int> lsizes;
  if (current) {
    lsizes = current->m_page_splitter->sizes();
    // don't allow any zero size for panels. This is necessary to avoid having the
    // attachment list being almost invisible to the user
    for (int i=0; i<3; i++) {
      if (lsizes.at(i)==0) {
	lsizes.replace(i, splitter_default_sizes[i]);
      }
    }
  }
  else {
    for (int i=0; i<3; i++) {
      int sz;
      QString key;
      key.sprintf("display/msglist/panel%d_size", i+1);
      if (get_config().exists(key)) {
	sz=get_config().get_number(key);
	if (sz==0)
	  sz=splitter_default_sizes[i];
      }
      else
	sz=splitter_default_sizes[i];
      lsizes.append(sz);
    }
  }

  /* avoid changing the listview's height each time the attachments view
     is shown or hidden */
  l->setStretchFactor(l->indexOf(m_qlist), 0);
  l->setStretchFactor(l->indexOf(m_msgview), 1);
  l->setStretchFactor(l->indexOf(m_qAttch), 0);

  l->setSizes(lsizes);
  m_qAttch->hide();
  m_wSearch=NULL;

  connect(m_qlist,SIGNAL(selection_changed()), this,SLOT(mails_selected()));

  connect(m_qlist,SIGNAL(doubleClicked(const QModelIndex&)),
	  this,SLOT(mail_reply_all()));

  connect(m_qlist, SIGNAL(scroll_page_down()), m_msgview, SLOT(page_down()));
  connect(m_qlist, SIGNAL(note_icon_clicked()), this, SLOT(edit_note()));

  /* Use queued connection because fetch_segment() may replace the list
     contents. */
  connect(m_qlist, SIGNAL(change_segment(int)), this, SLOT(fetch_segment(int)),
	  Qt::QueuedConnection);

  if (m_pages->next_page()) {
    // we're in the middle of a page list, and asked to go forward.
    // let's remove all the pages that are after the current position
    m_pages->cut_pages(m_pages->next_page());
  }
  int max_pages=get_config().get_number("msg_window_pages");
  if (max_pages<2) max_pages=2;
  if (m_pages->count() >= max_pages) {
    free_msgs_page();
    if (m_pages->count() >= max_pages) {
      // Still no room for a new page? OK, forget it
      DBG_PRINTF(5,"not enough pages!");
      return;
    }
  }

  // allocate and use a new page
  msgs_page* page = new msgs_page();
  page->m_page_filter = m_filter;
  page->m_page_msgview = m_msgview;
  page->m_page_attach = m_qAttch;
  page->m_page_qlist = m_qlist;
  page->m_page_current_item = NULL;
  page->m_page_splitter = l;
  page->m_msgs_window = this;
  page->m_query_lvitem_id = m_query_lv->current_id();
  m_pages->add_page(page);
  m_pages->raise_page(page);
  m_tags_box->reset_tags();
  enable_forward_backward();
  enable_segments();
}

