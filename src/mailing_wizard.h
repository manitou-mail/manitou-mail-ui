/* Copyright (C) 2004-2025 Daniel Verite

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

#ifndef INC_MAILING_WIZARD_H
#define INC_MAILING_WIZARD_H

#include "mailing.h"
#include "identities.h"
#include "addresses.h"

#include <QWizard>
#include <QWizardPage>
#include <QStringList>
#include <QFile>

class QRadioButton;
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
class QLabel;
class QComboBox;
class QCheckBox;
class QProgressBar;
class QPushButton;

class mailing_wizard_page: public QWizardPage
{
protected:
  void cleanupPage();
  mailing_options* get_options();
};

class mailing_wizard_page_title : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_title();
  void initializePage();
  mail_address sender_address();
  bool isComplete() const;
private:
  identities m_identities;
  QLineEdit* m_title_widget;
  QComboBox* m_cb_sender;
};

class mailing_wizard_page_import_data : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_import_data();
  int nextId() const;
  void initializePage();
private:
  QRadioButton* m_rb[3];
  QLabel* m_label_variables;
};

#if 0
class mailing_wizard_page_format : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_format();
  bool validatePage();
private:
  QRadioButton* m_rb[3];
};

class mailing_wizard_page_html_template : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_html_template();
};

class mailing_wizard_page_text_template : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_text_template();
};
#endif

class mailing_wizard_page_properties : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_properties();
};

class mailing_wizard_page_data_file : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_data_file();
  bool validatePage();
  int nextId() const;
  void initializePage();
public slots:
  void browse();
private:
  QLineEdit* m_filename;
  QLabel* m_label_csv_rules;
};

class mailing_wizard_page_paste_address_list : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_paste_address_list();
  QStringList get_addresses();
  int nextId() const;
private:
  QPlainTextEdit* m_textarea;
};

class double_file_input: public QWidget
{
  Q_OBJECT
public:
  double_file_input(QWidget* parent=NULL);
  QPushButton* m_btn_browse1;
  QPushButton* m_btn_browse2;
  QLabel* m_label_html;
  QLabel* m_label_text;
  QLabel* m_label_filename_text;  
  QLineEdit* m_filename_html;
  QLineEdit* m_filename_text;
  bool check_files(int which);
  int submitted_files();
  void format_requested(mailing_db::template_format format);
public slots:
  void browse_html_file();
  void browse_text_file();
private:
  bool check_file_existence(const QString path);
};

class mailing_wizard_page_template : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_template();
  bool validatePage();
  int selected_template_id();
  int selected_mailing_id();
  QString html_template_filename();
  QString text_template_filename();
  typedef enum  {
    template_source_files=1,
    template_source_db_tmpl=2,
    template_source_prev_mailing=3
  } tmpl_template_source;
  tmpl_template_source template_source();
  mailing_db::template_format body_format() const;
  void load_templates(mailing_options*);
  void load_file_templates(const QString filename_text, 
			   const QString filename_html, 
			   QString* text, QString* html);
  QString generate_text_part(const QString);
  QStringList m_template_variables;
public slots:
  void format_chosen(int);
  void template_source_chosen(int);
private:
  void hide_all();
  mailing_db::template_format m_format;
  tmpl_template_source m_template_source;
  QListWidget* m_wtmpl;
  QListWidget* m_wmailings;
  QRadioButton* m_rb_file_template;
  QRadioButton* m_rb_database_template;
  QRadioButton* m_rb_previous_mailing_template;
  QRadioButton* m_rb[4];
  QLabel* m_label_mailings;
  QLabel* m_label_template;
  double_file_input* m_wpart_files;
};

class mailing_wizard_page_parse_data : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_parse_data();
  void initializePage();
  int nextId() const;
  bool validatePage();
  bool isComplete() const;
  QStringList& recipient_list();
  static bool parse_line(const QString ql, QStringList& recipients);
private:
  QLabel* m_result;
  QLabel* m_errors_label;
  QLabel* m_stop_text;
  QLabel* m_ignore_errors_text;
  QCheckBox* m_ignore_errors_checkbox;
  QPlainTextEdit* m_errors_textarea;
  bool m_file_checked;
  int m_nb_lines;
  int m_nb_addresses;
  int m_nb_errors;
  //  int m_max_errors;  // number of errors at which we give up on parsing
  QStringList m_error_lines;
  QStringList m_recipient_list;
  void check_addresses_file(QFile&);
  void check_pasted_addresses(const QStringList&);
  void check_csv_file(QFile&);
private slots:
  void ignore_error_state_changed(int state);
};

class mailing_wizard_page_final : public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_final();
  QLabel* m_progress_label;
  QProgressBar* m_progress_bar;
private slots:
  void preview_mailing();
signals:
  void preview();
};

class mailing_wizard_page_error_identities: public mailing_wizard_page
{
  Q_OBJECT
public:
  mailing_wizard_page_error_identities();
  bool isComplete() const;
};

class mailing_wizard : public QWizard
{
  Q_OBJECT
public:
  mailing_wizard();
  bool validateCurrentPage();
  QStringList get_pasted_addresses();
  enum {
    page_title, page_template, page_import_data, page_parse_data,
    page_data_file, page_paste_address_list, page_format, 
    page_properties, page_test_mail, page_final, page_error_identities
  };
  bool launch_mailing();
  mailing_options m_options;
  QStringList m_template_variables;
public slots:
  void preview_mailmerge();
private:
  void generate_text_part();
  void gather_options();
  void load_file_templates(const QString text_file, const QString html_file, QString* text, QString* html);
};

#endif
