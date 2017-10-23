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

#ifndef INC_QUERY_LISTVIEW_H
#define INC_QUERY_LISTVIEW_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <map>
#include "db.h"
#include "selectmail.h"
#include "tags.h"
#include <QSet>
#include <QMap>
#include <QBrush>

class QMouseEvent;

class tag_node;

// mail_id => status
typedef std::map<mail_id_t,int> qs_mail_map;
// mail_id => priority
typedef std::map<mail_id_t,int> priority_map;

typedef QMap<int,int> archived_tag_map;

// tag_id => map of current tagged mails
typedef std::map<uint,qs_mail_map*> qs_tag_map;

class query_lvitem : public QTreeWidgetItem
{
public:
  query_lvitem(const QString name);
  query_lvitem(QTreeWidgetItem* parent, int item_type, const QString name);
  query_lvitem(QTreeWidget* parent, const QString name);
  virtual ~query_lvitem();

  void set_brush(QBrush); /* childs inherit from the brush */

  // display title and counts of unread and unprocessed messages if available
  void set_title(const QString base, const qs_mail_map* status_map=NULL);

  void remove_children();

  int m_type;			/* a value from the enum below */
  enum item_type {
    tree_node,
    new_all,
    new_not_tagged,
    current_all,
    current_not_tagged,
    current_prio,
    current_tagged,
    archived_tagged,
    archived_untagged,
    virtfold_sent,
    virtfold_trashcan,
    user_defined
  };

  void set_type(enum item_type type) {
    m_type = type;
  }

  void show_count(int cnt1, int cnt2=0);
  void show_archived_count(int cnt1, bool show_zero=true);

  QString m_sql;
  int m_unique_id;
  
  /* Text for the tooltips for leafs inside the 'Current messages' branch */
  static QString current_tooltip_text(int nb, int unread);

private:
  static int id_generator;
  QString m_name;

  // case-insensitive sort
  bool operator<(const QTreeWidgetItem &other) const {
    int column = treeWidget()->sortColumn();
    return text(column).toLower() < other.text(column).toLower();
  }
};

class query_tag_lvitem : public query_lvitem
{
public:
  query_tag_lvitem(QTreeWidgetItem* parent, int item_type, QString name, uint id=0) :
  query_lvitem(parent,item_type,name), m_tag_id(id)
  {
  }

  // use this constructor only for the root of tags
  query_tag_lvitem(QTreeWidget* parent, int item_type, QString name) :
  query_lvitem(parent,name), m_tag_id(0)
  {
    m_type = item_type;
  }

  uint m_tag_id;
};

class query_listview: public QTreeWidget
{
  Q_OBJECT
public:
  query_listview(QWidget* parent=0);
  virtual ~query_listview();
  void init();
  void set_focus_on_id(int id);
  int current_id();
  void clear_selection();
  void refresh();
  void mail_status_changed(mail_msg*,int);
  void mail_tag_changed(const mail_msg&, uint tag_id, bool added);

  void archive_tag_changed(int tag_id, int diff);

  int highlight_entry(query_lvitem::item_type type, uint tag_id=0);
  static bool has_unread_messages(const qs_mail_map*);
  static int count_unread_messages(const qs_mail_map*);


public slots:
  // Update the counters for archived mail due to transitions
  void change_archive_counts(const QList<tag_counter_transition> cnt_tags_changed);

  void reload_user_queries();
  void tags_restructured();
  void got_new_mail(mail_id_t);
  void context_menu(const QPoint&);

protected:
  void mousePressEvent(QMouseEvent*);

private:
  int m_all_unread_count;
  int m_all_unprocessed_count;
  int m_unread_untagged_count;
  //int m_unprocessed_untagged_count;
  int m_unprocessed_prioritized_count;
  enum {
    // vertical index along the main tree
    // Currently only the "Current" branch is mentioned because it's the only
    // that can get recreated
    index_branch_current=1
  };

  query_lvitem* create_branch_current(const tag_node* root);

  // Update the counters for current mail due to mail status transitions or mail deletion
  void update_status_counters();

  bool fetch_tag_map();
  void add_current_tag(uint);
  int map_count(int mask_not_set, uint tag_id);

  /* expand sub-nodes down to depth */
  void expand_depth(query_lvitem* start_item, int depth);

  void free_mail_maps();
  void make_item_current_tags(const tag_node* root);
  int insert_child_tags(tag_node* r, query_tag_lvitem* item, int type, QSet<uint>* set);
  void update_tag_current_counter(uint tag_id);
  void display_counter(query_lvitem::item_type type);
  void store_expanded_state(QTreeWidgetItem* parent, QSet<uint>* set);
  query_tag_lvitem* find_tag(query_lvitem* tree, uint tag_id);

  // set item to be current without emitting any signal. If item=NULL,
  // clear the selection
  void set_item_no_signal(query_lvitem* item);

  /* m_tagged: a map tag_id=>qs_mail_map* m, where 'm' is a map:
     mail_id=>status. Only for current messages (not archived). */
  qs_tag_map m_tagged;

  priority_map m_prio_map;

  /* tag=>count for archived messages */
  archived_tag_map m_archtag_map;

  // built-in query branches
  query_tag_lvitem* m_item_tags;
  query_lvitem* m_item_archived;
  query_lvitem* m_item_archived_untagged;
  query_lvitem* m_item_current_tags;
  query_lvitem* m_item_new_all;
  query_lvitem* m_item_current_all;
  query_lvitem* m_item_new_untagged;
  query_lvitem* m_item_current_untagged;
  query_lvitem* m_item_current;
  query_lvitem* m_item_current_prio;
  query_lvitem* m_item_user_queries;
  query_lvitem* m_item_virtfold_sent;
  query_lvitem* m_item_virtfold_trashcan;

signals:
  void run_selection_filter(const msgs_filter& f);
};

#endif // INC_QUERY_LISTVIEW_H
