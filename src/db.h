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

#ifndef INC_DB_H
#define INC_DB_H


#ifdef WITH_PGSQL
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#endif

#include <QString>
#include <QVariant>

#include <list>

#include "dbtypes.h"
#include "date.h"
#include "database.h"


int ConnectDb(const char*, QString*);
void DisconnectDb();

#ifdef WITH_PGSQL
PGconn* GETDB();
void DBEXCPT(PGconn*);
#endif


#endif // INC_DB_H
