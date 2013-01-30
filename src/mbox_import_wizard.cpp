/* Copyright (C) 2004-2013 Daniel Verite

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
#include "mbox_import_wizard.h"
#include "mbox_file.h"
#include "identities.h"
#include "tags.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QProgressBar>
#include <QTimer>

void
mbox_import_wizard_page::cleanupPage()
{
  /* called on pressing the Back button. The base function resets the
     fields to the values they had before initializePage() We don't
     want that so we don't call it
  */
}

//
// mbox_import_wizard_page_mbox_file
//

mbox_import_wizard_page_mbox_file::mbox_import_wizard_page_mbox_file()
{
  setTitle(tr("Mailbox file"));
  setSubTitle(tr("Locate the mbox file"));

  QVBoxLayout* layout = new QVBoxLayout(this);
  QHBoxLayout* ll = new QHBoxLayout;
  ll->addWidget(new QLabel(tr("Filename:")));
  m_wfilename = new QLineEdit;
  ll->addWidget(m_wfilename);
  registerField("mbox_filename*", m_wfilename);
  QPushButton* btn = new QPushButton(tr("Browse"));
  ll->addWidget(btn);
  layout->addLayout(ll);
  connect(btn, SIGNAL(clicked()), this, SLOT(browse()));

  QString txt = "<br>";
  txt.append(tr("The file format must be <b>mbox</b>, in either the mboxo or mboxrd variant.<br>The mboxcl and mboxcl2 variants are not supported."));
  layout->addWidget(new QLabel(txt));
}

bool
mbox_import_wizard_page_mbox_file::validatePage()
{
  return mbox_file::check_format(m_wfilename->text());
}

void
mbox_import_wizard_page_mbox_file::browse()
{
  QString s = QFileDialog::getOpenFileName(this);
  if (!s.isEmpty())
    m_wfilename->setText(s);
}


//
// mbox_import_wizard_page_options
//

mbox_import_wizard_page_options::mbox_import_wizard_page_options()
{
  setTitle(tr("Import options"));
  setSubTitle(tr("Import options"));

  QFormLayout* fl = new QFormLayout;
  setLayout(fl);

  m_cbox_status = new QComboBox;
  m_cbox_status->addItem(tr("New (unread)"), QVariant(mail_msg::statusNew));
  m_cbox_status->addItem(tr("Read"), QVariant(mail_msg::statusRead));
  m_cbox_status->addItem(tr("Archived"), QVariant(mail_msg::statusArchived+mail_msg::statusRead));
  m_cbox_status->addItem(tr("Sent"), QVariant(mail_msg::statusArchived+mail_msg::statusSent+mail_msg::statusRead));
  m_cbox_status->setCurrentIndex(2); // archived by default

  fl->addRow(tr("Set status:"), m_cbox_status);

  m_apply_filters = new QCheckBox();
  m_apply_filters->setChecked(false);
  fl->addRow(tr("Apply filters:"), m_apply_filters);

  m_cbox_tag = new tag_selector(this);
  m_cbox_tag->init(true);
  fl->addRow(tr("Assign tag:"), m_cbox_tag);

  m_auto_purge = new QCheckBox(tr("Purge import data on completion"));
  m_auto_purge->setChecked(false);
  fl->addRow(tr("Automatic purge:"), m_auto_purge);

  registerField("status", m_cbox_status);
  registerField("filters", m_apply_filters);
  registerField("auto_purge", m_auto_purge);
  registerField("tag", m_cbox_tag);

}

void
mbox_import_wizard_page_options::initializePage()
{
  QString path = field("mbox_filename").toString();
  
}

mbox_import_wizard_page_upload::mbox_import_wizard_page_upload()
{
  setTitle(tr("Contents upload"));
  setSubTitle(tr("Please wait while the contents are uploaded..."));

  QVBoxLayout* layout = new QVBoxLayout;
  setLayout(layout);
  m_progress_bar = new QProgressBar();
  m_progress_bar->setRange(0,100);
  QHBoxLayout* pbar1_layout = new QHBoxLayout;
  layout->addLayout(pbar1_layout);
  pbar1_layout->addWidget(new QLabel(tr("Progress:")));
  pbar1_layout->addWidget(m_progress_bar, 15);
  m_btn_abort_upload = new QPushButton(tr("Abort"));
  m_btn_abort_upload->setEnabled(false);
  pbar1_layout->addWidget(m_btn_abort_upload);
  //downsize_button(m_btn_abort_upload);

  m_label_result = new QLabel;
  m_label_waiting = new QLabel;
  layout->addWidget(m_label_result);
  layout->addSpacing(10);
  layout->addWidget(m_label_waiting);
#if 0
  m_waiting_timer = new QTimer(this);
  connect(m_waiting_timer, SIGNAL(timeout()), this, SLOT(check_import()));
#endif
}

