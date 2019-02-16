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
#include "msg_properties.h"
#include "selectmail.h"

#include <QPushButton>
#include <QLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include "users.h"
#include <QMessageBox>
#include <QSpinBox>
#include <QDateTimeEdit>

properties_dialog::properties_dialog(mail_msg* msg, QWidget* parent) :
  QDialog (parent)
{
  setWindowTitle(tr("Properties"));
  QVBoxLayout* topLayout = new QVBoxLayout (this);

  QHBoxLayout* hb0 = new QHBoxLayout();
  hb0->addWidget(new QLabel(QString("mail_id:%1").arg(msg->GetId())));
  if (msg->thread_id()) {
    hb0->addWidget(new QLabel(QString("thread_id:%1").arg(msg->thread_id())));
  }
  topLayout->addLayout(hb0);

  m_status_box = new select_status_box (false, this);

  msg->fetch_status();
  m_last_known_status = msg->status();

  m_status_box->set_mask(msg->status(), (~(msg->status()))&(mail_msg::statusMax*2-1));
  m_status_box->setFrameStyle(QFrame::Box | QFrame::Sunken);
  m_status_box->setLineWidth (1);

  topLayout->addWidget(m_status_box);

  QFormLayout* hb1=new QFormLayout;
  topLayout->addLayout(hb1);
  hb1->setMargin(10);
  hb1->setSpacing(4);
  m_spinbox = new QSpinBox(this);
  hb1->addRow(tr("Priority:"), m_spinbox);
  m_spinbox->setMinimum(-32768);
  m_spinbox->setMaximum(32767);
  m_spinbox->setValue(msg->priority());

  if (msg->status() & mail_msg::statusScheduled) {
    QDateTime dt = msg->get_send_datetime();
    if (dt.isValid()) {
      m_send_datetime = new QDateTimeEdit;
      hb1->addRow(tr("Send after:"), m_send_datetime);
      m_send_datetime->setDateTime(dt);
    }
  }
  else
    m_send_datetime = NULL;

  QHBoxLayout* hbox=new QHBoxLayout();
  topLayout->addLayout(hbox);
  hbox->setMargin(10);
  hbox->setSpacing(20);

  QPushButton* wok = new QPushButton(tr("OK"));
  QPushButton* wcancel = new QPushButton(tr("Cancel"));
  hbox->addWidget(wok);
  hbox->addWidget(wcancel);
  connect(wok, SIGNAL(clicked()), this, SLOT(ok()));
  connect(wcancel, SIGNAL(clicked()), this, SLOT(reject()));

  //  resize (200, 200);
  m_mail_id = msg->GetId();
  topLayout->addStretch(1);
}

void properties_dialog::ok()
{
  mail_msg m;
  bool success;
  bool pri_ok;
  int pri;
  m.SetId(m_mail_id);
  m.fetch_status();
  if (m.status() != m_last_known_status) {
    QString username = users_repository::name(m.status_last_user());
    if (username.isEmpty())
      username = tr("another user");
    QString msg=tr("The message has been modified by %1 while you were editing it.\n"
		   "Do you want to update it anyway?").arg(username);
    int res = QMessageBox::warning (this, APP_NAME, msg, tr("OK"), tr("Cancel"), 0, 0, 1);
    if (res==1) return;		// cancel
  }

  uint new_status = m_status_box->status();

  db_ctxt dbc;
  dbc.propagate_exceptions = true;
  QList<tag_counter_transition> cnt_tags_changed;

  try {
    dbc.m_db->begin_transaction();

    m.set_status(new_status);

    // Handle the transition in tags_counters (but don't update the status in db yet)
    if (db_manitou_config::has_tags_counters()) {
      sql_stream s("SELECT id,c FROM transition_status_tags(:id, :status) as t(id,c)", *dbc.m_db);
      s << m.get_id() << new_status;

      while (!s.eos()) {
	int tag_id, cnt;
	s >> tag_id >> cnt;
	cnt_tags_changed.append(tag_counter_transition(tag_id, cnt));
      }
    }

    // Update in database
    m.update_status(true, &dbc);
    pri = m_spinbox->text().toInt(&pri_ok);
    if (pri_ok) {
      m.set_priority(pri);
      m.update_priority();
    }

    // send later
    if (m_send_datetime != NULL) {
      if (m.status() & mail_msg::statusScheduled) {
	QDateTime dt = m_send_datetime->dateTime();
	m.store_send_datetime(dt, &dbc);
      }
      else if (m_last_known_status & mail_msg::statusScheduled) {
	/* if the scheduled bit was set, set an empty datetime to suppress
	   the send job. */
	m.store_send_datetime(QDateTime(), &dbc);
      }
    }
    success = true;
    dbc.m_db->commit_transaction();
  }
  catch(db_excpt& p) {
    DBG_PRINTF(3, "exception caught");
    dbc.m_db->rollback_transaction();
    success = false;
    DBEXCPT (p);
  }
  if (success && pri_ok) {
    emit change_status_request(m_mail_id, m_status_box->status(), pri);
    if (cnt_tags_changed.size() > 0)
      emit tags_counters_changed(cnt_tags_changed);
    accept();
  }
  else
    reject();
}
