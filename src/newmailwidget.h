/* Copyright (C) 2004-2012 Daniel Verite

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

#include <QMainWindow>
#include <QDialog>
#include <QStringList>
#include <QMap>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QUrl>

#include <map>

class tags_box_widget;
class attch_listview;
class edit_address_widget;
class html_editor;

class QComboBox;
class QGridLayout;
class QMenu;
class QCloseEvent;
class QLineEdit;
class QResizeEvent;
class QAction;
class QActionGroup;
class QLabel;
class QToolBar;
class QStackedWidget;


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
  void send();
  void cancel();
  void keep();
  void closeEvent(QCloseEvent*);
  void remove_field_editor();
  void add_field_editor();
  void attach_files();
  void insert_file();
  void change_identity();
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

signals:
/*  void change_status_request (uint id, uint mask_set, uint mask_unset);*/
  void refresh_request (mail_id_t m_id);
private:
  void set_wrap_mode();
  void join_address_lines (QString&);
  QString check_addresses(const QString addresses,
			  QStringList& bad_addresses,
			  bool *unparsable=NULL);

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
  identities m_ids;
  void make_toolbars();
  void create_actions();
  QString m_note;
  QString m_errmsg;
  void display_note();

  QAction* m_action_send_msg;
  QAction* m_action_attach_file;
  QAction* m_action_insert_file;
  QAction* m_action_edit_note;
  QAction* m_action_identity_other;
  QAction* m_action_edit_other;
  QAction* m_action_add_header;
  QAction* m_action_open_notepad;
  QAction* m_action_plain_text;
  QAction* m_action_html_text;
  QActionGroup* m_identities_group;
  QMap<QAction*,mail_identity*> m_identities_actions;
public:
  static QString m_last_attch_dir;
};

#endif
