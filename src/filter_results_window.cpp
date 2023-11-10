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

#include "filter_results_window.h"
#include "mail_listview.h"
#include "message_view.h"
#include "attachment_listview.h"
#include "icons.h"

#include <QBoxLayout>
#include <QHeaderView>
#include <QStatusBar>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QSplitter>
#include <QSlider>


filter_results_window::filter_results_window(QWidget* parent):
  QWidget(parent)
{
  setWindowTitle(tr("Filter matches"));

  m_msg_list = new mail_listview;
  m_msg_list->set_threaded(false);
  m_msg_list->init_columns();
  m_msg_list->setContextMenuPolicy(Qt::NoContextMenu);
  m_msg_list->header()->setContextMenuPolicy(Qt::NoContextMenu);
  m_msg_list->setSelectionMode(QAbstractItemView::SingleSelection);
  m_msg_list->setRootIsDecorated(false);
  m_msg_list->setToolTip(tr("Double-click or press Enter on a message to open it"));
  connect(m_msg_list,SIGNAL(activated(const QModelIndex&)),
	  this,SLOT(open_message()));

  m_expr_viewer = new filter_expr_viewer;

  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->addWidget(new QLabel(tr("Filter expression:")));
  top_layout->addWidget(m_expr_viewer);
  top_layout->addWidget(m_msg_list);
  
  QHBoxLayout* btn_layout = new QHBoxLayout();
  top_layout->addLayout(btn_layout);

#if 0
  m_open_message = new QPushButton(tr("Show message"));
  btn_layout->addWidget(m_open_message);
  connect(m_open_message, SIGNAL(clicked()), this, SLOT(open_message()));
#endif

  m_status_bar = new QStatusBar();
  top_layout->addWidget(m_status_bar);
  int left, top, right, bottom;
  top_layout->getContentsMargins(&left, &top, &right, &bottom);
  top_layout->setContentsMargins(0, 0, 0, 0);
  m_msg_list->setContentsMargins(3, 3, 3, 3);

  m_progress_bar = new QProgressBar(this);
  m_status_bar->addPermanentWidget(m_progress_bar);
  m_progress_bar->setMaximum(0);
  m_progress_bar->setMinimum(0);

  m_abort_button = new QPushButton(tr("Stop"), this);
  m_abort_button->setIcon(UI_ICON(ICON16_CANCEL));
  m_status_bar->addPermanentWidget(m_abort_button);
  connect(m_abort_button, SIGNAL(clicked()), this, SLOT(cancel_run()));

  hide_progressbar();

  resize(m_msg_list->width()+6, height());
}

filter_results_window::~filter_results_window()
{
}

void
filter_results_window::clear()
{
  m_msg_list->clear();
}

void
filter_results_window::show_filter_expression(const QString expr)
{
  DBG_PRINTF(4, "expr=%s", expr.toLocal8Bit().constData());
  m_expr_viewer->setPlainText(expr);
}

void
filter_results_window::show_status_message(QString msg)
{
  if (msg.isEmpty())
    m_status_bar->clearMessage();
  else
    m_status_bar->showMessage(msg);
}

void
filter_results_window::incorporate_message(mail_result& r)
{
  mail_msg* msg = new mail_msg(r);
  m_msg_list->add_msg(msg);
}

void
filter_results_window::cancel_run()
{
  hide_progressbar();
  emit stop_run();
}

void
filter_results_window::open_message()
{
  std::vector<mail_msg*> sel;
  m_msg_list->get_selected(sel);
  if (sel.size()==1) {
    filter_result_message_view* view = new filter_result_message_view();
    view->setWindowTitle(tr("Message #%1").arg(sel[0]->get_id()));
    view->set_message(sel[0]);
    view->resize(700,400);
    view->show();
  }  
}

int
filter_results_window::nb_results()
{
  return m_msg_list->model()->rowCount();
}

void
filter_results_window::show_progressbar()
{
  m_abort_button->show();
  m_progress_bar->show();
}

void
filter_results_window::hide_progressbar()
{
  m_abort_button->hide();
  m_progress_bar->hide();
}


filter_expr_viewer::filter_expr_viewer(QWidget* parent) : QTextBrowser(parent)
{
  setReadOnly(true);
  //  setFrameStyle(QFrame::NoFrame);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // We track resizings of the contents to adjust the height of
  // so that all the contents stay visible. 
  connect((const QObject*)document()->documentLayout(),
	  SIGNAL(documentSizeChanged(const QSizeF&)),
	  this,
	  SLOT(size_changed(const QSizeF&)));

  //  setFocusPolicy(Qt::NoFocus);	// no keyboard events?

}

void
filter_expr_viewer::size_changed(const QSizeF& newsize)
{
  setFixedHeight(newsize.height()+2);
}

filter_result_message_view::filter_result_message_view(QWidget* parent):
  QWidget(parent)
{
  QVBoxLayout* top_layout = new QVBoxLayout(this);

  QHBoxLayout* hb1 = new QHBoxLayout();
  top_layout->addLayout(hb1);
  QLabel* none = new QLabel(tr("No header"));
  hb1->addWidget(none);
  QSlider* slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(0);
  slider->setMaximum(2);
  slider->setTickPosition(QSlider::TicksBothSides);
  slider->setTickInterval(1);
  slider->setSliderPosition(1);
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(set_headers_visibility(int)));
  hb1->addWidget(slider);
  QLabel* all = new QLabel(tr("All headers"));
  hb1->addWidget(all);
  hb1->addStretch(8);

  QSplitter* splitter = new QSplitter(Qt::Vertical);
  top_layout->addWidget(splitter);
  m_msg_view = new message_view(splitter, NULL);
  m_attch_listview = new attch_listview(splitter);
  QList<int> lsizes;
  lsizes << 400 << 15;
  splitter->setSizes(lsizes);
}

void
filter_result_message_view::set_headers_visibility(int level)
{
  display_prefs prefs;
  prefs.init();
  if (level>2)
    level=2;
  prefs.m_show_headers_level = level;
  m_msg_view->display_body(prefs, message_view::default_conf);
}

void
filter_result_message_view::set_message(mail_msg* msg)
{
  m_msg_view->set_mail_item(msg);
  display_prefs prefs;
  prefs.init();
  m_msg_view->display_body(prefs, message_view::default_conf);

  // display attachments
  m_attch_listview->clear();

  attachments_list& attchs=msg->attachments();
  if (msg->has_attachments()) {
    attchs.fetch();
  }

  if (attchs.size()) {
    attachments_list::iterator iter;
    attch_lvitem* last_item=NULL; // used to insert the attachments in the same order as they're in the database
    for (iter=attchs.begin(); iter!=attchs.end(); ++iter) {
      attch_lvitem* item = new attch_lvitem(m_attch_listview, last_item, &(*iter));
      last_item = item;
      item->fill_columns();
    }
  }
  else
    m_attch_listview->hide();
}
