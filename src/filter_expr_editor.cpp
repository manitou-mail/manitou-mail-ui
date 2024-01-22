/* Copyright (C) 2004-2024 Daniel Verite

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

#include "filter_expr_editor.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>

void
filter_expr_editor::add_row(QGridLayout* gl, int row)
{
  int col=0;
  m_logop = new QComboBox;
  m_logop->addItem("NOT");
  m_logop->addItem("AND");
  m_logop->addItem("OR");
  m_logop->addItem(tr("AND (...)"));
  m_logop->addItem(tr("OR (...)"));

  m_combo_header = new QComboBox;
  m_combo_header->addItem(tr("Subject"));
  m_combo_header->addItem(tr("From"));
  m_combo_header->addItem(tr("To"));
  m_combo_header->addItem(tr("Cc"));

  m_op = new QComboBox;
  m_op->addItem(tr("is"));
  m_op->addItem(tr("is not"));
  m_op->addItem(tr("contains"));
  m_op->addItem(tr("starts with"));
  m_op->addItem(tr("ends with"));
  m_op->addItem(tr("matches regexp"));

  m_operand2 = new QLineEdit;

  col=0; row++;
  gl->addWidget(m_logop, row, col++ /*, Qt::AlignHCenter*/);
  gl->addWidget(m_combo_header, row, col++);
  gl->addWidget(m_op, row, col++);
  gl->addWidget(m_operand2, row, col++);
}

filter_expr_editor::filter_expr_editor(QWidget* parent): QDialog(parent)
{
  setWindowTitle(tr("Filter expression editor"));
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  QGridLayout* gl = new QGridLayout;
  top_layout->addLayout(gl);
  
  int row=0;
  int col=0;
  gl->addWidget(new QLabel(tr("Negate")), row, col++);
  gl->addWidget(new QLabel(tr("Argument")), row, col++);
  gl->addWidget(new QLabel(tr("Operator")), row, col++);
  gl->addWidget(new QLabel(tr("Operand")), row, col++);

  add_row(gl, ++row);

  QHBoxLayout* btns = new QHBoxLayout;
  top_layout->addLayout(btns);
  top_layout->addStretch(1);
  QPushButton* b1 = new QPushButton(tr("OK"));
  connect(b1, SIGNAL(clicked()), this, SLOT(accept()));
  QPushButton* b2 = new QPushButton(tr("Cancel"));
  connect(b2, SIGNAL(clicked()), this, SLOT(reject()));
  btns->addStretch(1);
  btns->addWidget(b1);
  btns->addWidget(b2);    
  btns->addStretch(1);

}

filter_expr_text_editor::filter_expr_text_editor(QWidget* parent): QDialog(parent)
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

  QPushButton* w2 = new QPushButton(tr("Help"), this);
  connect(w2, SIGNAL(clicked()), this, SLOT(help()));

  QPushButton* w1 = new QPushButton(tr("Cancel"), this);
  connect(w1, SIGNAL(clicked()), this, SLOT(cancel()));

  QPushButton* btns[]={w,w2,w1};
  for (int i=0; i<3; i++) {
    buttons->addStretch(10);
    buttons->addWidget(btns[i]);
  }
  buttons->addStretch(10);
  //  topLayout->addLayout (buttons, 10);
  setWindowTitle(tr("Filter expression"));
  resize(500, 200);
}

void
filter_expr_text_editor::help()
{
}

void
filter_expr_text_editor::save_close()
{
  done(1);
}

void
filter_expr_text_editor::cancel()
{
  done(0);
}

void
filter_expr_text_editor::set_text(const QString& s)
{
  m_edit->setPlainText(s);
}

const QString
filter_expr_text_editor::get_text() const
{
  return m_edit->toPlainText().trimmed();
}
