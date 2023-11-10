/* Copyright (C) 2004-2023 Daniel Verite

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
#include "db.h"
#include "db_listener.h"
#include "sqlstream.h"

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QBoxLayout>
#include <QGridLayout>
#include <QDebug>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

#include "mbox_file.h"

void
import_mbox_file()
{
  mbox_import_window* w = new mbox_import_window();
  w->show();
}

mbox_file::mbox_file()
{
}

// returns true if the file can be opened for reading and it has From_
// starting its first line.
//static
bool
mbox_file::check_format(const QString filename)
{
  bool format_ok=false;
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly)) {
    QString errmsg = QIODevice::tr(f.errorString().toLatin1().constData());
    QMessageBox::critical(NULL, QObject::tr("Open error"), QObject::tr("Can't open file '%2': %1").arg(errmsg).arg(filename));
    return false;
  }

  const int maxlength=1024;
  char buf[maxlength];
  qint64 sz = f.readLine(buf, maxlength);
  if (sz!=-1) {
    if (sz<maxlength-1 && strncmp(buf, "From ", 5)==0) {
      format_ok=true;
    }
    else {
      QMessageBox::critical(NULL, QObject::tr("Unrecognized format"),
			    tr("No From line found at the start of the file. This file cannot be imported as an mbox."));
      
    }
  }
  f.close();
  return format_ok;
}

bool
mbox_file::launch_import_job(int import_id)
{
  db_cnx db;
  try {
    db.begin_transaction();
    sql_stream sj("INSERT INTO jobs_queue(job_type, job_args) VALUES('import_mailbox', :p1)", db);
    sj << QString("%1").arg(import_id);
    sql_stream sn("NOTIFY job_request", db);
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

int
mbox_file::import(const mbox_import_options& opt)
{
  int import_id=0;
  m_abort_requested = false;
  db_cnx db;

  try {
    QFileInfo inf(opt.filename);
    QString filename = inf.baseName();
    sql_stream s("INSERT INTO import_mbox(tag_id,mail_status,apply_filters,filename,auto_purge) VALUES(:p1,:p2,:p3,:p4,:p5) RETURNING import_id", db);
    if (opt.tag_id>0)
      s << opt.tag_id;
    else
      s << sql_null();
    s << opt.mail_status << (opt.apply_filters?"Y":"N") << filename;
    s << (opt.auto_purge?"Y":"N");

    if (!s.eos()) {
      s >> import_id;
    }
    m_filename = opt.filename;

    if (!database_import(import_id)) {
      db.begin_transaction();
      sql_stream sd1("DELETE FROM import_message WHERE import_id=:p1", db);
      sd1 << import_id;
      sql_stream sd("DELETE FROM import_mbox WHERE import_id=:p1", db);
      sd << import_id;
      import_id=0; // import aborted or failed=>don't return the ID
      db.commit_transaction();
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    import_id=0;
  }
  return import_id;
}

void
mbox_file::abort_import()
{
  DBG_PRINTF(3, "mbox upload abort requested");
  m_abort_requested = true;
}

bool
mbox_file::database_import(int import_id)
{
  db_cnx db;
  bool ret=true;
  m_uploaded_count=0;
  try {
    PGconn* pgconn = db.connection();
    int mail_number=0;
    const char* query = "INSERT INTO import_message(import_id,mail_number,encoded_mail,status) VALUES($1,$2,$3,0)";

    QFile f(m_filename);
    if (!f.open(QIODevice::ReadOnly)) {
      QString errmsg = QIODevice::tr(f.errorString().toLatin1().constData());
      QMessageBox::critical(NULL, QObject::tr("Open error"), QObject::tr("Can't open file '%2': %1").arg(errmsg).arg(m_filename));
      return false;
    }

    const Oid param_types[] = {23, 23, 17}; // int4,int4,bytea
    // TODO: have a non-reusable name for the prepared statement
    PGresult* res_prep = PQprepare(pgconn, "ins_message_import",
				   query, 3, param_types);
    if (!res_prep || PQresultStatus(res_prep)!=PGRES_COMMAND_OK) {
      QMessageBox::critical(NULL, QObject::tr("Database error"), QObject::tr("Unable to prepare query: %1 %2").arg(query).arg(PQerrorMessage(pgconn)));
      return false;
    }

    QByteArray msg_bytes;
    qint64 total_size = f.size();
    qint64 read_size=0;

    db.begin_transaction();

    while (!f.atEnd() && f.error()==QFile::NoError && !m_abort_requested) {
      QApplication::processEvents();
      /* According to rfc-2822, a line must not exceed 998 bytes+cr+lf in length.
	 We truncate lines larger than 1023 to avoid importing large bogus
	 contents into the database */
      const int maxlength=1024;
      char buf[maxlength];
      qint64 sz = f.readLine(buf, maxlength);
      if (sz!=-1) { // else we expect f.error() will get us out of the loop
	read_size += sz;
	if (sz<maxlength-1 && strncmp(buf, "From ", 5)==0) {
	  /* /^From / both begins the first message and mark the end of
	     the previous message */
	  if (mail_number>0) {
	    import_message(import_id, mail_number, msg_bytes, pgconn);
	    emit progress_report(read_size*100.0/total_size);
	    mail_number++;
	    DBG_PRINTF(5, "Imported message number %d, import_id=%d", mail_number, import_id);
	    msg_bytes.clear();
	  }
	  else
	    mail_number=1;
	}
	else {
	  // DBG_PRINTF(2, "append: %s", buf);
	  msg_bytes.append(buf, sz);
	}
      }
    }

    if (!m_abort_requested) {

      if (f.error()!=QFile::NoError) {
	DBG_PRINTF(1, "mbox import has finished with QFile error=%d", f.error());
	ret=false;
      }

      if (msg_bytes.size()>0 && f.atEnd() && f.error()==QFile::NoError) {
	/* if mail_number is zero then we didn't find any line starting
	   with From followed by space. That would mean that the file is
	   not in mbox format. We still import it to allow for a single
	   mail file. */
	if (mail_number==0)
	  mail_number=1;
	import_message(import_id, mail_number, msg_bytes, pgconn);
      }

      emit progress_report(100);
    }
    else
      ret=false;

    if (res_prep)
      PQclear(res_prep);

    sql_stream srel("DEALLOCATE ins_message_import", db);
    if (ret) {
      db.commit_transaction();
      m_uploaded_count = mail_number;
    }
    else {
      db.rollback_transaction();
    }

    if (f.error()!=QFile::NoError) {
      QMessageBox::critical(NULL, tr("File error"), f.errorString());
    }
    f.close();
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
  return ret;
}

