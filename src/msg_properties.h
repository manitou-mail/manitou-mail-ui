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

#include <QDialog>
#include "message.h"

class QSpinBox;
class QDateTimeEdit;
class select_status_box;

class properties_dialog : public QDialog
{
  Q_OBJECT
public:
  properties_dialog (mail_msg* i, QWidget* parent);
  virtual ~properties_dialog() {}
private slots:
    void ok();
signals:
    void change_status_request(mail_id_t id,
			       uint status,
			       int priority);
    void tags_counters_changed(QList<tag_counter_transition>);
private:
  select_status_box* m_status_box;
  mail_id_t m_mail_id;
  int m_initial_priority;
  uint m_last_known_status;
  QSpinBox* m_spinbox;
  QDateTimeEdit* m_send_datetime;
};
