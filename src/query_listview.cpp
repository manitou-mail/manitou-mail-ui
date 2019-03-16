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

#include "main.h"

#include "db.h"
#include "message_port.h"
#include "msg_status_cache.h"
#include "query_listview.h"
#include "selectmail.h"
#include "sqlstream.h"
#include "tags.h"
#include "user_queries.h"

#include <QBrush>
#include <QCursor>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>
#include <QTreeWidgetItemIterator>

int
query_lvitem::id_generator;

query_listview::query_listview(QWidget* parent): QTreeWidget(parent)
{
  init();

  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(context_menu(const QPoint&)));
  // connect to any change to tags
  message_port::connect_receiver(SIGNAL(tags_restructured()),
				 this, SLOT(tags_restructured()));
  // subscribe to new messages notifications
  message_port::connect_receiver(SIGNAL(new_mail_imported(mail_id_t,int)),
				 this, SLOT(got_new_mail(mail_id_t,int)));
}

query_listview::~query_listview()
{
  free_mail_maps();
}

void
query_listview::free_mail_maps()
{
  qs_tag_map::iterator it;
  for (it=m_tagged.begin(); it!=m_tagged.end(); ++it) {
    // free each qs_mail_map
    delete it->second;
  }
  m_tagged.clear();
}

/* Return the number of unread messages inside the map (mail_id=>status) */
// static
int
query_listview::count_unread_messages(const qs_mail_map* m)
{  
  int cnt=0;
  for (qs_mail_map::const_iterator iter = m->begin();
       iter != m->end();
       ++iter)
    {
      if (((iter->second) & (mail_msg::statusRead|mail_msg::statusArchived|mail_msg::statusTrashed)) == 0)
	cnt++;
    }
  return cnt;
}

/* Return true if there is at least one unread message inside the map
   (mail_id=>status) */
// static
bool
query_listview::has_unread_messages(const qs_mail_map* m)
{  
  for (qs_mail_map::const_iterator iter = m->begin();
       iter != m->end();
       ++iter)
    {
      if (((iter->second) & (mail_msg::statusRead|mail_msg::statusArchived|mail_msg::statusTrashed)) == 0)
	return true;
    }
  return false;
}


/* Return the internal_id of the current item, or 0 if there's none */
int
query_listview::current_id()
{
  QTreeWidgetItem* selected = currentItem();
  if (!selected)
    return 0;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    if (*it == selected) {
      query_lvitem* item = static_cast<query_lvitem*>(*it);
      return item->m_unique_id;
    }
    ++it;
  }
  return 0;
}

/*
  Set the focus and highlight on a particular item within the tree.
  That might be expanded in the future to a vector of items
  if the listview gets multi-selectable
*/
void
query_listview::set_focus_on_id(int id)
{
  DBG_PRINTF(5, "set_focus_on_id(%d)", id);
  QTreeWidgetItemIterator it(this);
  while (id>0 && *it) {
    query_lvitem* item = static_cast<query_lvitem*>(*it);
    if (item->m_unique_id == id) {
      /* We block the signals here to avoid setCurrentItem() emit
	 a selectionChanged() signal that would launch a new query
	 while we just want the item to be displayed as current (reverse video
	 and focus set on it). */
      set_item_no_signal(item);
      return;
    }
    ++it;
  }
  set_item_no_signal(NULL);
}

void
query_listview::set_item_no_signal(query_lvitem* item)
{
  blockSignals(true);
  setCurrentItem(item);
  blockSignals(false);
}

/*
  Clear the selection status of whatever item is selected. To be
  called when a new set of messages is retrieved outside of the query
  listview
*/
void query_listview::clear_selection()
{
  set_item_no_signal(NULL);
}

/*
  Select an entry in the list and return its internal ID. Used when the selection
  comes from outside the list (a command key, a menu entry...)
*/
int
query_listview::highlight_entry(query_lvitem::item_type type, uint tag_id)
{
  query_lvitem* item;
  switch(type) {
  case query_lvitem::new_all:
    item = m_item_new_all;
    break;
  case query_lvitem::virtfold_sent:
    item = m_item_virtfold_sent;
    break;
  case query_lvitem::virtfold_trashcan:
    item = m_item_virtfold_trashcan;
    break;
  case query_lvitem::current_tagged:
    item = find_tag(m_item_current_tags, tag_id);
    break;
  case query_lvitem::archived_tagged:
    item = find_tag(m_item_tags, tag_id);
    break;
  case query_lvitem::archived_untagged:
    item = m_item_archived_untagged;
    break;
  default:
    item = NULL;
    break;
  }
  if (item != NULL) {
    if (!item->isSelected()) {
      blockSignals(true);
      item->setSelected(true);
      blockSignals(false);
    }
    return item->m_unique_id;
  }
  return 0;
}

