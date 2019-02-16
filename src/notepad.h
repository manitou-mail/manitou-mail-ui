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

#ifndef INC_NOTEPAD_H
#define INC_NOTEPAD_H

#include <QWidget>
#include <QString>

class QPushButton;
class QPlainTextEdit;
class QTimer;
class QCloseEvent;
class QToolBar;
class QAction;

class notepad : public QWidget
{
  Q_OBJECT
public:
  notepad();
  virtual ~notepad();
  static notepad* open_unique();
  void set_contents(const QString&);
protected:
  virtual void closeEvent(QCloseEvent*);
private:
  QPlainTextEdit* m_we;
  static notepad* m_current_instance;
  QTimer* m_timer;
  QPushButton* m_save_button;
  QAction* m_action_save;
private slots:
  void enable_save();
  void disable_save();
  bool auto_save();
  bool save();
  bool load();

};


#endif
