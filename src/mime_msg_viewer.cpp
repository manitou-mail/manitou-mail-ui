/* Copyright (C) 2004-2013 Daniel Verite

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
#include "mime_msg_viewer.h"
#include "mail_displayer.h"
#include "mailheader.h"
#include "message_view.h"
#include "attachment_listview.h"
#include "app_config.h"
#include "icons.h"

#include <QSplitter>
#include <QAction>
#include <QLayout>
#include <QToolBar>

mime_msg_viewer::mime_msg_viewer(const char* msg, const display_prefs& prefs)
{
  setWindowTitle(tr("Attached message"));
  QToolBar* toolbar = new QToolBar(tr("Message Operations"));
  toolbar->setFloatable(true);
  toolbar->setIconSize(QSize(16,16));
  
  QAction* action_edit =
    toolbar->addAction(UI_ICON(FT_ICON16_EDIT_COPY), tr("Copy"),
		       this, SLOT(edit_copy()));
  action_edit->setShortcut(Qt::CTRL+Qt::Key_C);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(5);
  layout->setMargin(5);

  layout->addWidget(toolbar);

  QSplitter* splitter = new QSplitter(Qt::Vertical, this);
  layout->addWidget(splitter);
  m_view = new message_view(splitter, NULL);
  QString font_name=get_config().get_string("display/font/msgbody");
  if (!font_name.isEmpty()) {
    QFont f;
    f.fromString(font_name);
    m_view->setFont(f);
  }
  else {
    m_view->setFont(this->font());
  }

  m_attchview = new attch_listview(splitter);
  m_attchview->hide();

  uint hlen = mail_header::header_length(msg);
  if (hlen==0)
    hlen=strlen(msg);
  /* We assume that the header can be represented in latin1
     if conformant, it should be us-ascii actually */
  m_header = QString::fromLatin1(msg, hlen);
  mail_header mh;
  QString header_html="<div id=\"manitou-header\">";
  mh.format(header_html, m_header);  
  header_html.append("</div><br>");

  QString body_html;
  format_body(body_html, msg+hlen, prefs);
  body_html.prepend("<html><body><div id=\"manitou-body\">");
  body_html.append("</div></body></html>");
  m_view->set_html_contents(body_html, 1); // content-type=text
  m_view->prepend_body_fragment(header_html);
  resize(800,600);
}

mime_msg_viewer::~mime_msg_viewer()
{
}

void
mime_msg_viewer::edit_copy()
{
  m_view->copy();
}

void
mime_msg_viewer::format_body(QString& output,
			     const char* body,
			     const display_prefs& prefs)
{
  // TODO: apply a codec to 'body' to ensure we get a proper unicode
  // string in 'b'. Probably that current code doesn't work with
  // non-latin encodings
  QString b=body;
  int startline=0;
  int endline;

  mail_displayer disp;
  disp.m_wrap_lines=false;

  do {
    endline = b.indexOf('\n', startline);
    if (endline<0) {
      endline = b.length();
    }
    QString html_line=disp.expand_body_line(b.mid(startline, endline-startline+1),
					    prefs);
    //    b2.append();
    //    b2.append("<br>");
    output.append(html_line);
//    output.append("<br>");
    startline = endline+1;
  } while (startline < b.length());
}
