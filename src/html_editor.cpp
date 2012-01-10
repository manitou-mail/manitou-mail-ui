/* Copyright (C) 2004-2011 Daniel Verite

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

#include "html_editor.h"

#include <QToolBar>
#include <QToolButton>
#include <QDebug>
#include <QVariant>
#include <QWebFrame>
#include <QAction>
#include <QLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QFile>
#include <QColorDialog>
#include <QMessageBox>
#include <QFontDatabase>
#include <QComboBox>
#include <QApplication>
#include <QDesktopServices>
#include <QDropEvent>

#ifdef MANITOU_DATADIR
#include "icons.h"
#else
 #ifndef HTML_ICON
   #define HTML_ICON(a) QIcon(a)
 #endif
#endif

struct html_editor::action_type
html_editor::m_action_definitions[] = {
#define NA QWebPage::NoWebAction

  // {name, shortcut, icon, WebAction
  {QT_TR_NOOP("Bold"), QT_TR_NOOP("Ctrl+B"), "icon-bold.png", SLOT(bold()), NA }
  ,{QT_TR_NOOP("Italic"), QT_TR_NOOP("Ctrl+I"), "icon-italic.png", SLOT(italic()), NA }
  ,{QT_TR_NOOP("Underline"), QT_TR_NOOP("Ctrl+U"), "icon-underline.png", SLOT(underline()), NA }
  ,{QT_TR_NOOP("Strike through"), QT_TR_NOOP("Ctrl+S"), "icon-strikethrough.png", SLOT(strikethrough()) , NA}
  ,{QT_TR_NOOP("Superscript"), NULL, "icon-superscript.png", SLOT(superscript()), NA }
  ,{QT_TR_NOOP("Subscript"), NULL, "icon-subscript.png", SLOT(subscript()), NA }

  // separator before 6
  ,{QT_TR_NOOP("Set foreground color"), NULL, "icon-foreground-color.png", SLOT(foreground_color()), NA }
  ,{QT_TR_NOOP("Set background color"), NULL, "icon-background-color.png", SLOT(background_color()), NA }
  ,{QT_TR_NOOP("Insert image"), NULL, "icon-image.png", SLOT(insert_image()), NA }
  ,{QT_TR_NOOP("Insert link"), NULL, "icon-link.png", SLOT(insert_link()), NA }
  ,{QT_TR_NOOP("Insert horizontal rule"), NULL, "icon-hr.png", SLOT(insert_hr()), NA }
  ,{QT_TR_NOOP("Insert unordered list"), NULL, "icon-bullet-list.png", SLOT(insert_unordered_list()), NA }
  ,{QT_TR_NOOP("Insert ordered list"), NULL, "icon-ordered-list.png", SLOT(insert_ordered_list()), NA }
  ,{QT_TR_NOOP("Remove format"), NULL, "icon-eraser.png", SLOT(remove_format()), NA }
  ,{QT_TR_NOOP("Cut"), NULL, "icon-cut.png", NULL, QWebPage::Cut }
  ,{QT_TR_NOOP("Copy"), NULL, "icon-copy.png", NULL, QWebPage::Copy }
  ,{QT_TR_NOOP("Paste"), NULL, "icon-paste.png", NULL, QWebPage::Paste }
  ,{QT_TR_NOOP("Undo"), QT_TR_NOOP("Ctrl+Z"), "icon-undo.png", NULL, QWebPage::Undo }
  ,{QT_TR_NOOP("Redo"), NULL, "icon-redo.png", NULL, QWebPage::Redo }
  ,{QT_TR_NOOP("Align left"), NULL, "icon-align-left.png", SLOT(align_left()), NA }
  ,{QT_TR_NOOP("Justify"), NULL, "icon-justify.png", SLOT(justify()), NA }
  ,{QT_TR_NOOP("Align right"), NULL, "icon-align-right.png", SLOT(align_right()), NA }
  ,{QT_TR_NOOP("Center"), NULL, "icon-center.png", SLOT(center()), NA }
  ,{QT_TR_NOOP("Indent right"), NULL, "icon-indent-right.png", SLOT(indent_right()), NA }
  ,{QT_TR_NOOP("Indent left"), NULL, "icon-indent-left.png", SLOT(indent_left()), NA }

#undef NA
};

link_editor::link_editor(QWidget* parent): QDialog(parent)
{
  setWindowTitle(tr("Edit link"));
  QVBoxLayout* layout = new QVBoxLayout(this);
  QGridLayout* grid = new QGridLayout();
  layout->addLayout(grid);

  m_link = new QLineEdit();
  m_text = new QLineEdit();
  m_link->setText("http://");

  grid->addWidget(new QLabel(tr("Link:")), 0, 0);
  grid->addWidget(m_link, 0, 1);

  grid->addWidget(new QLabel(tr("Text:")), 1, 0);
  grid->addWidget(m_text, 1, 1);

  QHBoxLayout* buttons_layout = new QHBoxLayout();
  layout->addLayout(buttons_layout);

  QPushButton* preview = new QPushButton(tr("Preview"));
  buttons_layout->addWidget(preview);
  connect(preview, SIGNAL(clicked()), SLOT(preview()));
  buttons_layout->addStretch(3);

  QPushButton* wok = new QPushButton(tr("OK"));
  buttons_layout->addWidget(wok);
  connect(wok, SIGNAL(clicked()), SLOT(accept()));
  wok->setDefault(true);
  buttons_layout->addStretch(2);

  QPushButton* wcancel = new QPushButton(tr("Cancel"));
  buttons_layout->addWidget(wcancel);
  connect(wcancel, SIGNAL(clicked()), SLOT(reject()));
}

link_editor::~link_editor()
{
}

void
link_editor::preview()
{
  if (!url().isEmpty()) {
    QDesktopServices::openUrl(QUrl(url(), QUrl::TolerantMode));
  }
}

QString
link_editor::url()
{
  return m_link->text();
}

QString
link_editor::text()
{
  return m_text->text();
}

html_editor::html_editor(QWidget* parent): QWebView(parent)
{
  create_actions();
  connect(this, SIGNAL(loadFinished(bool)), SLOT(load_finished(bool)));
  page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  connect(this, SIGNAL(linkClicked(const QUrl&)), SLOT(link_clicked(const QUrl&)));
}

html_editor::~html_editor()
{
  // delete the QActions we allocated
  for (uint i=0; i<sizeof(m_action_definitions)/sizeof(m_action_definitions[0]); i++) {
    struct action_type* def = &m_action_definitions[i];
    if (def->webaction == QWebPage::NoWebAction) {
      // we own only the actions that aren't provided by QWebPage
      QMap<const char*, QAction*>::const_iterator iter = m_actions.find(def->name);
      if (iter != m_actions.end())
	delete iter.value();
    }
  }
}

void
html_editor::insert_link()
{
  link_editor le(this);
  if (le.exec() == QDialog::Accepted) {
    QString url = le.url();
    QString text = le.text();
    if (text.isEmpty())
      text = url;
    url.replace("\"", "\\\""); // replace " by \"
    
    QString h = QString("<a href=\"%1\">%2</a>").arg(url).arg(text);
    insert_html(h);
  }
}

QString
html_editor::html_text() const
{
  return page()->mainFrame()->toHtml();
}

void
html_editor::set_html_text(const QString& text)
{
  m_load_finished = false;
  setHtml(text);
}

void
html_editor::bold()
{
  exec_command("Bold");
}

void
html_editor::italic()
{
  exec_command("Italic");
}

void
html_editor::underline()
{
  exec_command("underline");
}

void
html_editor::subscript()
{
  exec_command("subscript");
}

void
html_editor::superscript()
{
  exec_command("superscript");
}

void
html_editor::strikethrough()
{
  exec_command("strikeThrough");
}

void
html_editor::remove_format()
{
  exec_command("RemoveFormat");
}

void html_editor::undo()
{
  page()->triggerAction(QWebPage::Undo);
}

void html_editor::redo()
{
  page()->triggerAction(QWebPage::Redo);
}

void html_editor::align_right()
{
  exec_command("JustifyRight");
}

void html_editor::align_left()
{
  exec_command("JustifyLeft");
}

void html_editor::justify()
{
  exec_command("JustifyFull");
}

void html_editor::center()
{
  exec_command("JustifyCenter");
}

void html_editor::indent_left()
{
  exec_command("Outdent");
}

void html_editor::indent_right()
{
  exec_command("Indent");
}

void html_editor::insert_hr()
{
  exec_command("InsertHorizontalRule");
}

void html_editor::insert_unordered_list()
{
  exec_command("InsertUnorderedList");
}

void html_editor::insert_ordered_list()
{
  exec_command("InsertOrderedList");
}

void
html_editor::foreground_color()
{
  QColor color = QColorDialog::getColor(Qt::black, this);
  if (color.isValid())
    exec_command("foreColor", color.name());
}

void
html_editor::background_color()
{
  QColor color = QColorDialog::getColor(Qt::white, this);
  if (color.isValid())
    exec_command("hiliteColor", color.name());
}

void
html_editor::insert_image()
{
  QString filters;
  filters += tr("Image files (*.png *.jpg *.jpeg *.gif)") + ";;";
  filters += tr("PNG files (*.png)") + ";;";
  filters += tr("JPEG files (*.jpg *.jpeg)") + ";;";
  filters += tr("GIF files (*.gif)") + ";;";
  filters += tr("All Files (*)");

  QString fname = QFileDialog::getOpenFileName(this, tr("Open image..."),
					       QString(), filters);
  if (!fname.isEmpty() && QFile::exists(fname)) {
    QUrl url = QUrl::fromLocalFile(fname);
    exec_command("insertImage", url.toString());
  }
}

void
html_editor::exec_command(const QString cmd, const QString arg)
{
  QString jscript;
  if (!arg.isEmpty()) {
    QString h = arg;
    h.replace("'", "\\'");
    jscript = QString("document.execCommand('%1',false,'%2');").arg(cmd).arg(h);
  }
  else
    jscript = QString("document.execCommand('%1',false,null);").arg(cmd);
  QVariant res = page()->mainFrame()->evaluateJavaScript(jscript);
  //  qDebug() << res;  
}

const char*
html_editor::manitou_files_jscript =
"manitou_files = {"
"  get_references : function() {"
"    var result = [];"
"    manitou_files._process_references(document.getElementsByTagName('body')[0], result, null);"
"    return result;"
"  },"
"  replace_references : function(cids) {"
"    manitou_files._process_references(document.getElementsByTagName('body')[0], null, cids);"
"  },"
"  _process_references: function(node,arr,repl) {"
"    if (node.hasChildNodes) {"
"      for (var cn=0;cn<node.childNodes.length;cn++) {"
"	manitou_files._process_references(node.childNodes[cn], arr, repl);"
"      }"
"    }"
"    if (node.nodeType==1) {"
"      a = node.attributes.getNamedItem('src');"
"      if (a!=null) {"
"        s = a.value;"
"        if (s.length > 0 && s.toLowerCase().indexOf('file:///') != -1) {"
"          if (arr!=null)"
"            arr.push(s);"
"          if (repl!=null) {"
"            s1 = repl[s];"
"      	    if (s1!=null)"
"              node.setAttribute('src', 'cid:'+s1);"
"          }"
"        }"
"      }"
"    }"
"  }"
"};"
    ;

QStringList
html_editor::collect_local_references()
{
  page()->mainFrame()->evaluateJavaScript(manitou_files_jscript);
  QVariant res = page()->mainFrame()->evaluateJavaScript("manitou_files.get_references()");
  // TODO: unify "file://filename" and "filename" and remove duplicates
  return res.toStringList();
}

void
html_editor::replace_local_references(const QMap<QString,QString>& map)
{
  page()->mainFrame()->evaluateJavaScript(manitou_files_jscript);
  QString jscript = "var n=[];";
  QMap<QString,QString>::const_iterator i = map.constBegin();
  /* build a javascript snippet that maps each "source" reference
     (typically something like file:///home/user/foobar.gif) to each
     mime content id (cid:uniqueref@domain) */
  while (i != map.constEnd()) {
    QString source = i.key();
    source.replace("'", "\\'");
    QString dest = i.value();
    dest.replace("'", "\\'");
    jscript.append(QString("n['%1']='%2';").arg(source).arg(dest));
    ++i;
  }
  jscript.append("manitou_files.replace_references(n); 1");
  QVariant res=page()->mainFrame()->evaluateJavaScript(jscript);
}

