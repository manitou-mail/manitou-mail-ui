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

/* Various widgets used by the main composer class (new_mail_widget) */

#ifndef INC_COMPOSER_WIDGETS_H
#define INC_COMPOSER_WIDGETS_H

#include "identities.h"
#include "mail_displayer.h"
#include "edit_address_widget.h"

#include <QPlainTextEdit>
#include <QDialog>
#include <QUrl>

class QTreeWidgetItem;
class QTreeWidget;
class QGridLayout;
class QLabel;
class QLineEdit;
class QComboBox;
class QDropEvent;
class QDragMoveEvent;
class QDragEnterEvent;

class body_edit_widget : public QPlainTextEdit
{
  Q_OBJECT
public:
  body_edit_widget (QWidget* parent=NULL);
  virtual ~body_edit_widget() {}
protected:
  void dragEnterEvent(QDragEnterEvent*);
  void dragMoveEvent(QDragMoveEvent*);
  void dropEvent(QDropEvent*);

signals:
  void attach_file_request(const QUrl);
};

class input_addresses_widget : public QWidget
{
  Q_OBJECT
public:
  input_addresses_widget(const QString& addresses);
  virtual ~input_addresses_widget() {}
public slots:
  void ok();
  void cancel();
  void show_recent_to();
  void show_recent_from();
  void show_prio_contacts();
  void addr_selected(QTreeWidgetItem*,int);
  void find_contacts();
signals:
  void addresses_available(QString s);
private:
  void set_header_col1(const QString& text);
  /* For query-based access to selected parts of the address book
     (addresses to which mail was recently sent, or from which mail was
     recently received, or other criteria) */
  struct accel {
/*    QPushButton* b;*/
    int rows_displayed;
  };
  QTextEdit* m_wEdit;
  QString format_multi_lines (const QString);
  void show_recent(int what);
  struct accel m_recent_to;
  struct accel m_recent_from;
  struct accel m_prioritized;
  QTreeWidget* m_addr_list;	// in which accel results are displayed
  /* type of accel info currently displayed in m_addr_list
     0: nothing, 1: recent to, 2: recent from  */
  int m_accel_type;
  QLineEdit* m_wfind;
};

class identity_widget: public QDialog
{
  Q_OBJECT
public:
  identity_widget(QWidget* parent);
  virtual ~identity_widget();
  void set_email_address(const QString email);
  void set_name(const QString name);
  QString email_address();
  QString name();
private slots:
  void cancel();
  void ok();
private:
  QLineEdit* w_email;
  QLineEdit* w_name;
};

/* This class glues together (in a outer grid layout):
   - a combobox offering different types of header fields
   - a qlineedit with an address autocompletion feature
   - and a button that pops up a non-modal input_addresses_widget window
*/
class header_field_editor: public QObject
{
  Q_OBJECT
public:
  header_field_editor(QWidget* parent);
  ~header_field_editor();
  enum header_index {
    index_header_to=0,
    index_header_cc,
    index_header_bcc,
    index_header_replyto,
    index_header_add,
    index_header_remove,
  };
  void grid_insert(QGridLayout* layout, int row, int column);
  void set_type(header_index type);

  /* return a non-localized field name such as "To", "Cc", when the
     value is not empty and the combobox index is mapped to a real
     header field. Otherwise return an empty string. */
  QString get_field_name() const;

  /* return the current value from the lineedit */
  QString get_value() const;

  /* set the lineedit value */
  void set_value(const QString);
private:
  edit_address_widget* m_lineedit;
  QComboBox* m_cb;
  QPushButton* m_more_button;
  int m_old_index;
public slots:
  void addresses_offered(QString addresses);
  void more_addresses();
  void cb_changed(int);
signals:
  void add();
  void remove();
};

class html_source_editor : public QPlainTextEdit
{
  Q_OBJECT
public:
  html_source_editor(QWidget* parent=NULL);
protected:
  void resizeEvent(QResizeEvent*);
public slots:
  void position_label();
private:
  QLabel* m_sticker;
};

#endif
