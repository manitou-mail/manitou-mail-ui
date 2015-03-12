/* Copyright (C) 2004-2015 Daniel Verite

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
#include "icons.h"
#include "export_window.h"

#include <QLabel>
#include <QFile>
#include <QFileDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QBoxLayout>
#include <QProgressBar>
#include <QGridLayout>
#include <QHeaderView>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <QMessageBox>

void
open_export_window()
{
  export_window* w = new export_window();
  w->show();
}

export_window::export_window()
{
  setWindowIcon(UI_ICON(ICON16_EXPORT_MESSAGES));
  setWindowTitle(tr("Export messages"));

  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setMargin(5);

  /*
  QGridLayout* gridLayout = new QGridLayout();
  top_layout->addLayout(gridLayout);
  */

  QHBoxLayout* tagl = new QHBoxLayout();
  top_layout->addLayout(tagl);

  QLabel* lTag = new QLabel(tr("Tag:"), this);
  tagl->addWidget(lTag);
  m_qtag_sel = new tag_selector(this);
  m_qtag_sel->init(false);
  tagl->addWidget(m_qtag_sel);
  tagl->addStretch(2);

  m_progress_bar = new QProgressBar;
  top_layout->addWidget(m_progress_bar);
  m_progress_bar->setEnabled(false);

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->setSpacing(10);
  top_layout->addLayout(buttons);

  m_btn_start = new QPushButton(tr("Start"));
  buttons->addWidget(m_btn_start);

  m_btn_stop = new QPushButton(tr("Stop"));
  buttons->addWidget(m_btn_stop);
  
  top_layout->addStretch(2);

  connect(m_btn_start, SIGNAL(clicked()), this, SLOT(start()));
  connect(m_btn_stop, SIGNAL(clicked()), this, SLOT(stop()));

  stop();
}

export_window::~export_window()
{
}

void
export_window::start()
{
  int tag_id = m_qtag_sel->current_tag_id();
  if (!tag_id) {
    QMessageBox::warning(this, APP_NAME, tr("Please select a tag"));
    return;
  }

  QString dir = QFileDialog::getExistingDirectory(this, tr("Save to directory..."));
  if (!dir.isEmpty()) { // if accepted
    m_running = true;
    db_cnx db;
    sql_stream s0("SELECT count(*) from mail_tags WHERE tag=:tid", db);
    s0 << tag_id;
    int total_count=0;
    if (!s0.eos()) {
      s0 >> total_count;
    }

    if (total_count==0) {
      QMessageBox::warning(this, APP_NAME, tr("No message to export."));
      return;
    }

    m_btn_start->setEnabled(false);
    m_btn_stop->setEnabled(true);

    m_progress_bar->setMaximum(total_count);
    m_progress_bar->setEnabled(true);
    sql_stream s1("DECLARE c_id CURSOR WITH HOLD FOR SELECT mail_id FROM mail_tags WHERE tag=:tid", db);
    s1 << tag_id;
    bool end=false;
    int nb=0;
    while (!end && m_running) {
      sql_stream s2("FETCH FROM c_id", db);
      m_progress_bar->setValue(nb++);
      QApplication::processEvents();
      if (!s2.eos()) {
	mail_id_t id;
	s2 >> id;
	save_raw_file(id, db, dir);
      }
      else
	end=true;
    }
    sql_stream s3("CLOSE c_id", db);
    if (!m_running)
      m_progress_bar->reset();
  }

  stop();
}

void
export_window::save_raw_file(mail_id_t id, db_cnx& db, QString dirname)
{
  DBG_PRINTF(4, "dumping file for mail_id=%d", id);
  PGconn* pgconn = db.connection();

  QString query= QString("SELECT raw_file_contents(%1)").arg(id);

  QByteArray qb_query = query.toUtf8();
  PGresult* res = PQexecParams(pgconn,
			       qb_query.constData(),
			       0, // number of params
			       NULL, // param types,
			       NULL, // param values,
			       NULL, // param lengths
			       NULL, // param formats
			       1 // result format=binary
			       );
  if (res && PQresultStatus(res)==PGRES_TUPLES_OK) {
    int len = PQgetlength(res, 0, 0);
    QFile file(dirname+QString("/mail-%1.eml").arg(id));
    if (file.open(QIODevice::WriteOnly)) {
      qint64 written = file.write((const char*)PQgetvalue(res, 0, 0), len);
      DBG_PRINTF(4, "%ld / %ld bytes written", written, (qint64)len);
      file.close();
    }
  }
  if (res)
    PQclear(res);
}


void
export_window::stop()
{
  m_running=false;
  m_btn_start->setEnabled(true);
  m_btn_stop->setEnabled(false);
  m_progress_bar->setEnabled(false);
}
