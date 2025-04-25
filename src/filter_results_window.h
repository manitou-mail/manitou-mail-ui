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

#ifndef INC_FILTER_RESULTS_WINDOW_H
#define INC_FILTER_RESULTS_WINDOW_H

#include "main.h"
#include "selectmail.h"

#include <QWidget>
#include <QMainWindow>
//#include <QPlainTextEdit>
#include <QTextBrowser>

class mail_listview;
class QPushButton;
class QStatusBar;
class QProgressBar;
class QSplitter;
class message_view;
class attch_listview;
class mail_msg;

class filter_expr_viewer : public QTextBrowser
{
  Q_OBJECT
public:
  filter_expr_viewer(QWidget* parent=0);
public slots:
  void size_changed(const QSizeF&);
};

class filter_result_message_view : public QWidget
{
  Q_OBJECT
public:
  filter_result_message_view(QWidget* parent=0);
  void set_message(mail_msg*);
public slots:
  void set_headers_visibility(int);
private:
  message_view* m_msg_view;
  attch_listview* m_attch_listview;
};

class filter_results_window : public QWidget //QMainWindow
{
  Q_OBJECT
public:
  filter_results_window(QWidget* parent=0);
  ~filter_results_window();

  void incorporate_message(mail_result&);
  void clear();
  void show_status_message(const QString);
  int nb_results();
  void show_progressbar();
  void hide_progressbar();
  void show_filter_expression(const QString);
public slots:
  void cancel_run();
  void open_message();
private:
  mail_listview* m_msg_list;
  QPushButton* m_cancel;
  QPushButton* m_open_message;
  QStatusBar* m_status_bar;
  QPushButton* m_abort_button;
  QProgressBar* m_progress_bar;
  filter_expr_viewer* m_expr_viewer;
signals:
  void stop_run();
};

#endif
