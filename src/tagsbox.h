/* Copyright (C) 2004-2016 Daniel Verite

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

#ifndef INC_TAGSBOX_H
#define INC_TAGSBOX_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QBrush>

#include "db.h"
#include "tags.h"

#include <list>

class QPainter;
class tags_box_widget;

class tag_lvitem : public QTreeWidgetItem
{
public:
  tag_lvitem(QTreeWidget* parent, message_tag* mt, tags_box_widget*);
  tag_lvitem(tag_lvitem* parent, message_tag* mt, tags_box_widget*);
  virtual ~tag_lvitem();
  int tag_id() const {
    return m_id;
  }
  void set_on(bool b);
  tag_lvitem* parent() const {
    return static_cast<tag_lvitem*>(QTreeWidgetItem::parent());
  }
  Qt::CheckState last_state() const {
    return last_known_state;
  }
  void update_last_state();
  bool is_on() const;
  void colorize();
private:
  int m_id;
  tags_box_widget* m_owner;
  bool hierarchy_checkstate() const;
  Qt::CheckState last_known_state;
private:
  // case-insensitive sort
  bool operator<(const QTreeWidgetItem &other) const {
    int column = treeWidget()->sortColumn();
    return text(column).toLower() < other.text(column).toLower();
  }
};

class tags_box_widget : public QWidget
{
  Q_OBJECT
public:
  tags_box_widget(QWidget* parent=0);
  virtual ~tags_box_widget();
  void reset_tags();
  void set_tag (int tag_id);
  void set_tags (const std::list<uint>& l);
  void unset_tag (int tag_id);
  tag_lvitem* find(int id);
  void get_selected(std::list<uint>& sel_list);
  void reinit();
  const QBrush& default_brush() {
    return m_default_brush;
  }
private:
  tags_definition_list m_list;
  message_tag* get_tag_by_id(uint id);
  void fill_listview();
public slots:
  void toggle_tag_state(QTreeWidgetItem* item, int column);
  void tags_restructured();
private:
  QBrush m_default_brush;
  QTreeWidget* m_lv;
signals:
  void state_changed(int tag_id, bool checked);
  void state_changed_denied(int tag_id, bool checked);
};

#endif // INC_TAGSBOX_H
