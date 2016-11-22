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

#include "mail_listview.h"
#include "msg_list_window.h"
#include "selectmail.h"
#include "icons.h"
#include "date.h"

#include <QKeyEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QItemSelection>
#include <QItemSelectionModel>

flag_item::flag_item(const QIcon& icon) : QStandardItem(icon, "")
{
}

flag_item::flag_item() : QStandardItem("")
{
}

mail_item_model::mail_item_model(QObject* parent) : QStandardItemModel(parent)
{
}

mail_item_model::~mail_item_model()
{
}

const char*
mail_item_model::m_column_names[] = {
  QT_TR_NOOP("Subject"), 
  QT_TR_NOOP("Sender"),
  QT_TR_NOOP("Status"),
  QT_TR_NOOP("Priority"),
  QT_TR_NOOP("Attachment"),
  QT_TR_NOOP("Note"),
  QT_TR_NOOP("Date"),
  QT_TR_NOOP("Recipient")
};

const int
mail_item_model::ncols = sizeof(m_column_names)/sizeof(m_column_names[0]);

QString
mail_item_model::column_name(int index)
{
  return tr(m_column_names[index]);
}

void
mail_item_model::init()
{
  QStringList headers;
  for (int i=0; i<ncols; i++) {
    headers << tr(m_column_names[i]);
  }
  setHorizontalHeaderLabels(headers);
  setHorizontalHeaderItem((int)column_attch, new QStandardItem(UI_ICON(FT_ICON16_ATTACH), ""));
  setHorizontalHeaderItem((int)column_note, new QStandardItem(UI_ICON(FT_ICON16_EDIT_NOTE_GREY), ""));
  m_display_sender_mode = get_config().get_string("sender_displayed_as");
}

void
mail_item_model::clear()
{
  QStandardItemModel::clear();
  items_map.clear();
}

QStandardItem*
mail_item_model::item_from_id(mail_id_t mail_id) const
{
  QMap<mail_id_t, QStandardItem*>::const_iterator it = items_map.find(mail_id);
  if (it!=items_map.end()) {
    return it.value();
  }
  return NULL;
}

QStandardItem*
mail_item_model::first_top_level_item() const
{
  QMap<mail_id_t, QStandardItem*>::const_iterator it = items_map.constBegin();
  if (it==items_map.constEnd())
    return NULL;		// no item
  const QStandardItem* item = it.value();
  // traverse up to the top level
  while (item->parent()) {
    item=item->parent();
  }
  const QModelIndex index = item->index().sibling(0,0);
  if (index.isValid())
    return this->itemFromIndex(index);
  else
    return NULL;
}

// returns an icon showing the message status
QIcon*
mail_item_model::icon_status(uint status)
{
  static QIcon* inew;
  static QIcon* iread;
  static QIcon* ireplied;
  static QIcon* ifwded;
  static QIcon* itrashed;
  static QIcon* iarchived;
  static QIcon* isent;
  static QIcon* itosend;

  // the order of each test is relevant: the "preferred" icons come first
  if (status & mail_msg::statusTrashed) {
    if (!itrashed)
      itrashed = new STATUS_ICON(FT_ICON16_STATUS_TRASHED);
    return itrashed;
  }
  else if (status & mail_msg::statusSent) {
    if (!isent)
      isent = new STATUS_ICON(FT_ICON16_COMPOSED);
    return isent;
  }
  else if (status & mail_msg::statusFwded) {
    if (!ifwded) 
      ifwded = new STATUS_ICON(FT_ICON16_STATUS_FORWARDED);
    return ifwded;
  }
  else if (status & mail_msg::statusReplied) {
    if (!ireplied)
      ireplied = new STATUS_ICON(FT_ICON16_STATUS_REPLIED);
    return ireplied;
  }
  else if (status & mail_msg::statusArchived) {
    if (!iarchived)
      iarchived = new STATUS_ICON(FT_ICON16_STATUS_PROCESSED);
    return iarchived;
  }
  else if (status & mail_msg::statusOutgoing) {
    if (!itosend)
      itosend = new STATUS_ICON(FT_ICON16_TO_SEND);
    return itosend;
  }
  else if (status & mail_msg::statusRead) {
    if (!iread)
      iread = new STATUS_ICON(FT_ICON16_STATUS_READ);
    return iread;
  }
  else {
    if (!inew)
      inew = new STATUS_ICON(FT_ICON16_STATUS_NEW);
    return inew;
  }
}

