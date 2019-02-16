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

#include <QString>
#include <QWidget>
#include <QLabel>

#include "addresses.h"
#include <set>

class QLineEdit;
class QTextEdit;
class QCheckBox;
class QLabel;
class QSpinBox;


class addrbook_entry : public QWidget
{
  Q_OBJECT
public:
  addrbook_entry(QWidget* parent, const QString email);
  virtual ~addrbook_entry();
public slots:
  void ok();
  void cancel();
private:
  QLineEdit* m_qlname;
  QLineEdit* m_qlemail;
  QLineEdit* m_qlalias;
  QTextEdit* m_notes;
  QSpinBox* m_prio;

  mail_address m_addr;
  void widgets_to_addr(mail_address&);
};

#if 0
#ifndef Q_MOC_RUN
class address_book : public QWidget
{
  Q_OBJECT
public:
  address_book (QWidget* parent=NULL);
  virtual ~address_book ();

private slots:
void search();
  void address_selected();
  void aliases();
  void new_address();
  void apply();
  void OK();
  void cancel();

private:
  void save();
  bool m_is_modified;

  QLineEdit* m_email_search_w;
  QLineEdit* m_name_search_w;
  QLineEdit* m_nick_search_w;

  Q3ListView* m_result_list_w;

  QLineEdit* m_email_w;
  QLineEdit* m_name_w;
  QLineEdit* m_nick_w;
  Q3MultiLineEdit* m_notes_w;
  QCheckBox* m_invalid_w;
  QLabel* m_last_sent_w;
  QLabel* m_last_recv_w;
  QLabel* m_nb_sent_w;
  QLabel* m_nb_recv_w;

  mail_address_list m_result_list;
};

class address_book_aliases : public QWidget
{
  Q_OBJECT
public:
  address_book_aliases (const QString email, QWidget* parent=NULL);
  virtual ~address_book_aliases ();
private slots:
void add_aliases();
  void remove_alias();
  void search();
  void OK();
private:
  void init_aliases_list();

  QLineEdit* m_email_search_w;
  QLineEdit* m_name_search_w;
  QLineEdit* m_nick_search_w;

  Q3ListView* m_result_list_w;
  mail_address_list m_result_list;
  std::set<QString> m_remove_list;    // list of emails to be un-aliased
  Q3ListView* m_aliases_list_w;

  QString m_email;
};
#endif
#endif
