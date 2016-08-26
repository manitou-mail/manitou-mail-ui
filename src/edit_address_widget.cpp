/* Copyright (C) 2004-2016 Daniel Verite

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

#include "edit_address_widget.h"
#include "addresses.h"

/* Completer for email addresses */

edit_address_widget::edit_address_widget(QWidget* parent) : line_edit_autocomplete(parent)
{
}

QList<QString>
edit_address_widget::get_completions(const QString prefix)
{
  mail_address_list completions;
  QList<QString> res;
  if (completions.fetch_completions(prefix)) {
    mail_address_list::const_iterator iter;
    for (iter = completions.begin(); iter != completions.end(); iter++) {
      res.append(QString("%1 <%2>").arg(iter->name()).arg(iter->email()));
    }
  }
  return res;
}