void
mbox_file::import_message(int import_id, int mail_number, const QByteArray& bytes,
			  PGconn* pgconn)
{
  QByteArray p1=QString("%1").arg(import_id).toLatin1();
  QByteArray p2=QString("%1").arg(mail_number).toLatin1();
  const char* param_values[3] = {p1.constData(), p2.constData(), bytes.constData()};
  int param_lengths[3] = {0,0, bytes.length()};
  int param_formats[3]={0,0,1};

  PGresult* res = PQexecPrepared(pgconn, "ins_message_import", 3, param_values, param_lengths, param_formats, 0);
  
  if (!res || PQresultStatus(res)!=PGRES_COMMAND_OK) {
    QMessageBox::critical(NULL, QObject::tr("Database error"), QObject::tr("Unable to execute prepared statement: %1").arg(PQerrorMessage(pgconn)));
    return;
  }

  if (res)
    PQclear(res);
}

mbox_import_window::mbox_import_window(QWidget* parent): QWidget(parent)
{
  m_listener = NULL;
  m_import_id = 0;

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Import mbox file"));
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  QGridLayout* gl = new QGridLayout;
  top_layout->addLayout(gl);

  QFont smaller_font = font();
  smaller_font.setPointSize((smaller_font.pointSize()*8)/10);

  int row=0;
  gl->addWidget(new QLabel(tr("Mbox file:")), row, 0);
  m_filename = new QLineEdit();
  connect(m_filename, SIGNAL(textChanged(const QString&)), this, SLOT(reset_progress()));

  m_filename->setMinimumWidth(250);
  QHBoxLayout* hbl1 = new QHBoxLayout;
  hbl1->addWidget(m_filename);
  QPushButton* btn_browse = new QPushButton(tr("Browse"));
  hbl1->addWidget(btn_browse);
  gl->addLayout(hbl1, row, 1);
  connect(btn_browse, SIGNAL(clicked()), this, SLOT(browse_file()));

  row++;
  QHBoxLayout* hbl2 = new QHBoxLayout;
  m_cbox_status = new QComboBox;
  m_cbox_status->addItem(tr("New (unread)"), QVariant(mail_msg::statusNew));
  m_cbox_status->addItem(tr("Read"), QVariant(mail_msg::statusRead));
  m_cbox_status->addItem(tr("Archived"), QVariant(mail_msg::statusArchived+mail_msg::statusRead));
  m_cbox_status->setCurrentIndex(2); // archived by default

  gl->addWidget(new QLabel(tr("Set status:")), row, 0);
  hbl2->addWidget(m_cbox_status, 1);

  hbl2->addWidget(new QLabel(tr("Apply filters:")));
  m_apply_filters = new QCheckBox();
  m_apply_filters->setChecked(false);
  hbl2->addWidget(m_apply_filters);

  hbl2->addStretch(10);
  gl->addLayout(hbl2, row, 1);


  {
    row++;
    gl->addWidget(new QLabel(tr("Assign tag:")), row, 0);
    QHBoxLayout* hbl3 = new QHBoxLayout;
    m_cbox_tag = new tag_selector(this);
    m_cbox_tag->init(true);
    hbl3->addWidget(m_cbox_tag);
    hbl3->addStretch(10);
    gl->addLayout(hbl3, row, 1);
  }

  row++;
  m_progress_bar = new QProgressBar();
  m_progress_bar->setRange(0,100);
  gl->addWidget(new QLabel(tr("Progress:")), row, 0);
  QHBoxLayout* pbar1_layout = new QHBoxLayout;
  pbar1_layout->addWidget(m_progress_bar, 15);
  m_btn_abort_upload = new QPushButton(tr("Abort"));
  m_btn_abort_upload->setEnabled(false);
  pbar1_layout->addWidget(m_btn_abort_upload);
  downsize_button(m_btn_abort_upload);
    
  gl->addLayout(pbar1_layout, row, 1);

 
  row++;
  QHBoxLayout* hbl3 = new QHBoxLayout;
  hbl3->addStretch(10);
  m_btn_start = new QPushButton(tr("Start import"));
  connect(m_btn_start, SIGNAL(clicked()), this, SLOT(start()));
  m_btn_start->setEnabled(false);
  hbl3->addWidget(m_btn_start);
  hbl3->addStretch(10);
  gl->addLayout(hbl3, row, 0, 1, 2); // colspan=2

  top_layout->addStretch(1);
}

