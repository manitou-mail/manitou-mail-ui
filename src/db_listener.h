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

#ifndef INC_DB_LISTENER_H
#define INC_DB_LISTENER_H

class db_listener;

#include "database.h"
#include <QString>
#include <QObject>


/*
A listener class for database events sent by NOTIFY.
An instance of this class is tightly coupled with the db_cnx object it is created with.
Ownership is transferred to the db_cnx object.
If a disconnect occurs, db_cnx should be responsible to disconnect the db_listener's slot
If a reconnexion succeeds, db_cnx should reemit the proper LISTEN statements to the database and reconnect the slots without the need to do anything for the db_listener instances.
*/

class db_listener: public QObject
{
  Q_OBJECT
public:
  db_listener(db_cnx& db, const QString notif_name);
  virtual ~db_listener();
  const QString notification_name() const {
    return m_notif_name;
  }
public slots:
  void process_notification();
signals:
  void notified();
private:
  QString m_notif_name;
  database* m_db;
};


#endif