/* Set up the [Current Messages]->Tagged subtree */
void
query_listview::make_item_current_tags(const tag_node* root)
{
  m_item_current_tags = new query_lvitem(m_item_current, query_lvitem::tree_node, tr("Tagged"));

  qs_tag_map::const_iterator qsi;
  for (qsi=m_tagged.begin(); qsi!=m_tagged.end() ; ++qsi) {
    if (qsi->first==0) continue; // ignore non-tagged
    const tag_node* node = root->find(qsi->first);
    if (node) {
      query_tag_lvitem* q = new query_tag_lvitem(m_item_current_tags, query_lvitem::current_tagged, QString() /*title*/, qsi->first);
      q->set_title(node->hierarchy(), qsi->second);
    }
  }
  m_item_current_tags->setExpanded(true);
  m_item_current_tags->sortChildren(0, Qt::AscendingOrder);
}

/*
  Returns the number of messages within m_tagged[tag_id] whose status has the
  bits from mask_not_set CLEARED
  (that is, (status & mask_not_set) = 0)
*/
int
query_listview::map_count(int mask_not_set, uint tag_id)
{
  int cnt=0;
  qs_tag_map::iterator qsi;
  for (qsi=m_tagged.begin(); qsi!=m_tagged.end() ; ++qsi) {
    if (qsi->first==tag_id) {
      qs_mail_map* m = qsi->second;
      std::map<mail_id_t,int>::iterator it = m->begin();
      for (; it!=m->end(); ++it) {
	if ((it->second & mask_not_set)==0) {
	  cnt++;
	}
      }
    }
  }
  return cnt;
}

/*
 Recursively insert a tree of tag nodes into the treeview.
 expanded_set: set of tag_id whose tree must be shown expanded
   (otherwise it's folded)
*/
int
query_listview::insert_child_tags(tag_node* n,
				  query_tag_lvitem* parent_item,
				  int type,
				  QSet<uint>* expanded_set)
{
  int total_cnt = 0; // total of tag counters added at this node level
  std::list<tag_node*>::iterator iter;

  for (iter=n->m_childs.begin(); iter!=n->m_childs.end(); ++iter) {
    query_tag_lvitem* q = new query_tag_lvitem(parent_item,
					       type,
					       (*iter)->name(),
					       (*iter)->id());
    int branch_cnt=0;
    if (!(*iter)->m_childs.empty()) {
      branch_cnt += insert_child_tags(*iter, q, type, expanded_set);
      if (expanded_set!=NULL && expanded_set->contains((*iter)->id())) {
	q->setExpanded(true);
      }
      q->sortChildren(0, Qt::AscendingOrder);
    }

    if (type == query_lvitem::archived_tagged) {
      /* Search the counter associated to the tag. When found, set
	 name-of-tag (count) as the text of the item */
      archived_tag_map::const_iterator count_iter = m_archtag_map.constFind((*iter)->id());
      if (count_iter != m_archtag_map.constEnd()) {
	/* if a parent tag has children but a zero count, it's just a container.
	   In this case, ignore the count to lighten the display */
	bool show_zero = (*iter)->m_childs.empty();
	q->show_archived_count(count_iter.value(), show_zero);
	total_cnt += branch_cnt + count_iter.value();
      }
    }
  }
  return total_cnt;
}

query_lvitem*
query_listview::create_branch_current(const tag_node* root)
{
  // Current messages (not archived)
  m_item_current = new query_lvitem(tr("Current messages"));
  
  //  m_item_current->set_brush(QBrush(QColor(225,160,46))); // orange
  //  m_item_current->set_brush(QBrush(QColor(0xf4,0xe3,0x9b))); // light yellow
  insertTopLevelItem(index_branch_current, m_item_current);

  m_item_current_all = new query_lvitem(m_item_current, query_lvitem::current_all, tr("All"));

  make_item_current_tags(root);
  m_item_current_prio = new query_lvitem(m_item_current, query_lvitem::current_prio, tr("Prioritized"));

  m_item_current_untagged = new query_lvitem(m_item_current, query_lvitem::current_not_tagged, QString());
  update_tag_current_counter(0); // update counts of Current->[Not tagged] branch


  m_item_current->setExpanded(true);
  return m_item_current;
}