void
mail_item_model::create_row(mail_msg* msg, QList<QStandardItem*>& items)
{
  bool bold = ((msg->status() & mail_msg::statusRead)==0);
  QStandardItem* isubject = new QStandardItem(msg->Subject());
  if (bold) {
    QFont f=isubject->font();
    f.setBold(true);
    isubject->setFont(f);
  }
  // store mail_msg* into the data of the first item (mail's subject at column 0)
  QVariant v;
  v.setValue(msg);
  isubject->setData(v, mail_item_model::mail_msg_role);

  QStandardItem* ifrom = new QStandardItem();
  if (m_display_sender_mode!="name" || msg->sender_name().isEmpty())
    ifrom->setText(msg->From());
  else
    ifrom->setText(msg->sender_name());
  
  if (bold) {
    QFont f=ifrom->font();
    f.setBold(true);
    ifrom->setFont(f);
  }

  items.append(isubject);
  items.append(ifrom);
  QIcon* icon = icon_status(msg->status());
  if (icon)
    items.append(new QStandardItem(*icon,"")); // status
  else
    items.append(new QStandardItem()); // empty (shouldn't happen)

   // priority (number)
  QStandardItem* ipri = new QStandardItem();
  if (msg->priority()!=0)
    ipri->setText(QString("%1").arg(msg->priority()));
  if (bold) {
    QFont f=ipri->font();
    f.setBold(true);
    ipri->setFont(f);
  }
  items.append(ipri);

  // Attachment
  flag_item* a_item;
  if (msg->has_attachments()) {
    a_item = new flag_item(STATUS_ICON(FT_ICON16_ATTACH));
    a_item->setData(QVariant(true));
  }
  else {
    a_item = new flag_item();
    a_item->setData(QVariant(false));
  }
  items.append(a_item);

  // Note
  flag_item* n_item;
  if (msg->has_note()) {
    n_item = new flag_item(STATUS_ICON(FT_ICON16_EDIT_NOTE));
    n_item->setData(QVariant(true));
  }
  else {
    n_item = new flag_item();
    n_item->setData(QVariant(false));
  }
  items.append(n_item);

  // Date
  date_item* idate = new date_item(msg->msg_date(), m_date_format);
  if (bold) {
    QFont f=idate->font();
    f.setBold(true);
    idate->setFont(f);
  }
  items.append(idate);

  QStandardItem* irecipient = new QStandardItem();
  QList<QString> emails;
  QList<QString> names;
  QString recipients;
  mail_address::ExtractAddresses(msg->recipients(), emails, names);
  QList<QString>::const_iterator iter1 = emails.begin();
  QList<QString>::const_iterator iter2 = names.begin();
  for (; iter1!=emails.end() && iter2!=names.end(); ++iter1,++iter2) {
    if (!recipients.isEmpty())
      recipients.append(", ");
    if (iter2->isEmpty())
      recipients.append(*iter1);
    else
      recipients.append(*iter2);
  }
  
  irecipient->setText(recipients);
  if (bold) {
    QFont f=irecipient->font();
    f.setBold(true);
    irecipient->setFont(f);
  }
  items.append(irecipient);
}

QStandardItem*
mail_item_model::insert_msg(mail_msg* msg, QStandardItem* parent/*=NULL*/)
{
  QList<QStandardItem*> items;
  create_row(msg, items);
  if (parent)
    parent->appendRow(items);
  else
    appendRow(items);

  items_map.insert(msg->get_id(), items[0]);
  return items[0];
}

QStandardItem*
mail_item_model::insert_msg_sorted(mail_msg* msg, QStandardItem* parent,
				   int column, Qt::SortOrder order)
{
  DBG_PRINTF(4, "insert_msg_sorted order=%d column=%d", (int)order, column);
  QList<QStandardItem*> items;
  create_row(msg, items);
  QStandardItem* item = items[0];
  int row = insertion_point(items, parent, column, order);
  DBG_PRINTF(4, "insertion point=%d", row);
  if (parent) {
    parent->insertRow(row, items);
  }
  else {
    insertRow(row, items);
  }
  items_map.insert(msg->get_id(), item);
  return item;
}

/* Returns the insertion position for new_row in a sorted list, or -1
   if it should be appended */
int
mail_item_model::insertion_point(QList<QStandardItem*>& new_row,
				 QStandardItem* parent,
				 int column,
				 Qt::SortOrder order)
{
  const QStandardItem* item_col = new_row[column];
  Q_ASSERT(item_col!=NULL);
  // binary search
  int l=0;
  if (!parent) {
    parent=invisibleRootItem();
  }
  int r=parent->rowCount()-1;
  while (l<=r) {
    int mid=(l+r)/2;
    const QStandardItem* mid_item=parent->child(mid, column);
    Q_ASSERT(mid_item!=NULL);
    if (*mid_item < *item_col) {
      if (order==Qt::AscendingOrder)
	l=mid+1;
      else
	r=mid-1;
    }
    else {
      if (order==Qt::AscendingOrder)
	r=mid-1;
      else
	l=mid+1;
    }
  }
  return l;
}

