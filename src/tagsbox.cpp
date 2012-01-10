/* Copyright (C) 2004-2009 Daniel Vérité

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
#include "tagsbox.h"
#include "errors.h"
#include "message_port.h"
#include <map>

#include <QPainter>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QColor>
#include <QBrush>

tags_box_widget::tags_box_widget(QWidget* parent): QWidget(parent)
{
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setMargin(2);
  m_list.fetch();
  m_lv = new QTreeWidget(this);
  m_lv->setRootIsDecorated(true);
  m_lv->setHeaderLabel(tr("Message Tags"));
  connect(m_lv, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
	  this, SLOT(toggle_tag_state(QTreeWidgetItem*,int)));
  //  m_lv->setMultiSelection (true);

  try {
    fill_listview();
  }
  catch (ui_error e) {
    QMessageBox::information(this, APP_NAME, e.msg());
  }
  //  m_list_tag_boxes.push_back(this);

  m_default_brush = QBrush(QColor(0,0,0));

  top_layout->addWidget(m_lv);
  message_port::connect_receiver(SIGNAL(tags_restructured()), this,
				 SLOT(tags_restructured()));
}

void
tags_box_widget::fill_listview()
{
  tags_definition_list::iterator iter;
  std::map<int,tag_lvitem*> map_done;
  std::map<int,tag_lvitem*>::iterator ip;
  int cnt=m_list.size();
  int prev_cnt=cnt+1;

  m_lv->blockSignals(true);	// prevents itemChanged signals
  while (cnt!=0 && cnt<prev_cnt) {
    prev_cnt=cnt;
    for (iter=m_list.begin(); iter!=m_list.end(); ++iter) {
      ip = map_done.find(iter->getId());
      if (ip==map_done.end()) {
	if (!iter->parent_id()) {
	  map_done[iter->getId()] = new tag_lvitem(m_lv, &(*iter), this);
	  cnt--;
	}
	else {
	  ip = map_done.find(iter->parent_id());
	  if (ip!=map_done.end()) {
	    // if the parent exists, then create the child
	    map_done[iter->getId()] = new tag_lvitem(ip->second, &(*iter), this);
	    ip->second->setExpanded(true);
	    cnt--;
	  }
	}
      }
    }
  }

  m_lv->sortItems(0, Qt::AscendingOrder);
  m_lv->blockSignals(false);
  if (cnt>0) {
    DBG_PRINTF(2, "cnt=%d, prev_cnt=%d", cnt, prev_cnt);
    m_lv->clear();
    throw ui_error(tr("Inconsistant hierarchy in 'tags' table"));
  }
}

tag_lvitem*
tags_box_widget::find(int id)
{
  QTreeWidgetItemIterator it(m_lv);
  for ( ; *it; ++it) {
    tag_lvitem* item = (tag_lvitem*)*it;
    if (item->tag_id()==id)
      return item;
  }
  return NULL;
}

void
tags_box_widget::set_tag(int tag_id)
{
  DBG_PRINTF(5,"tags_box_widget::set_tag(%d)", tag_id);
  tag_lvitem* item = find(tag_id);
  if (item) {
    item->set_on(true);
  }
  else {
    DBG_PRINTF(5,"Error: can't find tag_lvitem with id=%d in tagsbox", tag_id);
  }
}

void
tags_box_widget::get_selected(std::list<uint>& sel_list)
{
  QTreeWidgetItemIterator it(m_lv);
  sel_list.clear();
  for ( ; *it; ++it) {
    tag_lvitem* item = (tag_lvitem*)*it;
    if (item->is_on())
      sel_list.push_back(item->tag_id());
  }
}

void
tags_box_widget::unset_tag(int tag_id)
{
  DBG_PRINTF(5,"tags_box_widget::unset_tag(%d)", tag_id);
  tag_lvitem* item = find(tag_id);
  if (item) {
    item->set_on(false);
  }
  else {
    DBG_PRINTF(5,"Error: can't tag_lvitem button with id=%d in tagsbox", tag_id);
  }
}

tags_box_widget::~tags_box_widget()
{
}

void
tags_box_widget::reset_tags()
{
  m_lv->clearSelection();
  QTreeWidgetItemIterator it(m_lv);
  for ( ; *it; ++it) {
    tag_lvitem* item = (tag_lvitem*)*it;
    item->set_on(false);
  }
}

void
tags_box_widget::set_tags(const std::list<uint>& l)
{
  reset_tags();
  std::list<uint>::const_iterator iter;
  for (iter = l.begin(); iter != l.end(); iter++) {
    set_tag((int)*iter);
  }
}

message_tag*
tags_box_widget::get_tag_by_id(uint id)
{
  tags_definition_list::iterator iter;
  for (iter=m_list.begin(); iter!=m_list.end(); iter++) {
    if (iter->getId()==(int)id) {
      return &(*iter);
    }
  }
  return NULL;
}

// slot. To be called when the definition of the tags tree has changed
void
tags_box_widget::tags_restructured()
{
  reinit();
}

// Reload the tags and reconstruct the tree, keeping the selection
// if possible
void
tags_box_widget::reinit()
{
  DBG_PRINTF(5, "tags_restructured(this=%p)", this);
  std::list<uint> sel;
  get_selected(sel);
  m_list.clear();
  m_lv->clear();
  m_list.fetch(true);
  try {
    fill_listview();
  }
  catch (ui_error& e) {
    e.display();
  }
  set_tags(sel);
}

void
tags_box_widget::toggle_tag_state(QTreeWidgetItem* item, int column)
{
  DBG_PRINTF(5, "toggle_tag_state");
  if (column!=0)
    return;
  tag_lvitem* tag_item = dynamic_cast<tag_lvitem*>(item);
  if (!tag_item)
    return;
  if (tag_item->checkState(0) != tag_item->last_state()) {
    tag_item->update_last_state();
    tag_item->colorize();
    emit state_changed(tag_item->tag_id(), (tag_item->checkState(0)==Qt::Checked));
  }
}

tag_lvitem::tag_lvitem(QTreeWidget* parent, message_tag* mt,
		       tags_box_widget* owner) :
  QTreeWidgetItem(parent, QStringList(mt->getName()))
{
  m_id=mt->id();
  m_owner=owner;
  setCheckState(0, Qt::Unchecked);
  last_known_state=Qt::Unchecked;
}

tag_lvitem::tag_lvitem(tag_lvitem* parent, message_tag* mt,
		       tags_box_widget* owner) :
  QTreeWidgetItem(parent, QStringList(mt->getName()))
{
  m_id=mt->id();
  m_owner=owner;
  setCheckState(0, Qt::Unchecked);
  last_known_state=Qt::Unchecked;
}

tag_lvitem::~tag_lvitem()
{
}

void
tag_lvitem::update_last_state()
{
  last_known_state=checkState(0);
}

void
tag_lvitem::colorize()
{
  // change the color depending on the current item and its children
  // states
  if (hierarchy_checkstate()) {
    setForeground(0, QBrush(QColor(238, 100, 20))); // orange/red
  }
  else {
    setForeground(0, m_owner->default_brush());
  }
  // change the color up in the hierarchy
  tag_lvitem* p=parent();
  while (p) {
    p->colorize();
    p=p->parent();
  }
}

bool
tag_lvitem::hierarchy_checkstate() const
{
  if (checkState(0)==Qt::Checked)
    return true;
  for (int i=0; i<childCount(); ++i) {
    tag_lvitem* item = static_cast<tag_lvitem*>(child(i));
    if (item->hierarchy_checkstate())
      return true;
  }
  return false;
}

void
tag_lvitem::set_on(bool b)
{
  treeWidget()->blockSignals(true);
  if ((checkState(0)==Qt::Unchecked && b==true) ||
      (checkState(0)==Qt::Checked && b==false)) {
    DBG_PRINTF(5, "set_on changes state");
    setCheckState(0, b ? Qt::Checked : Qt::Unchecked);
    last_known_state = (b ? Qt::Checked : Qt::Unchecked);
    colorize();
  }
  treeWidget()->blockSignals(false);
}

bool
tag_lvitem::is_on() const
{
  return checkState(0)==Qt::Checked;
}

