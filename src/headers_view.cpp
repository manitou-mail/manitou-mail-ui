/* Copyright (C) 2004-2008 Daniel Vérité

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

/* headers_view is a rich-text viewer for mail headers, with some additional features:
- xface and face rendering
- html links to trigger some actions
- filter logs display
 */

#include "headers_view.h"
#include <QTextEdit>
#include <QTextCursor>
#include <QUrl>

headers_view::headers_view(QWidget* parent) : QTextBrowser(parent)
{
  setFrameStyle(QFrame::NoFrame);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // We track resizings of the header contents to adjust the height of
  // m_headersv so that all headers stay visible. 
  connect((const QObject*)document()->documentLayout(),
	  SIGNAL(documentSizeChanged(const QSizeF&)),
	  this,
	  SLOT(size_headers_changed(const QSizeF&)));

  setFocusPolicy(Qt::NoFocus);	// no keyboard events?
//  document()->setDefaultStyleSheet("body { background: ivory; }");

  connect(this, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(link_clicked(const QUrl&)));
}

headers_view::~headers_view()
{
}

void
headers_view::set_contents(const QString& html_text)
{
  m_base_text = html_text;
  setHtml(QString("<html><body>")+html_text+"</body></html>");
}

void
headers_view::size_headers_changed(const QSizeF& newsize)
{
  setFixedHeight(newsize.height());
  /* calling update() on the layout appears unnecessary at the moment
     but it was before (for an unknown reason), so keep the code to be
     uncommented later if required (m_layout relates to the parent widget) */
#if 0
  m_layout->update();
#endif
}


bool
headers_view::set_xface(QString& xface)
{
  return m_xface_pixmap.loadFromData((const uchar*)xface.toLatin1().constData(), xface.length());
}

bool
headers_view::set_face(const QByteArray& png)
{
  return m_xface_pixmap.loadFromData(png, "PNG");
}

void
headers_view::clear_selection()
{
  QTextCursor cursor = this->textCursor();
  cursor.clearSelection();
  setTextCursor(cursor);
}
  

void
headers_view::set_show_on_demand(bool b)
{
  if (b) {
    setHtml("<h2><a href=\"#show\">" + tr("Fetch on demand mode: click this link to display the message") + "</a></h2>");
  }
  else {
    redisplay();
  }
}

QVariant
headers_view::loadResource(int type, const QUrl& name)
{
  Q_UNUSED(type);
  if (name.toString()=="/xface.xpm") {
    QVariant v(m_xface_pixmap);
    return v;
  }
  return QVariant();
}

void
headers_view::clear()
{
  setHtml("<html><body></body></html>");
  m_base_text.truncate(0);
  QList<QTextEdit::ExtraSelection> qlist;
  // remove any selection
  setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

QString
headers_view::command_links()
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
      bool b=m_enabled_commands.value(cmd);
      if (b==true) {
	line.append(QString(" &nbsp;&nbsp;&nbsp;<a href=\"#%1\">%2</a>").arg(commands[i]).arg(tr(commands[i+1])));
      }
    }
  }
  return line;
}

void
headers_view::redisplay()
{
  QString links = command_links();
  if (!links.isEmpty())
    links.prepend("<br>");
  setHtml(QString("<html><body>")+m_base_text+links+"</body></html>");
}

void
headers_view::reset_commands()
{
  m_enabled_commands.clear();
}

void
headers_view::enable_command(const QString command, bool enable)
{
  m_enabled_commands.insert(command, enable);
  redisplay();
}

void
headers_view::link_clicked(const QUrl& url)
{
  const QString cmd=url.toString();
  if (cmd=="#fetch") {
    emit fetch_ext_contents();
  }
  else if (cmd=="#to_text") {
    emit to_text();
  }
  else if (cmd=="#to_html") {
    emit to_html();
  }
  else if (cmd=="#show") {
    emit msg_display_request();
  }
  else if (cmd=="#complete_load") {
    emit complete_load_request();
  }
}

void
headers_view::highlight_terms(const std::list<searched_text>& lst)
{
  QList<QTextEdit::ExtraSelection> qlist;
  if (lst.empty()) {
    setExtraSelections(qlist);
    return;
  }
  std::list<searched_text>::const_iterator it = lst.begin();
  QTextEdit::ExtraSelection qsel;
  QTextCharFormat qfmt;
  qfmt.setBackground(QBrush(QColor("yellow")));
  qsel.format = qfmt;
  QTextCursor tcursor;

  QTextDocument::FindFlags flags=0;
  if (it->m_is_cs)
    flags |= QTextDocument::FindCaseSensitively;
  if (it->m_is_word) {
    flags |= QTextDocument::FindWholeWords;
  }

  for ( ; it!=lst.end(); ++it) {
    int index=0;
    do {
      tcursor = document()->find(it->m_text, index, flags);
      if (!tcursor.isNull()) {
	qsel.cursor=tcursor;
	qlist.append(qsel);
	index = tcursor.position()+1;
      }
    } while (!tcursor.isNull());
  }
  setExtraSelections(qlist);	// even if qlist is empty (otherwise we get a blank rectangle colored as the selection)
}
