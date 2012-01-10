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

#ifndef INC_UI_CONTROLS_H
#define INC_UI_CONTROLS_H

#include <QFrame>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QMap>

class button_group : public QFrame
{
  Q_OBJECT
public:
  button_group(QBoxLayout::Direction dir, QWidget* parent=0) : QFrame(parent) {
    g = new QButtonGroup;
    l = new QBoxLayout(dir);
    this->setLayout(l);
    this->setFrameStyle(QFrame::Box+QFrame::Raised);
  }
  void addButton(QAbstractButton* b, int id, const QString code=QString()) {
    g->addButton(b, id);
    l->addWidget(b);
    code_map.insert(id, code);
  }
  void setButton(int id) {
    QAbstractButton* b = g->button(id);
    if (b)
      b->setChecked(true);
  }
  QAbstractButton* selected() const {
    return g->checkedButton();
  }
  int selected_id() const {
    return g->checkedId();
  }
  const QString selected_code() const {
    int id = g->checkedId();
    if (id >= 0) {
      return code_map.value(id);
    }
    else
      return QString();
  }
  int code_to_id(const QString code) {
    QMapIterator<int, QString> iter(code_map);
    while (iter.hasNext()) {
      iter.next();
      if (iter.value()==code)
	return iter.key();
    }
    return -1;
  }
  virtual ~button_group() {}
  QButtonGroup* group() {
    return g;
  }
private:
  QButtonGroup* g;
  QBoxLayout* l;
  QMap<int, QString> code_map;
};

#endif // INC_UI_CONTROLS_H