mbox_import_window::~mbox_import_window()
{
  if (m_listener) {
    db_cnx db;
    DBG_PRINTF(4, "delete listener=%p", m_listener);
    delete m_listener;
  }
}

void
mbox_import_window::closeEvent(QCloseEvent* e)
{
  Q_UNUSED(e);
  if (m_import_id && m_import_running) {
    QMessageBox::information(this, tr("Import"), tr("The import will continue until completion at the server"));
  }
}

void
mbox_import_window::reset_progress()
{
  m_btn_start->setEnabled(true);
  m_progress_bar->setValue(0);
  m_btn_abort_upload->setEnabled(false);
}

void
mbox_import_window::browse_file()
{
  QString path=QFileDialog::getOpenFileName(this, tr("Location of mbox file"));
  if (!path.isEmpty())
    m_filename->setText(path);
}

void
mbox_import_window::update_progress(double percent)
{
  m_progress_bar->setValue((int)percent);
  m_progress_bar->repaint();
}

void
mbox_import_window::update_progress_import()
{
  db_cnx db;
  try {
    sql_stream s("SELECT completion,status FROM import_mbox WHERE import_id=:p1", db);
    s << m_import_id;
    float percent;
    int status;
    if (!s.eof()) {
      s >> percent >> status;
      if (status==3) { // finished
	m_import_running=false;
	m_btn_abort_upload->setEnabled(false);
	m_btn_start->setEnabled(true);
      }
      m_progress_bar->setValue((int)(percent*100));
      m_progress_bar->repaint();
    }
  }
  catch(db_excpt& p) {
  }
}

