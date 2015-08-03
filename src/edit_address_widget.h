/* Copyright (C) 2004-2015 Daniel Verite

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

#ifndef INC_EDIT_ADDRESS_WIDGET_H
#define INC_EDIT_ADDRESS_WIDGET_H

#include <QLineEdit>

class QListWidget;
class QListWidgetItem;
class QTimer;

/* light version of QCompleter. We don't use QCompleter because its
   prefix-based completion algorithm can't be overriden by our
   specific rules. */
class email_completer : public QObject
{
  Q_OBJECT
public:
  email_completer(QObject* parent);
  virtual ~email_completer();
  bool eventFilter(QObject *, QEvent *);

  /* return the zero-based offset of the completion prefix inside 'text' when
     the cursor is at 'cursor_pos', 'cursor_pos' being 0 when 'text' is empty.
     return -1 if no completion prefix is found. */
  virtual int get_prefix_pos(const QString text, int cursor_pos);

  QLineEdit* lineedit;
  QListWidget* popup;
public slots:
  void completion_chosen(QListWidgetItem*);
private:
  bool eatFocusOut;
};

class edit_address_widget: public QLineEdit
{
  Q_OBJECT
public:
  edit_address_widget(QWidget* parent=NULL);
  virtual ~edit_address_widget();
  void show_popup();
  void enable_completer(bool);
public slots:
  void check_completions(const QString&);
  void show_completions();
  //  void activate();
private:
  QListWidget* popup;
  email_completer* completer;
  bool m_completer_enabled;
  QTimer* m_timer;
};

#endif // INC_EDIT_ADDRESS_WIDGET_H