void
query_listview::init()
{
  tags_definition_list tag_list;
  tag_node root_tag;

  setHeaderLabel(tr("Quick selection"));
  setRootIsDecorated(true);

  fetch_tag_map();
  tag_list.fetch();
  root_tag.get_child_tags(tag_list);

  // New messages
  query_lvitem* item_new = new query_lvitem(this, tr("Unread messages"));
  m_item_new_all = new query_lvitem(item_new, query_lvitem::new_all, tr("All"));
  m_item_new_untagged = new query_lvitem(item_new, query_lvitem::new_not_tagged, tr("Not tagged"));
  if (get_config().get_number("display/quicksel/expand_unread") > 0)
    item_new->setExpanded(true);
  else
    item_new->setExpanded(false);

  create_branch_current(&root_tag);
  int current_depth = get_config().get_number("display/quicksel/expand_current");
  if (current_depth==0)
    m_item_current->setExpanded(false);
  else
    expand_depth(m_item_current, current_depth);

  // Tagged messages
  // The root_tag is a pseudo-tag with an id=0
  m_item_archived = new query_lvitem(this, tr("Archived messages"));
  m_item_archived_untagged = new query_lvitem(m_item_archived,
					      query_lvitem::archived_untagged,
					      tr("Not tagged"));

  m_item_tags = new query_tag_lvitem(m_item_archived,
				     query_lvitem::archived_tagged,
				     tr("Tagged"));

  insert_child_tags(&root_tag,
		    m_item_tags,
		    query_lvitem::archived_tagged,
		    NULL);
  m_item_tags->sortChildren(0, Qt::AscendingOrder);

  int archive_depth = get_config().get_number("display/quicksel/expand_archived");
  if (archive_depth > 0) {
    expandItem(m_item_archived);
    expand_depth(m_item_tags, archive_depth);
  }

  m_item_virtfold_sent = new query_lvitem(this, tr("Sent mail"));
  m_item_virtfold_sent->set_type(query_lvitem::virtfold_sent);

  m_item_virtfold_trashcan = new query_lvitem(this, tr("Trashcan"));
  m_item_virtfold_trashcan->set_type(query_lvitem::virtfold_trashcan);

  // User queries
  m_item_user_queries = new query_lvitem(this, tr("User queries"));
  reload_user_queries();

  display_counter(query_lvitem::new_all);
  display_counter(query_lvitem::current_all);
  display_counter(query_lvitem::new_not_tagged);
  display_counter(query_lvitem::archived_untagged);
}

/* Recursively expand the sub-items down to depth */
void
query_listview::expand_depth(query_lvitem* start_item, int depth)
{
  if (depth<=0 || start_item==NULL)
    return;
  start_item->setExpanded(true);
  if (depth==1)
    return;
  for (int i=0; i<start_item->childCount(); i++) {
    // start_item->child(i)->setExpanded(true);
    expand_depth(dynamic_cast<query_lvitem*>(start_item->child(i)), depth-1);
  }
}

void
query_listview::reload_user_queries()
{
  DBG_PRINTF(5, "reload_user_queries");
  user_queries_repository uq;
  std::map<QString,QString>::const_iterator iterq;
  // delete existing elements
  QTreeWidgetItem* lv;
  while ((lv=m_item_user_queries->child(0))) {
    m_item_user_queries->removeChild(lv);
    delete lv;
  }
  if (uq.fetch()) {
    //    query_lvitem* prev=NULL;
    for (iterq=uq.m_map.begin(); iterq!=uq.m_map.end(); ++iterq) {
      query_lvitem* q = new query_lvitem(m_item_user_queries, query_lvitem::user_defined, iterq->first);
      q->m_sql = iterq->second;
    }
  }
}

/*
  Store in 'set' the tag IDs of the items that are in expanded state
  and are descendants of 'parent'
*/
void
query_listview::store_expanded_state(QTreeWidgetItem* parent,
				     QSet<uint>* set)
{
  for (int i=0; i<parent->childCount(); ++i) {
    QTreeWidgetItem* item = parent->child(i);
    if (item->isExpanded()) {
      query_tag_lvitem* t = static_cast<query_tag_lvitem*>(item);
      set->insert(t->m_tag_id);
    }
    if (item->childCount()>0)
      store_expanded_state(item, set);
  }
}

/*
  Slot. Called on new mail notifications. The global status map should already know about
  the new mail so we just update some counters
*/
void
query_listview::got_new_mail(mail_id_t id, int status)
{
  DBG_PRINTF(5, "got_new_mail(%d)", id);
  mail_msg msg;
  msg.set_mail_id(id);
  mail_status_changed(&msg, status);
}

