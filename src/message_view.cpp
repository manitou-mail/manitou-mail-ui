/* Copyright (C) 2004-2010 Daniel Verite

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

/* message_view combines headers_view and body_view stacked vertically
   with a shared scrollbar */

#include "db.h"
#include "main.h"

#include "message_view.h"
#include "headers_view.h"
#include "body_view.h"
#include "mail_displayer.h"
#include "msg_list_window.h"

#include <QFrame>
#include <QWebFrame>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QPalette>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QVariant>

#if QT_VERSION>=0x040600
#include <QWebElement>
#endif

message_view::message_view(QWidget* parent, msg_list_window* sub_parent) : QScrollArea(parent)
{
  m_pmsg=NULL;
  m_content_type_shown = 0;
  m_zoom_factor=1.0;
  m_parent = sub_parent;
  setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Active, QPalette::Background, Qt::white);
  pal.setColor(QPalette::Inactive, QPalette::Background, Qt::white);
  setPalette(pal);

  enable_page_nav(false, false);
  m_headersv = new headers_view();
  m_bodyv = new body_view();
  QVBoxLayout* top_layout=new QVBoxLayout(this);
  m_frame = new QFrame();
  m_frame->setFrameStyle(QFrame::Box | QFrame::Raised);
  m_frame->setLineWidth(0);
  m_frame->setMinimumSize(QSize(200,200));

//  m_frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  top_layout->addWidget(m_frame);
  QVBoxLayout* l =new QVBoxLayout(m_frame);

  l->setContentsMargins(0,0,0,0);
  l->setSpacing(0);
  l->addWidget(m_headersv);
  l->setStretchFactor(m_headersv, 0);

  l->addWidget(m_bodyv);
  l->setStretchFactor(m_bodyv, 1);

  this->setWidget(m_frame);	// for the scrollarea

  connect(m_bodyv, SIGNAL(wheel(QWheelEvent*)), this, SLOT(wheel_body(QWheelEvent*)));
  connect(m_bodyv, SIGNAL(loadFinished(bool)), this, SLOT(load_finished(bool)));
  connect(m_bodyv, SIGNAL(external_contents_requested()), this,
	  SLOT(ask_for_external_contents()));
  connect(m_bodyv, SIGNAL(key_space_pressed()), this, SLOT(page_down()));
  connect(m_bodyv, SIGNAL(popup_body_context_menu()), SIGNAL(popup_body_context_menu()));
  if (m_parent) {
    connect(m_bodyv->page(),
	    SIGNAL(linkHovered(const QString&,const QString, const QString&)),
	    m_parent, SLOT(show_status_message(const QString&)));
  }
  connect(m_headersv, SIGNAL(fetch_ext_contents()), this, SLOT(allow_external_contents()));
  connect(m_headersv, SIGNAL(to_text()), this, SLOT(show_text_part()));
  connect(m_headersv, SIGNAL(to_html()), this, SLOT(show_html_part()));
  connect(m_headersv, SIGNAL(complete_load_request()), this, SLOT(complete_body_load()));
  m_header_has_selection=false;
  connect(m_headersv, SIGNAL(copyAvailable(bool)), this, SLOT(headers_own_selection(bool)));
  connect(m_headersv, SIGNAL(selectionChanged()), this, SLOT(headers_selection_changed()));
  connect(m_headersv, SIGNAL(msg_display_request()), this, SIGNAL(on_demand_show_request()));
  connect(m_bodyv->page(), SIGNAL(selectionChanged()), this, SLOT(body_selection_changed()));
}

void
message_view::headers_own_selection(bool yes)
{
  m_header_has_selection=yes;
}

QSize
message_view::sizeHint() const
{
  return QSize(400,200);
}

message_view::~message_view()
{
}

static inline int max(int a, int b)
{
  return a>b?a:b;
}

