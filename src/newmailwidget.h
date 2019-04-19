/* Copyright (C) 2004-2019 Daniel Verite

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

#ifndef INC_NEWMAILWIDGET_H
#define INC_NEWMAILWIDGET_H

#include "db.h"
#include "identities.h"
#include "mail_displayer.h"
#include "composer_widgets.h"
#include "mail_template.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMap>
#include <QProcess>
#include <QStringList>
#include <QTemporaryFile>
#include <QUrl>

#include <map>

class tags_box_widget;
class attch_listview;
class edit_address_widget;
class html_editor;

class QAction;
class QActionGroup;
class QCloseEvent;
class QComboBox;
class QDateTimeEdit;
class QGridLayout;
class QLabel;
class QLineEdit;
class QMenu;
class QProgressBar;
class QPushButton;
class QResizeEvent;
class QStackedWidget;
class QToolBar;

class new_mail_widget : public QMainWindow
{
  Q_OBJECT
public:
  new_mail_widget(mail_msg*, QWidget *parent=0);
  virtual ~new_mail_widget() {}
  void set_body_text(const QString& b) {
    m_bodyw->setPlainText(b);
  }
  void insert_signature();
  void prepend_html_paragraph(const QString para);
  void append_html_paragraph(const QString para);

  // interpolate variables inside the signature
  QString expand_signature(const QString signature, const mail_identity& identity);

  const mail_identity* get_current_identity();
  void start_edit();
  // message handling
  void make_message(const QMap<QString,QString>& user_headers);
  mail_msg& get_message() { return m_msg; }
  void show_tags();

  // returns empty or an error message produced at init time
  QString errmsg() const {
    return m_errmsg;
  }

  enum edit_mode {
    plain_mode=1,
    html_mode=2
  };
  void format_html_text();
  void format_plain_text();
public slots:
  void print();

  void send_now();
  void send_later();
  void cancel();
  void keep();
  void closeEvent(QCloseEvent*);
  void remove_field_editor();
  void add_field_editor();
  void attach_files();
  void insert_file();
  void change_identity();
  void launch_external_editor();
  //  void tag_selected(int);
  void toggle_edit_source(bool);
  void toggle_wrap(bool);
  void toggle_tags_panel(bool);
  void change_font();
  void store_settings();
  void edit_note();
  void load_template();
  void save_template();
  void other_identity();
  void open_global_notepad();

  void to_format_html_text();
  void to_format_plain_text();
  void attach_file(const QUrl);

  void run_edit_action(const char*);

signals:
/*  void change_status_request (uint id, uint mask_set, uint mask_unset);*/
  void refresh_request (mail_id_t m_id);
  void tags_counters_changed(QList<tag_counter_transition>);

private slots:
  void external_editor_finished(int,QProcess::ExitStatus);
  void external_editor_error(QProcess::ProcessError error);
  void external_editor_cleanup();
  void abort_external_editor();

private:
  struct external_edit {
    QProcess* process = NULL;	// reinstantiated for each edit session
    QTemporaryFile tmpf;	// removed after each edit session
    bool active = false;	// waiting for the editor to finish
    bool inserted = false;	/* whether contents from the editor
				   have been imported at least once */
    QPushButton* m_abort_button = NULL;
  } m_external_edit;

  /* tests that must be done before storing the new message into the database */
  bool check_validity();
  /* called by send_now() or send_later() after validity is checked */
  void send();
  void set_wrap_mode();
  void join_address_lines (QString&);
  QString check_addresses(const QString addresses,
			  QStringList& bad_addresses,
			  bool *unparsable=NULL);

  int text_signature_location();

  mail_msg m_msg;
  body_edit_widget* m_bodyw;
  html_editor* m_html_edit;
  html_source_editor* m_html_source_editor;
  QStackedWidget* m_edit_stack;
  edit_mode m_edit_mode;
  mail_template m_template;

  attch_listview* m_qAttch;
  std::map<QString,QString> m_suffix_map;
  QList<header_field_editor*> m_header_editors;
  QLineEdit* m_wSubject;
  QLabel* lSubject;
  QGridLayout* gridLayout;
  tags_box_widget* m_wtags;
//  std::map<int,input_addresses_widget*> m_map_ia_widgets;

  bool m_wrap_lines;
  bool m_close_confirm;		/* ask user confirmation on close? */
  QMenu* m_pMenuFormat;
  QMenu* m_ident_menu;
  QToolBar* m_toolbar_html1;
  QToolBar* m_toolbar_html2;
  void load_identities(QMenu*);
  QString m_from;		// email only (no name, has to match an identity)
  QString m_other_identity_email;
  QString m_other_identity_name;
  QString m_signature_marker;	// unique identifier to refer to the signature inside HTML text
  QDateTime m_send_datetime;	// empty = send immediately
  identities m_ids;
  void make_toolbars();
  void create_actions();
  QString m_note;
  QString m_errmsg;
  void display_note();

  QAction* m_action_send_msg;
  QAction* m_action_send_later;
  QAction* m_action_attach_file;
  QAction* m_action_insert_file;
  QAction* m_action_edit_note;
  QAction* m_action_external_editor;
  QAction* m_action_identity_other;
  QAction* m_action_edit_other;
  QAction* m_action_add_header;
  QAction* m_action_open_notepad;
  QAction* m_action_plain_text;
  QAction* m_action_html_text;
  QActionGroup* m_identities_group;
  QMap<QAction*,mail_identity*> m_identities_actions;

  QProgressBar* m_progress_bar;
public:
  static QString m_last_attch_dir;
};

class schedule_delivery_dialog : public QDialog
{
  Q_OBJECT
public:
  schedule_delivery_dialog(QWidget* parent=NULL);
  QDateTimeEdit* m_send_datetime;
};

#endif
