/* Copyright (C) 2004-2008 Daniel Vérité

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
#include <QLabel>
#include <QFrame>
#include "users.h"
#include <QMessageBox>
#include <QSpinBox>

properties_dialog::properties_dialog(mail_msg* i, QWidget* parent) :
  QDialog (parent)
{
  setWindowTitle(tr("Properties"));
  QVBoxLayout* topLayout = new QVBoxLayout (this);

  QHBoxLayout* hb0 = new QHBoxLayout();
  hb0->addWidget(new QLabel(QString("mail_id:%1").arg(i->GetId())));
  if (i->thread_id()) {
    hb0->addWidget(new QLabel(QString("thread_id:%1").arg(i->thread_id())));
  }
  topLayout->addLayout(hb0);

  m_status_box = new select_status_box (false, this);

  i->fetch_status();
  m_last_known_status = i->status();

  m_status_box->set_mask(i->status(), (~(i->status()))&(mail_msg::statusMax*2-1));
  m_status_box->setFrameStyle(QFrame::Box | QFrame::Sunken);
  m_status_box->setLineWidth (1);

  topLayout->addWidget(m_status_box);

  QHBoxLayout* hb1=new QHBoxLayout();
  topLayout->addLayout(hb1);
  hb1->setMargin(10);
  hb1->setSpacing(4);
  hb1->addWidget(new QLabel(tr("Priority:")));
  m_spinbox = new QSpinBox(this);
  hb1->addWidget(m_spinbox);
  m_spinbox->setMinimum(-32768);
  m_spinbox->setMaximum(32767);
  m_spinbox->setValue(i->priority());

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
  m_mail_id = i->GetId();
  topLayout->addStretch(1);
}

void properties_dialog::ok()
{
  mail_msg m;
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
  m.setStatus(m_status_box->status());
  m.update_status(true);
  bool ok;
  int pri = m_spinbox->text().toInt(&ok);
  if (ok) {
    m.set_priority(pri);
    m.update_priority();
  }
  emit change_status_request(m_mail_id, m_status_box->status(), pri);
  accept();
}
