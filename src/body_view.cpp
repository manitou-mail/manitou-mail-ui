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

#include "main.h"
#include "body_view.h"
#include "browser.h"
#include "mailheader.h"
#include "attachment.h"
#include "db.h"


#include <QDebug>
#include <QTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QFrame>
#include <QWebPage>
#include <QWebFrame>
#include <QTimer>
#include <QScrollArea>

body_view::body_view(QWidget* parent) : QWebView(parent)
{
  m_pmsg=NULL;
  m_loaded=false;
  connect(this, SIGNAL(linkClicked(const QUrl&)), SLOT(link_clicked(const QUrl&)));

  page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  m_netman = new network_manager(this);
  page()->setNetworkAccessManager(m_netman);
  connect(m_netman, SIGNAL(external_contents_requested()), this,
	  SLOT(ask_perm_for_contents()));
//  setMinimumSize(QSize(200,200));
  page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
  page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

  QWebSettings* settings=page()->settings();
  settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
  settings->setAttribute(QWebSettings::JavascriptEnabled, false);
  settings->setAttribute(QWebSettings::JavaEnabled, false);
  settings->setAttribute(QWebSettings::PluginsEnabled, false);
  settings->setAttribute(QWebSettings::LinksIncludedInFocusChain, false);
  
  set_body_style("");
}

body_view::~body_view()
{
  delete m_netman;
}

void
body_view::wheelEvent(QWheelEvent* e)
{
  /* As of Qt-4.5, our QWebView instance won't handle this event so as
     a workaround we emit it for the larger scrollarea to handle
     it. */
  emit wheel(e);
}

void
body_view::set_mail_item(mail_msg* p)
{
  m_pmsg=p;
  m_netman->m_pmsg=p;
}

QSize
body_view::sizeHint() const
{
  return QSize(400,200);
}

void
body_view::display(const QString& html_body)
{
  m_netman->m_ext_download_permission_asked=false;
  m_netman->m_ext_download_permitted=false;
  m_html_text = html_body;
  m_loaded=false;
  setHtml(html_body, QUrl("/"));
}

void
body_view::force_style_sheet()
{
  /* we use a different dummy URL each time to avoid webkit cache
     (and we don't use QWebSettings::setObjectCacheCapacities since it appears
     to have adverse side effects) */
  static int counter;
  page()->settings()->setUserStyleSheetUrl(QString("style://manitou-%1").arg(++counter));
}

void
body_view::set_body_style(const QString& s)
{
  m_netman->m_body_style=s;
  m_netman->m_body_style.append("span.searchword-manitou { background-color: yellow;}");
  force_style_sheet();
}

void
body_view::redisplay()
{
  m_loaded=false;
  setHtml(m_html_text, QUrl("/"));
}

void
body_view::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  page()->mainFrame()->render(&painter, event->region());
}

void
body_view::resizeEvent(QResizeEvent* e)
{
  if (page()!=NULL) {
    page()->setViewportSize(e->size());
  }
}

void
body_view::copy()
{
  triggerPageAction(QWebPage::Copy);
}

void
body_view::set_wrap(bool on)
{
  Q_UNUSED(on);
  // for text parts, we'll probably need to implement wrapmode by HTML-styling the contents
#ifndef TODO_WEBKIT
  if (on) {
    setWordWrapMode(QTextOption::WordWrap);
    setLineWrapMode(QTextEdit::WidgetWidth);
  }
  else {
    setLineWrapMode(QTextEdit::NoWrap);
    setWordWrapMode(QTextOption::NoWrap);
  }
#endif
}


// private slot
void
body_view::link_clicked(const QUrl& url)
{
  if (url.scheme()=="mailto") {
    // TODO: use more headers, particularly "body"
    mail_header header;
    if (!url.path().isEmpty())
      header.m_to=url.path();
    if (url.hasQueryItem("subject")) {
      header.m_subject=url.queryItemValue("subject");
    }
    gl_pApplication->start_new_mail(header);
  }
  else {
    browser::open_url(url);
  }
}