/*
  Slot. Called when changes occur in tags definitions
*/
void
query_listview::tags_restructured()
{
  DBG_PRINTF(5, "tags_restructured");
  tags_definition_list tag_list;
  tag_list.fetch();
  tag_node root_tag;
  root_tag.get_child_tags(tag_list);

  /* Update the m_item_tags tree (all tags) */


  // memorize which items are opened
  QSet<uint> opened;
  store_expanded_state(m_item_tags, &opened);

  // destroy the tree
  m_item_tags->remove_children();

  // recreate the tree
  insert_child_tags(&root_tag, m_item_tags, query_lvitem::archived_tagged, &opened);
  m_item_tags->sortChildren(0, Qt::AscendingOrder);

  /* Update m_item_current_tags tree (tagged, non-archived messages) */
  QSet<query_tag_lvitem*> remove_set;
  for (int idx=0; idx<m_item_current_tags->childCount(); ++idx) {
    query_tag_lvitem* t = static_cast<query_tag_lvitem*>(m_item_current_tags->child(idx));
    const tag_node* n = root_tag.find(t->m_tag_id);
    if (n) {
      update_tag_current_counter(t->m_tag_id);
    }
    else {
      remove_set.insert(t);
    }
  }
  // remove items
  for (QSet<query_tag_lvitem*>::iterator it=remove_set.begin();
       it!=remove_set.end();
       ++it)
    {
      delete(*it);
    }
  // sort
  m_item_current_tags->sortChildren(0, Qt::AscendingOrder);
}

/* Also fill a pri_map for priorities */
bool
query_listview::fetch_tag_map()
{
  db_cnx db;
  try {
    // status is current (=NOT(sent OR archived OR trashed))
    sql_stream s(QString("SELECT m.mail_id,mt.tag,m.status,m.priority "
			 "FROM mail m LEFT OUTER JOIN mail_tags mt ON mt.mail_id=m.mail_id "
			 "WHERE m.status&status_mask('archived')=0 AND m.status&%1=0")
		 .arg(mail_msg::statusSent|mail_msg::statusTrashed),
		 db);
    mail_id_t mail_id;
    unsigned int tag, status;
    int pri;
    qs_tag_map::iterator it;

    m_all_unread_count = 0;
    // m_unprocessed_untagged_count = 0;
    // m_unread_untagged_count = 0;
    m_unprocessed_prioritized_count = 0;
    m_all_unprocessed_count = 0;

    while (!s.eos()) {
      s >> mail_id >> tag >> status >> pri;

      msg_status_cache::update(mail_id, status);

      m_prio_map[mail_id] = pri;
      if (pri>0)
	m_unprocessed_prioritized_count++;

      if ((status & mail_msg::statusRead) == 0)
	m_all_unread_count++;

      it = m_tagged.find(tag);	// tag is 0 if no tag is assigned to mail_id
      qs_mail_map* m;
      if (it != m_tagged.end()) {
	m = it->second;
      }
      else {
	m = new qs_mail_map();
	m_tagged[tag] = m;
      }
      (*m)[mail_id] = status;
    }
    m_all_unprocessed_count = m_prio_map.size();

    if (db_manitou_config::has_tags_counters()) {
      sql_stream s1("SELECT tag_id, sum(cnt) FROM tags_counters"
		    " GROUP BY tag_id",
		    db);
      m_archtag_map.clear();
      while (!s1.eos()) {
	int tag_id, cnt;
	s1 >> tag_id >> cnt;
	m_archtag_map[tag_id] = cnt;
      }
    }

  }
  catch(db_excpt& p) {
    db.handle_exception(p);
    return false;
  }
  return true;
}

/*
  Iterate through the tree items of the Current mail (=unprocessed) tags
  to find the one matching 'tag_id'.
  If found, change its counters.
*/
void
query_listview::update_tag_current_counter(uint tag_id)
{
  if (tag_id!=0) {
    if (!m_item_current_tags)
      return;
    int child_index=0;
    query_tag_lvitem* q = static_cast<query_tag_lvitem*>(m_item_current_tags->child(child_index));
    while (q) {
      if (q->m_tag_id==tag_id) {
	DBG_PRINTF(5, "update_tag_current_counter(%d)", tag_id);
	qs_tag_map::const_iterator it = m_tagged.find(tag_id);
	q->set_title(tags_repository::hierarchy(tag_id),
		     it != m_tagged.end() ? it->second : NULL);
	break;	// tag found, stop iterating
      }
      q = static_cast<query_tag_lvitem*>(m_item_current_tags->child(++child_index));
    }
  }
  else {
    if (m_item_current_untagged) {
      DBG_PRINTF(5, "update_tag_current_counter(%d)", tag_id);
      qs_tag_map::const_iterator it = m_tagged.find(0); // 0=>not tagged
      m_item_current_untagged->set_title(tr("Not tagged"),
					 it != m_tagged.end() ? it->second : NULL);

    }
  }
}

