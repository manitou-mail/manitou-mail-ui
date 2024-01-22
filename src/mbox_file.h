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

#ifndef INC_MBOX_FILE_H
#define INC_MBOX_FILE_H

#include <QString>
#include <QWidget>

#include "db.h"
#include "tags.h"

class QProgressBar;
class QComboBox;
class QCheckBox;
class QPushButton;
class QCloseEvent;
class m_listener;

class mbox_import_options
{
public:
  QString filename;
  int tag_id;
  int mail_status;
  bool apply_filters;
  bool auto_purge;
};

class mbox_file: public QObject
{
  Q_OBJECT
public:
  mbox_file();
  int import(const mbox_import_options&);
  int uploaded_count() const { return m_uploaded_count; }
  static bool check_format(const QString filename);
  bool launch_import_job(int);
  bool instantiate_job();
private:
  bool database_import(int);
  void import_message(int, int, const QByteArray&, PGconn*);
  QString m_filename;
  bool m_abort_requested;
  int m_uploaded_count;
signals:
  void progress_report(double); // percent towards completion
public slots:
  void abort_import();
};

class mbox_import_window: public QWidget
{
  Q_OBJECT
public:
  mbox_import_window(QWidget* parent=0);
  ~mbox_import_window();
private slots:
  void start();
  void browse_file();
  void update_progress(double percent);
  void update_progress_import();
  void upload_aborted();
  void reset_progress();
  void abort_import();
protected:
  void closeEvent(QCloseEvent*);
private:
  void delete_imported_messages();
  void downsize_button(QPushButton*);

  int m_import_id;
  bool m_import_running; // the import, not the upload
  db_listener* m_listener;
  QProgressBar* m_progress_bar;
  QComboBox* m_cbox_status;
  tag_selector* m_cbox_tag;
  QLineEdit* m_filename;
  QCheckBox* m_apply_filters;
  QPushButton* m_btn_start;
  QPushButton* m_btn_abort_upload;
  QPushButton* m_btn_abort_import;
};

#endif
