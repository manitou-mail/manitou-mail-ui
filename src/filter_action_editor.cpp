/* Copyright (C) 2004-2020 Daniel Verite

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

#include "filter_action_editor.h"
#include "tagsdialog.h"
#include "main.h"
#include "identities.h"

#include <QVBoxLayout>
#include <QStackedLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QEvent>
#include <QFontMetrics>

//
// action_line
//
action_line::action_line(QWidget* parent, const QString& label, 
			 const QString& type) : QWidget(parent)
{
  Q_UNUSED(label);
  m_type = type;
  //  setContentsMargins(0,0,0,0);
  m_default_layout = new QVBoxLayout(this);
  m_default_layout->setAlignment(Qt::AlignTop|Qt::AlignLeft);
  m_descr = new QLabel();
  m_descr->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_descr->setWordWrap(true);
  m_descr->setMargin(8);
  m_default_layout->addWidget(m_descr);
  m_default_layout->addSpacing(10);
}

void
action_line::set_description(const QString text)
{
  m_descr->setText(text);
}

QBoxLayout*
action_line::default_layout()
{
  return m_default_layout;
}

//
// action_tag
//
action_tag::action_tag(QWidget* parent) :
  action_line(parent, tr("Assign tag"), "tag")
{
  set_description(tr("This action assigns a tag to the current message. If the desired tag does not exist already, use the 'Edit tags' button to create it with the tag editor. To assign more than one tag, create several actions."));

  QBoxLayout* layout = default_layout();
  m_qc_tag = new tag_selector(this);
  layout->addWidget(m_qc_tag);
  m_qc_tag->init(false);
  layout->setMargin(4);
  QHBoxLayout* btnl = new QHBoxLayout;
  layout->addLayout(btnl);
  m_edit_btn = new QPushButton(tr("Edit tags"));
  btnl->setAlignment(Qt::AlignCenter);
  //setStretchFactor(m_edit_btn, 0);
  btnl->addWidget(m_edit_btn);
  connect(m_edit_btn, SIGNAL(clicked()), SLOT(edit_tags()));
}

/*
  Launch the tag editor in modal mode, and when it gets closed,
  refresh the tag list to reflect the modifications
*/
void
action_tag::edit_tags()
{
  tags_dialog w(0);
  int r = w.exec();
  if (r==QDialog::Accepted) {
    m_qc_tag->init(false);
  }
}

QString
action_tag::get_param()
{
  return m_qc_tag->currentText();
}

void
action_tag::set_param(const QString& tag)
{
  int i = m_qc_tag->findText(tag);
  if (i == -1) { // if not found, then add the item
    m_qc_tag->addItem(tag);
    i = m_qc_tag->findText(tag);
  }
  m_qc_tag->setCurrentIndex(i);
}

void
action_tag::reset()
{
  m_qc_tag->setCurrentIndex(-1);
}

action_discard::action_discard(QWidget* parent):
  action_line(parent, tr("Discard"), "discard")
{
  set_description(tr("Delete the message or move it to the trash can. Once a message is deleted, it cannot be recovered."));
  m_trash = new QRadioButton(tr("Move the message to the trash can."), this);
  m_delet = new QRadioButton(tr("Physically delete the message"), this);
  m_trash->setChecked(true);
  default_layout()->addWidget(m_trash);
  default_layout()->addWidget(m_delet);  
}

QString
action_discard::get_param()
{
  return m_trash->isChecked() ? "trash" : "delete";
}

void
action_discard::set_param(const QString& param)
{
  m_trash->setChecked(param=="trash");
  m_delet->setChecked(param=="delete");
}

//
// action_status
//
const char* action_status::status_text[nb_status] = {
  QT_TR_NOOP("Read"),
  QT_TR_NOOP("Archived"),
  QT_TR_NOOP("Forwarded")
};

const char action_status::status_letter[nb_status] = {
  'R', 'A', 'F'
};

action_status::action_status(QWidget* parent):
  action_line(parent, tr("Set status"), "status")
{
  set_description(tr("Set the status of the message."));
  for (int i=0; i<nb_status; i++) {
    m_check[i] = new QCheckBox(tr(status_text[i]), this);
    CHECK_PTR(m_check[i]);
    default_layout()->addWidget(m_check[i]);
  }  
}

