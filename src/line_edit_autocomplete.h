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

#ifndef INC_EDIT_LINE_EDIT_AUTOCOMPLETE_H
#define INC_EDIT_LINE_EDIT_AUTOCOMPLETE_H

#include <QLineEdit>

class QListWidget;
class QListWidgetItem;
class QTimer;

/* Light version of QCompleter with a QLineEdit.
   We don't use QCompleter because its prefix-based completion
   algorithm can't be overriden by our specific rules. */
class line_edit_autocomplete: public QLineEdit
{
  Q_OBJECT
public:
  line_edit_autocomplete(QWidget* parent=NULL);
  virtual ~line_edit_autocomplete();
  void show_popup();

  void enable_completer(bool);

  /* time to open autocompletion popup, in milliseconds */
  void set_popup_delay(int delay) { m_delay = delay; }

  /* return the completions, given the substring. Subclasses must reimplement. */
  virtual QList<QString> get_completions(const QString substring)=0;
  /* return all completions, or an empty list if impractical. */
  virtual QList<QString> get_all_completions()=0;

  /* return the zero-based offset of the completion prefix inside 'text' when
     the cursor is at 'cursor_pos', 'cursor_pos' being 0 when 'text' is empty.
     return -1 if no completion prefix is found. */
  virtual int get_prefix_pos(const QString text, int cursor_pos);

  /* provides an event filter to dispatch events coming from both popup
     and lineedit, in order to synchronize their behavior */
  class dispatcher : public QObject
  {
  public:
    dispatcher(QObject* parent);
    bool eventFilter(QObject *, QEvent *);
    line_edit_autocomplete* lineedit;
    QListWidget* popup;
  private:
    bool eatFocusOut;
  };


public slots:
  virtual void completion_chosen(QListWidgetItem*);
  void check_completions(const QString&);
  void show_completions();
  void show_all_completions();
  //  void activate();
private:
  void redisplay_popup(const QList<QString>& completions);
  int m_delay;
  QListWidget* popup;
  dispatcher* m_merger;
  bool m_completer_enabled;
  QTimer* m_timer;
};

#endif
