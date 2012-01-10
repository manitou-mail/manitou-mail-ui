/* Copyright (C) 2004-2010 Daniel Verite

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

#include "main.h"
#include "db.h"
#include "sqlstream.h"
#include "tags.h"
#include <QStringList>

// separator between parent and child tag
// unicode right arrow
#define TAG_SEPARATOR QChar(0x2192)
//#define TAG_SEPARATOR QChar(0x2799)

//static
std::map<int,message_tag> tags_repository::m_tags_map;

int tags_repository::m_tags_map_fetched=0;

/* Fill in the cache */
void
tags_repository::fetch()
{
  db_cnx db;
  try {
    sql_stream s("SELECT tag_id,name,coalesce(parent_id,0) FROM tags", db);
    while (!s.eof()) {
      int id, parent_id;
      QString name;
      s >> id >> name >> parent_id;
      message_tag tag(id, name);
      if (parent_id)
	tag.set_parent_id(parent_id);
      m_tags_map[id] = tag;
    }
    m_tags_map_fetched=true;
  }
  catch(db_excpt& p) {
    m_tags_map.clear();
    m_tags_map_fetched=false;
    DBEXCPT(p);
  }
}


//static
/*
void
tags_repository::get_sorted_list(QStringList* taglist)
{
  if (!m_tags_map_fetched) {
    fetch();
  }
  std::map<int,QString>::const_iterator iter;
  for (iter=tags_repository::m_tags_map.begin();
       iter!=tags_repository::m_tags_map.end();
       ++iter) {
    taglist->push_front(iter->second);
  }
  taglist->sort();
}
*/

/* Empty the cache */
void
tags_repository::reset()
{
  m_tags_map.clear();
  m_tags_map_fetched = 0;
}

/*
  Return a list of tag names from a list of tag id's (<id_list>),
  sorted case-insensitively
*/
QStringList
tags_repository::names_list(std::list<uint>& id_list)
{
  if (!m_tags_map_fetched) {
    fetch();
  }
  QStringList result;
  std::map<int,message_tag>::const_iterator iterm;
  std::list<uint>::const_iterator iterl = id_list.begin();
  QMap<QString, QString> m;
  for (; iterl != id_list.end(); ++iterl) {
    iterm = m_tags_map.find((int)*iterl);
    if (iterm != m_tags_map.end()) {
      QString tag = tags_repository::hierarchy(iterm->second.id());
      m.insert(tag.toLower(), tag);
    }
  }
  // this returns the strings in the order of the map according to
  // Qt's documentation
  return m.values();
}

QString
tags_repository::name(int id)
{
  if (!m_tags_map_fetched) {
    fetch();
  }
  std::map<int,message_tag>::const_iterator i;
  i = m_tags_map.find(id);
  if (i!=m_tags_map.end())
    return i->second.name();
  else
    return "";
}

/*
  Return the name of the tag including its parent hierarchy
*/
QString
tags_repository::hierarchy(int id)
{
  if (!m_tags_map_fetched) {
    fetch();
  }
  std::map<int,message_tag>::const_iterator i;
  i = m_tags_map.find(id);
  if (i == m_tags_map.end())
    return "";

  QString s;
  int parent_id = i->second.parent_id();
  if (parent_id) {
    s = hierarchy(parent_id);
    s.append(TAG_SEPARATOR); //"->");
    s.append(i->second.name());
  }
  else
    s=i->second.name();
  return s;
}

//static
QString
message_tag::convert_separator_to_ascii(const QString& orig)
{
  QString s=orig;
  s.replace(TAG_SEPARATOR, "->");
  return s;
}

//static
QString
message_tag::convert_separator_from_ascii(const QString& orig)
{
  QString s=orig;
  s.replace("->", TAG_SEPARATOR);
  return s;
}

//static
bool
message_tag::is_valid_name(const QString name, QString* errmsg/*=NULL*/)
{
  if (name.indexOf("->")>=0) {
    if (errmsg) {
      *errmsg = QObject::tr("A tag's name cannot contain '->' since it is used for expressing tag hierarchies");
    }
    return false;
  }
  if (name=="/") {
    if (errmsg)
      *errmsg = QObject::tr("/ is not allowed as a name for a tag");
    return false;
  }
  return true;
}

