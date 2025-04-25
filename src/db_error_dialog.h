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
#include <QDialog>

class QButtonGroup;

class db_error_dialog: public QDialog
{
  Q_OBJECT
public:
  db_error_dialog(const QString);
private slots:
  void handle_error();
  void copy_error();
private:
  enum actions {
    action_continue=1,
    action_reconnect,
    action_quit
  };
  QString m_error_text;
  QButtonGroup* m_btn_group;
};
