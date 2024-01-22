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

#include "line_edit_autocomplete.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QListWidget>
#include <QScrollBar>
#include <QTimer>

/* Generic completer. Inherit and provide an implementation for get_completions() */
line_edit_autocomplete::line_edit_autocomplete(QWidget* parent) : QLineEdit(parent),
  m_delay(500)
{
  m_timer=NULL;
  popup = new QListWidget();
  popup->setWindowFlags(Qt::Popup);
  popup->setFocusPolicy(Qt::NoFocus);
  popup->setFocusProxy(this);
  popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_merger = new dispatcher(this);
  m_merger->popup=popup;
  m_merger->lineedit=this;

  this->installEventFilter(m_merger);
  popup->installEventFilter(m_merger);

  connect(popup, SIGNAL(itemClicked(QListWidgetItem*)),
	  this, SLOT(completion_chosen(QListWidgetItem*)));

  connect(popup, SIGNAL(clicked(QModelIndex)), popup, SLOT(hide()));

  m_completer_enabled = true;

  connect(this, SIGNAL(textEdited(const QString&)),
	  this, SLOT(check_completions(const QString&)));
}

line_edit_autocomplete::~line_edit_autocomplete()
{
}

void
line_edit_autocomplete::enable_completer(bool enable)
{
  m_completer_enabled = enable;
  if (!enable && popup && popup->isVisible()) {
    popup->clear();
    popup->hide();
  }
}

void
line_edit_autocomplete::show_popup()
{
  QWidget* widget=this;
  QRect rect;

  const QRect screen = QApplication::desktop()->availableGeometry(widget);
  Qt::LayoutDirection dir = widget->layoutDirection();
  QPoint pos;
  int rw, rh, w;
  int h = (popup->sizeHintForRow(0) * qMin(7, popup->model()->rowCount()) + 3) + 3;
  QScrollBar *hsb = popup->horizontalScrollBar();
  if (hsb && hsb->isVisible())
    h += popup->horizontalScrollBar()->sizeHint().height();

  if (rect.isValid()) {
    rh = rect.height();
    w = rw = rect.width();
    pos = widget->mapToGlobal(dir == Qt::RightToLeft ? rect.bottomRight() : rect.bottomLeft());
  }
  else {
    rh = widget->height();
    rw = widget->width();
    pos = widget->mapToGlobal(QPoint(0, widget->height() - 2));
    w = widget->width();
  }

  if ((pos.x() + rw) > (screen.x() + screen.width()))
    pos.setX(screen.x() + screen.width() - w);
  if (pos.x() < screen.x())
    pos.setX(screen.x());
  if (((pos.y() + rh) > (screen.y() + screen.height())) && ((pos.y() - h - rh) >= 0))
    pos.setY(pos.y() - qMax(h, popup->minimumHeight()) - rh + 2);

  popup->setGeometry(pos.x(), pos.y(), w, h);

  if (!popup->isVisible()) {
    popup->show();
  }
}

void
line_edit_autocomplete::show_all_completions()
{
  QList<QString> completions = get_all_completions();
  redisplay_popup(completions);
}

void
line_edit_autocomplete::redisplay_popup(const QList<QString>& completions)
{
  if (!completions.empty()) {
    popup->clear();
    QList<QString>::const_iterator iter;
    for (iter = completions.begin(); iter != completions.end(); iter++) {
      popup->addItem(*iter);
    }
    show_popup();
  }
  else {
    popup->clear();
    popup->hide();
  }
}

void
line_edit_autocomplete::show_completions()
{
  if (!m_completer_enabled)
    return;

  QString prefix; // substring on which the completion is to be based

  int pos = cursorPosition();
  int start = get_prefix_pos(text(), pos);
  if (start>=0) {
    prefix=text().mid(start, pos-start);
  }

  if (prefix.isEmpty()) {
    // nothing to complete
    if (popup->isVisible()) {
      popup->clear();
      popup->hide();
    }
  }
  else {
    QList<QString> completions = get_completions(prefix);
    redisplay_popup(completions);
  }
}