QString
action_status::get_param()
{
  QString res;
  for (int i=0; i<nb_status; i++) {
    if (m_check[i]->isChecked()) {
      if (!res.isEmpty())
	res.append('+');
      res.append(status_letter[i]);
    }
  }
  return res;
}

/*
  Set the status checkboxes according to 'param', which should be
  of the form R+A+... (read+archived+...)
*/
void
action_status::set_param(const QString& param)
{
  reset();
  QStringList l=param.split('+', QString::KeepEmptyParts);
  for (QStringList::Iterator it=l.begin(); it!=l.end(); ++it) {
    QChar s=(*it).at(0);
    for (int i=0; i<nb_status; i++) {
      if (s==status_letter[i])
	m_check[i]->setChecked(true);
    }
  }
}

void
action_status::reset()
{
  for (int i=0; i<nb_status; i++) {
    m_check[i]->setChecked(false);
  }
}

//
// action_prio
//
action_prio::action_prio(QWidget* parent):
  action_line(parent, tr("Set priority"), "priority")
{
  set_description(tr("Change the internal priority of the message. The priority can either be set, replacing the current value, or adjusted by adding a positive or negative increment to the current value."));
  QBoxLayout* layout = default_layout();
  m_check_set=new QRadioButton(tr("Set priority"), this);
  connect(m_check_set, SIGNAL(toggled(bool)), this, SLOT(toggle_set(bool)));
  layout->addWidget(m_check_set);

  m_prio_set = new QSpinBox(this);
  m_prio_set->setMinimum(-1000);
  m_prio_set->setMaximum(1000);
  QFontMetrics fm(m_prio_set->font());
  m_prio_set->setMaximumWidth(fm.width("0")*10);
  layout->addWidget(m_prio_set);

  m_check_add = new QRadioButton(tr("Add priority"), this);
  connect(m_check_add, SIGNAL(toggled(bool)), this, SLOT(toggle_add(bool)));
  layout->addWidget(m_check_add);

  m_prio_add = new QSpinBox(this);
  m_prio_add->setMinimum(-1000);
  m_prio_add->setMaximum(1000);
  m_prio_add->setMaximumWidth(fm.width("0")*10);
  layout->addWidget(m_prio_add);

  m_check_set->setChecked(true); // default
}


void
action_prio::toggle_set(bool b)
{
  if (b) {
    m_prio_add->setEnabled(false);
    m_check_add->setChecked(false);
    m_prio_set->setEnabled(true);
  }
}

void
action_prio::toggle_add(bool b)
{
  if (b) {
    m_prio_set->setEnabled(false);
    m_check_set->setChecked(false);
    m_prio_add->setEnabled(true);
  }
}


void
action_prio::reset()
{
  DBG_PRINTF(6, "action_prio::reset");
  blockSignals(true);
  m_check_set->setChecked(false);
  m_check_add->setChecked(false);
  m_prio_add->setValue(0);
  m_prio_set->setValue(0);
  m_prio_add->setEnabled(false);
  m_prio_set->setEnabled(false);
  blockSignals(false);
}

QString
action_prio::get_param()
{
  QString res;
  if (m_check_set->isChecked())
    res = "=" + m_prio_set->text();
  if (m_check_add->isChecked())
    res = "+=" + m_prio_add->text();
//  DBG_PRINTF(6, "action_prio::get_param returns '%s'\n", res.latin1());
  return res;
}

/*
  'param' should be "=value" or "+=value", value being an integer
*/
void
action_prio::set_param(const QString& param)
{
//  DBG_PRINTF(6, "action_prio::set_param('%s')\n", param.latin1());
  blockSignals(true);		// avoid signals for change of value
  if (param.at(0)=='=') {
    m_prio_set->setValue(param.mid(1).toInt());
    m_check_set->setChecked(true);
    m_check_add->setChecked(false);
  }
  else if (param.mid(0,2)=="+=") {
    m_prio_add->setValue(param.mid(2).toInt());
    m_check_add->setChecked(true);
    m_check_set->setChecked(false);
  }
  // else error
  blockSignals(false);
}

