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

#ifndef INC_FILTER_EXPR_EDITOR_H
#define INC_FILTER_EXPR_EDITOR_H

#include <QDialog>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QGridLayout;
class QPlainTextEdit;

class filter_expr_editor: public QDialog
{
  Q_OBJECT
public:
  filter_expr_editor(QWidget* parent);
private:
  void add_row(QGridLayout*, int);
  QComboBox* m_combo_header;
  QComboBox* m_op;
  //  QCheckBox* m_negate;
  QComboBox* m_logop;
  QLineEdit* m_operand2;
};

class filter_expr_text_editor: public QDialog
{
  Q_OBJECT
public:
  filter_expr_text_editor(QWidget* parent);
  virtual ~filter_expr_text_editor() {}
  void set_text(const QString& s);
  const QString get_text() const;
private slots:
  void save_close();
  void cancel();
  void help();
private:
  QPlainTextEdit* m_edit;
};

#endif


