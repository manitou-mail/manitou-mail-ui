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

#include "searchbox.h"
#include "main.h"
#include "icons.h"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QButtonGroup>
#include <QToolButton>
#include <QStyle>

/* search_edit comes from
   http://labs.trolltech.com/blogs/2007/06/06/lineedit-with-a-clear-button/
*/
search_edit::search_edit(QWidget* parent) : QLineEdit(parent)
{
  m_clear_button = new QToolButton(this);
  QPixmap pixmap = FT_MAKE_ICON(ICON16_CLEAR_TEXT);
  m_clear_button->setIcon(QIcon(pixmap));
  m_clear_button->setIconSize(pixmap.size());
  m_clear_button->setCursor(Qt::ArrowCursor);
  m_clear_button->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  m_clear_button->setToolTip(tr("Clear the text"));
  m_clear_button->setEnabled(false);
  connect(m_clear_button, SIGNAL(clicked()), this, SLOT(clear()));
  connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(m_clear_button->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(qMax(msz.width(), m_clear_button->sizeHint().width() + frameWidth * 2 + 2),
		 qMax(msz.height(), m_clear_button->sizeHint().height() + frameWidth * 2 + 2));
  
}

void
search_edit::resizeEvent(QResizeEvent *)
{
  QSize sz = m_clear_button->sizeHint();
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  m_clear_button->move(rect().right() - frameWidth - sz.width(),
		    (rect().bottom() + 2 - sz.height())/2);
}

void
search_edit::updateCloseButton(const QString& text)
{
  m_clear_button->setEnabled(!text.isEmpty());
}


search_box::search_box(QWidget* parent) : QWidget(parent)
{
  QVBoxLayout* topLayout = new QVBoxLayout(this);

  QHBoxLayout* hle = new QHBoxLayout();
  topLayout->addLayout(hle);
  setWindowTitle(tr("Search within the current list"));

  hle->addWidget(new QLabel("Find text:", this));
  m_wEdit = new QLineEdit(this);
  connect(m_wEdit, SIGNAL(returnPressed()), SLOT(find()));
  hle->addWidget(m_wEdit);

  QHBoxLayout* main_hb = new QHBoxLayout();
  topLayout->addLayout(main_hb);
  QVBoxLayout* hb=new QVBoxLayout();
  main_hb->addLayout(hb);
  m_gbutt1=new QButtonGroup(this);
  hb->addWidget(new QCheckBox(tr("Case insensitive"), this));
  hb->addWidget(new QCheckBox(tr("Wrap around"), this));
  hb->addWidget(new QCheckBox(tr("Auto-close"), this));

  m_gbutt2=new QButtonGroup(this);
  QVBoxLayout* hb2=new QVBoxLayout();
  main_hb->addLayout(hb2);
  static const char* where_str[] = {
    QT_TR_NOOP("In bodies"),
    QT_TR_NOOP("In subjects"),
    /*    QT_TR_NOOP("In authors"),*/ // TODO
    QT_TR_NOOP("In headers")
  };
  for (uint i=0; i<sizeof(where_str)/sizeof(where_str[0]); i++) {
    QCheckBox* cb = new QCheckBox(tr(where_str[i]), this);
    hb2->addWidget(cb);
  }

  QHBoxLayout* hbox=new QHBoxLayout();
  topLayout->addLayout(hbox);
  hbox->setMargin(10);
  hbox->setSpacing(20);
  QPushButton* wFind=new QPushButton(tr("Find"), this);
  hbox->addWidget(wFind);
  QPushButton* wClose=new QPushButton(tr("Close"), this);
  hbox->addWidget(wClose);

  connect(m_gbutt1, SIGNAL(buttonClicked(int)), SLOT(sel_options(int)));
  connect(m_gbutt2, SIGNAL(buttonClicked(int)), SLOT(sel_where(int)));
  connect(wFind, SIGNAL(clicked()), SLOT(find()));
  connect(wClose, SIGNAL(clicked()), SLOT(close()));
  m_options=0;

  set_where(FT::searchInBodies+FT::searchInSubjects+ /*FT::searchInAuthors+*/
	    FT::searchInHeaders);
}

void
search_box::closeEvent(QCloseEvent* e)
{
  emit search_closed();
  e->accept();
}

void
search_box::find()
{
  emit mail_find(m_wEdit->text(), m_searchWhere, m_options);
}

void
search_box::sel_options(int id)
{
  if (m_gbutt1->button(id)->isChecked())
    m_options |= 1<<id;
  else
    m_options &= ~(1<<id);
}

void
search_box::sel_where(int id)
{
  if (m_gbutt2->button(id)->isChecked())
    m_searchWhere |= 1<<id;
  else
    m_searchWhere &= ~(1<<id);
}

void
search_box::set_options(int mask)
{
  m_options=mask;
  int id=0;
  for (int i=1; i<=FT::optionsMax; i=i<<1) {
    QCheckBox* b=(QCheckBox*)m_gbutt1->button(id++);
    if (b)
      b->setChecked((mask&i)==i);
  }
}

void
search_box::set_where(int mask)
{
  m_searchWhere=mask;
  int id=0;
  for (int i=1; i<=FT::searchMax; i=i<<1) {
    QCheckBox* b=(QCheckBox*)m_gbutt2->button(id++);
    if (b) {
      b->setChecked((mask&i)==i);
    }
  }
}