action_add_header::action_add_header(QWidget* parent):
  action_line(parent, tr("Set header"), "set header")
{
  QBoxLayout* layout = default_layout();
  layout->addWidget(new QLabel(tr("Header field")));
  m_header_name = new QLineEdit();
  layout->addWidget(m_header_name);

  layout->addWidget(new QLabel(tr("Operation")));
  m_set_value = new QRadioButton(tr("Set or replace the field contents by the value"));
  m_set_value->setChecked(true);
  m_prepend_value = new QRadioButton(tr("Prepend the value to the field contents"));
  m_append_value = new QRadioButton(tr("Append the value to the field contents"));
  layout->addWidget(m_set_value);
  layout->addWidget(m_prepend_value);
  layout->addWidget(m_append_value);

  layout->addWidget(new QLabel(tr("Value")));
  m_header_value = new QLineEdit();
  layout->addWidget(m_header_value);
  set_description(tr("Set a header field. If the field is already present, its contents will be updated with the new value. Otherwise the field will be added."));
}

void
action_add_header::set_param(const QString& param)
{
  int i = param.indexOf(':');
  if (i>0) {
    m_header_name->setText(param.mid(0,i));
    int j = param.indexOf(' ', i+1);
    if (j>0) {
      QString op = param.mid(i+1, j-i-1);
      if (op=="set") m_set_value->setChecked(true);
      else if (op=="append") m_append_value->setChecked(true);
      else if (op=="prepend") m_prepend_value->setChecked(true);
      m_header_value->setText(param.mid(j+1));
    }
    else
      m_header_value->setText(param.mid(i+1));
  }
}

void
action_add_header::reset()
{
  m_header_name->clear();
  m_header_value->clear();
}

QString
action_add_header::get_param()
{
  QString op = "set";
  if (m_append_value->isChecked())
    op = "append";
  else if (m_prepend_value->isChecked())
    op = "prepend";
  return QString("%1:%2 %3").arg(m_header_name->text()).arg(op).arg(m_header_value->text());
}

action_remove_header::action_remove_header(QWidget* parent):
  action_line(parent, tr("Remove header"), "remove header")
{
  QBoxLayout* layout = default_layout();
  layout->addWidget(new QLabel(tr("Header field name")));
  m_header_name = new QLineEdit();
  layout->addWidget(m_header_name);
  set_description(tr("Remove any occurrence of the designated field from the header."));
}

void
action_remove_header::set_param(const QString& param)
{
  m_header_name->setText(param);
}

void
action_remove_header::reset()
{
  m_header_name->clear();
}

QString
action_remove_header::get_param()
{
  return m_header_name->text();
}

action_set_identity::action_set_identity(QWidget* parent) :
  action_line(parent, tr("Set identity"), "set identity")
{
  QBoxLayout* layout = default_layout();
  layout->addWidget(new QLabel(tr("Identity:")));
  set_description(tr("Change the identity assigned to the message. The identity is one of our email addresses as defined in the Identity tab of the preferences. For incoming message, the value is determined at import time by manitou-mdx. For outgoing messages, it can be selected in the Identity menu of the composer window. The purpose of this action is to override this identity."));
  m_cb_ident = new QComboBox();
  layout->addWidget(m_cb_ident);
  identities ids;
  if (ids.fetch()) {
    identities::iterator iter;
    for (iter = ids.begin(); iter != ids.end(); ++iter) {
      mail_identity* p = &iter->second;
      m_cb_ident->addItem(p->m_email_addr);
    }
  }
}

void
action_set_identity::set_param(const QString& email)
{
  int i = m_cb_ident->findText(email);
  if (i == -1) { // if not found, then add the item
    m_cb_ident->addItem(email);
    i = m_cb_ident->findText(email);
  }
  m_cb_ident->setCurrentIndex(i);
}

QString
action_set_identity::get_param()
{
  return m_cb_ident->currentText();
}


action_stop::action_stop(QWidget* parent) :
  action_line(parent, tr("Stop filters"), "stop")
{
  set_description(tr("Stop the filtering process for the current message. There are no parameters for this action."));
}


QString
action_line::get_param()
{
  return QString();
}

void
action_line::reset()
{
}

void
action_line::set_param(const QString& param)
{
  Q_UNUSED(param);		// base action has no parameter
}

/* action_redirect */

action_redirect::action_redirect(QWidget* parent) :
  action_line(parent, tr("Redirect to"), "redirect")
{
  m_redirect = new QLineEdit;
  QBoxLayout* layout = default_layout();
  QLabel* label = new QLabel(tr("Email address to redirect to:"));
  layout->addWidget(label);
  layout->addWidget(m_redirect);
}


