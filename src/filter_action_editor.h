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

#ifndef INC_FILTER_ACTION_EDITOR_H
#define INC_FILTER_ACTION_EDITOR_H

#include <QWidget>
#include <QString>
#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>

#include "filter_rules.h"

class QCheckBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QStackedLayout;
class QBoxLayout;
class QButtonGroup;
class QRadioButton;
class QComboBox;
class QEvent;


class tag_selector;

/*
  Base class to implement the boxes of widgets used as input for the
  action parameters. A new action can be created by implementing a
  subclass of action_line that deals internally with its own input
  widgets but exposes the interface defined here.
*/
class action_line : public QWidget
{
  Q_OBJECT
public:
  action_line(QWidget* parent=0L) : QWidget(parent) {}
  action_line(QWidget* parent, const QString& label, const QString& type);

  // enable or disable the widgets for input
//  virtual void enable(bool)=0;
  // get a string representation from the widgets current contents
  virtual QString get_param();
  // set the widgets contents from a string representation
  virtual void set_param(const QString&);
  // reset the widgets
  virtual void reset();

  QString getval() {
    return m_type + ": " + get_param();
  }
  QBoxLayout* default_layout();
  void set_description(const QString);
//  action_radio_button* m_rb;
  QString m_type;
  // enum of the different kinds of actions supported by the UI
  // idx_max is used by foreign classes to iterate through them
  // (see filter_edit::w_actions)
  enum {
    idx_tag=0,
    idx_status,
    idx_prio,
    idx_redirect,
    idx_stop,
    idx_add_header,
    idx_remove_header,
    idx_set_identity,
    idx_discard,
    idx_max
  };
  static QString label(int action_idx);
  static QString description(int action_idx);

  struct action_description {
    const char* label;
    const char* description;
  };
  static struct action_description m_descriptions[];
signals:
  // emit this signal when the user has changed an action's parameter
  void new_value(QString action_type, QString val);
protected:
  QString m_description_text;	// long text describing the action
  QBoxLayout* m_default_layout; // vbox layout
  QLabel* m_descr;
};

class filter_action_chooser_radio_button: public QRadioButton
{
  Q_OBJECT
public:
  filter_action_chooser_radio_button(const QString& text, QWidget* parent=0);
protected:
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
signals:
  void entered(int);
  void left(int);
};

class filter_action_chooser: public QWidget
{
  Q_OBJECT
public:
  filter_action_chooser(QWidget* parent=0);
  void reset_choice();
public slots:
  void cont();
  void action_choosen(int);
  void hover_on_choice(int);
  void hover_off_choice(int);
private:
  QLabel* m_description;
  QButtonGroup* m_group;
signals:
  void open_action(int);
  void cancelled();
};

class filter_action_editor: public QDialog
{
  Q_OBJECT
public:
  filter_action_editor(QWidget* parent=NULL);
  QStackedLayout* m_stackl;
  action_line* w_actions[action_line::idx_max];
  filter_action get_action();
public slots:
  void display_action(const filter_action*);
  void display_action(int action_id);
  void display_action_chooser();
private:
  void reset_actions();
  filter_action_chooser* m_chooser;
  filter_action m_current_action;
};


class action_stop: public action_line
{
  Q_OBJECT
public:
  action_stop(QWidget* parent=0);
};

class action_discard: public action_line
{
  Q_OBJECT
public:
  action_discard(QWidget* parent=0);
  QString get_param();
  void set_param(const QString&);
private:
  QRadioButton* m_delet;
  QRadioButton* m_trash;
};

class action_redirect: public action_line
{
  Q_OBJECT
public:
  action_redirect(QWidget* parent=0);
  QString get_param() {
    return m_redirect->text();
  }
  void set_param(const QString& address) {
    m_redirect->setText(address);
  }
  void reset() {
    m_redirect->setText("");
  }
private:
  QLineEdit* m_redirect;
};


class action_tag: public action_line
{
  Q_OBJECT
public:
  action_tag(QWidget* parent=0);
  tag_selector* m_qc_tag;
  QString get_param();
  void set_param(const QString&);
  void reset();
private slots:
  void edit_tags();
private:
  QPushButton* m_edit_btn;
};

class action_set_identity: public action_line
{
  Q_OBJECT
public:
  action_set_identity(QWidget* parent=0);
  void set_param(const QString&);
  QString get_param();
private:
  QComboBox* m_cb_ident;
};

class action_status: public action_line
{
  Q_OBJECT
public:
  action_status(QWidget* parent=0);
  QString get_param();
  void set_param(const QString&);
  void reset();
  enum { nb_status=3 };
  static const char* status_text[nb_status];
  static const char status_letter[nb_status];
  QCheckBox* m_check[nb_status];
};

class action_prio: public action_line
{
  Q_OBJECT
public:
  action_prio(QWidget* parent=0);
  void reset();
  void set_param(const QString&);
  QString get_param();
  QRadioButton* m_check_set;
  QRadioButton* m_check_add;
  QSpinBox* m_prio_set;
  QSpinBox* m_prio_add;
private slots:
  void toggle_set(bool);
  void toggle_add(bool);
};

class action_add_header: public action_line
{
  Q_OBJECT
public:
  action_add_header(QWidget* parent=0);
  QLineEdit* m_header_name;
  QLineEdit* m_header_value;
  QRadioButton* m_set_value;
  QRadioButton* m_append_value;
  QRadioButton* m_prepend_value;
  QString get_param();
  void set_param(const QString&);
  void reset();
};

class action_remove_header: public action_line
{
  Q_OBJECT
public:
  action_remove_header(QWidget* parent=0);
  QString get_param();
  void set_param(const QString&);
  void reset();
  QLineEdit* m_header_name;
};

#endif
