/* Copyright (C) 2004-2019 Daniel Verite

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

#ifndef INC_MAILING_WINDOW_H
#define INC_MAILING_WINDOW_H

#include <QWidget>
#include <QMap>
#include <QTreeWidgetItem>

#include "mailing.h"

class QTreeWidget;
class QPushButton;
class QProgressBar;
class QTimer;

class mailing_window_widget_item: public QTreeWidgetItem
{
public:
  static const int index_column_date=1;
  virtual bool operator<(const QTreeWidgetItem&) const;
  static const int mailing_id_role = Qt::UserRole;
  static const int mailing_date_role = Qt::UserRole+1;
};

class mailing_window: public QWidget
{
  Q_OBJECT
public:
  mailing_window();
  ~mailing_window();
  static mailing_window* open_unique();
private slots:
  void item_changed(QTreeWidgetItem*, QTreeWidgetItem*);
  void refresh();
  void new_mailing();
  void view_mailing();
#if 0
  void clone_mailing();
#endif
  void start_stop();
  void delete_mailing();
  void new_mailing_started();
private:
  static mailing_window* m_current_instance;
  mailing_window_widget_item* get_item(int mailing_id);
  mailing_window_widget_item* add_item(const mailing_db* m);
  void display_entry(QTreeWidgetItem* item, const mailing_db* m);
  mailing_db* get_mailing(int mailing_id);
  mailing_db* selected_item();
  void update_buttons(mailing_db*);
  void load();
  mailings m_mailings;
  QMap<int,QProgressBar*> m_progress_bars;
  QTimer* m_timer;
  QTreeWidget* m_treewidget;
  QPushButton* m_btn_stop_restart;
  QPushButton* m_btn_delete;
  QPushButton* m_btn_new;
  QPushButton* m_btn_view;
  QPushButton* m_btn_clone;
  QPushButton* m_btn_refresh;
};

#endif
