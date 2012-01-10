/* Copyright (C) 2004-2010 Daniel Verite

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

#include "main.h"
#include "notepad.h"
#include "db.h"
#include "icons.h"

#include <QPushButton>
#include <QPlainTextEdit>
#include <QLayout>
#include <QTimer>
#include <QCloseEvent>
#include <QToolBar>
#include <QIcon>
#include <QAction>

notepad*
notepad::m_current_instance; // singleton

notepad*
notepad::open_unique()
{
  if (!m_current_instance)
    m_current_instance = new notepad();
  return m_current_instance;
}

notepad::notepad() : QWidget(NULL)
{
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  m_we = new QPlainTextEdit(this);

  QHBoxLayout* hbl = new QHBoxLayout;
  top_layout->addLayout(hbl);

//  m_save_button = new QPushButton(tr("Save"));

  QToolBar* toolbar = new QToolBar();
  toolbar->setIconSize(QSize(16,16));
  toolbar->setFloatable(true);
  m_action_save = toolbar->addAction(tr("Save"), this, SLOT(save()));
  
  top_layout->addWidget(toolbar);
  top_layout->addWidget(m_we);
//  hbl->addWidget(m_save_button);
//  hbl->setStretchFactor(m_save_button, 0);
//  hbl->addStretch(1);

  QIcon ico_cut(UI_ICON(FT_ICON16_EDIT_CUT));
  QIcon ico_copy(UI_ICON(FT_ICON16_EDIT_COPY));
  QIcon ico_paste(UI_ICON(FT_ICON16_EDIT_PASTE));
  QIcon ico_undo(UI_ICON(FT_ICON16_UNDO));
  QIcon ico_redo(UI_ICON(FT_ICON16_REDO));
  toolbar->addSeparator();

  toolbar->addAction(ico_cut, tr("Cut"), m_we, SLOT(cut()));
  toolbar->addAction(ico_copy, tr("Copy"), m_we, SLOT(copy()));
  toolbar->addAction(ico_paste, tr("Paste"), m_we, SLOT(paste()));
  toolbar->addAction(ico_undo, tr("Undo"), m_we, SLOT(undo()));
  toolbar->addAction(ico_redo, tr("Redo"), m_we, SLOT(redo()));

  //  connect(m_save_button, SIGNAL(clicked()), this, SLOT(save()));
  load();
  disable_save();

  m_timer = new QTimer(this);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(auto_save()));
  m_timer->start(60*1000); // auto-save every minute

  connect(m_we, SIGNAL(textChanged()), this, SLOT(enable_save()));

  setWindowTitle(tr("Global Notepad"));
  setWindowIcon(UI_ICON(ICON16_NOTEPAD));
  resize(640, 400);
}

notepad::~notepad()
{
  save();
}

void
notepad::enable_save()
{
  //  m_save_button->setEnabled(true);
  m_action_save->setEnabled(true);
}

void
notepad::disable_save()
{
  //  m_save_button->setEnabled(false);
  m_action_save->setEnabled(false);
}

void
notepad::closeEvent(QCloseEvent* event)
{
  save();
  event->accept();
}

bool
notepad::load()
{
  db_cnx db;
  try {
    sql_stream s ("SELECT contents FROM global_notepad", db);
    if (!s.eof()) {
      QString contents;
      s >> contents;
      set_contents(contents);
    }
  }
  catch (db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

void
notepad::set_contents(const QString& txt)
{
  m_we->setPlainText(txt);
  m_we->document()->setModified(false);
}

bool
notepad::auto_save()
{
  if (m_we->document()->isModified())
    DBG_PRINTF(1, "notepad auto_save");

  return save();
}

bool
notepad::save()
{
  if (!m_we->document()->isModified())
    return true;

  const QString& t=m_we->toPlainText();
  db_cnx db;
  try {
    sql_stream s1("SELECT 1 FROM global_notepad", db);
    if (s1.eof()) {
      sql_stream s2("INSERT INTO global_notepad(contents,last_modified) VALUES(:p1, now())", db);
      s2 << t;
    }
    else {
      sql_stream s2("UPDATE global_notepad SET contents=:p1,last_modified=now()", db);
      s2 << t;
    }
  }
  catch (db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  disable_save();
  m_we->document()->setModified(false);
  return true;
}
