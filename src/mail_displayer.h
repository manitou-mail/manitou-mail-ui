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


#ifndef INC_MAIL_DISPLAYER_H
#define INC_MAIL_DISPLAYER_H

#include <QString>
#include <list>
#include "filter_log.h"

class mail_msg;
class body_view;
class message_view;

class display_prefs {
public:
  void init();
  int m_show_headers_level;		/* 0=none,1=most,2=all */
  bool m_show_tags;
  bool m_threaded;
  bool m_wrap_lines;
  int m_hide_quoted;
  bool m_clickable_urls;
  bool m_show_filters_trace;
  bool m_show_tags_in_headers;
  //    bool m_body_fixed_font;
};

class mail_displayer
{
public:
  mail_displayer(message_view* w=NULL);
  virtual ~mail_displayer();
  QString expand_body_line(const QString&, const display_prefs& prefs);
  bool m_wrap_lines;
  message_view* msg_widget;
  static void find_urls(const QString& s, std::list<std::pair<int,int> >* matches);
  QString sprint_headers(int show_headers_level, mail_msg* msg);
  QString text_body_to_html(const QString &b, const display_prefs& prefs);
  QString format_filters_trace(const filter_log_list&);
  QString sprint_additional_headers(const display_prefs& prefs,
				    mail_msg* msg);
  static QString& htmlize(QString);
private:

};

#endif // INC_MAIL_DISPLAYER_H