void
mbox_import_window::start()
{
  m_import_running=false;
  mbox_import_options opt;
  opt.filename = m_filename->text();
  opt.tag_id = m_cbox_tag->current_tag_id();
  opt.apply_filters = m_apply_filters->isChecked();
  int idx = m_cbox_status->currentIndex();
  opt.mail_status = (idx>=0) ? m_cbox_status->itemData(idx).toInt() : 0;

  mbox_file imp;
  m_progress_bar->setValue(0);
  m_progress_bar->show();
  m_progress_bar->setFormat(tr("Upload %p%"));
  connect(&imp, SIGNAL(progress_report(double)), this, SLOT(update_progress(double)));
  connect(m_btn_abort_upload, SIGNAL(clicked()), &imp, SLOT(abort_import()));
  connect(m_btn_abort_upload, SIGNAL(clicked()), this, SLOT(upload_aborted()));
  m_btn_abort_upload->setEnabled(true);

  db_cnx db;
  if (!m_listener) {
    m_listener = new db_listener(db, "mbox_import_progress");
    connect(m_listener, SIGNAL(notified()), this, SLOT(update_progress_import()));
  }
  connect(m_btn_abort_upload, SIGNAL(clicked()), this, SLOT(upload_aborted()));
  m_btn_start->setEnabled(false);
  m_import_id = imp.import(opt);

  if (m_import_id) {
    // Upload has finished and is successful, now we just keep track of the
    // server-side import
    disconnect(m_btn_abort_upload, SIGNAL(clicked()));
    connect(m_btn_abort_upload, SIGNAL(clicked()), this, SLOT(abort_import()));
    m_progress_bar->setFormat(tr("Server-side import %p%"));
    m_progress_bar->setValue(0);
    m_import_running=true;
  }
}

void
mbox_import_window::upload_aborted()
{
  m_progress_bar->setValue(0);
  m_btn_abort_upload->setEnabled(false);
  reset_progress();
}

void
mbox_import_window::downsize_button(QPushButton* btn)
{
  QFont font = btn->font();
  font.setPointSize((font.pointSize()*8)/10);
  btn->setFont(font);
}

void
mbox_import_window::abort_import()
{
  DBG_PRINTF(3, "abort_import()");
  db_cnx db;
  try {
    sql_stream s("UPDATE import_mbox SET status=2 WHERE import_id=:p1", db);
    s << m_import_id;
    m_import_running=false;
    reset_progress();
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}

void
mbox_import_window::delete_imported_messages()
{
  db_cnx db;
  try {
    sql_stream s("SELECT mail_id FROM import_message WHERE import_id=:p1 ORDER BY mail_id", db);
    s << m_import_id;
    QList<mail_id_t> list;
    while (!s.eos()) {
      mail_id_t id;
      s >> id;
      list.append(id);
    }
    if (!list.isEmpty()) {
      db.begin_transaction();
      bool success=true;
      for (int i=0; i<list.size() && success; i++) {
	mail_msg msg;
	msg.set_mail_id(list.at(i));
	success = msg.mdelete();
      }
      if (success)
	db.commit_transaction();
      else
	db.rollback_transaction();
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }
}
