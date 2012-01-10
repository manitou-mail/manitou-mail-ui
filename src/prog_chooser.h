/* Copyright (C) 2005-2007 Daniel Vérité

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

#ifndef INC_PROG_CHOOSER_H
#define INC_PROG_CHOOSER_H

#include <QDialog>

class prog_chooser: public QDialog
{
public:
  prog_chooser(QWidget *parent);
  virtual ~prog_chooser();
  void init(const QString& mime_type);
  static QString find_in_path(const QString& program);
};

#endif // INC_PROG_CHOOSER_H