void
query_listview::display_counter(query_lvitem::item_type type)
{
  unsigned int count;
  QFont f;
  switch(type) {
  case query_lvitem::new_all:
    count = msg_status_cache::unread_count();
    m_item_new_all->setText(0, tr("All (%1)").arg(count));
    f = m_item_new_all->font(0);
    f.setBold(count>0);
    m_item_new_all->setFont(0, f);
    break;

  case query_lvitem::current_all:
    {
      int unread_count = msg_status_cache::unread_count();
      int unprocessed_count = msg_status_cache::unprocessed_count();
      if (unread_count>0) {
	m_item_current_all->setText(0, tr("All (%1<%2)").arg(unprocessed_count).arg(unread_count));	
      }
      else {
	m_item_current_all->setText(0, tr("All (%1)").arg(unprocessed_count));
      }
      f = m_item_current_all->font(0);
      f.setBold(unread_count>0);
      m_item_current_all->setFont(0, f);
      m_item_current_all->setToolTip(0, query_lvitem::current_tooltip_text(unprocessed_count, unread_count));
    }
    break;

  case query_lvitem::new_not_tagged:
    {
      int unread_untagged_count = 0;
      qs_tag_map::const_iterator it = m_tagged.find(0);
      if (it != m_tagged.end())
	unread_untagged_count = count_unread_messages(it->second);
	DBG_PRINTF(5, "update unread_untag_count=%d", unread_untagged_count);

      m_item_new_untagged->setText(0, tr("Not tagged (%1)").arg(unread_untagged_count));
      f = m_item_new_untagged->font(0);
      f.setBold(unread_untagged_count>0);
      m_item_new_untagged->setFont(0, f);
    }
    break;

  case query_lvitem::archived_untagged:
    {
      archived_tag_map::const_iterator it = m_archtag_map.constFind(0);
      if (it != m_archtag_map.constEnd()) {
	m_item_archived_untagged->show_archived_count(it.value());
      }
    }
    break;

  default:
    break;
  }
}


query_tag_lvitem*
query_listview::find_tag(query_lvitem* tree, uint tag_id)
{
  QTreeWidgetItemIterator it(tree);
  while (*it) {
    query_tag_lvitem* item = static_cast<query_tag_lvitem*>(*it);
    if (item->m_tag_id == tag_id)
      return item;
    ++it;
  }
  return NULL;
}

void
query_listview::add_current_tag(uint tag_id)
{
  if (!m_item_current_tags)
    return;
  tags_definition_list tag_list;
  tag_list.fetch();
  tag_node root_tag;
  root_tag.get_child_tags(tag_list);
  const tag_node* node = root_tag.find(tag_id);
  query_tag_lvitem* q = new query_tag_lvitem(m_item_current_tags, query_lvitem::current_tagged, QString(), tag_id);
  q->set_title(node->hierarchy());
  m_item_current_tags->sortChildren(0, Qt::AscendingOrder);
  qs_mail_map* m = new qs_mail_map();
  m_tagged[tag_id] = m;
}

/*
  Called by an msg_list_window to update counters when a mail tag
  has changed. 'added' is true if the tag has been added, false if removed
*/
void
query_listview::mail_tag_changed(const mail_msg& msg, uint tag_id, bool added)
{
  DBG_PRINTF(8, "mail_tag_changed mail_id=%u, tag=%u %s\n", msg.get_id(),
	     tag_id, added?"added":"removed");
  qs_tag_map::iterator itt = m_tagged.find(tag_id);
  qs_tag_map::iterator it_no_tag = m_tagged.find(0);
  if (itt!=m_tagged.end()) {
    // the leaf for the tag already exists
    qs_mail_map* m = itt->second;
    if (added) {
      (*m)[msg.get_id()] = msg.status();
    }
    else {
      m->erase(msg.get_id());
      // if the message no longer has any tag, update the 'Not Tagged' branches
      if (msg.get_cached_tags().empty() && it_no_tag!=m_tagged.end()) {
	qs_mail_map* m_no_tag = it_no_tag->second;
	(*m_no_tag)[msg.get_id()] = msg.status();
	update_tag_current_counter(0);
	display_counter(query_lvitem::new_not_tagged);
      }	
    }
    update_tag_current_counter(tag_id);
    // update the 'Not Tagged' branch counter if the mail was counted in it
    if (added) {
      if (it_no_tag!=m_tagged.end()) {
	m = it_no_tag->second;
	qs_mail_map::iterator qi;
	qi = m->find(msg.get_id());
	if (qi!=m->end()) {
	  m->erase(msg.get_id());
	  update_tag_current_counter(0);
	}
      }
    }
  }
  else {
    DBG_PRINTF(8, "create a new tag branch for '%u' in query_listview\n", tag_id);
    add_current_tag(tag_id);	// adds tag_id to m_tagged
    itt = m_tagged.find(tag_id);
    if (itt!=m_tagged.end()) {
      qs_mail_map* m = itt->second;
      (*m)[msg.get_id()] = msg.status();
      update_tag_current_counter(tag_id);
    }
    else
      DBG_PRINTF(1, "ERR: couldn't find tag map for tag_id=%u just added\n", tag_id);
  }
}

