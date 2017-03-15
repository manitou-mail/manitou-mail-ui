/* Copyright (C) 2016-2017 Daniel Verite

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

#ifndef INC_STATISTICS_H
#define INC_STATISTICS_H

#include <QWidget>
#include <QMainWindow>
#include <QList>
#include <QStringList>
#include <QWebView>
#include <QColor>

class QComboBox;
class QDateEdit;
class QListWidget;
class QResizeEvent;
class QLineEdit;
class QListWidgetItem;
class QLabel;

class tag_line_edit_selector;

class stats_webview: public QWebView
{
  Q_OBJECT
public:
  stats_webview(QWidget* parent=NULL);
  QString m_template;
  void display(QString&);
  void clear();
private:
  int m_x;			// canvas size
  int m_y;
  bool m_queue_redisplay;
private slots:
  void redisplay();
protected:
  virtual void resizeEvent(QResizeEvent* event);
};

class stats_view: public QMainWindow
{
  Q_OBJECT
public:
  stats_view(QWidget* parent);
  ~stats_view();
public slots:
  void pdf_export();
  void csv_export();
  void show_graph(QList<QString>& labels);
  void run_query();
  void clear_graph();
private:
  struct results {
    QStringList dates;
    QList<int> values;
  } m_last_results;

  struct result_msgcount {
    QColor color;
    QList<int> values;
    QDate mindate;
    QDate maxdate;
  };
  QList<struct result_msgcount> m_all_results;

  QString m_last_pdf_filename;
  QString m_last_csv_filename;
  QColor m_color;
  QLabel* m_color_label;

  void init_identities();
  void set_color_label();
  QList<int> selected_identities();
  stats_webview* m_webview;
  QComboBox* m_query_sel;
  QDateEdit* m_date1;
  QDateEdit* m_date2;
  tag_line_edit_selector* m_tag;
  QListWidget* m_list_idents;
private slots:
  void ident_changed(QListWidgetItem*);
  void change_color();
};


#endif