filter_action_chooser::filter_action_chooser(QWidget* parent): QWidget(parent)
{
  QVBoxLayout* l0 = new QVBoxLayout(this);
  QHBoxLayout* l1 = new QHBoxLayout; // radiobuttons + descr text

  l0->addLayout(l1);
  QVBoxLayout* rbl = new QVBoxLayout;
  l1->addLayout(rbl);

  m_group = new QButtonGroup(this);
  connect(m_group, SIGNAL(buttonClicked(int)), this, SLOT(action_choosen(int)));
  for (int idx=0; idx<action_line::idx_max; idx++) {
    filter_action_chooser_radio_button* rb = new filter_action_chooser_radio_button(action_line::label(idx));
    m_group->addButton(rb, idx);
    connect(rb, SIGNAL(entered(int)), this, SLOT(hover_on_choice(int)));
    connect(rb, SIGNAL(left(int)), this, SLOT(hover_off_choice(int)));
    rbl->addWidget(rb);
  }

  QVBoxLayout* vbl = new QVBoxLayout;
  l1->addLayout(vbl);
  m_description = new QLabel;
  // set the description to ~40 chars wide
  m_description->setWordWrap(true);
  QFontMetrics fm(m_description->font());
  m_description->setFixedWidth(fm.averageCharWidth()*40);
  vbl->addWidget(m_description);
  vbl->addStretch(1);

  QHBoxLayout* btn_row = new QHBoxLayout;
  // move btn_row to the bottom right
  l0->addStretch(1);
  l0->addLayout(btn_row);
  btn_row->addStretch(1);
  QPushButton* cont = new QPushButton(tr("Add action"));
  connect(cont, SIGNAL(clicked()), this, SLOT(cont()));
  btn_row->addWidget(cont);

  QPushButton* cancel = new QPushButton(tr("Cancel"));
  btn_row->addWidget(cancel);
  connect(cancel, SIGNAL(clicked()), this, SIGNAL(cancelled()));
}

void
filter_action_chooser::hover_on_choice(int id)
{
  if (!m_group->checkedButton()) {
    QString descr = action_line::description(id);
    m_description->setText(tr("Description:")+"<br><br>"+descr);
  }
}

void
filter_action_chooser::hover_off_choice(int id)
{
  Q_UNUSED(id);
  if (!m_group->checkedButton()) {
    m_description->setText("");
  }
}

filter_action_chooser_radio_button::filter_action_chooser_radio_button(const QString& text, QWidget* parent) : QRadioButton(text, parent)
{
}

void
filter_action_chooser_radio_button::enterEvent(QEvent* event)
{
  QButtonGroup* g = group();
  if (g) {
    emit entered(g->id(this));
  }
  QRadioButton::enterEvent(event);
}

void
filter_action_chooser_radio_button::leaveEvent(QEvent* event)
{
  QButtonGroup* g = group();
  if (g) {
    emit left(g->id(this));
  }
  QRadioButton::leaveEvent(event);
}


void
filter_action_chooser::action_choosen(int id)
{
  QString descr = action_line::description(id);
  m_description->setText(tr("Description:")+"<br><br>"+descr);
}

void
filter_action_chooser::cont()
{
  // switch the panel to the specialized widget for the choosen action
  int action_id = m_group->checkedId();
  emit open_action(action_id);
}

void
filter_action_chooser::reset_choice()
{
  QRadioButton* btn = dynamic_cast<QRadioButton*>(m_group->checkedButton());
  if (btn) {
    m_group->setExclusive(false);
    btn->setChecked(false);
    m_group->setExclusive(true);
    m_description->clear();
  }
}

//static
struct action_line::action_description
action_line::m_descriptions[action_line::idx_max] = {
  { QT_TRANSLATE_NOOP("filter/action_line", "Assign tag"),
    QT_TRANSLATE_NOOP("filter/action_line", "Assign a tag to the message.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Set status"),
    QT_TRANSLATE_NOOP("filter/action_line", "Force the status of the message as already read, archived or trashed.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Set priority"),
    QT_TRANSLATE_NOOP("filter/action_line", "Set the priority of the message, either with a fixed value or a relative increment.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Redirect"),
    QT_TRANSLATE_NOOP("filter/action_line", "Resend the message to a different email address.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Stop filters"),
    QT_TRANSLATE_NOOP("filter/action_line", "Stop the filtering process for the current message.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Set header"),
    QT_TRANSLATE_NOOP("filter/action_line", "Set a message header field. If the field is already present, it will be updated with the new value, otherwise the field will be added.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Remove header"),
    QT_TRANSLATE_NOOP("filter/action_line", "Remove a message header field. If it appears multiple times, all occurrences will be removed.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Set identity"),
    QT_TRANSLATE_NOOP("filter/action_line", "Change the identity assigned to the message to one of the other email addresses defined in the preferences.") },
  { QT_TRANSLATE_NOOP("filter/action_line", "Discard"),
    QT_TRANSLATE_NOOP("filter/action_line", "Discard the current message.") }
};

