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

#ifndef INC_MAIL_LISTVIEW_H
#define INC_MAIL_LISTVIEW_H

#include "main.h"
#include <vector>
#include <QMap>
#include <QSet>
#include <QEvent>
#include <QMouseEvent>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>

#include "dbtypes.h"
#include "db.h" // mail_msg
#include "app_config.h"

#include <list>
#include <map>

class msg_list_window;
class QIcon;
class QMenu;
class QKeyEvent;
class command_item;

class mail_item_model : public QStandardItemModel
{
  Q_OBJECT
public:
  mail_item_model(QObject* parent=0);
  virtual ~mail_item_model();
  QStandardItem* item_from_id(mail_id_t) const;
  void init();
  QStandardItem* insert_msg(mail_msg* msg, QStandardItem* parent=NULL);
  QStandardItem* insert_msg_sorted(mail_msg* msg, QStandardItem* parent,
				   int column,
				   Qt::SortOrder order);
  void remove_msg(mail_msg* msg);
  QStandardItem* reparent_msg(mail_msg* msg, mail_id_t parent_id);
  void update_msg(const mail_msg *msg);
  mail_msg* find(mail_id_t mail_id);
  command_item* add_segment_selector(const QString);
  static const int mail_msg_role = Qt::UserRole+2;
  static const int type_command = QStandardItem::UserType+1;
  static const int type_flag = QStandardItem::UserType+2;

  // TODO: see if the date format could be kept in the view only
  // currently we need it when instantiating the QStandardItemModels
  void set_date_format(int d) { m_date_format=d; }
  int get_date_format() const { return m_date_format; }
  static const int ncols;
  static const char* m_column_names[];
  static QString column_name(int);
  enum {
    column_subject=0,
    column_sender,
    column_status,
    column_pri,
    column_attch,
    column_note,
    column_date,
    column_recipient
  };
  void clear();
  QStandardItem* first_top_level_item() const;

  void sort(int column, Qt::SortOrder order);

  /* row count independently of folding  */
  int total_row_count() const;

  QString m_display_sender_mode;

private:
  // returns an icon showing the mail status
  QIcon* icon_status(uint status);

  // instantiate the items for one row
  void create_row(mail_msg* msg, QList<QStandardItem*>& items);

  // binary search for insertion point in sorted model
  int insertion_point(QList<QStandardItem*>& new_row,
		      QStandardItem* parent,
		      int column,
		      Qt::SortOrder order);

  // Association between the mail_id and the first QStandardItem of
  // the corresponding row (the item at column 0, also containing the mail_msg*
  // in QVariant form)
  QMap<mail_id_t, QStandardItem*> items_map;

  int m_date_format;
};

class flag_item: public QStandardItem
{
public:
  flag_item();
  flag_item(const QIcon&);
  int type() const { return mail_item_model::type_flag; }
  bool operator< (const QStandardItem & other) const {
    bool has_flag = data().toBool();
    bool other_has_flag = other.data().toBool();
    return !has_flag && other_has_flag;
  }
};

// For items that run a command when selected
class command_item: public QStandardItem
{
public:
  command_item(const QString& text);
  int type() const {
    return mail_item_model::type_command;
  }
};

class mail_listview : public QTreeView
{
  Q_OBJECT
public:
  mail_listview(QWidget* parent=0);
  virtual ~mail_listview();

  void set_threaded(bool b) {
    m_display_threads=b;
  }
  void add_msg(mail_msg*);
  void remove_msg(mail_msg*, bool select_next=true);
#if 0
  void reparent_msg(mail_msg*,mail_id_t);
#endif
  // get the list of currently selected items
  void get_selected(std::vector<mail_msg*>&);

  // get the thread_id's of the currently selected messages
  std::set<mail_thread> selected_threads();

  void clear();
  bool empty() const;

  void scroll_to_bottom();
  int selection_size() const;
  void init_columns();
  mail_msg* find(mail_id_t mail_id);
  msg_list_window* m_msg_window;

  void update_msg(const mail_msg *msg);

  mail_item_model* model() const {
    return static_cast<mail_item_model*>(QTreeView::model());
  }

  mail_msg* first_msg() const;
  mail_msg* nearest_msg(const mail_msg* msg, int direction) const;

  QStandardItem* nearest(const mail_msg* msg, int direction) const;
  void select_nearest(const mail_msg* msg);
  void select_below(const mail_msg* msg);
  void select_above(const mail_msg* msg);
  void select_msg(const mail_msg* msg);

  void add_segment_selector(const QString segment);

  // select the message and scroll to it if ensure_visible is true
  void select_message(mail_id_t id, bool ensure_visible);

  // select all messages from a set of threads (passed as a set of mail.thread_id)
  int select_threads(const QSet<uint>&);

  // flags for insert_list()
  static const int full_sort = 1;    // sort the whole listview after the insert
  static const int per_msg_sort = 2; // compute insertion points at each insert
  static const int no_preexist_check = 4;  // no duplicates possible in list
  void insert_list(std::list<mail_msg*>& list, int insert_flags=0);

  void swap_sender_recipient(bool);
  bool sender_recipient_swapped() const {
    return m_sender_column_swapped;
  }
  void change_display_config(const app_config& conf);

protected:
  void keyPressEvent(QKeyEvent*);
  void selectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void make_tree(std::list<mail_msg*>& list);
  void collect_expansion_states(QStandardItem* item,
				QSet<QStandardItem*>& expanded_set);
  QStandardItem* insert_sub_tree(std::map<mail_id_t,mail_msg*>&,
				 mail_msg*);



  bool m_display_threads;
  bool m_sender_column_swapped;

  // create a popup menu to select the columns to show/hide
  QMenu* make_header_menu(QWidget* parent);

  void compute_visible_sections();

  static const char* m_column_names[];
  int m_nb_visible_sections;

private slots:
  void item_clicked(const QModelIndex&);

public slots:
  void popup_ctxt_menu(const QPoint&);
  void popup_ctxt_menu_headers(const QPoint&);
  void change_msg_status (mail_id_t id, uint mask_set, uint mask_unset);
  void force_msg_status (mail_id_t id, uint status, int priority);
  void refresh(mail_id_t id);


signals:
  void selection_changed();
  void scroll_page_down();
  void note_icon_clicked();
  void change_segment(int);
};


#endif // INC_MAIL_LISTVIEW_H