/*
  Called by msg_list_window to update counters when archived mail tags
  change. 'diff' is how many messages must be added (if diff>0) or subtracted 
  (if diff<0) to the tag's counter.
  If tag_id==0, the message has no tag and the "Untagged" branch must be updated
  instead of a branch corresponding to a tag.
  Unlike mail_tag_changed(), the mail_id affected is not known or used,
  as query_listview does not maintain a list of mail_id for archived mail.
*/
void
query_listview::archive_tag_changed(int tag_id, int diff)
{
  DBG_PRINTF(4, "archive_tag_changed tag=%d : %d", tag_id, diff);

  archived_tag_map::iterator it = m_archtag_map.find(tag_id);

  if (it == m_archtag_map.end()) {
    /* The tag does not exist in the map yet. Create it if diff>=0.
       If diff<0 and the tag doesn't exist, we probably have missed
       previous events when the counter was incremented. Do nothing
       rather than displaying a wrong value */
    if (diff >= 0) {
      m_archtag_map[tag_id] = 0;
      it = m_archtag_map.find(tag_id);
    }
    else {
      DBG_PRINTF(1, "ERR: couldn't find archive tag map for tag_id=%d (diff=%d)\n",
		 tag_id, diff);
      // fall through to not found condition
    }
  }

  if (it != m_archtag_map.end()) {
    query_lvitem* item;
    if (tag_id == 0) {
      item = m_item_archived_untagged;
    }
    else
      item = find_tag(m_item_tags, tag_id);
    (*it) = (*it) + diff;
    if (item) {
      item->show_archived_count(*it);
    }
  }
}


/*
  Update the global counters due to mail status transitions or mail deletion
*/
void
query_listview::update_status_counters()
{
  display_counter(query_lvitem::new_all);
  display_counter(query_lvitem::current_all);
  display_counter(query_lvitem::new_not_tagged);
}

/*
  Slot. Update the counters for archived mail due to transitions.
*/
void
query_listview::change_archive_counts(const QList<tag_counter_transition> changed)
{
  for (int i=0; i<changed.size(); i++) {
    archive_tag_changed(changed.at(i).tag_id, changed.at(i).count_change);
  }
}