void
message_view::resizeEvent(QResizeEvent* event)
{
  int xr=event->size().width();
  int yr=event->size().height();
  /* Resizing the frame triggers a resize of m_bodyv, which triggers rendering
     with the new width, which updates contentsSize() that we need to know
     the new height. Then we resize the frame to that height. */
  m_frame->resize(xr, yr);
  // pass an arbitrary height at this point (but non zero); the final
  // height will be known only after rendering with 'xr' width
  m_headersv->resize(xr, 100);

  int vp_height=m_bodyv->page()->mainFrame()->contentsSize().height() +
    m_headersv->height() + 2*m_frame->frameWidth();
  xr = m_bodyv->page()->mainFrame()->contentsSize().width();
  if (m_headersv->width() > xr) {
    xr = m_headersv->width();
  }
  m_frame->resize(xr, vp_height);

  QScrollArea::resizeEvent(event);
}

void
message_view::keyPressEvent(QKeyEvent* event)
{
  if (event->modifiers()==0 && event->key()==Qt::Key_Space) {
    page_down();
  }
  QScrollArea::keyPressEvent(event);
}

// this slot is to be connected to the body_view's wheelEvent
void
message_view::wheel_body(QWheelEvent* e)
{
  QScrollArea::wheelEvent(e);
}

void
message_view::load_finished(bool ok)
{
  if (ok) {
    DBG_PRINTF(5, "load_finished");
    m_bodyv->set_loaded(true);
    if (!m_highlight_words.empty()) {
      m_bodyv->highlight_terms(m_highlight_words);
    }
    resize_frame();
    int h = height();
    resize(width(),h-1);
    resize(width(),h);
  }
  else {
    DBG_PRINTF(2, "load_finished not OK");
  }
}

void
message_view::reset_state()
{
  m_has_text_part=false;
  m_has_html_part=false;
  m_bodyv->set_loaded(false);
  m_ext_contents=false;
  m_headersv->reset_commands();
  m_zoom_factor=1.0;
  m_bodyv->setTextSizeMultiplier(m_zoom_factor);
  m_highlight_words.clear();
}

void
message_view::resize_frame()
{
  int w=m_bodyv->page()->mainFrame()->contentsSize().width()+
    2*m_frame->frameWidth();
  int h=m_headersv->height()+m_bodyv->page()->mainFrame()->contentsSize().height()
    + 2*m_frame->frameWidth();
  m_frame->resize(w,h);
}

// content_type is 1 if contents come from text part, 2 if html part
void
message_view::set_html_contents(const QString& headers, const QString& html_body,
				int content_type)
{
  m_ext_contents=false;
  m_bodyv->set_loaded(false);
  /* header contents _must_ be set before displaying the body */
  m_headersv->set_contents(headers);

  if (m_parent) { // hack: do this only if inside a msg_list_window,
		  // for in other contexts, attachments capabilities are lacking
		  // to switch between alternate representations
    m_headersv->enable_command("to_text", content_type==2 && m_has_text_part);
    m_headersv->enable_command("to_html", content_type==1 && m_has_html_part);
  }
  m_content_type_shown = content_type;
  m_bodyv->display(html_body);
}

/* Return values:
   0: not yet computed
   1: plain text
   2: html
*/
int
message_view::content_type_shown() const
{
  return m_content_type_shown;
}

void
message_view::set_mail_item (mail_msg* p)
{
  m_pmsg=p;
  m_bodyv->set_mail_item(p);
}

void
message_view::enable_page_nav(bool back, bool forward)
{
  m_can_move_back=back;
  m_can_move_forward=forward;
}

void
message_view::highlight_terms(const std::list<searched_text>& lst)
{
  m_headersv->highlight_terms(lst);
  if (m_bodyv->is_loaded()) {
    m_bodyv->highlight_terms(lst);
  }
  else {
    m_highlight_words=lst;
  }
}

void
message_view::page_down()
{
  QScrollBar* bar = verticalScrollBar();
  if (bar) {
    bar->setValue(bar->value()+bar->pageStep());
  }
}

void
message_view::set_wrap(bool b)
{
  Q_UNUSED(b);
}

/* Replace the whole display by the "fetch on demand" href link or
   toggle back to normal display */