/* 
  Update the contents for the message pointed to by 'msg', if it's in the
  model, otherwise ignore the request.  Currently the contents are the
  status (icon) and the priority. Other changes such as in the subject
  or the date could be visually reflected by this function if the UI
  permitted those changes.
*/
void
mail_item_model::update_msg(const mail_msg* msg)
{
  DBG_PRINTF(4, "update_msg of " MAIL_ID_FMT_STRING, msg->get_id());
  QStandardItem* item=item_from_id(msg->get_id());
  if (!item)
    return;
  QModelIndex index=item->index();
  QStandardItem* isubject = itemFromIndex(index);
  bool bold=isubject->font().bold();
  DBG_PRINTF(5, "mail_id=" MAIL_ID_FMT_STRING ", status=%d, bold=%d",
	     msg->get_id(), msg->status(), bold);
  if ((msg->status()!=0 && bold) || (msg->status()==0 && !bold)) {
    // reverse bold attribute
    QFont f=isubject->font();
    f.setBold(!bold);
    isubject->setFont(f);
    itemFromIndex(index.sibling(index.row(), column_sender))->setFont(f);
    itemFromIndex(index.sibling(index.row(), column_date))->setFont(f);
    itemFromIndex(index.sibling(index.row(), column_pri))->setFont(f);
    itemFromIndex(index.sibling(index.row(), column_recipient))->setFont(f);
  }

  // status icon
  QIcon* icon = icon_status(msg->status());
  QStandardItem* istatus = itemFromIndex(index.sibling(index.row(), column_status));
  if (istatus) {
    if (icon)
      istatus->setIcon(*icon);
    else
      istatus->setIcon(QIcon());
  }

  // priority
  QStandardItem* ipri = itemFromIndex(index.sibling(index.row(), column_pri));
  if (ipri) {
    if (msg->priority()!=0)
      ipri->setText(QString("%1").arg(msg->priority()));
    else
      ipri->setText("");
  }

  // note
  QStandardItem* inote = itemFromIndex(index.sibling(index.row(), column_note));
  if (inote) {
    inote->setData(msg->has_note());
    if (msg->has_note())
      inote->setIcon(STATUS_ICON(FT_ICON16_EDIT_NOTE));
    else
      inote->setIcon(QIcon());
  }

}

void
mail_item_model::remove_msg(mail_msg* msg)
{
  DBG_PRINTF(8, "remove_msg(mail_id=" MAIL_ID_FMT_STRING ")", msg->get_id());
  QMap<mail_id_t, QStandardItem*>::iterator it = items_map.find(msg->get_id());
  if (it!=items_map.end()) {
    QStandardItem* item = it.value();
    QStandardItem* parent_item = item->parent();
    while (item->child(0,0)!=NULL) {
      // reparent childs of the item to remove
      QList<QStandardItem*> ql = item->takeRow(0);
      if (parent_item) {
	DBG_PRINTF(9, "reparenting child to immediate parent row=%d", item->index().row());
	parent_item->insertRow(item->index().row(), ql);
      }
      else {
	DBG_PRINTF(9, "reparenting child to root");
	insertRow(item->index().row(), ql);
      }
    }
    // 
    DBG_PRINTF(9, "removing mail_id=" MAIL_ID_FMT_STRING
	       " from model at index.row=%d index.column=%d",
	       msg->get_id(), item->index().row(),
	       item->index().column());
    QModelIndex index = item->index();
    removeRow(index.row(), index.parent());
    items_map.erase(it);
  }
  else {
    DBG_PRINTF(1, "ERR: mail_id=" MAIL_ID_FMT_STRING " not found in items_map", msg->get_id());
  }
}

#if 0
/* The code below does work, but is way too slow for large lists, and
   is kept here only for reference. Both takeRow() and appendRow()
   seem to need too much time, especially when there are child
   items. The alternative method we use to flatten the tree is
   to recreate it from scratch as a list, see msg_list_window::toggle_threaded() */
void
mail_item_model::flatten()
{
  QMap<mail_id_t, QStandardItem*>::iterator it;
  for (it=items_map.begin(); it!=items_map.end(); ++it) {
    QStandardItem* item = it.value();
    if (item->parent()) {
      QModelIndex index=this->indexFromItem(item);
      QList<QStandardItem*> ql=item->parent()->takeRow(index.row());
      this->appendRow(ql);
    }
  }
}
#endif