void
line_edit_autocomplete::check_completions(const QString& newtext)
{
  // Filter out control characters. A common character that can appear
  // at this point is linefeed, and we don't want these in mail addresses
  bool modified = false;
  QString copy = newtext;
  int j=0;
  for (int i=0; i<newtext.length(); i++) {
    if (!newtext.at(i).isPrint()) {
      copy.remove(j, 1);
      modified = true;
    }
    else
      j++;
  }
  if (modified) {
    setText(copy);		// will not recurse
  }

  // Cancel the timer if it's already running.
  if (m_timer)
    delete m_timer;
  m_timer = new QTimer(this);
  m_timer->setSingleShot(true);
  m_timer->setInterval(m_delay);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(show_completions()));
  m_timer->start();
}

line_edit_autocomplete::dispatcher::dispatcher(QObject* parent) : QObject(parent)
{
  eatFocusOut=true;
}

void
line_edit_autocomplete::completion_chosen(QListWidgetItem* item)
{
  if (!item) return;
  QString text= this->text();
  int pos = get_prefix_pos(text, this->cursorPosition());
  QString before = text.mid(0, pos);
  QString after = text.mid(this->cursorPosition());
  this->setText(before + item->text() + after);
}

/*
 Return the zero-based offset of the completion prefix inside 'text' when
 the cursor is at 'cursor_pos', 'cursor_pos' being 0 when 'text' is empty.
 return -1 if no completion prefix is found.
*/
int
line_edit_autocomplete::get_prefix_pos(const QString text, int cursor_pos)
{
  if (cursor_pos==0)
    return -1;
  int pos = cursor_pos-1;
  while (pos>=0 && text.at(pos).isLetterOrNumber()) {
    pos--;
  }
  /* pos will be -1 if we went up to the start of the string */
  if (pos<0 || text.at(pos)==' ' || text.at(pos)==',')
    return pos+1;
  else
    return -1;
}

bool line_edit_autocomplete::dispatcher::eventFilter(QObject* o, QEvent* e)
{
  if (eatFocusOut && o==lineedit && e->type()==QEvent::FocusOut && popup->isVisible()) {
    return true;
  }
  if (o != popup)
    return QObject::eventFilter(o, e);

  switch (e->type()) {
  case QEvent::KeyPress:
    {
      int row=popup->currentRow();
      QKeyEvent *ke = static_cast<QKeyEvent *>(e);
      const int key = ke->key();
      switch (key) {
      case Qt::Key_End:
      case Qt::Key_Home:
	if (ke->modifiers() & Qt::ControlModifier)
	  return false;
	break;

      case Qt::Key_Up:
	if (row>0) {
	  popup->setCurrentRow(row-1);
	  return true;
	}
	else if (popup->count()>0) {
	  popup->setCurrentRow(popup->count()-1);
	  return true;
	}
	else
	  return false;

      case Qt::Key_Down:
	if (row==popup->count()-1) {
	  popup->setCurrentRow(0);
	  return true;
	}
	else if (row < popup->count()-1) {
	  popup->setCurrentRow(row+1);
	  return true;
	}
	else
	  return false;

      case Qt::Key_PageUp:
      case Qt::Key_PageDown:
	return false;
      }

      eatFocusOut = false;
      (static_cast<QObject *>(lineedit))->event(ke);
      eatFocusOut = true;
      if (e->isAccepted() || !popup->isVisible()) {
	// widget lost focus, hide the popup
	if (!lineedit->hasFocus())
	  popup->hide();
	if (e->isAccepted())
	  return true;
      }

        // default implementation for keys not handled by the widget when popup is open
        switch (key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            popup->hide();
            if (popup->currentItem()) {
	      lineedit->completion_chosen(popup->currentItem());
	    }
            break;

        case Qt::Key_F4:
            if (ke->modifiers() & Qt::AltModifier)
                popup->hide();
            break;

        case Qt::Key_Backtab:
        case Qt::Key_Escape:
            popup->hide();
            break;

        default:
            break;
        }
	return true;

    } // QEvent::KeyPress
    break;

  case QEvent::MouseButtonPress:
    if (!popup->underMouse()) {
      popup->hide();
      return true;
    }

  case QEvent::InputMethod:
  case QEvent::ShortcutOverride:
    QApplication::sendEvent(lineedit, e);
    break;

  default:
    return false;
  }
  return false;
}
