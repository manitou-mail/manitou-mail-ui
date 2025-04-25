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



/* WORK IN PROGRESS, UNUSABLE AS IS */

#if 0
#include "dbtypes.h"
#include <q3dragobject.h>

class mail_drag : public Q3StoredDrag
{
public:
  mail_drag(QWidget* dragsource=0, const char* name=0): Q3StoredDrag("message/rfc822", dragsource,name) {
  }
  virtual ~mail_drag() {}
  void setMailId(uint mail_id) {
    m_mail_id=mail_id;
  }
  void setEncodedData(const QByteArray& array _UNUSED_) {
  }
  QByteArray encodedData(const char*) const {
    QByteArray* data=new QByteArray(10);
    data->fill('a',10);
    return *data;
  }
  static bool canDecode(const QMimeSource *e) {
    return e->format() && strcmp(e->format(), "message/rfc822")==0;
  }
private:
  uint m_mail_id;
};
#endif