QStandardItem*
mail_item_model::reparent_msg(mail_msg* msg, mail_id_t parent_id)
{
  DBG_PRINTF(7, "reparent_msg");
  QMap<mail_id_t, QStandardItem*>::iterator itp = items_map.find(parent_id);
  if (itp==items_map.end())
    return NULL;
  QMap<mail_id_t, QStandardItem*>::iterator itc = items_map.find(msg->get_id());
  if (itc!=items_map.end()) {
    QModelIndex index_child=this->indexFromItem(itc.value());
    QStandardItem* old_parent=itc.value()->parent();
    QStandardItem* new_parent=itp.value();
    if (old_parent==new_parent)
      return NULL;
    QList<QStandardItem*> ql;
    if (!old_parent) {
      ql=takeRow(index_child.row());
    }
    else {
      DBG_PRINTF(9,"old_parent for mail_id=%d", msg->get_id());
      ql=old_parent->takeRow(index_child.row());
    }
    new_parent->appendRow(ql);
    DBG_PRINTF(9, "reparented %d as child of %d", msg->get_id(), parent_id);
    return new_parent;
  }
  else {
    DBG_PRINTF(1, "ERR: mail_id=%d not found in items_map", msg->get_id());
  }
  return NULL;
}

mail_msg*
mail_item_model::find(mail_id_t mail_id)
{
  QMap<mail_id_t, QStandardItem*>::iterator it = items_map.find(mail_id);
  if (it!=items_map.end()) {
    QStandardItem* item=it.value();
    QVariant v=item->data(mail_item_model::mail_msg_role);
    mail_msg* msg=v.value<mail_msg*>();
    if (msg->get_id()!=mail_id) {
      DBG_PRINTF(1, "ERR: items_map doesn't match the model for mail_id=%d", mail_id);
    }
    return msg;
  }
  return NULL;
}

mail_listview::mail_listview(QWidget* parent): QTreeView(parent)
{
  m_msg_window = NULL;
  m_sender_column_swapped = false;

  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setUniformRowHeights(true);
  // Qt5: lines too close to each other as in the default QTreeView style
  // don't feel good for the list of messages, hence this hard-wired
  // 3px margin
  this->setStyleSheet("QTreeView::item { margin-bottom: 3px; }");
  mail_item_model* model = new mail_item_model(this);
  this->setModel(model);
  model->set_date_format(get_config().get_date_format_code());

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(popup_ctxt_menu(const QPoint&)));

  header()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(header(), SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(popup_ctxt_menu_headers(const QPoint&)));

  setEditTriggers(QAbstractItemView::NoEditTriggers);
}

mail_listview::~mail_listview()
{
}

QMenu*
mail_listview::make_header_menu(QWidget* parent)
{
  QMenu* menu = new QMenu(tr("Show columns"), parent);

  // i=logical index of the QHeaderView's section
  for (int i=0; i<mail_item_model::ncols; i++) {
    QAction* a = menu->addAction(mail_item_model::column_name(i));
    a->setCheckable(true);
    // The column displaying the subject can't be removed
    if (i==0)
      a->setEnabled(false);

    a->setChecked(!header()->isSectionHidden(i));
    a->setData(QVariant(i));
  }
  
  return menu;
}

void
mail_listview::compute_visible_sections()
{
  m_nb_visible_sections=0;
  for (int i=0; i<mail_item_model::ncols; i++) {
    if (!header()->isSectionHidden(i))
      m_nb_visible_sections++;
  }
}

void
mail_listview::popup_ctxt_menu_headers(const QPoint& pos)
{
  QModelIndex index = indexAt(pos);
  if (!index.isValid())
    return;
  QMenu* menu = make_header_menu(this);
  QAction* action = menu->exec(mapToGlobal(pos));
  
  if (action!=NULL && action->data().isValid()) {
    int section = action->data().toInt();
    if (action->isChecked()) {
      // show
      header()->showSection(section);
    }
    else {
      header()->hideSection(section);
    }
    compute_visible_sections();
  }
  delete menu;
}