void
html_editor::insert_html(const QString html)
{
  exec_command("InsertHTML", html);
}

bool
html_editor::eval_jscript(const QString script)
{
  QString fragment = script;
  fragment.prepend("try {");
  fragment.append("\n} catch(e) { e; }");
  QVariant res = page()->mainFrame()->evaluateJavaScript(fragment);
  return true; // TODO: peek into result
}

QList<QToolBar*>
html_editor::create_toolbars()
{
  QList<QToolBar*> toolbars;

  QToolBar* toolbar1 = new QToolBar(tr("HTML input"), this);
  toolbar1->setIconSize(QSize(16,16));

  QToolBar* toolbar2 = new QToolBar(tr("HTML operations"), this);
  toolbar2->setIconSize(QSize(16,16));

  QComboBox* fonts = new QComboBox();
  m_font_chooser = fonts;
  QStringList families = QFontDatabase().families();
  fonts->addItems(families);
  connect(fonts, SIGNAL(activated(const QString&)),
	  this, SLOT(change_font(const QString&)));
  toolbar1->addWidget(fonts);

  QStringList sizes;
  sizes << "xx-small";
  sizes << "x-small";
  sizes << "small";
  sizes << "medium";
  sizes << "large";
  sizes << "x-large";
  sizes << "xx-large";
  QComboBox* sizes_box = new QComboBox();
  m_para_format_chooser = sizes_box;
  sizes_box->addItems(sizes);
  connect(sizes_box, SIGNAL(activated(int)), this, SLOT(change_font_size(int)));
  toolbar1->addWidget(sizes_box);
  
  QStringList headerN;
  for (uint i=1; i<=6; i++) {
    headerN << QString(tr("Header %1")).arg(i);
  }
  QComboBox* headers = new QComboBox();
  headers->addItems(headerN);
  m_headers_chooser = headers;
  connect(headers, SIGNAL(activated(int)), this, SLOT(format_header(int)));
  toolbar1->addWidget(headers);

  for (uint i=0; i<sizeof(m_action_definitions)/sizeof(m_action_definitions[0]); i++) {
    struct action_type* def = &m_action_definitions[i];
    QMap<const char*, QAction*>::const_iterator iter = m_actions.find(def->name);
    if (iter != m_actions.end()) {
      if (i <= 5)
	toolbar1->addAction(iter.value());
      else
	toolbar2->addAction(iter.value());
    }
  }

  toolbars.push_back(toolbar1);
  toolbars.push_back(toolbar2);
  return toolbars;
}