void
message_view::set_show_on_demand(bool b)
{
  DBG_PRINTF(3, "set_show_on_demand(%d)", (int)b);
  m_headersv->set_show_on_demand(b);
  if (b) {
    m_bodyv->clear();
    m_bodyv->page()->setViewportSize(QSize(m_headersv->width(), 10));
    m_bodyv->clear();
    update();
  }
  else {
    if (m_parent)
      display_body(m_parent->get_display_prefs(), 0);    
  }
}

void
message_view::print()
{
  QPrinter printer;
  QPrintDialog *dialog = new QPrintDialog(&printer, this);
  dialog->setWindowTitle(tr("Print Document"));
  if (dialog->exec() != QDialog::Accepted)
    return;

  delete dialog;
  m_pmsg->fetch_body_text(false);
  QString html_body = m_bodyv->page()->mainFrame()->toHtml();

  mail_displayer disp(this);
  body_view* printview = new body_view(NULL);
  printview->set_mail_item(m_bodyv->mail_item());
  printview->display(html_body);

  const display_prefs& prefs = m_parent->get_display_prefs();
  QString h = disp.sprint_headers(prefs.m_show_headers_level, m_pmsg);
  QString hmore = disp.sprint_additional_headers(prefs, m_pmsg);
  h.append(hmore);
  printview->prepend_body_fragment(h);

  printview->print(&printer);
  delete printview;
}

void
message_view::setFont(const QFont& font)
{
  m_headersv->setFont(font);
  QString s="body {";
  if (!font.family().isEmpty()) {
    s.append("font-family:\"" + font.family() + "\";");
  }
  if (font.pointSize()>1) {
    s.append(QString("font-size:%1 pt;").arg(font.pointSize()));
  }
  else if (font.pixelSize()>1) {
    s.append(QString("font-size:%1 px;").arg(font.pixelSize()));
  }
  if (font.bold()) {
    s.append("font-weight: bold;");
  }
  if (font.style()==QFont::StyleItalic) {
    s.append("font-style: italic;");
  }
  else if (font.style()==QFont::StyleOblique) {
    s.append("font-style: oblique;");
  }
  s.append("}\n");
  m_bodyv->set_body_style(s);
  //  m_bodyv->redisplay();
}

const QFont
message_view::font() const
{
  return m_headersv->font();
}

void
message_view::clear()
{
  m_headersv->clear();
  m_bodyv->clear();
}


void
message_view::copy()
{
  if (m_header_has_selection)
    m_headersv->copy();
  else {
    m_bodyv->page()->triggerAction(QWebPage::Copy);
  }
}

void
message_view::headers_selection_changed()
{
  /* body and headers are two widgets glued together to appear as a
     single text pane. That's why when a selection is made in one, we
     clear the selection in the other */
  m_bodyv->clear_selection();
}

void
message_view::body_selection_changed()
{
  /* body and headers are two widgets glued together to appear as a
     single text pane. That's why when a selection is made in one, we
     clear the selection in the other */
  m_headersv->clear_selection();
}

void
message_view::allow_external_contents()
{
  m_bodyv->authorize_external_contents(true);
  m_ext_contents=true;
  m_bodyv->redisplay();
  m_headersv->enable_command("fetch", false);
}

void
message_view::ask_for_external_contents()
{
  m_headersv->enable_command("fetch", true);
}