void
mail_listview::popup_ctxt_menu(const QPoint& pos)
{
  QAction* action_thread=NULL;
  QAction* action_same_from=NULL;
  QAction* action_more_results=NULL;

  QModelIndex index = indexAt(pos);
  if (!index.isValid())
    return;
  // get to the first column
  index=index.sibling(index.row(), 0);
  QStandardItem* item = model()->itemFromIndex(index);
  QVariant v=item->data(mail_item_model::mail_msg_role);
  mail_msg* msg = v.value<mail_msg*>();

  if (!msg) {
    DBG_PRINTF(1, "no msg found");
    return;			// can happen?
  }

  QMenu qmenu(this);
  QString s;

  // Same thread
  if (msg->thread_id()) {
    action_thread = qmenu.addAction(tr("Show thread on new page"));
  }

  // More results
  action_more_results = qmenu.addAction(tr("More results"));

  if (!msg->From().isEmpty()) {
    s=tr("Last messages from '%1'").arg(msg->From());
    action_same_from = qmenu.addAction(s);
  }

  /* For each tag currently assigned to the message, a menu entry is added
     to select 'All messages tagged as ...this_tag' */
  std::list<uint>& tags = msg->get_tags();
  std::list<uint>::const_iterator itt;
  for (itt=tags.begin(); itt!=tags.end(); ++itt) {
    s=tr("All messages tagged as '%1'").arg(tags_repository::hierarchy(*itt));
    QAction* a = qmenu.addAction(s);
    if (a)
      a->setData(QVariant(*itt));	// *itt=tag_id
  }

  msgs_filter filter;
  bool action_opens_page = false;

  QAction* action=qmenu.exec(mapToGlobal(pos));
  if (action==NULL) {
    return;
  }
  else if (action == action_thread) {
    if (!(filter.m_thread_id=msg->threadId())) {
      filter.m_sql_stmt = QString("%1").arg(msg->get_id()); // will produce: mail_id IN (%1)
    }
    action_opens_page =true;
  }
  else if (action == action_more_results) {
    // FIXME: postprocess should be done here
    if (m_msg_window) {
      m_msg_window->fetch_more();
      m_msg_window->set_title();
    }
    return;
  }
  else if (action == action_same_from) {
    filter.m_sAddress=msg->From();
    filter.m_nAddrType=msgs_filter::rFrom;
    filter.set_date_order(-1);	// latest results first
    action_opens_page = true;
  }
  else {
    bool variant_data_ok=false;
    uint selected_tag = action->data().toUInt(&variant_data_ok);
    if (variant_data_ok) {
      if (m_msg_window) {
	m_msg_window->sel_tag(selected_tag);
	m_msg_window->clear_quick_query_selection();
      }
      return;
    }
    else {
      DBG_PRINTF(1, "Error: couldn't extract tag_id from action.data()");
      return;
    }
  }

  if (action_opens_page && m_msg_window) {
    m_msg_window->add_msgs_page(&filter,true);
    m_msg_window->clear_quick_query_selection();
  }
}

// slot
void
mail_listview::refresh(mail_id_t id)
{
  DBG_PRINTF(8, "refresh id=%d", id);
  mail_msg* msg=find(id);
  if (msg) {
    msg->refresh();		// get latest status from database
    update_msg(msg);
  }
}

/*
  Update the contents the message pointed to by 'msg', if it's
  displayed in the listview, otherwise ignore the request.
*/
void
mail_listview::update_msg(const mail_msg *msg)
{
  DBG_PRINTF(8, "update_msg(mail_id=" MAIL_ID_FMT_STRING ")", msg->get_id());
  model()->update_msg(msg);
}

// slot
void
mail_listview::change_msg_status(mail_id_t id, uint mask_set, uint mask_unset)
{
  DBG_PRINTF(8, "change_msg_status(mail_id=" MAIL_ID_FMT_STRING ")", id);
  mail_msg* pm=find(id);
  if (pm)
    pm->setStatus((mask_set | pm->status()) & ~mask_unset);
}

/*
  Visually reflect the new status and priority of the message whose
  mail_id is 'id'. TODO: see if the propagate_status call shouldn't be
  moved to msg_list_window instead
*/
void
mail_listview::force_msg_status(mail_id_t id, uint status, int priority)
{
  DBG_PRINTF(8, "force_msg_status(mail_id=" MAIL_ID_FMT_STRING ")", id);
  mail_msg* pm=find(id);
  if (pm) {
    pm->set_status(status);
    pm->set_priority(priority);
    update_msg(pm);
    if (m_msg_window)
      m_msg_window->propagate_status(pm);
  }
}


/*
 Returns the first item of the list, which should be the first child
 of the treeview's root
*/
mail_msg*
mail_listview::first_msg() const
{
  // take any item from the model, and go up to the root
  QStandardItem* first_child = model()->first_top_level_item();
  if (!first_child)
    return NULL;
  QVariant v=first_child->data(mail_item_model::mail_msg_role);
  mail_msg* msg=v.value<mail_msg*>();
  return msg;
}

/*
  direction: 1=below only, 2=above only, 3=below then above
*/
mail_msg*
mail_listview::nearest_msg(const mail_msg* msg, int direction) const
{
  QStandardItem* item = this->nearest(msg, direction);
  if (!item)
    return NULL;
  QVariant v=item->data(mail_item_model::mail_msg_role);
  mail_msg* msg1=v.value<mail_msg*>();
  return msg1;
}

/*
  direction: 1=below only, 2=above only, 3=below then above
*/
QStandardItem*
mail_listview::nearest(const mail_msg* msg, int direction) const
{
  mail_item_model* model = this->model();
  QStandardItem* item = model->item_from_id(msg->get_id());
  if (item!=NULL) {
    if (direction==1 || direction==3) {
      QModelIndex index_below=indexBelow(item->index());
      if (index_below.isValid()) {
	return model->itemFromIndex(index_below);
      }
    }
    if (direction==2 || direction==3) {
      QModelIndex index_above=indexAbove(item->index());
      if (index_above.isValid()) {
	return model->itemFromIndex(index_above);
      }
    }
  }
  return NULL;
}