// TODO: reflect that on the parent scrollarea
void
body_view::focusInEvent(QFocusEvent* e)
{
#ifndef TODO_WEBKIT
  setFrameStyle(QFrame::WinPanel+QFrame::Plain);
#endif
  QWebView::focusInEvent(e);
}

// TODO: reflect that on the parent scrollarea
void
body_view::focusOutEvent(QFocusEvent* e)
{
#ifndef TODO_WEBKIT
  setFrameStyle(QFrame::WinPanel+QFrame::Sunken);
#endif
  QWebView::focusOutEvent(e);
}

//static
void
body_view::rich_to_plain(QString& s)
{
  QTextEdit tmp; 
  /* hack: remove <br /> or the conversion to plaintext will insert
     question marks at line break positions. <p> on the other hand
     is correctly replaced by '\n' */
  s.replace("<br />", "<p>");
  tmp.setText(s);
  s = tmp.toPlainText();
}

void
body_view::contextMenuEvent(QContextMenuEvent* e)
{
  Q_UNUSED(e);
  emit popup_body_context_menu();
}

void
body_view::keyPressEvent(QKeyEvent* e)
{
  if (e->modifiers()==0 && e->key()==Qt::Key_Space) {
    emit key_space_pressed();
  }
  QWebView::keyPressEvent(e);
}


void
body_view::clear()
{
  setHtml("<html><body></body></html>");
}

