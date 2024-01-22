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

#include "attachment_listview.h"
#include "app_config.h"
#include "main.h"
#include "image_viewer.h"

#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QPlainTextEdit>
#include <QUrl>
#include <QDialog>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>

// *************** //
//  attch_lvitem
// *************** //

void
attch_lvitem::fill_columns()
{
  QString fsize;
  if (m_type==type_attachment) {
    setText(0, m_attach.mime_type());
    setText(1, m_attach.display_filename());
    if (m_attach.getId() == 0)	// file on local filesystem
      if (!m_attach.filename().isEmpty())
	setToolTip(1, m_attach.filename());
    fsize = m_attach.human_readable_size();
  }
  else if (m_type==type_note) {
    setText(0, "(private note)");
    setText(1, m_note);
    fsize.sprintf("%d", m_note.length());
    QBrush brush(QColor("#f1db20"));
    setBackground(0, brush);
    setBackground(1, brush);
    setBackground(2, brush);
  }
  else
    return; // shouldn't happen

  setText(2, fsize);
  setTextAlignment(0, Qt::AlignTop);
  setTextAlignment(1, Qt::AlignTop);
  setTextAlignment(2, Qt::AlignTop);
}


void
attch_lvitem::set_note(const QString& note)
{
  m_note = note;
  m_type = type_note;
}

attachment*
attch_lvitem::get_attachment()
{
  if (m_type == type_attachment) {
    m_attach.set_final_filename(this->text(1));
    return &m_attach;
  }
  else
    return NULL;
}

/* Download the attachment into 'destfilename'
   stop when finished or when *abort becomes true, which may
   happen in the same thread by way of progress_report()
   calling processEvents().
   May emit an app_exception.
 */
bool
attch_lvitem::download(const QString destfilename, bool *abort)
{
  DBG_PRINTF(5, "saving attachment into %s", destfilename.toLocal8Bit().constData());
  struct attachment::lo_ctxt lo;
  if (m_attach.open_lo(&lo)) {
    QByteArray qba = destfilename.toLocal8Bit();
    QFile file(destfilename);
    if (file.open(QIODevice::WriteOnly)) {
      lview()->progress_report(-(int)((lo.size+lo.chunk_size-1)/lo.chunk_size));
      int steps=0;
      int r;
      do {
	r = m_attach.streamout_chunk(&lo, &file);
	lview()->progress_report(++steps);
      } while (r && (!abort || *abort==false));
      file.close();
    }
    else {
      // can't open file
      m_attach.close_lo(&lo);
      throw app_exception(file.errorString());
    }
    m_attach.close_lo(&lo);

    if (*abort) {
      QFile::remove(destfilename);
    }
    return true;
  }
  return false;
}

// Save to a file and return the directory
// TODO: let the caller deal with the directory (return nothing?)
//  and probably take an opened QIODevice
// May emit an app_exception.
QString
attch_lvitem::save_to_disk(const QString fname, bool* abort)
{
  download(fname, abort);
  return QFileInfo(fname).canonicalPath();
}

/* Open an in-place editor to change the filename of an attachment when
   composing. Other columns are made non-editable by this delegate. */
QWidget*
attach_item_editor_delegate::createEditor(QWidget* parent,
					  const QStyleOptionViewItem &option,
					  const QModelIndex &index) const
{
  // index 0 is for the MIME content type, index 1 for the filename.
  if (index.column() == 1) {
    attached_filename_editor* ed = new attached_filename_editor(parent);
    /* The editor seems to get destroyed when it looses focus, so
       that returning from a slot might crash if it's called directly
       and it has shifted the focus elsewhere (for instance displaying
       a clickable box). We use a QueuedConnection to avoid that. */
    connect(ed, SIGNAL(invalid_filename(const QString)),
	    this, SLOT(show_warning(const QString)), Qt::QueuedConnection);
    return ed;
  }
  else
    return nullptr;
}

void
attach_item_editor_delegate::show_warning(const QString errmsg)
{
  QMessageBox::warning(NULL, tr("Warning"), errmsg);
}