void
mail_listview::select_msg(const mail_msg* msg)
{
  QStandardItem* item=model()->item_from_id(msg->get_id());
  if (item) {
    setCurrentIndex(item->index());
  }
}

/*
  Select all messages from a set of threads (passed as a set of mail.thread_id).
  Return the number of selected messages.
*/
int
mail_listview::select_threads(const QSet<uint>& threads)
{
  if (threads.empty())
    return 0;

  int cnt=0;

  QStandardItem* item = model()->first_top_level_item();
  QItemSelectionModel* sel = this->selectionModel();

  while (item) {
    QVariant v = item->data(mail_item_model::mail_msg_role);
    mail_msg* msg = v.value<mail_msg*>();
    QModelIndex index = item->index();
    if (threads.contains(msg->thread_id())) {
      DBG_PRINTF(4, "selecting index for mail_id=%d", msg->get_id());
      sel->select(index, QItemSelectionModel::Select|QItemSelectionModel::Rows);
      cnt++;
    }
    // next item
    QModelIndex index_below  = indexBelow(index);
    if (index_below.isValid()) {
      item = model()->itemFromIndex(index_below);
    }
    else
      item=NULL;
  }
  return cnt;
}

void
mail_listview::select_below(const mail_msg* msg)
{
  QStandardItem* item=nearest(msg, 1);
  if (item) {
    setCurrentIndex(item->index());
  }
}

void
mail_listview::select_above(const mail_msg* msg)
{
  QStandardItem* item=nearest(msg, 2);
  if (item) {
    setCurrentIndex(item->index());
  }
}

void
mail_listview::select_nearest(const mail_msg* msg)
{
  QStandardItem* item=nearest(msg, 3);
  if (item) {
    setCurrentIndex(item->index());
  }
}

/* Recursively traverse all the child items and record every expanded item */
void
mail_listview::collect_expansion_states(QStandardItem* item,
					QSet<QStandardItem*>& expanded_set)
{
  QStandardItem* citem;
  int row=0;
  while ((citem=item->child(row,0))!=NULL) {
    if (isExpanded(citem->index())) {
      expanded_set.insert(citem);
    }
    if (citem->hasChildren()) {
      collect_expansion_states(citem, expanded_set);
    }
    row++;
  }
}

void
mail_listview::remove_msg(mail_msg* msg, bool select_next)
{
  DBG_PRINTF(8, "mail_listview::remove_msg(mail_id=%d, select_next=%d)", msg->get_id(), select_next);

  mail_item_model* model = this->model();
  QStandardItem* item = model->item_from_id(msg->get_id());
  if (!item)
    return;

  QStandardItem* nearest = NULL;
  QSet<QStandardItem*> expanded_set;

  if (select_next) {
    QModelIndex index_below = indexBelow(item->index());
    if (index_below.isValid()) {
      nearest = model->itemFromIndex(index_below);
      DBG_PRINTF(5, "nearest got from index_below");
    }
    else {
      QModelIndex index_above = indexAbove(item->index());
      if (index_above.isValid()) {
	nearest = model->itemFromIndex(index_above);
	DBG_PRINTF(5, "nearest got from index_above");
      }
    }
    if (nearest) {
      /* record the expansion states of all child items to set them
	 back after the parent's removal */
      collect_expansion_states(item, expanded_set);
    }
  }
  model->remove_msg(msg);
  if (select_next && nearest) {
    QModelIndex index = nearest->index();
    DBG_PRINTF(5,"making current index row=%d col=%d parent is valid=%d", 
	       index.row(), index.column(), index.parent().isValid()?1:0);
    foreach (QStandardItem *child, expanded_set) {
      setExpanded(child->index(), true);
    }
    setCurrentIndex(nearest->index());
  }
}

void
mail_listview::reparent_msg(mail_msg* msg, mail_id_t parent_id)
{
#if QT_VERSION<0x040303
  QStandardItem* parent=model->reparent_msg(msg, parent_id);
  /* Each row is expanded individually because of a probable bug in
     Qt<4.3.3. The bug makes the treeview appear to be empty on screen
     after the expandAll() call */
  if (parent) {
    expand(model()->indexFromItem(parent));
  }
#else
  model()->reparent_msg(msg, parent_id);
#endif
}

mail_msg*
mail_listview::find(mail_id_t mail_id)
{
  return model()->find(mail_id);
}

void
mail_listview::get_selected_indexes(QModelIndexList& list)
{
  list = selectedIndexes();
}

