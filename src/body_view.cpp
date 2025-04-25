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

#include "attachment.h"
#include "body_view.h"
#include "db.h"
#include "mailheader.h"
#include "main.h"
#include "xface/xface.h"

#include <QKeyEvent>
#include <QPainter>
#include <QTextEdit>
#include <QTimer>
#include <QUrlQuery>
#include <QWebFrame>
#include <QWebPage>

body_view::body_view(QWidget* parent) : QWebView(parent)
{
  m_pmsg=NULL;
  m_loaded=false;

  page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  m_netman = new network_manager(this);
  page()->setNetworkAccessManager(m_netman);
  connect(m_netman, SIGNAL(external_contents_requested()), this,
	  SLOT(ask_perm_for_contents()));

  QWebSettings* settings=page()->settings();
  settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
  settings->setAttribute(QWebSettings::JavascriptEnabled, false);
  settings->setAttribute(QWebSettings::JavaEnabled, false);
  settings->setAttribute(QWebSettings::PluginsEnabled, false);
  settings->setAttribute(QWebSettings::LinksIncludedInFocusChain, false);
  
  set_body_style();
}

body_view::~body_view()
{
  delete m_netman;
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
#if QT_VERSION<0x040600
  /* we use a different dummy URL each time to avoid webkit cache
     (and we don't use QWebSettings::setObjectCacheCapacities since it appears
     to have adverse side effects) */
  static int counter;
  
  page()->settings()->setUserStyleSheetUrl(QString("style://manitou-%1").arg(++counter));
#endif
}

void
body_view::set_body_style()
{
  static const char* default_style =
    "span.searchword-manitou { background-color:yellow;}\n";
  QString body_style=QString("body { margin: 5px; padding: 0px; background-color: #ffffff; %1}\n").arg(m_font_css);
  QString header_style=QString("div#manitou-header { text-align:left; color:#000000; background-color: #ffffff; %1}\n").arg(m_font_css);
#if QT_VERSION<0x040600
  m_netman->m_body_style=default_style + body_style + header_style;
  force_style_sheet();
#else
  QString style = default_style + body_style + header_style;
  QByteArray b = style.toLatin1().toBase64();
  page()->settings()->setUserStyleSheetUrl(QUrl(QString("data:text/css;charset=utf-8;base64,")+QString(b)));  
#endif
}

void
body_view::set_font(const QFont& font)
{
  QString s;
  if (!font.family().isEmpty()) {
    s.append("font-family:\"" + font.family() + "\";");
  }
  if (font.pointSize()>1) {
    s.append(QString("font-size:%1pt;").arg(font.pointSize()));
  }
  else if (font.pixelSize()>1) {
    s.append(QString("font-size:%1px;").arg(font.pixelSize()));
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
  m_font_css=s;
  set_body_style();
}

void
body_view::redisplay()
{
  m_loaded=false;
  setHtml(m_html_text, QUrl("/"));
}

void
body_view::copy()
{
  triggerPageAction(QWebPage::Copy);
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
  QString js = QString("try {var b=document.getElementsByTagName('body')[0]; var p=document.createElement('div'); p.innerHTML='%1'; p.id='manitou-header'; b.insertBefore(p, b.firstChild); body.style.cssText=''; 1;} catch(e) { e; }").arg(s);
  QVariant v = page()->mainFrame()->evaluateJavaScript(js);
  page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void
body_view::highlight_terms(const QList<searched_text>& lst)
{
  QList<searched_text>::const_iterator it = lst.begin();
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


internal_img_network_reply::internal_img_network_reply(const QNetworkRequest& req, const QString& encoded_img, int type, QObject* parent) : QNetworkReply(parent)
{
  /* internal_img_network_reply is modeled after:
     http://qt.gitorious.org/qt-labs/graphics-dojo/blobs/master/url-rendering/main.cpp
  */
  position=0;
  setRequest(req);
  if (type==1) { // Face
    m_buffer = QByteArray::fromBase64(encoded_img.toLatin1().constData());
  }
  else { // X-Face
    QImage qi;
    QString s;
    xface_to_xpm(encoded_img.toLatin1().constData(), s);
    if (qi.loadFromData((const uchar*)s.toLatin1().constData(), s.length(), "XPM")) {
      QBuffer b(&m_buffer);
      qi.save(&b, "PNG");
    }
  }
  setOperation(QNetworkAccessManager::GetOperation);
  setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
  setHeader(QNetworkRequest::ContentLengthHeader, m_buffer.size());
  open(ReadOnly);
  setUrl(req.url());
  QTimer::singleShot(0, this, SLOT(go()));
}

qint64
internal_img_network_reply::readData(char* data, qint64 size)
{
  qint64 r=qMin(size, (qint64)(m_buffer.size()-position));
  memcpy(data, m_buffer.constData()+position, r);
  position += r;
  return r;
}

qint64
internal_img_network_reply::bytesAvailable() const
{
  return m_buffer.size() - position + QNetworkReply::bytesAvailable();
}

bool
internal_img_network_reply::seek(qint64 pos)
{
  if (pos<0 || pos>=m_buffer.size())
    return false;
  position=pos;
  return true;
}

void
internal_img_network_reply::go()
{
  position=0;
  emit readyRead();
  emit finished();
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


QNetworkReply*
network_manager::empty_network_reply(Operation op, const QNetworkRequest& req)
{
  QNetworkRequest req2 = req;
  req2.setUrl(QUrl());	// empty URL to avoid accessing external contents
  return QNetworkAccessManager::createRequest(op, req2, NULL);
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
  const QUrl& url = req.url();
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
  else if (url.scheme()=="manitou" && (url.authority()=="xface" || url.authority()=="face")) {
    if (url.hasQuery()) {
      QUrlQuery qq(url);
      if (qq.hasQueryItem("id") && qq.hasQueryItem("o")) {
	QString headers = m_pmsg->get_headers();
	bool id_ok, o_ok;
	mail_id_t id = mail_msg::id_from_string(qq.queryItemValue("id"), &id_ok);
	int offset = qq.queryItemValue("o").toInt(&o_ok);
	if (id_ok && o_ok && id == m_pmsg->get_id()) {
	  int lf_pos = headers.indexOf('\n', offset);
	  QString ascii_line;
	  if (lf_pos>0) {
	    ascii_line = headers.mid(offset, lf_pos-offset);
	  }
	  else {
	    ascii_line = headers.mid(offset);
	  }
	  ascii_line.replace(" ", "");
	  int type = url.authority()=="face" ? 1:2;
	  return new internal_img_network_reply(req, ascii_line, type, this);
	}
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
