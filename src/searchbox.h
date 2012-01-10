/* Copyright (C) 2004-2009 Daniel Vérité

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

#ifndef INC_SEARCHBOX_H
#define INC_SEARCHBOX_H

#include <QWidget>
#include <QLineEdit>

class QCloseEvent;
class QButtonGroup;
class QToolButton;

class searched_text
{
public:
  searched_text() : m_is_word(false),m_is_cs(false) {}
  virtual ~searched_text() {}
  QString m_text;
  bool m_is_word;		// word only
  bool m_is_cs;			// case sensitive
};

class search_edit: public QLineEdit
{
  Q_OBJECT
public:
  search_edit(QWidget* parent=NULL);
protected:
  void resizeEvent(QResizeEvent *);
private slots:
  void updateCloseButton(const QString &text);
private:
  QToolButton *m_clear_button;
};

class search_box : public QWidget
{
  Q_OBJECT
public:
  search_box(QWidget* parent);
  virtual ~search_box() {}
  void set_options(int mask);	// from FT::searchOptionsEnum
  void set_where(int mask);	// from FT::searchInEnum
  int options() const { return m_options; }
  int where() const { return m_searchWhere; }
private:
  QLineEdit* m_wEdit;
  int m_searchWhere;
  int m_options;
  QButtonGroup* m_gbutt1;
  QButtonGroup* m_gbutt2;
protected slots:
  void find();
  void sel_options(int);
  void sel_where(int);
signals:
  void mail_find(const QString& t, int where, int options);
  void search_closed();
protected:
  virtual void closeEvent (QCloseEvent* e);
};

#endif // INC_SEARCHBOX_H
