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

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialog>

#include "db.h"
#include "tags.h"

#include <set>

class QPushButton;
class QDropEvent;
class QCloseEvent;

class tag_item: public QTreeWidgetItem
{
public:
  tag_item(QTreeWidget* q, const message_tag& mq);
  tag_item(QTreeWidgetItem* q, const message_tag& mq);
  ~tag_item();
  message_tag& tag() {
    return m_tag;
  }
  const message_tag& tagconst() const {
    return m_tag;
  }
  void set_dirty(bool b) {
    m_dirty=b;
  }
  bool is_dirty() const {
    return m_dirty;
  }
  QString fullname() const;

private:
  // the tag object itself
  message_tag m_tag;
  bool m_dirty;
  void init(const message_tag&);

  // case-insensitive sort
  bool operator<(const QTreeWidgetItem &other) const {
    int column = treeWidget()->sortColumn();
    return text(column).toLower() < other.text(column).toLower();
  }
};

class tags_treeview: public QTreeWidget
{
  Q_OBJECT
public:
  tags_treeview(QWidget* parent);
  virtual ~tags_treeview();
  tag_item* find_new_item();
  void set_tag_default_name(const QString& name) {
    m_tag_default_name=name;
  }
  QTreeWidgetItem* last_failed;
protected:
  Qt::DropActions supportedDropActions() const;
  void dropEvent(QDropEvent* event);
private:
  QString m_tag_default_name;
public slots:
  void reedit();
};

class tags_dialog : public QDialog
{
  Q_OBJECT
public:
  tags_dialog (QWidget* parent=0);
  ~tags_dialog ();
private:
  void collect_childs(tag_item*, std::set<int>&);
  tags_treeview* m_qlist;
  QPushButton* m_btn_new;
  QPushButton* m_btn_edit;
  QPushButton* m_btn_delete;
  tags_definition_list m_list;
  std::set<int> m_delete;	// set of tags to delete from db
  message_tag m_root_tag;
  tag_item* m_root_item;
  QString m_new_tag_default_name;
protected:
  virtual void closeEvent(QCloseEvent*);
  
private slots:
  // called when one of the buttons (New,Edit,Delete) is clicked
  void action_new();
  void action_delete();
  void action_edit();
  void sel_changed();
  void item_changed(QTreeWidgetItem * item, int col);
  void ok();
signals:
  void tags_restructured();

private:
  int m_new_id;
};
