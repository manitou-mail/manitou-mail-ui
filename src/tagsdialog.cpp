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

#include "tagsdialog.h"

#include <QLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDropEvent>
#include <QCloseEvent>
#include <QTimer>

#include "main.h"
#include "db.h"
#include "tags.h"
#include "errors.h"
#include "message_port.h"
#include "icons.h"

//
// tags_dialog
//
tags_dialog::tags_dialog(QWidget* parent) : QDialog(parent)
{
  m_new_tag_default_name = tr("(New Tag)");
  m_new_id=-1;
  //  setWFlags(getWFlags() | Qt::WDestructiveClose);
  setWindowTitle(tr("Message Tags"));
  setWindowIcon(UI_ICON(FT_ICON16_TAGS));
  resize(400, 600);
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setMargin(5);
  top_layout->setSpacing(5);

  m_qlist=new tags_treeview(this);
  m_qlist->set_tag_default_name(m_new_tag_default_name);
  m_qlist->setRootIsDecorated(true);

  top_layout->addWidget(m_qlist);

  m_qlist->setHeaderLabel(tr("Tag name"));
  connect(m_qlist, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	  this, SLOT(sel_changed()));

  QHBoxLayout* hb = new QHBoxLayout();
  top_layout->addLayout(hb);
  hb->setSpacing(5);
  m_btn_new = new QPushButton(tr("New"));
  m_btn_edit = new QPushButton(tr("Edit"));
  m_btn_delete = new QPushButton(tr("Delete"));
  hb->addWidget(m_btn_new);
  hb->addWidget(m_btn_edit);
  hb->addWidget(m_btn_delete);
  connect(m_btn_new, SIGNAL(clicked()), this, SLOT(action_new()));
  connect(m_btn_edit, SIGNAL(clicked()), this, SLOT(action_edit()));
  connect(m_btn_delete, SIGNAL(clicked()), this, SLOT(action_delete()));

  QHBoxLayout* hbl = new QHBoxLayout();
  top_layout->addLayout(hbl);

  hbl->addStretch(1);
  QPushButton* bclose=new QPushButton(tr("OK"), this);
  hbl->addWidget(bclose);
  connect(bclose, SIGNAL(clicked()), this, SLOT(ok()));
  hbl->setStretchFactor(bclose, 3);
  hbl->addStretch(1);
  QPushButton* bcancel=new QPushButton(tr("Cancel"), this);
  connect(bcancel, SIGNAL(clicked()), this, SLOT(reject()));
  hbl->addWidget(bcancel);
  hbl->setStretchFactor(bcancel, 3);
  hbl->addStretch(1);

  m_root_tag.setName(tr("(Root)"));
  m_root_item = new tag_item(m_qlist, m_root_tag);
  m_root_item->setFlags(m_root_item->flags() & ~(Qt::ItemIsEditable|Qt::ItemIsDragEnabled));
  m_root_item->setExpanded(true);

  m_list.fetch();
  int cnt=m_list.size();
  tags_definition_list::iterator iter;
  std::map<int,tag_item*> map_done;
  std::map<int,tag_item*>::iterator ip;
  int prev_cnt=cnt+1;

  while (cnt!=0 && cnt<prev_cnt) {
    prev_cnt=cnt;
    for (iter=m_list.begin(); iter!=m_list.end(); ++iter) {
      ip = map_done.find(iter->getId());
      if (ip==map_done.end()) {
	if (!iter->parent_id()) {
	  map_done[iter->getId()] = new tag_item(m_root_item, *iter);
	  cnt--;
	}
	else {
	  ip = map_done.find(iter->parent_id());
	  if (ip!=map_done.end()) {
	    // if the parent exists, then create the child
	    map_done[iter->getId()] = new tag_item(ip->second, *iter);
	    ip->second->setExpanded(true);
	    cnt--;
	  }
	}
      }
    }
  }
  if (cnt>0) {
    DBG_PRINTF(2, "cnt=%d, prev_cnt=%d", cnt, prev_cnt);
    m_qlist->clear();
    throw ui_error(tr("Inconsistant hierarchy in 'tags' table"));
  }

  m_qlist->sortItems(0, Qt::AscendingOrder);
  sel_changed();

  connect(m_qlist, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
	  this, SLOT(item_changed(QTreeWidgetItem*, int)));

  message_port::connect_sender(this, SIGNAL(tags_restructured()), SLOT(tags_updated()));

}

tags_dialog::~tags_dialog()
{
}


void
tags_dialog::closeEvent(QCloseEvent* event)
{
  bool need_save = false;

  QTreeWidgetItemIterator it(m_qlist);
  for ( ; !need_save && *it; ++it) {
    tag_item* item = dynamic_cast<tag_item*>(*it);
    need_save = item->is_dirty();
  }

  if (need_save) {
    QString msg = tr("There are unsaved changes.\nSave now?");
    int rep = QMessageBox::question(this, tr("Confirmation"), msg, QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);

    if (rep==QMessageBox::Cancel) {
      event->ignore();
      return;
    }
    if (rep==QMessageBox::Save) {
      event->ignore();
      ok();
    }
    else
      event->accept();
  }
}