/*
  Called by an msg_list_window to update counters when a mail status
  has changed. 'nstatus' is -1 if the mail has been deleted.
*/
void
query_listview::mail_status_changed(mail_msg* msg, int nstatus)
{
  mail_id_t mail_id = msg->get_id();
  DBG_PRINTF(8, "mail_status_changed mail_id=" MAIL_ID_FMT_STRING ",status=%d", mail_id, nstatus);
  msg_status_cache::update(mail_id, nstatus);

  qs_tag_map::iterator itt;
  qs_mail_map::iterator itm;
  const int archd = mail_msg::statusArchived;
  const int trshd = mail_msg::statusTrashed;
  bool status_counter_updated = false;
  if (nstatus>=0 && !(nstatus&archd) && !(nstatus&trshd)) {
    // if the msg is not archived and not trashed,
    // then we try to get its tags and update the corresponding counters
    DBG_PRINTF(8, "mail_id=" MAIL_ID_FMT_STRING " is candidate for query_listview", mail_id);
    std::list<uint>& tags = msg->get_tags();
    std::list<uint>::iterator it;
    for (it=tags.begin(); it!=tags.end(); ++it) {
      itt = m_tagged.find(*it);
      if (itt!=m_tagged.end()) {
	// the leaf for tag_id=(*it) already exists
	qs_mail_map* m = itt->second;
	if (!status_counter_updated) {
	  // update global counters once per message, not once per tag
	  update_status_counters();
	  status_counter_updated = true;
	}
	(*m)[mail_id] = nstatus;
	update_tag_current_counter(*it);
      }
      else {
	// the message has tags that arent't in the treeview yet
	DBG_PRINTF(8, "create a new tag branch for '%u' in query_listview!\n", *it);
	add_current_tag(*it);
	itt = m_tagged.find(*it);
	if (itt!=m_tagged.end()) {
	  qs_mail_map* m = itt->second;
	  if (!status_counter_updated) {
	    // update global counters once per message, not once per tag
	    update_status_counters();
	    status_counter_updated = true;
	  }
	  (*m)[mail_id] = nstatus;
	  update_tag_current_counter(*it);
	}
	else
	  DBG_PRINTF(1, "ERR: couldn't find tag map for tag_id=%u just added\n", *it);
      }
    }
    if (tags.empty()) {
      itt = m_tagged.find(0); // 0=>not tagged.
      if (itt!=m_tagged.end()) {
	qs_mail_map* m = itt->second;
	(*m)[mail_id] = nstatus;
	// update the Current->Untagged counter
	update_tag_current_counter(0);
	update_status_counters();
      }
    }
    return;  // EXIT
  }

  // The message is getting trashed or archived or deleted.
  // Check if a tag has been removed or if the message has been removed and
  // had tags before, and update counters accordingly.
  for (itt=m_tagged.begin(); itt!=m_tagged.end(); ++itt) {
    qs_mail_map* m = itt->second;
    itm = m->find(mail_id);
    if (itm!=m->end()) {
      int ostatus = itm->second;

      if (nstatus<0
	  || (nstatus & archd) != (ostatus & archd)
	  || (nstatus & trshd) != (ostatus & trshd))
	{
	  if (nstatus<0 || nstatus & (archd|trshd)) {
	    // msg was current and now is archived, or trashed, or deleted
	    DBG_PRINTF(8, "mail_id=%u is being arch/trshd/del\n", mail_id);
	    m->erase(mail_id);
	    if (!status_counter_updated) {
	      update_status_counters();
	      status_counter_updated = true;
	    }
	    update_tag_current_counter(itt->first);
	  }
	  else {
	    // msg wasn't current and now is.
	    // actually that shouldn't happen since the msg wouldn't be
	    // in the 'm' map in the first place
	    DBG_PRINTF(8, "mail_id=%u is being unarchived\n", mail_id);
	    if (!status_counter_updated) {
	      update_status_counters();
	      status_counter_updated = true;
	    }
	    (*m)[mail_id] = nstatus;
	    update_tag_current_counter(itt->first);
	  }
	}
    }
    else {
      /* mail_id is not found in the cache of current mail */
    }
  }
}

void
query_listview::context_menu(const QPoint& pos)
{
  query_lvitem* item = dynamic_cast<query_lvitem*>(itemAt(pos));

  extern void save_filter_query(msgs_filter*, int, const QString); // FIXME

  if (item && item != currentItem())
    setCurrentItem(item);

  if (item && item == m_item_user_queries) {
    // root of the "user queries" tree
    QMenu qmenu(this);
    QAction* action_new = qmenu.addAction(tr("New query"));
    QAction* selected = qmenu.exec(QCursor::pos());
    if (selected == action_new) {
      save_filter_query(NULL, 1, "");
      QTimer::singleShot(0, this, SLOT(reload_user_queries()));
    }
  }
  else if (item && item->m_type==query_lvitem::user_defined) {
    // contextual menu
    QMenu qmenu(this);
    qmenu.setTitle(item->text(0));
    QAction* action_run = qmenu.addAction(tr("Run query"));
    QAction* action_edit = qmenu.addAction(tr("Edit"));
    QAction* action_remove = qmenu.addAction(tr("Remove"));
    QAction* selected=qmenu.exec(QCursor::pos());

    // user action
    if (selected==action_remove) {
      // remove this user query
      int r=QMessageBox::warning(this, tr("Please confirm"), tr("Delete user query?"), tr("OK"), tr("Cancel"), QString());
      if (r==0) {
	if (user_queries_repository::remove_query(item->text(0))) {
	  delete item;
	}
      }
    }
    else if (selected==action_edit) {
      // edit the user query
      msgs_filter f;
      f.set_user_query(item->m_sql);
      save_filter_query(&f, 1, item->text(0));
      reload_user_queries();
    }
    else if (selected==action_run) {
      // run the query
      msgs_filter f;
      f.m_sql_stmt = item->m_sql;
      emit run_selection_filter(f);
    }
    return;
  }
}