//static
QString
action_line::label(int action_idx)
{
  return (action_idx>=0 && action_idx<action_line::idx_max) ?
    QCoreApplication::translate("filter/action_line",
				m_descriptions[action_idx].label)
    : QString("");
}

//static
QString
action_line::description(int action_idx)
{
  return (action_idx>=0 && action_idx<action_line::idx_max) ?
    QCoreApplication::translate("filter/action_line",
				m_descriptions[action_idx].description)
    : QString("");
}

filter_action_editor::filter_action_editor(QWidget* parent) : QDialog(parent)
{
  m_stackl = new QStackedLayout(this);
  m_chooser = new filter_action_chooser;
  connect(m_chooser, SIGNAL(open_action(int)), this, SLOT(display_action(int)));
  connect(m_chooser, SIGNAL(cancelled()), this, SLOT(reject()));
  m_stackl->insertWidget(0, m_chooser);

  for (int idx=0; idx<action_line::idx_max; idx++) {
    if (idx==action_line::idx_tag)
      w_actions[idx] = new action_tag();
    else if (idx==action_line::idx_status)
      w_actions[idx] = new action_status();
    else if (idx==action_line::idx_prio)
      w_actions[idx] = new action_prio();
    else if (idx==action_line::idx_stop)
      w_actions[idx] = new action_stop();
    else if (idx==action_line::idx_redirect)
      w_actions[idx] = new action_redirect();
    else if (idx==action_line::idx_add_header)
      w_actions[idx] = new action_add_header();
    else if (idx==action_line::idx_remove_header)
      w_actions[idx] = new action_remove_header();
    else if (idx==action_line::idx_set_identity)
      w_actions[idx] = new action_set_identity();
    else if (idx==action_line::idx_discard)
      w_actions[idx] = new action_discard();
    else
      continue;

    QBoxLayout* layout = w_actions[idx]->default_layout();
    layout->addStretch(1);
    QHBoxLayout* btns = new QHBoxLayout;
    layout->addLayout(btns);
    QPushButton* b1 = new QPushButton(tr("OK"));
    connect(b1, SIGNAL(clicked()), this, SLOT(accept()));
    QPushButton* b2 = new QPushButton(tr("Cancel"));
    connect(b2, SIGNAL(clicked()), this, SLOT(reject()));
    btns->addStretch(1);
    btns->addWidget(b1);
    btns->addWidget(b2);    
    btns->addStretch(1);
    
    m_stackl->insertWidget(idx+1, w_actions[idx]);
  }

}

filter_action
filter_action_editor::get_action()
{
  filter_action a;
  int idx = m_stackl->currentIndex()-1;
  if (idx>=0 && idx<action_line::idx_max) {
    a.m_str_atype = w_actions[idx]->m_type;
    a.m_action_arg = w_actions[idx]->get_param();
  }
  return a;
}

void
filter_action_editor::reset_actions()
{
  int idx = m_stackl->currentIndex()-1;
  if (idx>=0 && idx<action_line::idx_max) {
    w_actions[idx]->reset();
  }
  m_stackl->setCurrentIndex(0);
  m_chooser->reset_choice();
}

void
filter_action_editor::display_action_chooser()
{
  m_stackl->setCurrentIndex(0);
}

void
filter_action_editor::display_action(const filter_action* a)
{
  for (int i=0; i<action_line::idx_max; i++) {
    if (w_actions[i]->m_type == a->m_str_atype) {
      w_actions[i]->set_param(a->m_action_arg);
      m_stackl->setCurrentIndex(i+1);
      break;
    }
  }
}

void
filter_action_editor::display_action(int i)
{
  m_stackl->setCurrentIndex(i+1);
}
