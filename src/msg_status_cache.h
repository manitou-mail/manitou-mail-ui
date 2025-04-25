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

#ifndef INC_MSG_STATUS_CACHE_H
#define INC_MSG_STATUS_CACHE_H

#include "dbtypes.h"
#include "db_listener.h"
#include "message.h"
#include <map>

// mail_id => status
typedef std::map<mail_id_t,int> msg_status_map;

class msg_status_cache: public QObject
{
  Q_OBJECT
public:
  static void init_db();
  static uint unread_count();
  static uint unprocessed_count();
  static bool has_unread_messages();
  static void reset();
  static const int c_mask_unread=mail_msg::statusRead | mail_msg::statusTrashed | mail_msg::statusArchived;
  static const int c_mask_unprocessed = mail_msg::statusTrashed | mail_msg::statusArchived | mail_msg::statusSent;
  static inline void update(mail_id_t id, int status) {
    if (status==-1 /*|| (status&(mail_msg::statusTrashed | mail_msg::statusArchived))!=0*/)
      global_status_map.erase(id);
    else {
      global_status_map[id] = status;
      if (id > m_max_mail_id)
	m_max_mail_id = id;
    }
  }
  // high water mark of mail_id we encountered
  static mail_id_t m_max_mail_id;
private:
  static msg_status_cache* m_this;
  static msg_status_map global_status_map;
public slots:
  void db_new_mail_notif();
signals:
  void new_mail_notified(mail_id_t);
};

#endif