void
query_listview::mousePressEvent(QMouseEvent* event)
{
  // suppress itemPressed signal on right click
  if (event->button() != Qt::RightButton)
    QTreeWidget::mousePressEvent(event);
}

void
query_listview::refresh()
{
  msg_status_cache::reset();
  free_mail_maps();
  fetch_tag_map();
  if (m_item_current)
    delete m_item_current;

  tags_definition_list tag_list;
  tag_list.fetch();
  tag_node root_tag;
  root_tag.get_child_tags(tag_list);
  create_branch_current(&root_tag);

  display_counter(query_lvitem::new_all);
  display_counter(query_lvitem::current_all);
  display_counter(query_lvitem::new_not_tagged);
}

query_lvitem::query_lvitem(QTreeWidgetItem* parent, int item_type, const QString name) :
  QTreeWidgetItem(parent, QStringList(name)), m_name(name)
{
  m_type=item_type;
  m_unique_id = ++id_generator;
  set_brush(parent->foreground(0));
}

query_lvitem::query_lvitem(QTreeWidget* parent, const QString name) :
  QTreeWidgetItem(parent, QStringList(name)), m_name(name)
{
  m_type=tree_node;		// 1st level node (=directory)
  m_unique_id = ++id_generator;
}

query_lvitem::query_lvitem(const QString name) : QTreeWidgetItem(QStringList(name)), m_name(name)
{
}

query_lvitem::~query_lvitem()
{
}

/*
  Show label preceded by checkmark and followed by count.
  Zero counts are displayed only if show_zero is true.
*/
void
query_lvitem::show_archived_count(int cnt, bool show_zero)
{
  if (cnt < 0)
    cnt = 0; // no negative count
  if (cnt != 0 || show_zero)
    setText(0, QString("%1 %2 (%3)").arg(QChar(0x2713)).arg(m_name).arg(cnt));
  else
    setText(0, QString("%1 %2").arg(QChar(0x2713)).arg(m_name));
}

void
query_lvitem::show_count(int cnt1, int cnt2)
{
  /* No negative count */
  if (cnt1 < 0)
    cnt1 = 0;
  if (cnt2==0)
    setText(0, QString("%1 (%2)").arg(m_name).arg(cnt1));
  else
    setText(0, QString("%1 (%2-%3)").arg(m_name).arg(cnt1).arg(cnt2));
}

/* Text for the tooltips for leafs inside the 'Current messages' branch */
//static
QString
query_lvitem::current_tooltip_text(int nb, int unread)
{
  QString s;
  if (unread==0) {
    switch(nb) {
    case 0:
      s=QObject::tr("No message to process");
      break;
    case 1:
      s=QObject::tr("1 message to process");
      break;
    default:
      s=QObject::tr("%1 messages to process").arg(nb);
      break;
    }
  }
  else if (unread==nb) {
    switch(nb) {
    case 1:
      s=QObject::tr("1 unread message to process");
      break;
    default:
      s=QObject::tr("%1 unread messages to process").arg(nb);
      break;
    }
  }
  else {
    switch(nb) {
    case 1:
      s=QObject::tr("1 unread message to process, including 1 unread").arg(nb).arg(unread);
      break;

    default:
      if (unread>0) {
	//: more than 1 unread messages
	s=QObject::tr("%1 messages to process, including %2 unread", "several unread messages").arg(nb).arg(unread);
      }
      else {
	s=QObject::tr("%1 messages to process, including 1 unread").arg(nb).arg(unread);
      }
      break;
    }
  }
  return s;
}

/* status_map (if set) must be a map of unprocessed messages,
   otherwise the tooltips would be wrong */
void
query_lvitem::set_title(const QString t, const qs_mail_map* status_map/*=NULL*/)
{
  if (status_map) {
    int unread = query_listview::count_unread_messages(status_map);
    QString title;
    int nb=status_map->size();
    if (unread==0) {
      // no unread
      title = QString("%1 (%2)").arg(t).arg(nb);
      setToolTip(0, current_tooltip_text(nb, unread));
    }
    else {
      title = QString("%1 (%2<%3)").arg(t).arg(nb).arg(unread);
      setToolTip(0, current_tooltip_text(nb, unread));
    }
    setText(0, title);
    QFont f = font(0);
    f.setBold(unread>0);
    setFont(0, f);
  }
  else
    setText(0, t);
}


void
query_lvitem::remove_children()
{
  QTreeWidgetItem* item;
  while ((item=this->child(0))!=NULL) {
    delete item;
  }
}

void
query_lvitem::set_brush(QBrush brush)
{
  setForeground(0, brush);
  //setBackground(0, brush);
}
