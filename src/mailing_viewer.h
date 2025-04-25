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

#ifndef INC_MAILING_VIEWER_H
#define INC_MAILING_VIEWER_H

#include <QWidget>
#include "mailing.h"

class QRadioButton;
class QPushButton;
class QWebView;
class QLabel;
class QLineEdit;

class mailing_viewer: public QWidget
{
  Q_OBJECT
public:
  mailing_viewer(QWidget* parent=NULL, Qt::WindowFlags f=Qt::Window);
  ~mailing_viewer();
  void show_merge();
  void show_merge_existing(mailing_db*);
  void set_data(mailing_options& opt);
  void set_data_and_own_it(mailing_options* opt);
private:
  void init();
  void prepend_body_fragment(const QString&);
  QString htmlize_header(const QString);
  void show_number(int);
  // controls
  QWebView* m_webview;
  QRadioButton* m_rb_html;
  QRadioButton* m_rb_text;
  QPushButton* m_btn_close;
  QLabel* m_label_count;
  QPushButton* m_btn_prev;
  QPushButton* m_btn_next;
  QLineEdit* m_number;
  // data
  QString m_current_html;
  QString m_current_text;
  QString m_current_header;
  int m_current_index;
  bool m_options_owned;
  mailing_options* m_options; // the class doesn't own this data, it's from set_data
private slots:
  void load_finished(bool);
  void to_html();
  void to_plain_text();
  void goto_message();
  void next_message();
  void prev_message();
};

#endif
