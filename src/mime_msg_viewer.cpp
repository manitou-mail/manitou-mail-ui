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
#include <QTextCodec>
#include <QStringList>

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
  QString font_name = get_config().get_string("display/font/msgbody");
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

  mail_header mh;
  mh.set_raw(msg, hlen);

  QString header_html="<div id=\"manitou-header\">";
  mh.format(header_html, mh.raw_headers());
  header_html.append("</div><br>");

  QStringList ct = mh.get_header("Content-Type");
  if (ct.size() >= 1) {
    /* rfc2045 simplified parser */
    QString field = ct.at(0);
    QRegExp rx("^[a-z]+/[a-z]+;", Qt::CaseInsensitive);
    if (field.indexOf(rx) == 0) {
      // match
      int l = rx.matchedLength();
      QStringList params = field.mid(l).split(";");
      if (params.size() >= 1) {
	QRegExp rx("charset=\"?([a-z_0-9-]+)\"?", Qt::CaseInsensitive);
	for (int i=0; i<params.size(); i++) {
	  if (rx.indexIn(params.at(i)) != -1) {
	    set_encoding(rx.cap(1));
	    break;
	  }
	}
      }
    }
  }

  display_prefs disp = prefs;

  QStringList cte = mh.get_header("Content-Transfer-Encoding");
  if (cte.size() >= 1) {
    if (cte.at(0) == "quoted-printable") {
      disp.m_decode_qp = true;
    }
  }

  QString body_html;
  format_body(body_html, msg+hlen, disp);
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
			     display_prefs& prefs)
{
  QString b;
  int startline=0;
  int endline;
  QTextCodec* codec = NULL;

  if (!m_encoding.isEmpty()) {
    codec = QTextCodec::codecForName(m_encoding.toLocal8Bit());
    /* If quoted-printable, we expect ASCII and including QP sequences
       to decode with the codec in the displayer.
       If not (FIXME: base64), we expect that the bytes represent
       the characters of the target body (without any other intermediate layer) */
    if (codec && !prefs.m_decode_qp) {
      b = codec->toUnicode(body);
    }
    else
      b = QString::fromLatin1(body);
  }
  else {
    b = QString::fromLatin1(body);
  }

  mail_displayer disp;

  /* Choose an arbitrary monobyte encoding if none specified.
     US-ASCII could do in theory, use latin1 to be nice. */
  prefs.m_codec = codec ? codec : QTextCodec::codecForName("ISO-8859-1");

  do {
    endline = b.indexOf('\n', startline);
    if (endline < 0) {
      endline = b.length();
    }
    QString html_line = disp.expand_body_line(b.mid(startline, endline-startline+1),
					      prefs);
    output.append(html_line);
    startline = endline+1;
  } while (startline < b.length());
}
