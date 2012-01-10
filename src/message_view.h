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

#ifndef INC_MESSAGE_VIEW_H
#define INC_MESSAGE_VIEW_H

/* message_view combines headers_view and body_view stacked vertically
   with a shared scrollbar */

#include <QScrollArea>
#include <QString>
#include <list>

#include "body_view.h"
#include "mail_displayer.h"

// TODO: incorporate the attachements panel into this view and merge it
// with mime_msg_viewer

class headers_view;
class mail_msg;
class QSizeF;
class QFrame;
class QResizeEvent;
class QKeyEvent;
class QWheelEvent;
class msg_list_window;

class message_view : public QScrollArea
{
  Q_OBJECT
public:
  message_view(QWidget* parent, msg_list_window* sup_parent);
  ~message_view();
  void clear();
  void enable_page_nav(bool back, bool forward);
  void setFont(const QFont&);
  const QFont font() const;
  void print();
  void copy();
  void set_mail_item (mail_msg*);
  void set_wrap(bool);
  void set_show_on_demand(bool);
  void highlight_terms(const std::list<searched_text>&);

  headers_view* headers() {
    return m_headersv;
  }
  QSize sizeHint() const;
  void reset_state();
  void display_body(const display_prefs& prefs, int preferred_format=0);
  void set_html_contents(const QString& headers, const QString& body, int type);
  QString selected_text() const;
  int content_type_shown() const;
  QString selected_html_fragment();
  QString body_as_text() const;
protected:
  void resizeEvent(QResizeEvent* event);
  void keyPressEvent(QKeyEvent*);
public slots:
  void wheel_body(QWheelEvent* );
  void allow_external_contents();
  void ask_for_external_contents();
  void page_down();
  void show_text_part();
  void show_html_part();
  void headers_own_selection(bool);
  void headers_selection_changed();
  void body_selection_changed();
  void change_zoom(int);
private slots:
  void load_finished(bool);
  void resize_frame();
  void complete_body_load();
private:
  headers_view* m_headersv;
  msg_list_window* m_parent;
  mail_msg* m_pmsg;
  body_view* m_bodyv;
  bool m_can_move_forward, m_can_move_back;
  QFrame* m_frame;
  bool m_loaded;
  bool m_ext_contents;
  bool m_has_text_part;
  bool m_has_html_part;
  bool m_header_has_selection;
  qreal m_zoom_factor;
  std::list<searched_text> m_highlight_words;
  int m_content_type_shown;
signals:
  void on_demand_show_request();
  void popup_body_context_menu();
  void page_back();
  void page_forward();
};

#endif // INC_MESSAGE_VIEW_H
