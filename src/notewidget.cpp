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

#include "notewidget.h"
#include "icons.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QBrush>

note_widget::note_widget(QWidget* parent): QDialog(parent)
{
  setWindowIcon(UI_ICON(FT_ICON16_EDIT_NOTE));
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing(5);
  topLayout->setMargin(5);

  m_edit=new QPlainTextEdit(this);
  m_edit->resize(600,400);
  m_edit->setTabChangesFocus(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Active, QPalette::Base, QColor("#f1db20"));
  pal.setColor(QPalette::Inactive, QPalette::Text, QColor("#f1db20"));
  m_edit->setPalette(pal);

  topLayout->addWidget(m_edit);

  QHBoxLayout* buttons = new QHBoxLayout();
  topLayout->addLayout(buttons);
  QPushButton* w = new QPushButton(tr("Save/close"), this);
  connect (w, SIGNAL(clicked()), this, SLOT(save_close()));

  QPushButton* w2 = new QPushButton(tr("Delete"), this);
  connect (w2, SIGNAL(clicked()), this, SLOT(delete_note()));

  QPushButton* w1 = new QPushButton(tr("Cancel"), this);
  connect (w1, SIGNAL(clicked()), this, SLOT(cancel()));

  buttons->addWidget (w);
  buttons->addStretch (10);
  buttons->addWidget (w2);
  buttons->addStretch (10);
  buttons->addWidget (w1);
  setWindowTitle (tr("Note"));
}

void note_widget::save_close()
{
  done(1);
}

void note_widget::cancel()
{
  done(0);
}

void note_widget::delete_note()
{
  m_edit->setPlainText(QString());
  done(1);
}

void note_widget::set_note_text(const QString& s)
{
  m_edit->setPlainText(s);
  m_edit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
}

const QString
note_widget::get_note_text() const
{
  return m_edit->toPlainText().trimmed();
}
