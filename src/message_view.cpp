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

#include "db.h"
#include "main.h"

#include "body_view.h"
#include "browser.h"
#include "mail_displayer.h"
#include "message_view.h"
#include "msg_list_window.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMessageBox>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShortcut>
#include <QTextDocument>
#include <QVariant>
#include <QWebFrame>

#if QT_VERSION>=0x040600
#include <QWebElement>
#endif

message_view::message_view(QWidget* parent, msg_list_window* sub_parent) : QWidget(parent)
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
  m_bodyv = new body_view();
  QVBoxLayout* top_layout=new QVBoxLayout(this);
  top_layout->setContentsMargins(0,0,0,0);
  top_layout->setSpacing(0);

  top_layout->addWidget(m_bodyv);

  connect(m_bodyv, SIGNAL(loadFinished(bool)), this, SLOT(load_finished(bool)));
  connect(m_bodyv, SIGNAL(external_contents_requested()), this,
	  SLOT(ask_for_external_contents()));
  connect(m_bodyv, SIGNAL(popup_body_context_menu()), SIGNAL(popup_body_context_menu()));
  if (m_parent) {
    connect(m_bodyv->page(),
	    SIGNAL(linkHovered(const QString&,const QString, const QString&)),
	    this, SLOT(display_link(const QString&)));
  }
  connect(m_bodyv, SIGNAL(linkClicked(const QUrl&)), this, SLOT(link_clicked(const QUrl&)));

  QShortcut* t = new QShortcut(QKeySequence(tr("Ctrl+A", "Select all")), this);
  t->setContext(Qt::WidgetWithChildrenShortcut);
  connect(t, SIGNAL(activated()), this, SLOT(select_all_text()));
}


void
message_view::select_all_text()
{
  m_bodyv->page()->triggerAction(QWebPage::SelectAll);
}


void
message_view::display_link(const QString& link)
{
  if (m_parent && link.indexOf("#manitou-")!=0) {
    m_parent->show_status_message(link);
    m_hovered_link = link;
  }
}

void
message_view::copy_link_clipboard()
{
  m_bodyv->page()->triggerAction(QWebPage::CopyLinkToClipboard);
}

void
message_view::link_clicked(const QUrl& url)
{
  if (url.scheme()=="mailto") {
#if QT_VERSION<0x050000 // later
    // TODO: use more headers, particularly "body"
    mail_header header;
    if (!url.path().isEmpty())
      header.m_to=url.path();
    if (url.hasQueryItem("subject")) {
      header.m_subject=url.queryItemValue("subject");
    }
    gl_pApplication->start_new_mail(header);
#endif
  }
  else if (url.scheme().isEmpty()) {
    const QString cmd=url.toString();
    if (cmd=="#manitou-fetch") {
      allow_external_contents();
    }
    else if (cmd=="#manitou-to_text") {
      show_text_part();
    }
    else if (cmd=="#manitou-to_html") {
      show_html_part();
    }
    else if (cmd=="#manitou-show") {
      emit on_demand_show_request();
      display_commands();
    }
    else if (cmd=="#manitou-complete_load") {
      complete_body_load();
      enable_command("complete_load", false);
      display_commands();
    }    
  }
  else {
    browser::open_url(url);
  }
}

QSize
message_view::sizeHint() const
{
  return QSize(400,200);
}

message_view::~message_view()
{
}

void
message_view::keyPressEvent(QKeyEvent* event)
{
  if (event->modifiers()==0 && event->key()==Qt::Key_Space) {
    page_down();
  }
  QWidget::keyPressEvent(event);
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
  m_zoom_factor=1.0;
  m_bodyv->setTextSizeMultiplier(m_zoom_factor);
}

