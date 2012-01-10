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

#include "log_window.h"
#include <QPlainTextEdit>
#include <QBoxLayout>

log_window* log_window::m_window;

log_window::log_window()
{
  setWindowTitle(tr("Log window"));
  QVBoxLayout* layout = new QVBoxLayout(this);
  m_edit = new QPlainTextEdit;
  layout->addWidget(m_edit);
  resize(600,400);
}

//static
void
log_window::log(const QString msg)
{
  if (!m_window) {
    m_window = new log_window();
    m_window->show();
  }
  m_window->m_edit->appendPlainText(msg);
    
}
