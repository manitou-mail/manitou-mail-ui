/* Copyright (C) 2004-2011 Daniel Verite

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

#ifndef INC_ABOUT_H
#define INC_ABOUT_H

#include <QDialog>
#include <QTextBrowser>

class QTabWidget;

class about_box : public QDialog
{
  Q_OBJECT
public:
  about_box(QWidget* parent=0);
  virtual ~about_box();
public slots:
  void link_clicked(const QUrl&);
private:
  QTabWidget* m_tabw;
  QWidget* m_license_tab;
  QWidget* m_support_tab;
  QWidget* m_version_tab;

  void init_support_tab();
  void init_license_tab();
  void init_version_tab();
};

class about_panel : public QTextBrowser
{
  Q_OBJECT
public:
  about_panel(QWidget* parent) : QTextBrowser(parent) {}
  ~about_panel() {}
  QVariant loadResource(int type,const QUrl & name);
public slots:
  void setSource(const QUrl& url);
};

#endif // INC_ABOUT_H
