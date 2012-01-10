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

#ifndef INC_NOTEWIDGET_H
#define INC_NOTEWIDGET_H

#include <QDialog>
#include <QString>

class QPlainTextEdit;

class note_widget: public QDialog
{
  Q_OBJECT
public:
  note_widget(QWidget* parent);
  virtual ~note_widget() {}
  void set_note_text(const QString& s);
  const QString get_note_text() const;
private slots:
  void save_close();
  void cancel();
  void delete_note();
private:
  QPlainTextEdit* m_edit;
};

#endif // INC_NOTEWIDGET_H
