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

#ifndef INC_DB_TYPES_H
#define INC_DB_TYPES_H


#define DB_MAIL_ID_32
//#define DB_MAIL_ID_64

#include <QtGlobal>

#if defined(DB_MAIL_ID_32)
typedef quint32 mail_id_t;
typedef quint32 attach_id_t;
#define MAIL_ID_FMT_STRING "%u"
#elif defined(DB_MAIL_ID_64)
typedef quint64 mail_id_t;
typedef quint64 attach_id_t;
#define MAIL_ID_FMT_STRING "%llu"
#endif

#endif // INC_DB_TYPES_H