bool
message_tag::store()
{
  bool result=true;
  try {
    db_cnx db;
    if (m_tag_id<=0) {
      db.next_seq_val("seq_tag_id", &m_tag_id);
      sql_stream s("INSERT INTO tags(tag_id,name,parent_id) VALUES (:p1,:p2,:p3)", db);
      s << m_tag_id << m_name;
      if (m_parent_id)
	s << m_parent_id;
      else
	s << sql_null();
    }
    else {
      sql_stream s("UPDATE tags SET name=':p1',parent_id=:p3 where tag_id=:p2", db);
      s << m_name;
      if (m_parent_id)
	s << m_parent_id;
      else
	s << sql_null();
      s << m_tag_id;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
message_tag::remove()
{
  bool result=true;
  db_cnx db;
  try {
    DBG_PRINTF(5,"message_tag::remove(%u)\n", getId());
    db.begin_transaction();
    sql_stream s1("DELETE FROM mail_tags WHERE tag=:p1", db);
    s1 << getId();
    sql_stream s2("DELETE FROM tags WHERE tag_id=:p1", db);
    s2 << getId();
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
message_tag::reload()
{
  bool result=true;
  db_cnx db;
  try {
    sql_stream s("SELECT name,parent_id from tags WHERE tag_id=:p1", db);
    s << m_tag_id;
    if (!s.eos()) {
      s >> m_name >> m_parent_id;
    }
    else {
      m_name=QString::null;
      m_parent_id=0;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}

bool
tags_definition_list::fetch(bool force /*=false*/)
{
  bool result=true;
  if (m_bFetched && !force)
    return true;

  try {
    db_cnx db;
    sql_stream s ("SELECT tag_id,name,parent_id FROM tags ORDER BY name", db);
    while (!s.eos()) {
      int id, parent_id;
      QString name;
      s >> id >> name >> parent_id;
      message_tag tag(id, name);
      tag.set_parent_id(parent_id);
      push_back(tag);
    }
    m_bFetched=true;
  }
  catch (db_excpt& p) {
    DBEXCPT(p);
    result=false;
  }
  return result;
}


tag_node::~tag_node()
{
  clear();
}

void
tag_node::clear()
{
  std::list<tag_node*>::iterator iter;
  for (iter=m_childs.begin(); iter!=m_childs.end(); ++iter) {
    (*iter)->clear();
    delete *iter;
  }
  m_childs.clear();
}

tag_qitem::tag_qitem(QListWidget* parent, tag_node* n, const QString txt_entry) :
  QListWidgetItem(parent), m_node(n)
{
  setText(txt_entry);
  m_id=n->id();
}

tag_qitem::~tag_qitem()
{
}

tag_selector::tag_selector(QWidget* parent) : QComboBox(parent)
{
}

tag_selector::~tag_selector()
{
}

void
tag_node::get_child_tags(tags_definition_list& l)
{
  tags_definition_list::iterator iter;
  for (iter=l.begin(); iter!=l.end(); ++iter) {
    if (iter->parent_id()==(int)m_id) {
      tag_node* t = new tag_node(this);
      t->m_id = iter->getId();
      t->m_name = iter->getName();
      t->m_parent_id = this->m_id;
      this->m_childs.push_back(t);
      t->get_child_tags(l);
    }
  }
}

void
tag_selector::insert_childs(tag_node* n, int level)
{
  std::list<tag_node*>::iterator iter;
  for (iter=n->m_childs.begin(); iter!=n->m_childs.end(); ++iter) {
    QString s=(*iter)->hierarchy();
    addItem(s, QVariant((*iter)->id()));
    if (!(*iter)->m_childs.empty()) {
      insert_childs(*iter, level+1);
    }
  }
}

bool
tag_selector::init(bool first_is_empty)
{
  setEditable(false);
  tags_definition_list l;
  if (!l.fetch())
    return false;

  clear();
  m_root.clear();
  if (first_is_empty) {
    // An empty entry at first position meaning "no selection made"
    addItem(QString(""), QVariant(0));
  }

  m_root.get_child_tags(l);
  insert_childs(&m_root, 0);

  return true;
}

const tag_node*
tag_selector::selected_node() const
{
  int idx=currentIndex();
  if (idx<0) return NULL;
  QVariant v=itemData(idx);
  return m_root.find(v.toInt());
}

uint
tag_selector::current_tag_id() const
{
  const tag_node* n = selected_node();
  return n ? n->id(): 0;
}

void
tag_selector::set_current_tag_id(uint id)
{
  int idx=findData(QVariant(id));
  if (idx>=0) {
    setCurrentIndex(idx);
  }
}

/*
  Return the name of the tag including its parent hierarchy
*/
QString
tag_node::hierarchy() const
{
  QString s=m_name;
  tag_node* p=m_parent_node;
  while (p) {
    if (p->id()!=0) {		// skip the root node, which has an id=0 and no name
      s.prepend(TAG_SEPARATOR); //"->");
      s.prepend(p->name());
    }
    p=p->parent_node();
  }
  return s;
}

const tag_node*
tag_node::find(uint tag_id) const
{
  if (tag_id==0) return NULL;	// special case, we never want to find the root
  if (tag_id==m_id) return this;
  std::list<tag_node*>::const_iterator iter;
  for (iter=m_childs.begin(); iter!=m_childs.end(); ++iter) {
    if ((*iter)->id()==tag_id)
      return *iter;
    const tag_node* n = (*iter)->find(tag_id);
    if (n) return n;
  }
  return NULL;
}
