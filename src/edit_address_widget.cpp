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

#include "addresses.h"
#include "edit_address_widget.h"

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

/*
 Return the zero-based offset of the completion prefix inside 'text' when
 the cursor is at 'cursor_pos', 'cursor_pos' being 0 when 'text' is empty.
 return -1 if no completion prefix is found.
 Compared to the base class implementation, dash/dot/at-sign are considered
 part of the prefix.
*/
int
edit_address_widget::get_prefix_pos(const QString text, int cursor_pos)
{
  if (cursor_pos==0)
    return -1;
  int pos = cursor_pos-1;
  while (pos>=0 && (text.at(pos).isLetterOrNumber()
		    || text.at(pos) == '-' || text.at(pos) == '.'
		    || text.at(pos) == '@'))
  {
    pos--;
  }
  /* pos will be -1 if we went up to the start of the string */
  if (pos<0 || text.at(pos)==' ' || text.at(pos)==',')
    return pos+1;
  else
    return -1;
}