void
mbox_import_wizard_page_upload::initializePage()
{
  m_upload_finished=false;
  m_import_id=0;
  QTimer::singleShot(0, this, SLOT(do_upload()));
}

void
mbox_import_wizard_page_upload::do_upload()
{
  m_upload_finished=false;
#if 0
  m_import_started=false;
#endif
  mbox_import_options* opt = get_options();
  connect(&m_box, SIGNAL(progress_report(double)), this, SLOT(update_progress_bar(double)));
  m_progress_bar->setFormat(tr("Upload %p%"));
  connect(m_btn_abort_upload, SIGNAL(clicked()), &m_box, SLOT(abort_import()));
  m_btn_abort_upload->setEnabled(true);
  m_import_id = m_box.import(*opt);
  // after the import is finished
  disconnect(&m_box, SIGNAL(progress_report(double)), this, SLOT(update_progress_bar(double)));
  disconnect(m_btn_abort_upload, SIGNAL(clicked()), &m_box, SLOT(abort_import()));

  m_upload_finished=true;
  emit completeChanged();
  if (m_import_id) {
    m_label_result->setText(tr("The mailbox has been uploaded to the server.\n%1 message(s) were extracted.").arg(m_box.uploaded_count()));
#if 0
    m_label_waiting->setText(tr("Waiting for the import to start..."));
    m_waiting_secs=0;
    m_waiting_timer->start(1000); // check every second
#else
    m_label_waiting->setText(tr("Click on Finish to signal the server to start importing these messages."));
#endif
  }
  else {
    m_label_result->setText(tr("The upload was not successful."));
  }
}

bool
mbox_import_wizard_page_upload::validatePage()
{
  if (m_import_id) {
    m_box.launch_import_job(m_import_id);
    return true;
  }
  else
    return false;
}

#if 0
void
mbox_import_wizard_page_upload::check_import()
{
  if (!m_upload_finished || !m_import_id)
    return;
  // check that at least one message was imported
  db_cnx db;
  try {
    sql_stream s("SELECT 1 FROM import_mbox WHERE import_id=:id AND completion>0", db);
    s << m_import_id;
    if (!s.eos()) {
      m_label_waiting->setText(tr("Import started."));
    }
    else {
      m_label_waiting->setText(tr("Waiting for the import to start since %1 s.").arg(m_waiting_secs++));
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}
#endif

void
mbox_import_wizard_page_upload::update_progress_bar(double progress)
{
  m_progress_bar->setValue(progress);
}

bool
mbox_import_wizard_page_upload::isComplete() const
{
  return m_upload_finished;
}


mbox_import_wizard::mbox_import_wizard()
{
  setWindowTitle(tr("Import a mailbox"));
  identities ids;
  ids.fetch();

  setPage(page_mbox_file, new mbox_import_wizard_page_mbox_file);
  setPage(page_options, new mbox_import_wizard_page_options);
  setPage(page_upload, new mbox_import_wizard_page_upload);
}

#if 0
// remove if finally unnecessary
bool
mbox_import_wizard::validateCurrentPage()
{
  return true;
}
#endif

mbox_import_options*
mbox_import_wizard_page::get_options()
{
  mbox_import_wizard* z = static_cast<mbox_import_wizard*>(wizard());
  return z ? z->get_options() : NULL;
}

mbox_import_options*
mbox_import_wizard::get_options()
{
  m_options.filename = field("mbox_filename").toString();
  mbox_import_wizard_page_options* p = static_cast<mbox_import_wizard_page_options*>(page(page_options));
  m_options.tag_id = p->m_cbox_tag->current_tag_id();
  int idx = p->m_cbox_status->currentIndex();
  m_options.mail_status = (idx>=0) ? p->m_cbox_status->itemData(idx).toInt() : 0;
  m_options.apply_filters = field("filters").toBool();
  m_options.auto_purge = field("auto_purge").toBool();
  return &m_options;
}
