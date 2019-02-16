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

#include "message_port.h"
#include "main.h"

//static
message_port* message_port::m_this;

message_port::message_port()
{
}

message_port::~message_port()
{
}

//static
message_port*
message_port::instance()
{
  return m_this;
}

// static
void
message_port::init()
{
  if (!m_this) {
    m_this=new message_port();
  }
}

//static
void
message_port::connect_receiver(const char* signal, QObject* receiver, const char* member)
{
  bool res;
  if (m_this) {
    res=m_this->connect(m_this, signal, receiver, member);
  }
}

//static
void
message_port::connect_sender(QObject* sender, const char* signal, const char* member)
{
  bool res;
  if (m_this) {
    res=m_this->connect(sender, signal, m_this, member);
  }
}

void
message_port::tags_updated()
{
  emit tags_restructured();
}

void
message_port::broadcast_new_mail(mail_id_t id)
{
  emit new_mail_imported(id);
}

void
message_port::broadcast_list_refresh_request()
{
  emit list_refresh_request();
}

void
message_port::broadcast_mail_deleted(mail_id_t id)
{
  emit mail_deleted(id);
}
