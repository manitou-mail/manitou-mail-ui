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
#include "db_error_dialog.h"

#include <QMessageBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QSizePolicy>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QClipboard>


db_error_dialog::db_error_dialog(const QString error) : m_error_text(error)
{
  setWindowTitle(tr("Database error"));
  QVBoxLayout* layout = new QVBoxLayout(this);

  setMinimumWidth(400);		// reasonable minimum

  QPlainTextEdit* label = new QPlainTextEdit;
  label->setPlainText(error);
  label->setReadOnly(true);
  label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  label->setFrameShape(QFrame::Box);
  label->setFrameShadow(QFrame::Raised);
  layout->addWidget(label);

  m_btn_group = new QButtonGroup(this);
  QButtonGroup* g = m_btn_group;

  QRadioButton* btn1 = new QRadioButton(tr("Continue"));
  g->addButton(btn1, 1);
  btn1->setChecked(true);

  QRadioButton* btn2 = new QRadioButton(tr("Try to reconnect"));
  g->addButton(btn2, 2);

  QRadioButton* btn3 = new QRadioButton(tr("Quit application"));
  g->addButton(btn3, 3);

  layout->addWidget(btn1);
  layout->addWidget(btn2);
  layout->addWidget(btn3);

  QPushButton* btn_OK = new QPushButton(tr("OK"));
  btn_OK->setDefault(true);

  QPushButton* copy_btn = new QPushButton(tr("Copy"));

  QDialogButtonBox* buttonBox = new QDialogButtonBox(Qt::Horizontal);
  buttonBox->addButton(copy_btn, QDialogButtonBox::ActionRole);
  buttonBox->addButton(btn_OK, QDialogButtonBox::AcceptRole);
  layout->addWidget(buttonBox);
  connect(copy_btn, SIGNAL(clicked()), this, SLOT(copy_error()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(this, SIGNAL(accepted()), this, SLOT(handle_error()));
}

/* copy the error text into the clipboard */
void
db_error_dialog::copy_error()
{
  QClipboard* clipboard = QApplication::clipboard();
  if (clipboard) {
    clipboard->setText(m_error_text);
    QMessageBox::information(this, APP_NAME, tr("The message has been copied to the clipboard."));
  }
}

void
db_error_dialog::handle_error()
{
  int btn_id = m_btn_group->checkedId();
  DBG_PRINTF(3, "handle_error clicked=%d", btn_id);

  if (btn_id == action_quit) {
    gl_pApplication->cleanup();
    // we want to really quit, not just leave the main event loop
    ::exit(1);
  }

  if (btn_id == action_reconnect) {
    db_cnx db;
    if (!db.ping()) {
      DBG_PRINTF(3, "No reply to database ping");
      if (!db.datab()->reconnect()) {
	DBG_PRINTF(3, "Failed to reconnect to database");
	return;
      }
      else {
	DBG_PRINTF(3, "Database reconnect successful");
      }
    }    
  }

  // when btn_id == action_continue, there is nothing to do.

  close();
}
