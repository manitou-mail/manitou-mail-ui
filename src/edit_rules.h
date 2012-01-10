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

#ifndef INC_EDIT_RULES_H
#define INC_EDIT_RULES_H

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QRadioButton>
#include <QTreeWidgetItem>
#include "filter_rules.h"

class headers_groupview;
class filter_action_editor;
class tag_selector;
class QCheckBox;
class QLabel;
class QSpinBox;
class QKeyEvent;
class QPushButton;
class button_group;
class QStackedLayout;
class QBoxLayout;
class QFocusEvent;
class QToolButton;
class QCloseEvent;

class QTimer;
class filter_results_window;

class msgs_filter;
class fetch_thread;

class action_lvitem;

class filter_treewidget: public QTreeWidget
{
  Q_OBJECT
public slots:
};

class expr_lvitem : public QTreeWidgetItem
{
public:
  expr_lvitem(QTreeWidget* parent=NULL);
  virtual ~expr_lvitem();
  filter_expr* m_expr;
  int m_id;			// unique across the listview
  bool m_db;			// fetched from database (as opposed to created in memory and not yet stored)
  // case-insensitive sort
  bool operator<(const QTreeWidgetItem &other) const;
  void set_expression_text(const QString text);
private:
  static int m_max_id;
};

class filter_edit;

class focus_line_edit: public QLineEdit
{
  Q_OBJECT
public:
  focus_line_edit(QWidget* parent, const QString& name, filter_edit* fe);
  void focusOutEvent(QFocusEvent*);
  void setText(const QString&);
private:
  filter_edit* m_form;		// parent form (owner)
  QString m_name;
  QString m_last_value;
public slots:
  void validate();
};

class expr_line_edit: public QLineEdit
{
  Q_OBJECT
public:
  expr_line_edit(QWidget* parent=NULL);
public slots:
  void validate();
  void show_button(int);
protected:
  void resizeEvent(QResizeEvent *);
private slots:
  void update_button(const QString &text);
  void button_clicked();
private:
  QToolButton *m_button;
signals:
  void toolbutton_clicked();
  void text_changed();
};

class action_listview : public QTreeWidget
{
  Q_OBJECT
public:
  action_listview(QWidget* parent=0) : QTreeWidget(parent) {}
  virtual ~action_listview() {}
  QString label(const QString action_type, const QString action_args);
  action_lvitem* selected_action();
  void move_filter_action(int, filter_expr*);
protected:
  void keyPressEvent(QKeyEvent*);
signals:
  void key_del();
};

class action_lvitem : public QTreeWidgetItem
{
public:
  action_lvitem(action_listview* parent, action_lvitem* after=0) :
    QTreeWidgetItem(parent, after), m_act_ptr(NULL) {}
  action_lvitem() : m_act_ptr(NULL) {}
  filter_action* m_act_ptr;
};



class filter_edit : public QWidget
{
  Q_OBJECT
public:
  filter_edit(QWidget* parent=0);
  ~filter_edit();
  void expr_update();
  bool expr_fields_filled() const;
#if 0
  void set_sel_list(const std::list<unsigned int>& l);
#endif
  // Column numbers
  enum {
    icol_number=0,
    icol_name,
    icol_expr,
    icol_last_hit
  };
protected:
  virtual void closeEvent(QCloseEvent*);
  
private slots:
  void zoom_on_expr();
  void show_eval_message();
  void end_test_requested();
  void display_expression_validity();
  void timer_done();
  void close_results_window();
  void filter_out_exprs(const QString&);
  void enable_up_down_buttons(bool);
  void move_filter_bottom();
  void move_filter_up();
  void move_filter_top();
  void move_filter_down();
  void direction_changed(int);
  void ok();
  void cancel();
  void help();
  void delete_expr();
  void test_expr();
  void expr_sort_order_changed(int,Qt::SortOrder);
#if 0
  void suggest_filter();
#endif
  void new_expr();
  void selection_changed();
  void expr_from_header(QTreeWidgetItem*); // create a new expr from an entry of a headers_groupview dialog
  void close_headers_groupview();
  void current_name_edited(const QString& text);
  void current_expr_edited(const QString& text);
  void add_action();
  void remove_action();
  void edit_action();
  void modify_action(QTreeWidgetItem*,int);
  void move_action_up();
  void move_action_down();
  void actions_selection_changed();
private:
  bool m_confirm_close;
  fetch_thread* m_fthread;
  msgs_filter* m_test_msgs_filter;
  QTimer* m_ftimer;
  QTimer* m_eval_timer;
  bool m_waiting_for_results;
  bool m_filter_run_stopped;
  filter_results_window* m_test_window_results;
  int m_nb_filter_test_match;

  // expr
  std::list<unsigned int> m_sel_list;
  QTreeWidget* lv_expr;
  int m_date_format;
  focus_line_edit* ql_expr_name;
  QPushButton* expr_btn_delete;
#if 0
  QPushButton* m_suggest_btn;
#endif
  button_group* m_dir;
  QFrame* m_expr_container;
  expr_line_edit* ql_expr_full;
  bool load();
  void clear_expr();
  void disable_all_expr();
  void enable_expr_edit(bool);
  void renumber_items();
  void dlg_fields_to_filter_expr(filter_expr *e);
  void filter_expr_to_dlg(filter_expr* e);
  void move_filter_item(int direction);
  // actions
  void untie_actions();
  void display_actions();
  void create_null_action();
  void reset_actions();

  void validate_expression();

  action_listview* lv_actions;
  expr_list m_expr_list;
  QLabel* m_lrules;
  QWidget* m_actions_container;
  filter_action_editor* m_action_editor;
  action_lvitem* m_current_action;
  filter_expr* m_current_expr;
  expr_lvitem* selected_item() const;
  headers_groupview* m_hd;

  QPushButton* m_btn_up;
  QPushButton* m_btn_down;
  QPushButton* m_btn_bottom;
  QPushButton* m_btn_top;

  QPushButton* m_btn_add_action;
  QPushButton* m_btn_edit_action;
  QPushButton* m_btn_remove_action;
  QPushButton* m_btn_up_action;
  QPushButton* m_btn_down_action;

  QToolButton* m_zoom_button;
};

// Local Variables: ***
// mode: c++ ***
// End: ***

#endif
