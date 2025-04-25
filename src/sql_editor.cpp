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


#include "sql_editor.h"
#include "helper.h"

#include <QLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

sql_editor::sql_editor(QWidget* parent): QDialog(parent)
{
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing(5);
  topLayout->setMargin(5);

  m_edit=new QPlainTextEdit(this);
//  m_edit->setWordWrap(QMultiLineEdit::FixedColumnWidth);
//  m_edit->setWrapColumnOrWidth(40);
  m_edit->setTabChangesFocus(true);

  topLayout->addWidget(m_edit);

  QHBoxLayout* buttons = new QHBoxLayout();
  topLayout->addLayout(buttons);
  QPushButton* w = new QPushButton(tr("OK"), this);
  connect(w, SIGNAL(clicked()), this, SLOT(save_close()));
#if 0
  // TODO when a help section will be available for SQL queries
  QPushButton* w2 = new QPushButton(tr("Help"), this);
  connect(w2, SIGNAL(clicked()), this, SLOT(help()));
#endif

  QPushButton* w1 = new QPushButton(tr("Cancel"), this);
  connect(w1, SIGNAL(clicked()), this, SLOT(cancel()));

  QPushButton* btns[]={w,/*w2,*/w1};
  for (uint i=0; i<sizeof(btns)/sizeof(btns[0]); i++) {
    buttons->addStretch(10);
    buttons->addWidget(btns[i]);
  }
  buttons->addStretch(10);
  //  topLayout->addLayout (buttons, 10);
  setWindowTitle(tr("SQL subquery"));
  resize(500, 200);
}

#if 0
void
sql_editor::help()
{
  helper::show_help("sql editor");
}
#endif

void sql_editor::save_close()
{
  done(1);
}

void sql_editor::cancel()
{
  done(0);
}

void sql_editor::set_text(const QString& s)
{
  m_edit->setPlainText(s);
}

const QString sql_editor::get_text() const
{
  return m_edit->toPlainText();
}
