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

#include "bitvector.h"
#include <stdlib.h>

bit_vector::bit_vector() : m_buf(NULL), m_size(0)
{
}

bit_vector::~bit_vector()
{
  clear();
}

uchar*
bit_vector::buf() const
{
  return m_buf;
}

uint
bit_vector::size() const
{
  return m_size;
}

void
bit_vector::get_values(std::list<int>& l) const
{
  for (uint o=0; o<m_size; o++) {
    uchar mask=0x01;
    uchar c=m_buf[o];
    for (uint i=0; i<8; i++) {
      if (c&mask) {
	l.push_back(o*8+i+1);
      }
      mask = mask << 1;
    }
  }
}

void bit_vector::set_buf(const uchar* buf, uint size, uint nz_offset/*=0*/)
{
  if (m_buf)
    m_buf = (uchar*)realloc(m_buf, size+nz_offset);
  else
    m_buf = (uchar*)malloc(size+nz_offset);
  if (size>0 && !m_buf) {
    throw "No memory";
  }
  m_size=size+nz_offset;
  if (nz_offset>0)
    memset((void*)m_buf, 0, (size_t)nz_offset);
  if (size>0)
    memcpy((void*)(m_buf+nz_offset), (const void*)buf, (size_t)size);
}

void
bit_vector::and_op(const bit_vector& v)
{
  uint sz = m_size;
  const uchar* vbuf = v.buf();
  if (v.size() < sz)
    sz = v.size();
  for (uint o=0; o<sz; o++) {
    m_buf[o] &= vbuf[o];
  }
  // shorten our size if v is smaller than us
  if (sz < m_size)
    m_size=sz;
}

void
bit_vector::clear()
{
  m_size=0;
  if (m_buf) {
    free(m_buf);
      m_buf=NULL;
  }
}
