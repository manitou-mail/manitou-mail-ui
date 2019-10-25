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

#ifndef INC_THREADS_DIALOG_H
#define INC_THREADS_DIALOG_H

#include <QTreeWidgetItem>
#include <QWidget>

#include "message.h"

class QAbstractButton;
class QPushButton;
class QTreeWidget;

class threads_dialog : public QWidget
{
  Q_OBJECT
public:
  threads_dialog(QWidget* parent=NULL);
  ~threads_dialog();

private slots:
  void ok();
  void help();
  void btn_clicked(QAbstractButton*);
  void selection_changed();

private:
  bool fetch(std::list<mail_thread>& list);

  std::list<mail_thread> m_threads_list;
  QTreeWidget* m_listw = NULL;
  QPushButton* m_del_btn = NULL;
};

class thread_lvitem : public QTreeWidgetItem
{
public:
  thread_lvitem(QTreeWidget* parent=NULL);
  virtual ~thread_lvitem();
  bool operator<(const QTreeWidgetItem& other) const;  // sort
};
#endif // INC_THREADS_DIALOG_H
