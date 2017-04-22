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

#ifndef INC_MIME_MSG_VIEWER_H
#define INC_MIME_MSG_VIEWER_H

#include <QWidget>
#include <QString>
#include "mail_displayer.h"

class message_view;
class attch_listview;

class mime_msg_viewer: public QWidget
{
  Q_OBJECT
public:
  mime_msg_viewer(const char* rawmsg, const display_prefs& prefs);
  virtual ~mime_msg_viewer();
  void set_encoding(const QString enc) {
    m_encoding = enc;
  }
private:
  /* Appends to 'html_output' an html representation of 'body' which is
     the body of a raw rfc822 message */
  void format_body(QString& html_output, const char* body, display_prefs& prefs);

  QString m_encoding;
  message_view* m_view;
  attch_listview* m_attchview;
  QString m_header;

private slots:
  void edit_copy();
};

#endif // INC_MIME_MSG_VIEWER_H
