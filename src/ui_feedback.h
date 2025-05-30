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

#ifndef INC_UI_FEEDBACK_H
#define INC_UI_FEEDBACK_H

#include <QObject>

/* To be used by non-UI classes to reflect on the progress of long
   operations. Typically this object gets connected to a
   QProgressBar. */
class ui_feedback: public QObject
{
  Q_OBJECT
public:
  ui_feedback(QObject* parent=NULL);
  virtual ~ui_feedback();
  void reset();
  void set_maximum(int);
  void set_value(int);
  void set_status_text(const QString);
signals:
  void set_max(int);
  void set_val(int);
  void set_text(const QString&);
};
#endif
