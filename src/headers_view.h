/* Copyright (C) 2004-2009 Daniel Vérité

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

#ifndef INC_HEADERS_VIEW_H
#define INC_HEADERS_VIEW_H

/* headers_view is a rich-text viewer for mail headers, with some additional features:
- xface and face rendering
- html links to trigger some actions
- filter logs display
 */

#include <QString>
#include <QTextBrowser>
#include <QPixmap>
#include <QByteArray>
#include <QMap>

#include "searchbox.h"
#include <list>

class QSizeF;
class QUrl;

class headers_view: public QTextBrowser
{
  Q_OBJECT
public:
  headers_view(QWidget* parent=NULL);
  virtual ~headers_view();
  QVariant loadResource(int type,const QUrl & name);
  void set_show_on_demand (bool b);
  bool set_xface (QString& xface);
  bool set_face (const QByteArray& png);
  void set_contents(const QString&); // set the base text
  void highlight_terms(const std::list<searched_text>&);
  void clear_selection();
public slots:
  // enable or disable a command link
  void enable_command(const QString, bool);
  void reset_commands();
  void clear();

private slots:
  void link_clicked(const QUrl& url);
  void size_headers_changed(const QSizeF&);
private:
  QPixmap m_xface_pixmap;
  QString m_base_text;
  QMap<QString,bool> m_enabled_commands;
  QString command_links();
  void redisplay();
signals:
  void to_text();
  void to_html();
  void fetch_ext_contents();
  void msg_display_request();
  void complete_load_request();
};

#endif // INC_HEADERS_VIEW_H
