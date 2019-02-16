/* Copyright (C) 2004-2019 Daniel Verite

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

#include "ui_feedback.h"
#include <QApplication>

ui_feedback::ui_feedback(QObject* parent) : QObject(parent)
{
}


ui_feedback::~ui_feedback()
{

}

void
ui_feedback::reset()
{
  emit set_max(0);
  QApplication::processEvents();
}

void
ui_feedback::set_maximum(int m)
{
  emit set_max(m);
  QApplication::processEvents();
}

void
ui_feedback::set_value(int v)
{
  emit set_val(v);
  QApplication::processEvents();
}

void
ui_feedback::set_status_text(const QString t)
{
  emit set_text(t);
  QApplication::processEvents();
}
