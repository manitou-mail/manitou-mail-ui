/* Copyright (C) 2004-2023 Daniel Verite

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

#ifndef INC_TAGS_H
#define INC_TAGS_H

#include <QString>
#include <list>
#include <map>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include "line_edit_autocomplete.h"

class QStringList;
class QMouseEvent;

/*
  A message_tag, also called just 'tag', is an object that can be used
  to mark and classify mail.  Tags are organized in a tree-like
  hierarchy.
  This class manages the tag definition and properties, not
  the association with any particular mail message.
*/
class message_tag
{
public:
  message_tag(): m_tag_id(0), m_parent_id(0) {}
  message_tag(int id, QString name) {
    m_name=name;
    m_tag_id=id;
    m_parent_id=0;
  }
  virtual ~message_tag() {}
  bool remove();
  bool reload();
  int getId() const { return m_tag_id; }
  int id() const { return m_tag_id; }
  const QString& getName() const { return m_name; }
  const QString& get_name() const { return m_name; }
  const QString& name() const { return m_name; }
  void setName(const QString s) { m_name=s; }
  void set_name(const QString s) { m_name=s; }
  static int nameMaxLength() { return 300; }
  void set_parent_id(int id) { m_parent_id=id; }
  int parent_id() const { return m_parent_id; }
  bool store();
  static bool is_valid_name(const QString name, QString* errmsg=NULL);

  /* return a string representing the tag hierarchy with the separator
     between hierarchical levels as an ascii string, as opposed to
     an unicode character */
  static QString convert_separator_to_ascii(const QString&);

  /* Reverse of convert_separator_to_ascii() */
  static QString convert_separator_from_ascii(const QString&);

private:
  int m_tag_id;
  int m_parent_id;
  QString m_name;
  // number of tagged messages, -1 if unknown
};

/** Container for a list of tags. It's used to contain all the tags of the
 * current mail database in static storage.
 */
class tags_repository
{
public:
  static QStringList names_list(std::list<uint>&);
  static std::map<int,message_tag> m_tags_map;
  static int m_tags_map_fetched;
  static void fetch();
  static void reset();
  //  static void get_sorted_list(QStringList*);
  static QString name (int id);
  static QString hierarchy(int id, QString sep=QString(QChar(0x2192)));
  static int depth(int id);
  /* search a tag by name (including hierarchy) in the repository */
  static int hierarchy_lookup(QString fullname);

  /* search tags matching a substring */
  static QList<QString> search_substring(QString substring);

};

/* Another container for a list of tags. FIXME: merge with tags_repository */
class tags_definition_list : public std::list<message_tag>
{
public:
  tags_definition_list() : m_bFetched(false) {}
  virtual ~tags_definition_list() {}
  bool fetch(bool force=false);
  /* When multi-user control access is used through Row-Level Security,
     return the list of tag_id that the connected user should not see
     according to identities_permissions and identities.root_tag
     Child tags are not in that list, only the top-level tag_id of each
     subtree to eliminate. */
  bool is_excluded_subtree(int tag_id);
private:
  bool m_bFetched;
  QSet<int> m_excluded_by_identity;
};

class tag_node
{
public:
  tag_node(tag_node* parent=0) : m_parent_id(0), m_id(0), m_parent_node(parent) {}
  virtual ~tag_node();
  const QString name() const {
    return m_name;
  }
  uint id() const {
    return m_id;
  }
  uint parent_id() const {
    return m_parent_id;
  }
  tag_node* parent_node() const {
    return m_parent_node;
  }
  int depth() const;
  void clear();
  QString hierarchy() const;
  void get_child_tags(tags_definition_list& l);
  const tag_node* find(uint tag_id) const;
  QString m_name;
  std::list<tag_node*> m_childs;
  uint m_parent_id;
  uint m_id;
  tag_node* m_parent_node;
};

class tag_qitem : public QListWidgetItem
{
public:
  tag_qitem(QListWidget* parent, tag_node* n, const QString txt_entry);
  virtual ~tag_qitem();
  uint tag_id() const {
    return m_id;
  }
  uint m_id;
  const tag_node* node() const {
    return m_node;
  }
private:
  tag_node* m_node;
};

class tag_selector : public QComboBox
{
public:
  tag_selector(QWidget* parent);
  virtual ~tag_selector();
  bool init(bool first_is_empty);
  const tag_node* selected_node() const;
  uint current_tag_id() const;
  void set_current_tag_id(uint id);
private:
  void insert_childs(tag_node* n, int level);
  tag_node m_root;
};


/* Reflects a change in tags_counters */
class tag_counter_transition
{
public:
  tag_counter_transition() : tag_id(0), count_change(0) {}
  tag_counter_transition(int id, int c) : tag_id(id), count_change(c) {}
  int tag_id;
  int count_change; // positive or negative
};

class tag_line_edit_selector: public line_edit_autocomplete
{
  // works with "->" as tag hierarchy separator
public:
  tag_line_edit_selector(QWidget* parent=NULL) : line_edit_autocomplete(parent) {
    set_popup_delay(250);
  }
  int current_tag_id() const {
    return tags_repository::hierarchy_lookup(this->text().trimmed());
  }
  void set_current_tag_id(int id) {
    this->setText(tags_repository::hierarchy(id, "->"));
  }

  QList<QString> get_completions(const QString prefix) {
    return tags_repository::search_substring(prefix.trimmed());
  }

  QList<QString> get_all_completions() {
    return tags_repository::search_substring(QString());
  }

  virtual void mousePressEvent(QMouseEvent*);

  int get_prefix_pos(const QString text, int cursor_pos) {
    Q_UNUSED(text);
    return (cursor_pos==0) ? -1 : 0;
  }
};


#endif // INC_TAGS_H