void
mail_listview::clear()
{
  DBG_PRINTF(4, "sort column=%d, nb columns=%d",
	     header()->sortIndicatorSection(),
	     header()->count());
  /* Keep the sort column and set it again after recreating the
     header. Otherwise sortIndicatorSection() is incremented by
     count() after init_columns(). (looks like a Qt bug. Example:
     section=6 before, section=14 after even if count()=8) */
  int section=-1;
  Qt::SortOrder order;
  if (header()->isSortIndicatorShown()) {
    section=header()->sortIndicatorSection();
    order=header()->sortIndicatorOrder();
  }
  model()->clear();
  init_columns(); // because clearing the model removes the column headers
  if (section>=0) {
    header()->setSortIndicator(section, order);
  }
  DBG_PRINTF(4, "sort column=%d, nb columns=%d",
	     header()->sortIndicatorSection(),
	     header()->count());
}

bool
mail_listview::empty() const
{
  return (model()->first_top_level_item()==NULL);
}


void
mail_listview::add_msg(mail_msg* msg)
{
  model()->insert_msg(msg);
}

void
mail_listview::selectionChanged(const QItemSelection& selected,
				const QItemSelection& deselected)
{
  DBG_PRINTF(8, "selectionChanged()");
  QTreeView::selectionChanged(selected, deselected);
  emit selection_changed();
}

void
mail_listview::get_selected(std::vector<mail_msg*>& vect)
{
  QModelIndexList li = selectedIndexes();
  for (int i=0; i<li.size(); i++) {
    const QModelIndex idx=li.at(i);
    if (idx.column()!=0)	// we only want one item per row
      continue;
    QStandardItem* item = model()->itemFromIndex(idx);
    QVariant v=item->data(mail_item_model::mail_msg_role);
    vect.push_back(v.value<mail_msg*>()); // add the mail_msg* to the vector
  }
}

int
mail_listview::selection_size() const
{
  int n = mail_item_model::ncols;
  int s = selectedIndexes().size();
  return s==0? 0: s/(m_nb_visible_sections>0 ?
		     m_nb_visible_sections : n);
}

void
mail_listview::keyPressEvent(QKeyEvent* e)
{
  if (e->key()==Qt::Key_A && (e->modifiers() & Qt::ControlModifier)) {
    // select all. We override the default behavior because as seen with Qt-4.3.3,
    // the child items don't get selected, only the top level ones.
    selectAll();
  }
  else if (e->key()==Qt::Key_Space && (e->modifiers()==Qt::NoModifier && selection_size()==1)) {
    emit scroll_page_down();
  }
  else
    QTreeView::keyPressEvent(e);
}

void
mail_listview::init_columns()
{
  DBG_PRINTF(4, "init_columns");
  model()->init();
  setAllColumnsShowFocus(true);
  static const int default_sizes[] = {280,170,32,32,24,24,115,170};
  QString s;
  const int ncols = (int)(sizeof(default_sizes)/sizeof(default_sizes[0]));
  for (int i=0; i<ncols; i++) {
    int sz;
    s.sprintf("display/msglist/column%d_size", i);
    if (get_config().exists(s)) {
      sz=get_config().get_number(s);
      setColumnWidth(i, sz==0?default_sizes[i]:sz);
    }
    else {
      setColumnWidth(i, default_sizes[i]);
    }
  }

  // Currently, a column number is a single digit
  // FIXME: extend this model to have more than 10 columns, by using letters
  bool shown[ncols];
  for (int i=0; i<ncols; i++) {
    shown[i]=false;
  }

  QString order=get_config().get_string("display/msglist/columns_ordering");
  if (order.isEmpty()) {
    // if no preference is set, all columns are shown by default
    // except sender
    for (int i=0; i<ncols; i++) {
      shown[i]=true;
    }
    shown[mail_item_model::column_recipient]=false;
  }
  for (int i=0; i<ncols && i<order.length(); i++) {
    int colnum=order.mid(i,1).toInt();
    if (colnum>=ncols) // can't trust the column numbers from the database
      continue;
    shown[colnum]=true;
    header()->moveSection(header()->visualIndex(colnum), i);
  }

  m_nb_visible_sections = 0;
  for (int i=0; i<ncols; i++) {
    if (!shown[i])
      header()->setSectionHidden(i, true);
    else
      m_nb_visible_sections++;
  }
}

/*
  This function triggers a very annoying bug with Qt-4.2.1: after the
  contents have been scrolled down by at least one page, when the user
  clicks on a row, the treeview scrolls back to the top and a
  different row actually gets selected.
  Apparently fixed in Qt-4.3.x
*/
void
mail_listview::scroll_to_bottom()
{
  if (model()->rowCount()==0)
    return;
  QStandardItem* item = model()->invisibleRootItem();
  if (item) {
    QStandardItem* i = item->child(model()->rowCount()-1);
    if (i) {
      QModelIndex index=model()->indexFromItem(i);
      scrollTo(index);
    }
  }
}