/*
  preferred_format: 0=from config, 1=text, 2=html
*/
void
message_view::display_body(const display_prefs& prefs, int preferred_format)
{
  if (!m_pmsg)
    return;

  if (preferred_format==0) {
    preferred_format = get_config().get_string("display/body/preferred_format").toLower()=="text" ? 1: 2;
  }

  reset_state();

  /* First we try to fetch html contents from body.bodyhtml
     If it's empty, we look for an HTML part (even if we won't load it later) */
  attachment* html_attachment;
  QString body_html = m_pmsg->get_body_html(); // from the body table
  if (body_html.isEmpty())
    html_attachment = m_pmsg->body_html_attached_part(); // from the attachments

  QString body_text = m_pmsg->get_body_text(true);

  attachments_list& attchs = m_pmsg->attachments();
  if (m_pmsg->has_attachments() > 0) {
    // Note: don't try to optimize here by removing the fetch: we need
    // it later for a lot of reasons, and the result is cached.
    attchs.fetch();
  }

  if (preferred_format==1) {
    if (body_text.isEmpty()) {
      if (attchs.size() > 0) {
	attachments_list::iterator iter;
	for (iter=attchs.begin(); iter!=attchs.end(); iter++) {
	  if (iter->filename().isEmpty() && iter->mime_type()=="text/plain") {
	    iter->append_decoded_contents(body_text);
	  }
	}
      }
    }
  }
  else {
    // preferred display format=html
    if (html_attachment && body_html.isEmpty()) {
      html_attachment->append_decoded_contents(body_html);
    }
  }

  // Format headers
  mail_displayer disp(this);
  QString h = disp.sprint_headers(prefs.m_show_headers_level, m_pmsg);
  QString hmore = disp.sprint_additional_headers(prefs, m_pmsg);
  h.append(hmore);

  set_wrap(prefs.m_wrap_lines);

  m_has_text_part = !body_text.isEmpty();
  m_has_html_part = (html_attachment!=NULL || !body_html.isEmpty());

  if (preferred_format==1 || body_html.isEmpty()) {
    QString b2 = disp.text_body_to_html(body_text, prefs);
    b2.prepend("<html>");
    b2.append("</html>");
    set_html_contents(h, b2, 1);
    // partial load?
    if (m_pmsg->body_length()>0 && m_pmsg->body_fetched_length() < m_pmsg->body_length()) {
      if (m_parent) {
	m_headersv->enable_command("complete_load", true);
	QString msg = QString(tr("Partial load (%1%)")).arg(m_pmsg->body_fetched_length()*100/ m_pmsg->body_length());
	m_parent->blip_status_message(msg);
      }
    }
  }
  else {
    set_html_contents(h, body_html, 2);
  }
}

void
message_view::show_text_part()
{
  if (m_parent)
    display_body(m_parent->get_display_prefs(), 1);
  m_headersv->enable_command("to_html", m_has_html_part);
  m_headersv->enable_command("to_text", false);
}

void
message_view::show_html_part()
{
  if (m_parent)
    display_body(m_parent->get_display_prefs(), 2);
  m_headersv->enable_command("to_text", m_has_text_part);
  m_headersv->enable_command("to_html", false);
}

void
message_view::change_zoom(int direction)
{
  if (direction>0)
    m_zoom_factor = m_zoom_factor*1.3;
  else if (direction<0)
    m_zoom_factor = m_zoom_factor/1.3;
  else
    m_zoom_factor=1.0;

  m_bodyv->setTextSizeMultiplier(m_zoom_factor);
  /* By setting a zero height, we force webkit to recompute the height
     instead of reusing the previous one, otherwise our outer frame's
     size wouldn't be adjusted */
  QSize s=m_bodyv->page()->viewportSize();
  s.setHeight(0);
  m_bodyv->page()->setViewportSize(s);
  resize_frame();
}

#if 0
void
message_view::set_partial_load(int percentage)
{
  Q_UNUSED(percentage);
  if (m_parent) {
    m_headersv->enable_command("complete_load", true);
  }
}
#endif

void
message_view::complete_body_load()
{
  m_pmsg->fetch_body_text(false);
  show_text_part();
}

QString
message_view::selected_text() const
{
  return m_bodyv->selectedText();
}

QString
message_view::selected_html_fragment()
{
  // TODO: check for the selection
#if QT_VERSION<0x040600
  QVariant res = m_bodyv->page()->mainFrame()->evaluateJavaScript("document.getElementsByTagName('body')[0].innerHTML");
  return res.toString();
#else
  QWebElement elt = m_bodyv->page()->mainFrame()->findFirstElement("body");
  if (!elt.isNull()) {
    return elt.toOuterXml();
  }
  else
    return "";
#endif
}

QString
message_view::body_as_text() const
{
  return m_bodyv->page()->mainFrame()->toPlainText();
}
