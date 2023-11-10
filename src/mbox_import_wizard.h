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

#ifndef INC_MBOX_IMPORT_WIZARD_H
#define INC_MBOX_IMPORT_WIZARD_H

#include <QWizard>
#include <QWizardPage>
#include <QStringList>
#include <QFile>
#include "mbox_file.h"

class QRadioButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QProgressBar;
class QPushButton;
class QLineEdit;
class tag_selector;

class mbox_import_wizard_page : public QWizardPage
{
protected:
  void cleanupPage();
  mbox_import_options* get_options();
};

class mbox_import_wizard_page_mbox_file: public mbox_import_wizard_page
{
  Q_OBJECT
public:
  mbox_import_wizard_page_mbox_file();
  bool validatePage();
private slots:
  void browse();
private:
  QLineEdit* m_wfilename;
};

class mbox_import_wizard_page_options: public mbox_import_wizard_page
{
  Q_OBJECT
public:
  mbox_import_wizard_page_options();
  void initializePage();
  QComboBox* m_cbox_status;
  tag_selector* m_cbox_tag;
  QCheckBox* m_apply_filters;
  QCheckBox* m_auto_purge;
};

/* This page uploads the entire mailbox to the database
   and waits for the import by manitou-mdx to (hopefully) start
   If it doesn't start, we're still done with the wizard.
*/
class mbox_import_wizard_page_upload: public mbox_import_wizard_page
{
  Q_OBJECT
public:
  mbox_import_wizard_page_upload();
  void initializePage();
  bool isComplete() const; // override
  bool validatePage();
public slots:
  void update_progress_bar(double);
  void do_upload();
#if 0
  void check_import();
#endif
private:
  QProgressBar* m_progress_bar;
  QPushButton* m_btn_abort_upload;
  QLabel* m_label_result;
  QLabel* m_label_waiting;
  bool m_upload_finished;
  int m_import_id;
  mbox_file m_box;
#if 0
  QTimer* m_waiting_timer;
  int m_waiting_secs;
  bool m_import_started;
#endif
};

class mbox_import_wizard_page_final: public mbox_import_wizard_page
{
  Q_OBJECT
public:
  mbox_import_wizard_page_final();
};

class mbox_import_wizard : public QWizard
{
  Q_OBJECT
public:
  mbox_import_wizard();
//  bool validateCurrentPage();
  enum {
    page_mbox_file, page_options, page_upload
  };
  mbox_import_options* get_options();
private:
  mbox_import_options m_options;
};


#endif