void
html_editor::enable_html_controls(bool enable)
{
  QMapIterator<const char*, QAction*> it(m_actions);
  while (it.hasNext()) {
    it.next();
    it.value()->setEnabled(enable);
  }
  m_font_chooser->setEnabled(enable);
  m_para_format_chooser->setEnabled(enable);
  m_headers_chooser->setEnabled(enable);
}

void
html_editor::change_font(const QString& family)
{
  exec_command("fontName", family);
  setFocus();
}

void
html_editor::change_font_size(int size_index)
{
  exec_command("fontSize", QString::number(size_index));
  setFocus();
}

void
html_editor::format_header(int size)
{
  exec_command("formatBlock", QString("h%1").arg(size+1));
  setFocus();
}

void
html_editor::create_actions()
{
  for (uint i=0; i<sizeof(m_action_definitions)/sizeof(m_action_definitions[0]); i++) {
    struct action_type* def = &m_action_definitions[i];
    QAction* a;
    if (def->webaction == QWebPage::NoWebAction) {
      // the action is owned by us
      a = new QAction(tr(def->name), this);
    }
    else {
      // action already provided by QWebPage
      a = page()->action(def->webaction);
    }
    if (a) {
      a->setIcon(HTML_ICON(def->icon));
      if (def->shortcut)
	a->setShortcut(tr(def->shortcut));
      m_actions[def->name] = a;
      if (def->slot)
	connect(a, SIGNAL(triggered()), this, def->slot);
    }
  }
}

