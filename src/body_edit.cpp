/* Copyright (C) 2004-2025 Daniel Verite

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
#include "body_edit.h"
#include "db.h"

#include <QPushButton>
#include <QTextEdit>
#include <QLayout>

body_edit::body_edit(QWidget* parent): QWidget(parent)
{
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  m_we = new QTextEdit(this);
  top_layout->addWidget(m_we);

  QHBoxLayout* hbl = new QHBoxLayout;
  top_layout->addLayout(hbl);
  hbl->addStretch(5);
  QPushButton* ok = new QPushButton(tr("OK"));
  hbl->addWidget(ok, 5);
  hbl->addStretch(5);
  QPushButton* cancel = new QPushButton(tr("Cancel"));
  hbl->addWidget(cancel, 5);
  hbl->addStretch(5);

  connect(ok, SIGNAL(clicked()), this, SLOT(ok()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(close()));
}

body_edit::~body_edit()
{
}

void
body_edit::set_contents(mail_id_t mail_id, const QString& txt)
{
  m_we->setPlainText(txt);
  m_we->document()->setModified(false);
  m_mail_id=mail_id;
  setWindowTitle(tr("Edit body of mail #%1").arg(mail_id));
}

void
body_edit::ok()
{
  const QString& t=m_we->toPlainText();
  mail_msg m;
  m.set_mail_id(m_mail_id);
  if (m_we->document()->isModified()) {
    if (m.update_body(t)) {
      emit text_updated(m_mail_id, &t);
      close();
    }
  }
  else {
    close();
  }
}
