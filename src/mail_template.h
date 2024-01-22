/* Copyright (C) 2004-2024 Daniel Verite

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


#ifndef INC_MAIL_TEMPLATE_H
#define INC_MAIL_TEMPLATE_H

#include <QList>
#include <QStringList>
#include <QDialog>

class QListWidget;

class mail_template
{
public:
  mail_template() : m_template_id(0), m_fetched(false) {}
  int m_template_id;
  QString m_title;
  bool load(int id);
  bool update();
  bool insert();
  bool fetch_contents();
  QString header();
  QString text_body();
  QString html_body();
  void set_header(const QString s) { m_header=s; }
  void set_text_body(const QString s) { m_text_body=s; }
  void set_html_body(const QString s) { m_html_body=s; }
private:
  QString m_header;
  QString m_text_body;
  QString m_html_body;
  bool m_fetched; // header and body fetched
};

class mail_template_list : public QList<mail_template>
{
public:
  // fetch the list of (id,title) of all templates (but not their contents)
  bool fetch_titles();
};

class template_dialog: public QDialog
{
  Q_OBJECT
public:
  template_dialog();
  int selected_template_id();
private:
  QListWidget* m_wlist;

};


#endif
