/* Copyright (C) 2004-2012 Daniel Verite

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

#ifndef INC_DATE_H
#define INC_DATE_H

#include <QString>
#include <QVariant>
#include <QStandardItem>

class date
{
public:
  date(): m_null(true) {}
  date(const QString date);
  virtual ~date() {}
  QString Output() const { return m_sDate; }
  QString OutputHM(int) const;
  QString output_24() const;
  QString output_8() const;
  QString FullOutput() const { return m_sYYYYMMDDHHMMSS; }
  bool is_null() const {
    return m_null;
  }
  bool operator<(const date&) const;
private:
  QString m_sDate;		// DD/MM/YYYY
  QString m_sYYYYMMDDHHMMSS;
  int m_day, m_month, m_year;
  int m_hour;
  int m_min;
  int m_sec;
  bool m_null;
};

class date_item : public QStandardItem
{
public:
  date_item(date d, int format);
  static const int date_role = Qt::UserRole+3;
  bool operator< (const QStandardItem& other) const;
};

Q_DECLARE_METATYPE(date)

#endif // INC_DATE_H