// content_type is 1 if contents come from text part, 2 if html part
void
message_view::set_html_contents(const QString& html_body, int content_type)
{
  m_ext_contents=false;
  m_bodyv->set_loaded(false);
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
message_view::highlight_terms(const QList<searched_text>& lst)
{
  m_highlight_words = lst;
  if (m_bodyv->is_loaded())
    m_bodyv->highlight_terms(lst);
}

void
message_view::page_down()
{
  QWebFrame* frame = m_bodyv->page()->mainFrame();
  if (!frame)
    return;
  QPoint pos = frame->scrollPosition();
  int y = m_bodyv->geometry().height() + pos.y();
  if (y > frame->contentsSize().height()-frame->geometry().height())
    y = frame->contentsSize().height()-frame->geometry().height();
  pos.setY(y);
  frame->setScrollPosition(pos);
}

/* Replace the whole display by the "fetch on demand" href link or
   toggle back to normal display */
void
message_view::set_show_on_demand(bool b)
{
  DBG_PRINTF(4, "set_show_on_demand(%d)", (int)b);
  if (b) {
    m_bodyv->clear();
    update();
  }
  else {
    if (m_parent)
      display_body(m_parent->get_display_prefs(), default_conf);
  }
}

void
message_view::print()
{
  QPrinter printer;
  QPrintDialog dialog(&printer, this);
  dialog.setWindowTitle(tr("Print message"));
  if (dialog.exec() == QDialog::Accepted)
    m_bodyv->print(&printer);
}

void
message_view::setFont(const QFont& font)
{
  QWidget::setFont(font);
  m_bodyv->set_font(font);
}

void
message_view::clear()
{
  m_bodyv->clear();
}


void
message_view::copy()
{
  m_bodyv->page()->triggerAction(QWebPage::Copy);
}


void
message_view::allow_external_contents()
{
  m_bodyv->authorize_external_contents(true);
  m_ext_contents=true;
  enable_command("fetch", false);
  QString html_body = m_bodyv->page()->mainFrame()->toHtml();
  m_bodyv->page()->mainFrame()->setHtml(html_body, QUrl("/"));
  display_commands();
}

void
message_view::ask_for_external_contents()
{
  enable_command("fetch", true);
}

void
message_view::display_body(const display_prefs& prefs, display_part preferred_part)
{
  if (!m_pmsg)
    return;

  if (preferred_part == default_conf) {
    preferred_part = get_config().get_string("display/body/preferred_format").toLower()=="text" ? text_part: html_part;
  }

  reset_state();
  reset_commands();

  /* First we try to fetch html contents from body.bodyhtml
     If it's empty, we look for an HTML part (even if we won't load it later) */
  attachment* html_attachment=NULL;
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

  if (preferred_part == text_part) {
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
  QString h = QString("<div id=\"manitou-header\">%1%2%3</div><p>")
    .arg(disp.sprint_headers(prefs.m_show_headers_level, m_pmsg))
    .arg(disp.sprint_additional_headers(prefs, m_pmsg))
    .arg(QString("<div id=\"manitou-commands\"></div>"));

  m_has_text_part = !body_text.isEmpty();
  m_has_html_part = (html_attachment!=NULL || !body_html.isEmpty());

  if (preferred_part==text_part || body_html.isEmpty()) {
    QString b2 = "<html><body>";
    b2.append(h);
    b2.append("<div id=\"manitou-body\">");
    b2.append(disp.text_body_to_html(body_text, prefs));
    b2.append("</div>");
    b2.append("</body></html>");

    set_html_contents(b2, 1);
    // partial load?
    if (m_pmsg->body_length()>0 && m_pmsg->body_fetched_length() < m_pmsg->body_length()) {
      if (m_parent) {
	enable_command("complete_load", true);
	QString msg = QString(tr("Partial load (%1%)")).arg(m_pmsg->body_fetched_length()*100/ m_pmsg->body_length());
	m_parent->blip_status_message(msg);
      }
    }
    if (m_parent) {
      enable_command("to_html", m_has_html_part);
    }
  }
  else {
    body_html.prepend("<div id=\"manitou-body\">");
    body_html.append("</div>");
    set_html_contents(body_html, 2);
    prepend_body_fragment(h);
    if (m_parent)
      enable_command("to_text", m_has_text_part);
  }
  display_commands();
}

void
message_view::prepend_body_fragment(const QString& fragment)
{
  QWebPage* page = m_bodyv->page();
  page->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  QString js = QString("try {var b=document.getElementsByTagName('body')[0]; var p=document.createElement('div'); p.innerHTML=\"%1\"; b.insertBefore(p, b.firstChild); 1;} catch(e) { e; }").arg(escape_js_string(fragment));
  QVariant v = page->mainFrame()->evaluateJavaScript(js);
  page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

QString
message_view::escape_js_string(const QString src)
{
  QString res;  
  res.reserve(src.length()*2);
  for (int i=0; i<src.length(); i++) {
    ushort code=src[i].unicode();
    if (code <= 127) {
      switch(code) {
      case (ushort)'\b':
	res.append(QLatin1String("\\b"));
	break;
      case (ushort)'\f':
	res.append(QLatin1String("\\f"));
	break;
      case (ushort)'\t':
	res.append(QLatin1String("\\t"));
	break;
      case (ushort)'\n':
	res.append(QLatin1String("\\n"));
	break;
      case (ushort)'\r':
	res.append(QLatin1String("\\r"));
	break;
      case (ushort)'"':
	res.append(QLatin1String("\\\""));
	break;
      case (ushort)'\\':
	res.append(QLatin1String("\\\\"));
	break;
      default:
	res.append(src[i]);
	break;
      }
    }
    else
      res.append(src[i]);
  }
  return res;
}

void
message_view::reset_commands()
{
  m_enabled_commands.clear();
}

void
message_view::display_commands()
{
  QWebPage* page = m_bodyv->page();
  page->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  QString s = command_links();
  s.replace("'", "\\'");
  QString js = QString("try {var p=document.getElementById(\"manitou-commands\"); p.innerHTML='%1'; 1;} catch(e) { e; }").arg(s);
  QVariant v = page->mainFrame()->evaluateJavaScript(js);
  page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void
message_view::enable_command(const QString command, bool enable)
{
  m_enabled_commands.insert(command, enable);
/*  display_commands();*/
}

QString
message_view::command_links()
{
  static const char* commands[] = {
    "fetch", QT_TR_NOOP("Fetch external contents"),
    "to_text", QT_TR_NOOP("Show text part"),
    "to_html", QT_TR_NOOP("Show HTML part"),
    "complete_load", QT_TR_NOOP("Complete load")
  };

  QString line;
  for (uint i=0; i<sizeof(commands)/sizeof(commands[0]); i+=2) {
    if (m_enabled_commands.contains(commands[i])) {
      const QString cmd=commands[i];
      if (m_enabled_commands.value(cmd))
	line.append(QString(" &nbsp;&nbsp;&nbsp;<a id=\"%1\" href=\"#manitou-%1\">%2</a>").arg(cmd).arg(tr(commands[i+1])));
    }
  }
  return line;
}

void
message_view::show_text_part()
{
  if (m_parent)
    display_body(m_parent->get_display_prefs(), text_part);
  enable_command("to_html", m_has_html_part);
  enable_command("to_text", false);
  m_displayed_part = message_view::text_part;
  display_commands();
}

void
message_view::show_html_part()
{
  if (m_parent)
    display_body(m_parent->get_display_prefs(), html_part);
  enable_command("to_text", m_has_text_part);
  enable_command("to_html", false);
  m_displayed_part = message_view::html_part;
  display_commands();
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
}

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
  QVariant res = m_bodyv->page()->mainFrame()->evaluateJavaScript("document.getElementsById('manitou-body').innerHTML");
  return res.toString();
#else
  QWebElement elt = m_bodyv->page()->mainFrame()->findFirstElement("div#manitou-body");
  if (!elt.isNull()) {
    return elt.toOuterXml();
  }
  else
    return "";
#endif
}

QString
message_view::body_as_text()
{
#if QT_VERSION<0x040600
  QVariant res = m_bodyv->page()->mainFrame()->evaluateJavaScript("document.getElementsById('manitou-body').innerHTML");
  return res.toString();
#else
  QWebElement elt = m_bodyv->page()->mainFrame()->findFirstElement("div#manitou-body");
  if (!elt.isNull()) {
    return elt.toPlainText();
  }
  else
    return "";
#endif
}