void
tags_dialog::item_changed(QTreeWidgetItem* item, int col)
{
  Q_UNUSED(col);
  bool cancel=false;
  tag_item* q = static_cast<tag_item*>(item);
  message_tag& t = q->tag();
  if (q->text(0).trimmed().isEmpty()) {
    // rule: no empty tag name
    QMessageBox::critical(this, tr("Error"), tr("A tag name cannot be empty or contain only spaces"));
    cancel=true;
  }
  else {
    // rule: no name duplicate at the same level of hierarchy
    QString comp_name = q->text(0).trimmed().toLower();
    QTreeWidgetItem* parent = item->parent();
    if (parent) {
      int index = parent->indexOfChild(item);
      for (int i=0; i<parent->childCount() && !cancel; i++) {
	if (index!=i && parent->child(i)->text(0).trimmed().toLower() == comp_name) {
	  QMessageBox::critical(this, tr("Error"),
				tr("This name is already used for another tag of the same branch"));
	  cancel=true;
	}
      }
    }
  }
	
  if (cancel) {
    /* Keep editing with the old value. */
    bool isblocked = item->treeWidget()->blockSignals(true);
    q->setText(0, t.name());
    item->treeWidget()->blockSignals(isblocked);
    m_qlist->last_failed = item;
    QTimer::singleShot(0, m_qlist, SLOT(reedit()));
  }
  else {
    t.set_name(q->text(0));
    q->set_dirty(true);
  }
}

void
tags_dialog::action_new()
{
  tag_item* nitem;
  tag_item* item = dynamic_cast<tag_item*>(m_qlist->currentItem());
  if (!item)
    item = m_root_item;

  message_tag mt(m_new_id--, m_new_tag_default_name);

  m_qlist->blockSignals(true); 	// prevents itemChanged signals
  nitem = new tag_item(item, mt);
  m_qlist->blockSignals(false);

  if (item->parent()) {
    nitem->tag().set_parent_id(item->tag().id());
  }
  m_qlist->setCurrentItem(nitem);
  m_qlist->scrollToItem(nitem);
  m_qlist->editItem(nitem);

  
}

void
tags_dialog::action_edit()
{
  QTreeWidgetItem* item = m_qlist->currentItem();
  if (item)
    m_qlist->editItem(item);
}



// Record the ids of all sub-tags and the parent
void
tags_dialog::collect_childs(tag_item* parent, std::set<int>& set)
{
  for (int i=0; i<parent->childCount(); ++i) {
    collect_childs((tag_item*)parent->child(i), set);
  }
  set.insert(parent->tag().id());
}

void
tags_dialog::action_delete()
{
  tag_item* t = dynamic_cast<tag_item*>(m_qlist->currentItem());
  if (!t) return;

  // remove tag
  if (t->tag().id()==0) {
    QMessageBox::warning(this, tr("Error"), tr("The root element cannot be deleted"));
    return;
  }

  QMessageBox box(this);
  box.setTextFormat(Qt::RichText);
  box.setIcon(QMessageBox::Warning);

  QString msg;
  QString fullname = t->fullname();
  if (t->childCount()==0) {
    msg = tr("Please confirm the removal of the tag:\n<center><b>%1</b></center>").arg(fullname);
  }
  else {
    msg = tr("Please confirm the removal of the tag:<br><center><b>%1</b></center><br> and all its sub-tags.").arg(fullname);
    box.setDetailedText(tr("This will include %1 sub-tag(s)").arg(t->childCount()));
  }
  box.setText(msg);
  box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  box.setDefaultButton(QMessageBox::Cancel);
  
  int rep = box.exec();
  if (rep==QMessageBox::Cancel)
    return;

   collect_childs(t, m_delete);
   // delete the UI object
   delete t;
}

void
tags_dialog::sel_changed()
{
  tag_item* q = (tag_item*) m_qlist->currentItem();
  // New child and Delete buttons are active when an item is selected
  m_btn_edit->setEnabled(q!=NULL);
  m_btn_delete->setEnabled(q!=NULL);
}


