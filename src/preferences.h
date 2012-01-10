/* Copyright (C) 2004-2011 Daniel Verite

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

#ifndef INC_PREFERENCES_H
#define INC_PREFERENCES_H

#include "main.h"
#include <QDialog>
#include <QList>
#include "identities.h"
#include "app_config.h"

class QTabWidget;
class QComboBox;
class QLineEdit;

class prefs_dialog : public QDialog
{
  Q_OBJECT
public:
  prefs_dialog(QWidget* parent=0);
  ~prefs_dialog();
  void conf_to_widgets(app_config&);
  void widgets_to_conf(app_config&);
protected slots:
  virtual void done(int);
  void help();
  void page_changed(QWidget*);
  void ident_changed(int);
private slots:
  void new_identity();
  void delete_identity();
  void email_lost_focus();
#if 0
  void other_conf(const QString&);
#endif
  void new_mimetype();
  void delete_mimetype();
  void edit_mimetype();
  void click_help_dir();
  void click_bitmaps_dir();
  void click_attachments_dir();
  void click_browser_path();
private:
  QTabWidget* m_tabw;
  QComboBox* m_conf_selector;
  struct preferences_widgets;
  struct preferences_widgets* m_widgets;
  QWidget* display_widget();
  QWidget* ident_widget();
  QWidget* paths_widget();
  QWidget* fetching_widget();
  QWidget* mimetypes_widget();
  QWidget* composer_widget();
  QString help_topic(QWidget*);
  void ask_directory(QLineEdit*);
  // the pages
  QWidget* m_display_page;
  QWidget* m_ident_page;
  QWidget* m_paths_page;
  QWidget* m_mimetypes_page;
  QWidget* m_fetching_page;
  QWidget* m_composer_page;

  // the identities stuff
  identities m_ids_map;
  QList<mail_identity*> m_ids;
  void widgets_to_ident();
  void ident_to_widgets();
  mail_identity* get_identity(const QString& email);
  mail_identity* m_curr_ident;
  int m_current_ident_id;
  bool update_identities_db();
  void add_new_identity();

  // viewers stuff
  void load_viewers();
  bool update_viewers_db();
  bool m_viewers_updated;
};

class viewer_edit_dialog : public QDialog
{
  Q_OBJECT
public:
  viewer_edit_dialog(QWidget* parent, const QString&, const QString&);
  virtual ~viewer_edit_dialog();
  QString mimetype() const;
  QString viewer() const;
private slots:
  void ok();
  void cancel();
private:
  QLineEdit* m_mimetype;
  QLineEdit* m_viewer;
};


#endif // INC_PREFERENCES_H
