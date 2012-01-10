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

#ifndef INC_MESSAGE_PORT_H
#define INC_MESSAGE_PORT_H

#include <QObject>
#include "dbtypes.h"

class message_port : public QObject
{
  Q_OBJECT
public:
  message_port();
  virtual ~message_port();
  static void connect_receiver(const char* signal, QObject* receiver, const char *member);
  static void connect_sender(QObject* sender, const char* signal, const char *member);
  static void init();
  static message_port* instance();
private:
  static message_port* m_this;
public slots:
  void tags_updated();
  void broadcast_new_mail(mail_id_t);
  void broadcast_list_refresh_request();
signals:
  void tags_restructured();
  void new_mail_imported(mail_id_t);
  void list_refresh_request();
};

#endif
