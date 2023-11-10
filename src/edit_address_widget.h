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

#ifndef INC_EDIT_ADDRESS_WIDGET_H
#define INC_EDIT_ADDRESS_WIDGET_H

#include "line_edit_autocomplete.h"

class edit_address_widget: public line_edit_autocomplete
{
public:
  edit_address_widget(QWidget* parent=NULL);
  virtual QList<QString> get_completions(const QString substring);
  virtual QList<QString> get_all_completions() {
    return QList<QString>();
  }
  virtual int get_prefix_pos(const QString text, int cursor_pos);
};

#endif // INC_EDIT_ADDRESS_WIDGET_H
