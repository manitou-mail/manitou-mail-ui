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

#include "attachment_listview.h"
#include "app_config.h"
#include "main.h"
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QPlainTextEdit>
#include <QUrl>

// *************** //
//  attch_lvitem
// *************** //

void
attch_lvitem::fill_columns()
{
  QString sSize;
  if (m_pAttachment && m_type==0) {
    setText(0, m_pAttachment->mime_type());
    setText(1, m_pAttachment->filename());
    sSize.sprintf("%d", m_pAttachment->size());
  }
  else if (m_type==1) {
    setText(0, "(private note)");
    setText(1, m_note);
    sSize.sprintf("%d", m_note.length());
    QBrush brush(QColor("#f1db20"));
    setBackground(0, brush);
    setBackground(1, brush);
    setBackground(2, brush);
  }
  setText(2, sSize);
  setTextAlignment(0, Qt::AlignTop);
  setTextAlignment(1, Qt::AlignTop);
  setTextAlignment(2, Qt::AlignTop);
}


void
attch_lvitem::set_note(const QString& note)
{
  m_note=note;
  m_type=1;
}

/* download the attachment into 'destfilename'
   stop when finished or when *abort becomes true, which may
   happen in the same thread by way of progress_report()
   calling processEvents() */
bool
attch_lvitem::download(const QString destfilename, bool *abort)
{
  attachment* pa = m_pAttachment;
  struct attachment::lo_ctxt lo;
  if (pa->open_lo(&lo)) {
    QByteArray qba = destfilename.toLocal8Bit();
    std::ofstream of(qba.constData(), std::ios::out|std::ios::binary);
    lview()->progress_report(-(int)((lo.size+lo.chunk_size-1)/lo.chunk_size));
    int steps=0;
    int r;
    do {
      r = pa->streamout_chunk(&lo, of);
      lview()->progress_report(++steps);
    } while (r && (!abort || *abort==false));
    pa->close_lo(&lo);
    of.close();
    if (*abort) {
      QFile::remove(destfilename);
    }
    return true;
  }
  return false;
}

QString
attch_lvitem::save_to_disk(const QString fname, bool* abort)
{
  download(fname, abort);
  return QFileInfo(fname).canonicalPath();
}

attch_listview::attch_listview(QWidget* parent):
  QTreeWidget(parent), m_pAttchList(NULL)
{
  QString fontname=get_config().get_string("display/font/msglist");
  if (!fontname.isEmpty() && fontname!="xft") {
    QFont f;
    if (fontname.at(0)=='-')
      f.setRawName(fontname);
    else
      f.fromString(fontname);
    setFont(f);
  }

  setColumnCount(3);
  QStringList labels;
  labels << tr("Type") << tr("Filename/Note") << tr("Size");
  setHeaderLabels(labels);
  static const int header_lengths[] = { 25, 50, 6 };
  QTreeWidgetItem* header = headerItem();
  QFontMetrics fm(header->font(0));
  QHeaderView* hv = this->header();
  for (uint col=0; col<sizeof(header_lengths)/sizeof(header_lengths[0]); col++) {
    hv->resizeSection(col, header_lengths[col]*fm.averageCharWidth());
  }
  setRootIsDecorated(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(popup_ctxt_menu(const QPoint&)));

}

attch_listview::~attch_listview()
{
}

void
attch_listview::popup_ctxt_menu(const QPoint& pos)
{
  attch_lvitem* item = dynamic_cast<attch_lvitem*>(itemAt(pos));
  if (!item)
    return;
  attachment* pa = item->get_attachment();
  if (!pa)
    return; // it may be a note

  QMenu menu(this);
  QAction* action_view_text = menu.addAction(tr("Display as text"));

  QAction* action = menu.exec(mapToGlobal(pos));
  if (action==NULL) {
    return;
  }
  else if (action==action_view_text) {
    const char* contents = pa->get_contents();
    if (contents) {
      QPlainTextEdit* v = new QPlainTextEdit(NULL);
      v->resize(640,480);
      v->setPlainText(contents);
      v->setWindowTitle(tr("Attachment as text"));
      v->show();
    }
  }
}

void
attch_listview::allow_delete(bool b)
{
  if (b) {
    QAction* m_delete_action = new QAction(this);
    m_delete_action->setShortcut(Qt::Key_Delete);
    this->addAction(m_delete_action);
    connect(m_delete_action, SIGNAL(triggered()), this, SLOT(remove_current_attachment()));
  }
}

void
attch_listview::remove_current_attachment()
{
  DBG_PRINTF(3, "remove attachment\n");
  attch_lvitem* pItem=dynamic_cast<attch_lvitem*>(currentItem());
  if (!pItem) return;
  QModelIndex midx = indexFromItem(pItem);
  int pos=midx.row();
  std::list<attachment>::iterator it=m_pAttchList->begin();
  for (int i=0; it!=m_pAttchList->end(); ++it,++i) {
    if (i==pos) {
      m_pAttchList->erase(it);
      delete pItem;
      break;
    }
  }
}

void
attch_listview::progress_report(int count)
{
  emit progress(count);
}


bool
attch_listview::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
  Q_UNUSED(parent);
  Q_UNUSED(index);
  Q_UNUSED(action);
  foreach (QUrl url, data->urls())  {
    emit attach_file_request(url);
  }
  return false;
}

Qt::DropActions
attch_listview::supportedDropActions() const
{
  return Qt::CopyAction | Qt::MoveAction;
}

QStringList
attch_listview::mimeTypes() const
{
  return QStringList("text/uri-list");
}