void
attch_lvitem::set_editable(bool b)
{
  setFlags(b? (flags()|Qt::ItemIsEditable) : (flags()&~Qt::ItemIsEditable));
}

attch_listview::attch_listview(QWidget* parent):
  QTreeWidget(parent)
{
  QString fontname=get_config().get_string("display/font/msglist");
  if (!fontname.isEmpty() && fontname!="xft") {
    QFont f;
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

QString attch_listview::m_last_attch_dir;

void
attch_listview::popup_ctxt_menu(const QPoint& pos)
{
  attch_lvitem* item = dynamic_cast<attch_lvitem*>(itemAt(pos));
  if (!item)
    return;
  attachment* pa = item->get_attachment();
  if (!pa)
    return; // it may be a note

  QString app_name = pa->default_os_application();
  QMenu menu(this);
  QAction* action_open_app = NULL;
  if (!app_name.isEmpty())
    action_open_app = menu.addAction(tr("Open with: %1").arg(app_name));

  QAction* action_save = menu.addAction(tr("Save to disk"));
  QAction* action_view_image = menu.addAction(tr("View image"));
  QAction* action_view_text = menu.addAction(tr("Display as text"));

  if (pa->mime_type().startsWith("image/")) {
    action_view_image->setEnabled(true);
    action_view_text->setEnabled(false);
  }
  else {
    action_view_image->setEnabled(false);
    action_view_text->setEnabled(true);
  }

  QAction* action = menu.exec(mapToGlobal(pos));
  if (action==NULL) {
    return;
  }
  else if (action == action_view_text) {
    const char* contents = pa->get_contents();
    if (contents) {
      QPlainTextEdit* v = new QPlainTextEdit(NULL);
      v->resize(640,480);
      v->setPlainText(contents);
      v->setWindowTitle(tr("Attachment as text"));
      v->setReadOnly(true);
      v->show();
    }
  }
  else if (action == action_view_image) {
    image_viewer* imgview = new image_viewer();
    imgview->show_attachment(pa);
  }
  else if (action == action_open_app) {
    QString tmpname = pa->get_temp_location();
    emit init_progress(tr("Downloading attached file: %1").arg(tmpname));
    m_abort=false;
    item->download(tmpname, &m_abort);
    emit finish_progress();
    if (!m_abort)
      pa->launch_os_viewer(tmpname);
  }
  else if (action==action_save) {
    save_attachments();
  }
}

void
attch_listview::download_aborted()
{
  m_abort=true;
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
  attch_lvitem* pItem = dynamic_cast<attch_lvitem*>(currentItem());
  if (!pItem) return;
  delete pItem;
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

bool
attch_listview::confirm_write(const QString fname)
{
  if (QFile::exists(fname)) {
    int res = QMessageBox::question(this, QObject::tr("Please confirm"), QObject::tr("A file '%1' already exists. Overwrite?").arg(fname), QObject::tr("&Yes"), QObject::tr("&No"), QString(), 0, 1);
    if (res==1)
      return false;
  }
  return true;
}

void
attch_listview::save_attachments()
{
  // list of attachments with a filename
  std::list<attch_lvitem*> list_name;
  // list of attachments without a filename
  std::list<attch_lvitem*> list_noname;

  QList<QTreeWidgetItem*> list = this->selectedItems();
  QList<QTreeWidgetItem*>::const_iterator itw = list.begin();
  for (; itw!=list.end(); ++itw) {
    attch_lvitem* item = static_cast<attch_lvitem*>(*itw);
    if (item->is_note() || !item->get_attachment())
      continue;
    if (!item->get_attachment()->filename().isEmpty())
      list_name.push_back(item);
    else
      list_noname.push_back(item);
  }
  if (list_name.empty() && list_noname.empty()) {
    QMessageBox::warning(this, APP_NAME, tr("Please select one or several attachments"));
    return;
  }

  QString fname;
  QString dir=attch_listview::m_last_attch_dir;
  if (dir.isEmpty()) {
    dir = get_config().get_string("attachments_directory");
  }
  std::list<attch_lvitem*>::iterator it;
  m_abort = false;
  if (list_name.size()>1) {
    // several named attachments: ask only for the directory
    dir = QFileDialog::getExistingDirectory(this, tr("Save to directory..."), dir);
    if (!dir.isEmpty()) { // if accepted
      for (it=list_name.begin(); it!=list_name.end() && !m_abort; ++it) {
	attachment* pa = (*it)->get_attachment();
	fname = dir + "/" + pa->filename();
	if (!confirm_write(fname))
	  continue;
	emit init_progress(tr("Downloading attachment into: %1").arg(fname));
	try {
	  dir = (*it)->save_to_disk(fname, &m_abort);
	}
	catch(app_exception excpt) {
	  QMessageBox::critical(this, APP_NAME, excpt.m_err_msg);
	  m_abort = true;
	}
	emit finish_progress();
      }
    }
    else
      return;			// cancel
  }

  if (list_name.size()==1) {
    it=list_name.begin();
    attachment* pa = (*it)->get_attachment();
    fname = pa->filename();
    if (!dir.isEmpty()) {
      fname = dir + "/" + fname;
    }
    fname = QFileDialog::getSaveFileName(this, tr("File"), fname);
    DBG_PRINTF(5, "fname=%s", fname.toLocal8Bit().constData());
    if (!fname.isEmpty()) {
      emit init_progress(tr("Downloading attached file: %1").arg(fname));
      try {
	dir=(*it)->save_to_disk(fname, &m_abort);
      }
      catch(app_exception excpt) {
	QMessageBox::critical(this, APP_NAME, excpt.m_err_msg);
	m_abort = true;
      }
      emit finish_progress();
    }
  }

  if (!list_noname.empty()) {
    m_abort = false;
    for (it=list_noname.begin(); it!=list_noname.end() && !m_abort; ++it) {
      fname = QFileDialog::getSaveFileName(this, tr("File"), dir);
      if (!fname.isEmpty()) {
	emit init_progress(tr("Downloading attachment into: %1").arg(fname));
	try {
	  dir=(*it)->save_to_disk(fname, &m_abort);
	}
	catch(app_exception excpt) {
	  QMessageBox::critical(this, APP_NAME, excpt.m_err_msg);
	  m_abort = true;
	}
	emit finish_progress();
      }
      else
	break;
    }
  }
  if (!dir.isEmpty())
    attch_listview::m_last_attch_dir = dir;
}

attch_dialog_save::attch_dialog_save()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(5);
  setWindowTitle(tr("Open or save"));
  m_label_file = new QLabel;
  m_label_file->setTextFormat(Qt::PlainText);
  layout->addWidget(m_label_file);
  m_rb_launch = new QRadioButton(tr("Open with: %1").arg(""));
  QRadioButton* b2 = new QRadioButton(tr("Save to disk"));
  b2->setChecked(true);
  layout->addWidget(m_rb_launch);
  layout->addWidget(b2);
  QDialogButtonBox* box = new QDialogButtonBox;
  layout->addWidget(box);
  box->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
  box->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
  connect(box, SIGNAL(accepted()), this, SLOT(accept()));
  connect(box, SIGNAL(rejected()), this, SLOT(reject()));
}

void
attch_dialog_save::set_app_name(const QString name)
{
  m_rb_launch->setText(tr("Open with: %1").arg(name));
}

void
attch_dialog_save::set_file_name(const QString name)
{
  m_label_file->setText(tr("File: %1").arg(name));
}

int
attch_dialog_save::choice()
{
  return (m_rb_launch->isChecked()) ? 2 : 1;
}

attached_filename_editor::attached_filename_editor(QWidget* parent) : QLineEdit(parent)
{
  connect(this, SIGNAL(returnPressed()), this, SLOT(validate()));
}

/* Check for unsafe characters in a filename for an attachment.
   Fix them if necessary, warning the user. */
void
attached_filename_editor::validate()
{
  QString errstr;
  if (!attachment::check_filename(this->text(), errstr)) {
    emit invalid_filename(tr("Characters that are unsafe in file names have been replaced by underscores."));
    QString s = this->text().trimmed();
    attachment::fixup_filename(s);
    setText(s);
  }
}
