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

#ifndef INC_IMPORT_WINDOW_H
#define INC_IMPORT_WINDOW_H

#include <QWidget>
#include <QMap>
#include <QTreeWidgetItem>

class QTreeWidget;
class QPushButton;
class QProgressBar;
class QTimer;


class import_mbox
{
public:
  import_mbox() {m_id=0;}
  import_mbox(int id) {
    m_id=id;
  }
  QString filename() const {
    return m_filename;
  }
  int status() const {
    return m_status;
  }
  void set_status(int s) {
    m_status=s;
  }
  int id() const {
    return m_id;
  }
#if 0
  date creation_date() const {
    return m_creation_date;
  }
#endif
  QString status_text() const;

  // remove the import and all its data from the database
  bool purge();

  bool stop();
  bool instantiate_job();

  enum {
    // must match import_mbox.status in the db
    status_not_started=0,
    status_running=1,
    status_stopped=2,
    status_finished=3
  };
  double completion() { return m_completion; }

  int m_id;
  int m_status; // 0:not started, 1:running, 2:stopped, 3:finished
  QString m_filename;
#if 0
  date m_creation_date;
#endif
  double m_completion;
};

class import_mbox_list : public QList<import_mbox>
{
public:
  bool load();
  bool id_exists(int id) const;
};


class import_window_widget_item: public QTreeWidgetItem
{
public:
  static const int index_column_date=1;
#if 0
  virtual bool operator<(const QTreeWidgetItem&) const;
#endif
  static const int import_mbox_id_role = Qt::UserRole;
  static const int import_mbox_date_role = Qt::UserRole+1;
};

class import_window: public QWidget
{
  Q_OBJECT
public:
  import_window();
  ~import_window();
  static import_window* open_unique();
private slots:
  void item_changed(QTreeWidgetItem*, QTreeWidgetItem*);
  void refresh();
  void new_import();
  void start_stop();
  void purge_import();
  void new_import_started();
private:
  static import_window* m_current_instance;
  import_window_widget_item* get_item(int import_mbox_id);
  import_window_widget_item* add_item(const import_mbox* m);
  void display_entry(QTreeWidgetItem* item, const import_mbox* m);
  import_mbox* get_import_mbox(int import_mbox_id);
  import_mbox* selected_item();
  void update_buttons(import_mbox*);
  void load();
  import_mbox_list m_list;
  QMap<int,QProgressBar*> m_progress_bars;
  QTimer* m_timer;
  QTreeWidget* m_treewidget;
  QPushButton* m_btn_stop_restart;
  QPushButton* m_btn_purge;
  QPushButton* m_btn_new;
  QPushButton* m_btn_view;
  QPushButton* m_btn_clone;
  QPushButton* m_btn_refresh;
};

#endif
