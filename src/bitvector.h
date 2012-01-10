/* Copyright (C) 2004,2005,2006 Daniel Vérité

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

#ifndef INC_BITVECTOR_H
#define INC_BITVECTOR_H

#include <qstring.h>
#include <list>

class bit_vector
{
public:
  bit_vector();
  virtual ~bit_vector();

  // get the list of bit numbers that are set
  void get_values(std::list<int>&) const;

  // set the entire vector from a buffer
  void set_buf(const uchar*, uint size, uint nz_offset=0);

  // direct access to contents
  uchar* buf() const;

  // size of the vector
  uint size() const;

  // intersection with another vector
  void and_op(const bit_vector& v);

  // set the vector to empty
  void clear();

private:
  uchar* m_buf;
  uint m_size;
};

#endif // INC_BITVECTOR_H