/*
  Insert an item with its parent. Recurse if the parent is not yet inserted
*/
QStandardItem*
mail_listview::insert_sub_tree(std::map<mail_id_t,mail_msg*>& m, mail_msg *msg)
{
  QStandardItem* parent=NULL; // default to top-level item
  mail_id_t parent_id=msg->inReplyTo();
  if (parent_id!=0) {
    std::map<mail_id_t,mail_msg*>::iterator parent_iter=m.find(parent_id);
    if (parent_iter!=m.end()) {
      // has a parent in the list
      parent = model()->item_from_id(parent_id);
      if (!parent) {
	// recurse to insert msg's parent before msg
	parent = insert_sub_tree(m, parent_iter->second);
      }
    }
  }
  if (isSortingEnabled()) {
    return model()->insert_msg_sorted(msg, parent,
				      header()->sortIndicatorSection(),
				      header()->sortIndicatorOrder());
  }
  else {
    return model()->insert_msg(msg, parent);
  }
}

void
mail_listview::make_tree(std::list<mail_msg*>& list)
{
  DBG_PRINTF(8, "make_tree()");
  std::list<mail_msg*>::iterator iter;
  std::map<mail_id_t,mail_msg*> m;

  /* build a map [ mail_id => pointer to the message ]. The map is
     used inside insert_sub_tree to quickly find if a parent belongs
     to the list or not */
  for (iter=list.begin(); iter!=list.end(); ++iter) {
    m[(*iter)->get_id()]=*(iter);
  }

  for (iter=list.begin(); iter!=list.end(); ++iter) {
    mail_msg* msg = model()->find((*iter)->get_id());
    if (!msg) {			// msg not already in list
      insert_sub_tree(m, *iter);
    }
  }
  expandAll();
}

void
mail_listview::insert_list(std::list<mail_msg*>& list)
{
  if (m_display_threads) {
    make_tree(list);
  }
  else {
    mail_item_model* m = model();    
    msgs_filter::mlist_t::iterator it;
    for (it=list.begin(); it!=list.end(); ++it) {
      mail_msg* existing_msg = m->find((*it)->get_id());
      if (!existing_msg)
	m->insert_msg(*it);
    }
  }    
}

/* Hide/show the sender column or show/hide the recipient column and
   swap their positions */
void
mail_listview::swap_sender_recipient(bool show_recipient)
{
  if (show_recipient) {
    header()->setSectionHidden(mail_item_model::column_sender, true);
    header()->setSectionHidden(mail_item_model::column_recipient, false);
    header()->swapSections(mail_item_model::column_sender, mail_item_model::column_recipient);
    m_sender_column_swapped = true;
  }
  else {
    if (m_sender_column_swapped) {
      m_sender_column_swapped = false;
      header()->swapSections(mail_item_model::column_sender, mail_item_model::column_recipient);
      header()->setSectionHidden(mail_item_model::column_sender, false);
      header()->setSectionHidden(mail_item_model::column_recipient, true);
    }
  }
}

/* Redisplay certain fields following configuration changes */
void
mail_listview::change_display_config(const app_config& conf)
{
  bool update_dates = (conf.get_date_format_code() != model()->get_date_format());
  bool update_senders = (conf.get_string("sender_displayed_as") != model()->m_display_sender_mode);
  if (!update_dates && !update_senders)
    return; // no change

  model()->set_date_format(conf.get_date_format_code());
  model()->m_display_sender_mode = conf.get_string("sender_displayed_as");
  bool display_name = (model()->m_display_sender_mode == "name");
  int date_format = conf.get_date_format_code();
  QStandardItem* item = model()->first_top_level_item();
  QModelIndex index, index_below;

  while (item) {
    index=item->index();
    if (update_dates) {
      date_item* idate = dynamic_cast<date_item*>(model()->itemFromIndex(index.sibling(index.row(), mail_item_model::column_date)));
      if (idate) {
	idate->redisplay(date_format);
      }
    }

    if (update_senders) {
      QVariant v=item->data(mail_item_model::mail_msg_role);
      mail_msg* msg = v.value<mail_msg*>();
      QStandardItem* ifrom = model()->itemFromIndex(index.sibling(index.row(), mail_item_model::column_sender));
      if (msg && ifrom) {
	ifrom->setText(!display_name || msg->sender_name().isEmpty() ?
		       msg->From() : msg->sender_name());
      }
    }
    
    index_below  = indexBelow(index);
    if (index_below.isValid()) {
      item = model()->itemFromIndex(index_below);
    }
    else
      item=NULL;
  }
}
