/* Copyright (C) 2004-2023 Daniel Verite

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

#ifndef INC_BODY_EDIT_H
#define INC_BODY_EDIT_H

#include <QWidget>
#include <QString>
#include "dbtypes.h"

class QTextEdit;

class body_edit : public QWidget
{
  Q_OBJECT
public:
  body_edit(QWidget* parent=0);
  virtual ~body_edit();
  void set_contents(mail_id_t mail_id, const QString&);
signals:
  void text_updated(mail_id_t mail_id, const QString*);
private:
  QTextEdit* m_we;
  mail_id_t m_mail_id;
private slots:
  void ok();
};


#endif