void
tags_dialog::ok()
{
  db_cnx db;
  bool update=false;

  try {
    db.begin_transaction();

    // Definitive removal
    std::set<int>::iterator itd;
    for (itd=m_delete.begin(); itd!=m_delete.end(); ++itd) {
      message_tag mt(*itd, QString::null);
      if (mt.remove()) {
	update=true;
      }
    }

    /* translation table of negative tag_id (=item in memory) to the
       corresponding positive id in database once the item has been
       inserted */
    std::map<int,int> trans_id;

    // New tags and renamed tags
    QTreeWidgetItemIterator it(m_qlist);
    for ( ; *it; ++it) {
      tag_item* item = dynamic_cast<tag_item*>(*it);
      if (item->is_dirty()) {
	update=true;
	message_tag& mq=item->tag();
	int prev_id=mq.id();
	if (mq.parent_id()<0) {
	  // if the parent has a negative id, find its positive counterpart
	  // the parent should be inserted in db already because we're
	  // scanning the tree from top to bottom
	  std::map<int,int>::iterator i2 = trans_id.find(mq.parent_id());
	  if (i2!=trans_id.end()) {
	    mq.set_parent_id(i2->second);
	  }
	}
	if (mq.store()) {
	  item->set_dirty(false);
	  if (prev_id != mq.id()) {
	    trans_id[prev_id] = mq.id();
	  }
	}
      }
    }

    // Commit and close the window
    db.commit_transaction();
    close();

    if (update) {
      tags_repository::reset();
      tags_repository::fetch();
      DBG_PRINTF(3, "emit tags_restructured");
      emit tags_restructured();
    }

    accept();			// QDialog::accept()
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
  }
}

//
// tag_item
//
tag_item::tag_item(QTreeWidget* q, const message_tag& mq): QTreeWidgetItem(q)


{
  init(mq);
}

tag_item::tag_item(QTreeWidgetItem* q, const message_tag& mq): QTreeWidgetItem(q)
{
  init(mq);
}

tag_item::~tag_item()
{
}

void
tag_item::init(const message_tag& mq)
{
  setFlags(flags()|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);
  setText(0, mq.getName());
  m_tag=mq;
  set_dirty(false);
}

QString
tag_item::fullname() const
{
  QString s=text(0);
  QTreeWidgetItem* p = parent();
  while (p && p->parent()) {		// skip the root node, which has an id=0 and no name
    s.prepend(QChar(0x2799)); //"->");
    s.prepend(p->text(0));
    p = p->parent();
  }
  return s;
}

//
// tags_treeview
//
tags_treeview::tags_treeview(QWidget* parent) : QTreeWidget(parent)
{
  setAcceptDrops(true);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  setSelectionMode(QAbstractItemView::SingleSelection);
}


tags_treeview::~tags_treeview()
{
}

Qt::DropActions
tags_treeview::supportedDropActions() const
{
  return Qt::MoveAction;/* | Qt::CopyAction;*/
}

void
tags_treeview::dropEvent(QDropEvent* event)
{
  DBG_PRINTF(5, "dropEvent");
  QList<QModelIndex> idxs = selectedIndexes();
  QTreeWidgetItem* dragged_item;
  tag_item* dragged_tag=NULL;
  if (idxs.size()==1) {
    dragged_item = itemFromIndex(idxs.at(0));
    if (dragged_item) {
      dragged_tag = dynamic_cast<tag_item*>(dragged_item);
    }
  }
  else
    return; // shouldn't happen since the treewidget is in single selection mode
    
  QModelIndex index = indexAt(event->pos());
  if (!index.isValid()) {
    event->setDropAction(Qt::IgnoreAction);
    return;
  }
  QTreeWidgetItem* item = itemFromIndex(index);
  tag_item* dest_item = dynamic_cast<tag_item*>(item);
  if (dest_item) {
    DBG_PRINTF(5, "tag dest=%s", dest_item->tagconst().name().toLocal8Bit().constData());
    // disallow moving a tag below its parent (it's a no-op)
    if (dest_item == dragged_item->parent()) {
      event->setDropAction(Qt::IgnoreAction);
      return;
    }
    if (dragged_item->parent() != NULL) { // should always be true
      // move = remove and insert
      int index = dragged_item->parent()->indexOfChild(dragged_item);
      tag_item* rem_item = dynamic_cast<tag_item*>(dragged_item->parent()->takeChild(index));
      dest_item->addChild(rem_item);
      dest_item->sortChildren(0, Qt::AscendingOrder);
      if (dragged_tag) {	// always true?
	dragged_tag->tag().set_parent_id(dest_item->tag().id());
	dragged_tag->set_dirty(true);
	setCurrentItem(dragged_tag);
      }
    }
  }
}

/* There should be only one new item that hasn't yet be renamed by the
   user from the default name. This function searches for this item
   and returns it */
tag_item*
tags_treeview::find_new_item()
{
  QTreeWidgetItemIterator it(this);
  QTreeWidgetItem* p;
  while ((p=*it)!=NULL) {
    tag_item* ti = static_cast<tag_item*>(p);
    // find an item without a tag_id but which is _not_ the root item
    if (ti->tag().id()<=0 && ti->parent() && ti->tag().name()==m_tag_default_name)
      return ti;
    ++it;
  }
  return NULL;
}


/* Slot. Force the user to re-edit an entry if our validation failed.
   We call it as a slot because doing it from the code called by the
   itemChanged signal doesn't work (outputs an "edit: editing failed"
   error message) */

void
tags_treeview::reedit()
{
  setCurrentItem(last_failed);
  editItem(last_failed);
}
