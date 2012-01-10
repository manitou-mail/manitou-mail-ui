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

#include "addressbook.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <qpushbutton.h>
#include <QSpinBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include <map>

addrbook_entry::addrbook_entry(QWidget* parent, const QString email) :
  QWidget(parent)
{
  m_addr.fetch_details(email);

  setWindowTitle(QString(tr("Details for %1")).arg(email));
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setMargin (4);

  QWidget* cont = new QWidget();
  top_layout->addWidget(cont);

  int row=0;
  QGridLayout* grid = new QGridLayout(cont);
  //  top_layout->addLayout(grid);

  grid->addWidget(new QLabel(tr("Name:"), cont), row, 0);
  m_qlname = new QLineEdit(cont);
  grid->addWidget(m_qlname, row, 1);
  m_qlname->setText(m_addr.name());
  row++;

  grid->addWidget(new QLabel(tr("Email:"), cont), row, 0);
  m_qlemail = new QLineEdit(cont);
  m_qlemail->setText(email);
  m_qlemail->setReadOnly(true);
  grid->addWidget(m_qlemail, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Alias:"), cont), row, 0);
  m_qlalias = new QLineEdit(cont);
  m_qlalias->setText(m_addr.nickname());
  grid->addWidget(m_qlalias, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Priority:"), cont), row, 0);
  //  QHBoxLayout* pri_box = new QHBoxLayout(cont);
  m_prio = new QSpinBox();
  //  pri_box->addWidget(m_prio);
  //  pri_box->setStretchFactor(new Q3HBox(pri_box), 1);
  m_prio->setMinimum(-32768);
  m_prio->setMaximum(32767);
  m_prio->setValue(m_addr.recv_pri());
  grid->addWidget(m_prio, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Notes:"), cont), row, 0);
  m_notes = new QTextEdit(cont);
  m_notes->setText(m_addr.m_notes);
  grid->addWidget(m_notes, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Incoming (From):"), cont), row, 0);
  QLineEdit* ql_sent = new QLineEdit(cont);
  ql_sent->setReadOnly(true);
  QString msg_string = QString(tr("%1 email(s)")).arg(m_addr.nb_from());
  if (!m_addr.last_recv().is_null()) {
    msg_string += QString(tr(", last: %1")).arg(m_addr.last_recv().OutputHM(1));
  }
  QFontMetrics fm(ql_sent->font());
  ql_sent->setMinimumWidth(fm.width(msg_string)+15);
  ql_sent->setText(msg_string);
  grid->addWidget(ql_sent, row, 1);
  row++;

  grid->addWidget(new QLabel(tr("Outgoing (To):"), cont), row, 0);
  QLineEdit* ql_sent1 = new QLineEdit(cont);
  ql_sent1->setReadOnly(true);
  msg_string = QString(tr("%1 email(s)")).arg(m_addr.nb_to());
  if (!m_addr.last_sent().is_null()) {
    msg_string += QString(tr(", last: %1")).arg(m_addr.last_sent().OutputHM(1));
  }
  QFontMetrics fm1(ql_sent1->font());
  ql_sent1->setMinimumWidth(fm1.width(msg_string)+15);
  ql_sent1->setText(msg_string);
  grid->addWidget(ql_sent1, row, 1);
  row++;

  QHBoxLayout* hbl = new QHBoxLayout();
  top_layout->addLayout(hbl);
  hbl->setSpacing(20);
  hbl->setMargin(20);
  QPushButton* btn_ok = new QPushButton(tr("OK"), this);
  QPushButton* btn_cancel = new QPushButton(tr("Cancel"), this);
  connect(btn_ok, SIGNAL(clicked()), this, SLOT(ok()));
  connect(btn_cancel, SIGNAL(clicked()), this, SLOT(cancel()));
  //  hbl->addStretch(1);
  hbl->addWidget(btn_ok);
  hbl->addWidget(btn_cancel);
  //  hbl->addStretch(1);
}

addrbook_entry::~addrbook_entry()
{
}

void
addrbook_entry::widgets_to_addr(mail_address& a)
{
  a.m_address=m_qlemail->text();
  a.m_nickname=m_qlalias->text();
  a.m_notes=m_notes->toPlainText();
  a.m_name=m_qlname->text();
  a.m_recv_pri=m_prio->cleanText().toInt();
}

void
addrbook_entry::ok()
{
  mail_address new_addr;
  widgets_to_addr(new_addr);
  m_addr.diff_update(new_addr);
  close();
}

void
addrbook_entry::cancel()
{
  close();
}

void open_address_book(QString email)
{
  if (!email.isEmpty()) {
    addrbook_entry* w = new addrbook_entry(0, email);
    w->show();
  }
}

#if 0
// The address book is not practical enough yet to be of much use
// Leave all this code commented until it gets improved
address_book::address_book (QWidget *parent) : QWidget(parent)
{
  m_is_modified = false;
  Q3BoxLayout* top_layout = new Q3VBoxLayout (this);
  top_layout->setMargin (4);
  top_layout->setSpacing (4);
  Q3HBox* hb = new Q3HBox (this);
  hb->setSpacing (6);
  top_layout->addWidget (hb);

  Q3VBox* box_left = new Q3VBox (hb);
  Q3VBox* box_right = new Q3VBox (hb);

  // left pane: search criteria and results
  Q3GroupBox* search_box = new Q3GroupBox (3, Qt::Horizontal, tr("Search"), box_left);
  Q3VBox* labels_box = new Q3VBox(search_box);
  Q3VBox* fields_box = new Q3VBox(search_box);
  m_email_search_w = new QLineEdit (fields_box);
  (void)new QLabel (tr("Address"), labels_box);

  m_name_search_w = new QLineEdit (fields_box);
  (void)new QLabel (tr("Name"), labels_box);

  m_nick_search_w = new QLineEdit (fields_box);
  (void)new QLabel (tr("Alias"), labels_box);

  QPushButton* button_search = new QPushButton (tr("Search"), search_box);
  connect (button_search, SIGNAL(clicked()), this, SLOT(search()));

  Q3GroupBox* result_box = new Q3GroupBox (1, Qt::Vertical, tr("Results"), box_left);
  m_result_list_w = new Q3ListView (result_box);
  m_result_list_w->addColumn (tr("Email address"));
  m_result_list_w->addColumn (tr("# msgs"), 50);
  connect(m_result_list_w, SIGNAL(selectionChanged()),
	  this, SLOT(address_selected()));

  // right pane: properties of the selected address
  Q3GroupBox* prop_box = new Q3GroupBox (tr("Properties"), box_right);
  Q3GridLayout *grid_layout = new Q3GridLayout (prop_box, 6, 2);
  grid_layout->setSpacing (4);
  // set a marging big enough for the QGroupBox title not being overwritten
  grid_layout->setMargin (15);
  int g_row = 0;		// grid row

  m_email_w = new QLineEdit (prop_box);
  grid_layout->addWidget (new QLabel (tr("Address:"), prop_box),g_row,0);
  grid_layout->addWidget (m_email_w, g_row, 1);

  g_row++;
  m_name_w = new QLineEdit (prop_box);
  grid_layout->addWidget (new QLabel (tr("Name:"), prop_box), g_row, 0);
  grid_layout->addWidget (m_name_w, g_row, 1);

  g_row++;
  m_nick_w = new QLineEdit (prop_box);
  grid_layout->addWidget (new QLabel (tr("Alias:"), prop_box), g_row, 0);
  grid_layout->addWidget (m_nick_w, g_row, 1);

  g_row++;
  m_notes_w = new Q3MultiLineEdit (prop_box);
  grid_layout->addWidget (new QLabel (tr("Notes:"), prop_box), g_row, 0);
  grid_layout->addWidget (m_notes_w, g_row, 1);

  g_row++;
  m_invalid_w = new QCheckBox (prop_box);
  grid_layout->addWidget (new QLabel (tr("Invalid:"), prop_box), g_row, 0);
  grid_layout->addWidget (m_invalid_w, g_row, 1);

  g_row++;
  QPushButton* button_aliases = new QPushButton(tr("Other addresses..."), prop_box);
  grid_layout->addWidget (button_aliases, 5, 1);
  connect (button_aliases, SIGNAL(clicked()), this, SLOT(aliases()));

  // buttons at the bottom of the window
  Q3HBox* buttons_box = new Q3HBox (this);
  buttons_box->setSpacing (4);
  top_layout->addWidget (buttons_box);

  new Q3HBox (buttons_box);	// takes the left space
  QPushButton* b1 = new QPushButton (tr("New"), buttons_box);
  connect (b1, SIGNAL(clicked()), this, SLOT(new_address()));
  QPushButton* b2 = new QPushButton (tr("Apply"), buttons_box);
  connect (b2, SIGNAL(clicked()), this, SLOT(apply()));
  QPushButton* b3 = new QPushButton (tr("OK"), buttons_box);
  connect (b3, SIGNAL(clicked()), this, SLOT(OK()));
  QPushButton* b4 = new QPushButton (tr("Cancel"), buttons_box);
  connect (b4, SIGNAL(clicked()), this, SLOT(cancel()));
}

address_book::~address_book()
{
}

void
address_book::search()
{
  QString email = m_email_search_w->text();
  QString name = m_name_search_w->text();
  QString nick = m_nick_search_w->text();

  m_result_list_w->clear();
  m_result_list.clear();

  m_result_list.fetchLike (email, "");
  mail_address_list::const_iterator iter;
  for (iter = m_result_list.begin(); iter != m_result_list.end(); iter++) {
    (void) new Q3ListViewItem(m_result_list_w, iter->get(), QString("%1").arg(iter->nb_from()));
  }
}

void
address_book::address_selected()
{
  Q3ListViewItem* item = m_result_list_w->selectedItem();
  if (!item)
    return;
  QString email = item->text(0);
  mail_address addr;
  addr.fetch_details(email);
  m_email_w->setText(email);
  m_name_w->setText(addr.name());
  m_nick_w->setText(addr.nickname());
  m_invalid_w->setChecked(addr.m_invalid!=0);
  m_is_modified = false;
}

void
address_book::aliases()
{
  address_book_aliases* w = new address_book_aliases (m_email_w->text(), NULL);
  w->show();
}

void
address_book::OK()
{
  save();
  close();
}

void
address_book::cancel()
{
  close();
}

void
address_book::save()
{
  mail_address address;
  QString email = m_email_w->text();
  QString name = m_name_w->text();
  QString nick = m_nick_w->text();
  QString notes = m_notes_w->text();
  int invalid_email = m_invalid_w->isChecked() ? 1 : 0;

  address.fetch_details(email);
  if (name != address.m_name ||
      nick != address.m_nickname ||
      notes != address.m_notes ||
      invalid_email != address.m_invalid)
    {
      address.m_name = name;
      address.m_nickname = nick;
      address.m_notes = notes;
      address.m_invalid = invalid_email;
      address.store();
    }
}

void
address_book::apply()
{
  save();
  m_is_modified = false;
}

void
address_book::new_address()
{
}

address_book_aliases::address_book_aliases (const QString email,
						QWidget *parent) :
  QWidget(parent)
{
  m_email = email;
  Q3BoxLayout* top_layout = new Q3VBoxLayout (this);
  top_layout->setMargin (4);
  top_layout->setSpacing (3);
  Q3HBox* hb = new Q3HBox (this);
  top_layout->addWidget (hb);

  Q3VBox* box_left = new Q3VBox (hb);
  Q3VBox* box_middle = new Q3VBox (hb);
  Q3VBox* box_right = new Q3VBox (hb);

  // left pane: search criteria and results
  Q3GroupBox* search_box = new Q3GroupBox (3, Qt::Horizontal, tr("Search"), box_left);
  Q3VBox* labels_box = new Q3VBox(search_box);
  Q3VBox* fields_box = new Q3VBox(search_box);
  m_email_search_w = new QLineEdit (fields_box);
  (void)new QLabel (tr("Address"), labels_box);

  m_name_search_w = new QLineEdit (fields_box);
  (void)new QLabel (tr("Name"), labels_box);

  m_nick_search_w = new QLineEdit (fields_box);
  (void)new QLabel (tr("Nickname"), labels_box);

  QPushButton* button_search = new QPushButton (tr("Search"), search_box);
  connect (button_search, SIGNAL(clicked()), this, SLOT(search()));

  Q3GroupBox* result_box = new Q3GroupBox (1, Qt::Vertical, tr("Results"), box_left);
  m_result_list_w = new Q3ListView (result_box);
  m_result_list_w->addColumn (tr("Email address"));
  m_result_list_w->addColumn (tr("# msgs"), 50);
  m_result_list_w->setMultiSelection (true);

  // middle pane: the "Add" and "Remove" buttons
  QPushButton* b_add = new QPushButton (tr("Add ->"), box_middle);
  connect (b_add, SIGNAL(clicked()), this, SLOT(add_aliases()));

  QPushButton* b_remove = new QPushButton (tr("Remove"), box_middle);
  connect (b_remove, SIGNAL(clicked()), this, SLOT(remove_alias()));

  // right pane: aliases
  Q3GroupBox* aliases_box = new Q3GroupBox (1, Qt::Vertical, tr("Aliases"), box_right);
  m_aliases_list_w = new Q3ListView (aliases_box);
  m_aliases_list_w->addColumn(tr("Email address"));
  init_aliases_list ();

  // buttons at the bottom of the window
  Q3HBox* buttons_box = new Q3HBox (this);
  buttons_box->setSpacing (4);
  top_layout->addWidget (buttons_box);

  new Q3HBox (buttons_box);	// takes the left space
  QPushButton* b2 = new QPushButton (tr("Reset"), buttons_box);
  QPushButton* b3 = new QPushButton (tr("OK"), buttons_box);
  connect (b3, SIGNAL(clicked()), this, SLOT(OK()));
  QPushButton* b4 = new QPushButton (tr("Cancel"), buttons_box);
  connect (b4, SIGNAL(clicked()), this, SLOT(close()));

  QString title = tr("Other email addresses for ");
  title += email;
  setWindowTitle (title);
}

address_book_aliases::~address_book_aliases()
{
}

void
address_book_aliases::search()
{
  QString email = m_email_search_w->text();
  QString name = m_name_search_w->text();
  QString nick = m_nick_search_w->text();

  m_result_list_w->clear();
  m_result_list.clear();

  m_result_list.fetchLike (email, "");
  mail_address_list::const_iterator iter;
  for (iter = m_result_list.begin(); iter != m_result_list.end(); iter++) {
    // exclude our email from the list of possible aliases
    if (m_email != iter->get()) {
      (void) new Q3ListViewItem(m_result_list_w, iter->get(),
			       QString("%1").arg(iter->nb_from()));
    }
  }
}


void
address_book_aliases::remove_alias ()
{
  Q3ListViewItem* item = m_aliases_list_w->selectedItem();
  if (!item)
    return;
  m_remove_list.insert (item->text(0));
  delete item;
}

void
address_book_aliases::add_aliases ()
{
  const Q3ListView* l = m_result_list_w;
  Q3ListViewItem* i =l->firstChild();
  while (i) {
    if (i->isSelected()) {
      QString email = i->text(0);

      // add only if the item isn't already part of the alias list
      bool found=false;
      Q3ListViewItemIterator it (m_aliases_list_w);
      for ( ; it.current() && !found; ++it )
	found = (it.current()->text(0) == email);

      if (!found)
	Q3ListViewItem* inew = new Q3ListViewItem (m_aliases_list_w, email);
    }
    i = i->nextSibling();
  }
}

/* fill the alias listview with entries from the database */
void
address_book_aliases::init_aliases_list ()
{
  m_aliases_list_w->clear();

  mail_address address;
  address.set (m_email);
  mail_address_list aliases_list;
  address.fetch_aliases (&aliases_list);

  std::list<mail_address>::const_iterator iter;
  for (iter = aliases_list.begin(); iter != aliases_list.end(); iter++) {
    Q3ListViewItem* p = new Q3ListViewItem (m_aliases_list_w, iter->get());
  }
}

void
address_book_aliases::OK ()
{
  // map [email => addr_id] of aliases to m_email in the database
  std::map<QString,int> alias_map;

  mail_address address;
  bool found;
  address.fetchByEmail (m_email, &found);
  if (!found) {
    close();
    return;
  }

  // remove the aliases marked for removal
  std::set<QString>::const_iterator iter1;
  for (iter1 = m_remove_list.begin(); iter1 != m_remove_list.end(); iter1++) {
    address.remove_alias (*(iter1));
  }

  mail_address_list aliases_list;
  address.fetch_aliases (&aliases_list);

  // fill in alias_map
  std::list<mail_address>::const_iterator iter;
  for (iter = aliases_list.begin(); iter != aliases_list.end(); iter++) {
    alias_map[iter->get()] = iter->id();
  }

  // find the entries of the aliases widget that are not in the alias
  // map and add these aliases to the database
  Q3ListViewItemIterator it (m_aliases_list_w);
  for ( ; it.current(); ++it ) {
    std::map<QString,int>::const_iterator iter =
      alias_map.find (it.current()->text(0));
    if (iter == alias_map.end())
      address.add_alias (it.current()->text(0));
  }

  close();
}
#endif