void
body_view::clear_selection()
{
  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  page()->mainFrame()->evaluateJavaScript("document.execCommand('unselect');");
  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void
body_view::prepend_body_fragment(const QString& fragment)
{
  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  QString s = fragment;
  s.replace("'", "\\'");
  QString js = QString("try {var b=document.getElementsByTagName('body')[0]; var p=document.createElement('div'); p.innerHTML='%1'; p.style.marginBottom='12px'; b.insertBefore(p, b.firstChild); 1;} catch(e) { e; }").arg(s);
  QVariant v = page()->mainFrame()->evaluateJavaScript(js);
  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void
body_view::highlight_terms(const std::list<searched_text>& lst)
{
  std::list<searched_text>::const_iterator it = lst.begin();
  if (it==lst.end())
    return;

  // JS code based on http://www.kryogenix.org/code/browser/searchhi/searchhi.js
  const char* jscript="searchhi = {"
"  highlightWord: function(node,word) {"
"    if (node.hasChildNodes) {"
"	    for (var hi_cn=0;hi_cn<node.childNodes.length;hi_cn++) {"
"		    searchhi.highlightWord(node.childNodes[hi_cn],word);"
"	    }"
"    }"
"    if (node.nodeType == 3) {"
"	    tempNodeVal = node.nodeValue.toLowerCase();"
"	    tempWordVal = word.toLowerCase();"
"	    if (tempNodeVal.indexOf(tempWordVal) != -1) {"
"		var pn = node.parentNode;"
"	        if (pn.className != 'searchword-manitou') {"
"		    var nv = node.nodeValue;"
"		    var ni = tempNodeVal.indexOf(tempWordVal);"
"		    var before = document.createTextNode(nv.substr(0,ni));"
"		    var docWordVal = nv.substr(ni,word.length);"
"		    var after = document.createTextNode(nv.substr(ni+word.length));"
"		    var hiwordtext = document.createTextNode(docWordVal);"
"		    var hiword = document.createElement('span');"
"		    hiword.className = 'searchword-manitou';"
"		    hiword.appendChild(hiwordtext);"
"		    pn.insertBefore(before,node);"
"		    pn.insertBefore(hiword,node);"
"		    pn.insertBefore(after,node);"
"		    pn.removeChild(node);"
"	        }"
"	    }"
"      }"
" return 1;"
"  }"
"};"
    ;

  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  QVariant v=page()->mainFrame()->evaluateJavaScript(jscript);

  // TODO: be case-sensitive if it->m_is_cs is set
  while (it != lst.end()) {
    QString s = it->m_text;
    s.replace("'", "\\'");
    QString jsearch = QString("searchhi.highlightWord(document.getElementsByTagName('body')[0],'%1');").arg(s);
    v=page()->mainFrame()->evaluateJavaScript(jsearch);
    ++it;
  }
  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void
body_view::authorize_external_contents(bool b)
{
  m_netman->m_ext_download_permitted=b;
}

void
body_view::ask_perm_for_contents()
{
  // transmit signal upwards
  emit external_contents_requested();
}


internal_style_network_reply::internal_style_network_reply(const QNetworkRequest& req, const QString& style, QObject* parent) : QNetworkReply(parent)
{
  setRequest(req);
  setOperation(QNetworkAccessManager::GetOperation);
  m_buf.setData(style.toLocal8Bit());
  open(QIODevice::ReadOnly);
  m_buf.open(QIODevice::ReadOnly);
  QTimer::singleShot(0, this, SLOT(go()));
}

qint64
internal_style_network_reply::readData(char* data, qint64 size)
{
  qint64 r=m_buf.read(data, size);
  if (r>0) {
    data[r]=0;
  }
  if (r<=0)
    emit finished();
  else
    emit readyRead();
  return r;
}

qint64
internal_style_network_reply::bytesAvailable() const
{
  return m_buf.size()+QNetworkReply::bytesAvailable();
}

void
internal_style_network_reply::abort()
{
}

void
internal_style_network_reply::go()
{
  setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200); 
  setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, "OK");
  emit metaDataChanged(); 
  emit readyRead();
}

#if 0
class no_content_network_reply : public QNetworkReply
{
public:
  no_content_network_reply(const QNetworkRequest& req, QObject* parent) : QNetworkReply(parent) {
    setRequest(req);
    setOperation(QNetworkAccessManager::GetOperation);
    //emit error(QNetworkReply::ContentOperationNotPermittedError);
    emit error(QNetworkReply::ContentNotFoundError);
    emit finished();
  }
  void abort() {}
  qint64 readData(char* data, qint64 size) {
    Q_UNUSED(data);
    Q_UNUSED(size);
    return 0;
  }
};
#endif

QNetworkReply*
network_manager::empty_network_reply(Operation op, const QNetworkRequest& req)
{
  //  Q_UNUSED(op);
#if 0
  return new no_content_network_reply(req, this);
#else
  //    qDebug() << "op rejected for " << req.url().toString();
  QNetworkRequest req2 = req;
  req2.setUrl(QUrl());	// empty URL to avoid accessing external contents
  return QNetworkAccessManager::createRequest(op, req2, NULL);
#endif
}

/* outgoingData is always 0 for Get and Head requests */
QNetworkReply*
network_manager::createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData)
{
  DBG_PRINTF(5, "createRequest for %s", req.url().toString().toLocal8Bit().constData());
  if (op!=GetOperation) {
    // only GET is currently supported, see if HEAD should be
    return empty_network_reply(op, req);
  }

  // the request refers to attached contents
  if (req.url().scheme() == "cid") {
    // refers to a MIME part that should be attached
    if (m_pmsg) {
      attachment* a = m_pmsg->attachments().get_by_content_id(req.url().path());
      if (a!=NULL) {
	attachment_network_reply* reply = a->network_reply(req, this);
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(download_error(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
		this, SLOT(download_progress(qint64,qint64)));
	connect(reply, SIGNAL(finished()), this, SLOT(download_finished()));
	return reply;
      }
      
    }
    return empty_network_reply(op, req);
  }
  else if (req.url().scheme()=="style") { // internal scheme for styling contents
    return new internal_style_network_reply(req, m_body_style, this);
  }
  // the request refers to external contents
  if (m_ext_download_permitted) {
    //        qDebug() << "op accepted for " << req.url().toString();
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
  }
  else {
    if (!m_ext_download_permission_asked) {
      // let know that contents were skipped so that the user can be
      // presented with the choice to fetch them or not
      emit external_contents_requested();
      m_ext_download_permission_asked=true;
    }
    return empty_network_reply(op, req);
  }
}

void
network_manager::download_finished()
{
  DBG_PRINTF(5, "download_finished\n");
}

void
network_manager::download_error(QNetworkReply::NetworkError err)
{
  DBG_PRINTF(2, "download_error: %d\n", (int)err);
}
void
network_manager::download_progress(qint64 received, qint64 total)
{
  DBG_PRINTF(5, "download_progress(%ld / %ld)\n", received, total);
}