void
html_editor::finish_load()
{
  while (!m_load_finished) {
    QApplication::processEvents();
  }
}

void
html_editor::load_finished(bool ok)
{
  m_load_finished=true;
  if (ok) {
    page()->setContentEditable(true);
    // let the cursor be visible
    //    page()->mainFrame()->evaluateJavaScript("window.getSelection().collapse(document.getElementsByTagName('body')[0], 0)");
  }
}

void
html_editor::append_paragraph(const QString& fragment)
{  
  QString h = fragment;
  h.replace("'", "\\'");
  h.replace("\n", "<br>");
  QString jscript = QString("try {var b=document.getElementsByTagName('body')[0]; var p=document.createElement('p'); p.innerHTML='%1'; b.appendChild(p); 1;} catch(e) { e; }").arg(h);
  QVariant res = page()->mainFrame()->evaluateJavaScript(jscript);
}

QString
html_editor::to_plain_text() const
{
  return page()->mainFrame()->toPlainText();
}

void
html_editor::link_clicked(const QUrl& url)
{
  if (!QDesktopServices::openUrl(QUrl(url))) {
    QMessageBox::critical(NULL, tr("Error"), tr("Unable to open URL"));
  }
}

void
html_editor::dragEnterEvent(QDragEnterEvent* event)
{
  /* it's necessary on Windows to explicitly accept the event since
     the default implementation doesn't seem to accept it for dragged
     files */
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
  else
    QWebView::dragEnterEvent(event);
}

void
html_editor::dragMoveEvent(QDragMoveEvent* event)
{
  
  /* We don't let the base dragMoveEvent handle URLs to avoid it
     taking control over the caret and not releasing it after the drop */
  if (!event->mimeData()->hasUrls())
    QWebView::dragMoveEvent(event);
}

void
html_editor::dropEvent(QDropEvent* event)
{
  if (event->mimeData()->hasUrls()) {
    foreach (QUrl url, event->mimeData()->urls())  {
      emit attach_file_request(url);
    }
    event->acceptProposedAction();    
  }
  else
    QWebView::dropEvent(event);
}
