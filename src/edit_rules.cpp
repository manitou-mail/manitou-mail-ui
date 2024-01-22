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

#include "main.h"
#include "filter_rules.h"
#include "edit_rules.h"
#include "filter_expr_editor.h"
#include "tags.h"
#include "icons.h"
#include "helper.h"
#include "ui_controls.h"
#include "headers_groupview.h"
#include "tagsdialog.h"
#include "app_config.h"
#include "searchbox.h"
#include "filter_action_editor.h"
#include "filter_results_window.h"

#include "selectmail.h"
#include "filter_eval.h"

#include <QMessageBox>
#include <QFrame>
#include <QHeaderView>
#include <QTimer>
#include <QPushButton>
#include <QLayout>
#include <QLineEdit>
#include <QButtonGroup>
#include <QToolButton>

#include <QLabel>
#include <QCheckBox>
#include <QStringList>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QGridLayout>
#include <QFocusEvent>

int expr_lvitem::m_max_id=0;

//
// expr_lvitem
//
expr_lvitem::expr_lvitem(QTreeWidget* parent): QTreeWidgetItem(parent)
{
  m_id = ++m_max_id;
}

expr_lvitem::~expr_lvitem()
{
  // don't delete m_expr, we don't own it
}

// Sort items
bool
expr_lvitem::operator<(const QTreeWidgetItem &other) const
{
  int column = treeWidget()->sortColumn();
  switch(column) {
  case filter_edit::icol_number:
    // sort by number
    return text(column).toInt() < other.text(column).toInt();
  case filter_edit::icol_name:
  case filter_edit::icol_expr:
    // sort case insensitively
    return text(column).toLower() < other.text(column).toLower();
  case filter_edit::icol_last_hit:
    const expr_lvitem* lv1 = static_cast<const expr_lvitem*>(&other);
    return m_expr->m_last_hit.FullOutput() < lv1->m_expr->m_last_hit.FullOutput();
    break;
  }
  return false; // never reached
}

void
expr_lvitem::set_expression_text(const QString text)
{
  QString t = text;
  // replace newlines by spaces to prevent multi-line expressions, at least visually
  t.replace(QChar('\n'), QChar(' '));
  setText(filter_edit::icol_expr, t);
}


//
// filter_edit
//
filter_edit::filter_edit(QWidget* parent): QWidget(parent)
{
  m_hd=NULL;
  m_test_window_results = NULL;
  m_confirm_close=true;
  m_eval_timer = NULL;

  setWindowTitle(tr("Filter rules"));
  setWindowIcon(UI_ICON(FT_ICON16_FILTERS));
  m_date_format = get_config().get_date_format_code();

  QVBoxLayout* top_layout = new QVBoxLayout(this);

  // Top of the window: title,blank space,edit box 
  QHBoxLayout* top_hboxl = new QHBoxLayout();
  //  top_layout->setMargin(5);
  QLabel* lexpr = new QLabel(tr("Conditions:"));
  QFont fnt1 = lexpr->font();
  fnt1.setBold(true);
  lexpr->setFont(fnt1);
  top_hboxl->addWidget(lexpr);
  top_hboxl->addStretch(8);
  QLabel* filter_title = new QLabel(tr("Filter:"));
  search_edit* filter_box = new search_edit();
  connect(filter_box, SIGNAL(textChanged(const QString&)),
	  this, SLOT(filter_out_exprs(const QString&)));
  top_hboxl->addWidget(filter_title);
  top_hboxl->addWidget(filter_box);

  top_layout->addLayout(top_hboxl);

  QVBoxLayout* box_expr = new QVBoxLayout();
  top_layout->addLayout(box_expr, 10);

  QVBoxLayout* arrows_box = new QVBoxLayout();
  m_btn_top = new QPushButton(UI_ICON(ICON16_GO_TOP), "");
  m_btn_top->setToolTip(tr("Move to the top of the list"));
  m_btn_up = new QPushButton(UI_ICON(ICON16_GO_UP), "");
  m_btn_up->setToolTip(tr("Move one line up"));
  m_btn_down = new QPushButton(UI_ICON(ICON16_GO_DOWN), "");
  m_btn_down->setToolTip(tr("Move one line down"));
  m_btn_bottom = new QPushButton(UI_ICON(ICON16_GO_BOTTOM), "");
  m_btn_bottom->setToolTip(tr("Move to the bottom of the list"));
  connect(m_btn_top, SIGNAL(clicked()), this, SLOT(move_filter_top()));
  connect(m_btn_up, SIGNAL(clicked()), this, SLOT(move_filter_up()));
  connect(m_btn_down, SIGNAL(clicked()), this, SLOT(move_filter_down()));
  connect(m_btn_bottom, SIGNAL(clicked()), this, SLOT(move_filter_bottom()));

  arrows_box->addWidget(m_btn_top);
  arrows_box->addWidget(m_btn_up);
  arrows_box->addWidget(m_btn_down);
  arrows_box->addWidget(m_btn_bottom);
  enable_up_down_buttons(false);

  //  expr_sv->setMaximumHeight(150);

  QHBoxLayout* buttons_plus_view = new QHBoxLayout();
  buttons_plus_view->addLayout(arrows_box);
  lv_expr = new QTreeWidget();
  buttons_plus_view->addWidget(lv_expr);
//  box_expr->setFrameStyle(QFrame::Panel|QFrame::Raised);
//  box_expr->setMargin(2);

  box_expr->addLayout(buttons_plus_view);

//  lv_expr->setMultiSelection(false);
  QStringList labels;
  labels << tr("Order") << tr("Name") << tr("Expression") << tr("Last hit");
  lv_expr->setHeaderLabels(labels);
  lv_expr->header()->resizeSection(icol_number, 40);	// width for "number" column
  lv_expr->header()->resizeSection(icol_name, 100);	// width for "Name" column
  lv_expr->header()->resizeSection(icol_expr, 400);	// width for "Expression" column
  lv_expr->header()->resizeSection(icol_last_hit, 50);	// width for "Expression" column

  connect(lv_expr->header(), SIGNAL(sortIndicatorChanged (int, Qt::SortOrder)),
	  this, SLOT(expr_sort_order_changed(int,Qt::SortOrder)));

  lv_expr->setRootIsDecorated(false);
  lv_expr->setAllColumnsShowFocus(true);
  connect(lv_expr, SIGNAL(itemSelectionChanged()),
	  this, SLOT(selection_changed()));

  QHBoxLayout* expr_btn_box = new QHBoxLayout();
  box_expr->addLayout(expr_btn_box);

  QPushButton* expr_new = new QPushButton(tr("New"));
  expr_btn_box->addWidget(expr_new);
  expr_btn_box->setStretchFactor(expr_new, 1);
  connect(expr_new, SIGNAL(clicked()), this, SLOT(new_expr()));

  QPushButton* expr_test = new QPushButton(tr("Try"));
  expr_btn_box->addWidget(expr_test);
  expr_btn_box->setStretchFactor(expr_test, 1);
  connect(expr_test, SIGNAL(clicked()), this, SLOT(test_expr()));

  expr_btn_delete = new QPushButton(tr("Delete"));
  expr_btn_box->addWidget(expr_btn_delete);
  expr_btn_box->setStretchFactor(expr_btn_delete, 1);

  connect(expr_btn_delete, SIGNAL(clicked()), this, SLOT(delete_expr()));
  expr_btn_delete->setEnabled(false);
#if 0
  m_suggest_btn = new QPushButton(tr("Suggest"));
  expr_btn_box->addWidget(m_suggest_btn);
  m_suggest_btn->setEnabled(false);
  expr_btn_box->setStretchFactor(m_suggest_btn, 1);
  connect(m_suggest_btn, SIGNAL(clicked()), this, SLOT(suggest_filter()));
#endif
  expr_btn_box->addStretch(8);

  QFrame* expr_cont = new QFrame();
  expr_cont->setContentsMargins(3,3,3,0);
  m_expr_container = expr_cont;
  expr_cont->setFrameStyle(QFrame::Box);
  box_expr->addWidget(expr_cont);
  QGridLayout* expr_grid = new QGridLayout(expr_cont);
  expr_grid->setContentsMargins(4,4,4,1);
  //  expr_grid->setSpacing(1);  
  int row=0;
  int col=0;

  QLabel* lexpr_name = new QLabel(tr("Condition's name:"), expr_cont);
  expr_grid->addWidget(lexpr_name, row, col++);
  QLabel* ql_expr_label = new QLabel(tr("Expression:"), expr_cont);
  expr_grid->addWidget(ql_expr_label, row, col++);

  row++; col=0;
  ql_expr_name = new focus_line_edit(expr_cont, "expr_name", this);
  connect(ql_expr_name, SIGNAL(textEdited(const QString&)),
	  this, SLOT(current_name_edited(const QString&)));
  expr_grid->addWidget(ql_expr_name, row, col++);

  QHBoxLayout* expr_layout = new QHBoxLayout();
  ql_expr_full = new expr_line_edit();
  expr_layout->addWidget(ql_expr_full);
  connect(ql_expr_full, SIGNAL(textEdited(const QString&)),
	  this, SLOT(current_expr_edited(const QString&)));
  connect(ql_expr_full, SIGNAL(toolbutton_clicked()),
	  this, SLOT(show_eval_message()));

  QIcon zoom_icon(UI_ICON(FT_ICON16_ZOOM_PAGE));
  m_zoom_button = new QToolButton();
  expr_layout->addWidget(m_zoom_button);
  m_zoom_button->setIcon(zoom_icon);
  m_zoom_button->setToolTip(tr("Zoom"));
  expr_grid->addLayout(expr_layout, row, col++);
  connect(m_zoom_button, SIGNAL(clicked()), this, SLOT(zoom_on_expr()));

  expr_grid->setColumnStretch(0, 1); // 1/5 for name
  expr_grid->setColumnStretch(1, 4); // 4/5 for expression

  row++; col=0;
  expr_grid->addWidget(new QLabel(tr("Filter's direction:")), row, col++);
  QHBoxLayout* dirlayout = new QHBoxLayout;
  dirlayout->setContentsMargins(0,0,0,0);
  
  m_dir = new button_group(QBoxLayout::LeftToRight);
  m_dir->setContentsMargins(0,0,0,0);
  m_dir->setLineWidth(0);
  m_dir->addButton(new QRadioButton(tr("Incoming mail")), 0);
  m_dir->addButton(new QRadioButton(tr("Outgoing mail")), 1);
  m_dir->addButton(new QRadioButton(tr("Both")), 2);
  m_dir->addButton(new QRadioButton(tr("None (disabled)")), 3);
  dirlayout->addWidget(m_dir, 0);
  dirlayout->addStretch(10);
  expr_grid->addLayout(dirlayout, row, col++);
  connect(m_dir->group(), SIGNAL(buttonClicked(int)), this, SLOT(direction_changed(int)));

  // -- Actions --

  m_lrules = new QLabel(tr("Actions:"));
  m_lrules->setFont(fnt1);
  top_layout->addWidget(m_lrules);


  QHBoxLayout* btn_row_actions = new QHBoxLayout();
  btn_row_actions->setAlignment(Qt::AlignLeft);
  m_btn_add_action = new QPushButton(tr("Add"));
  btn_row_actions->addWidget(m_btn_add_action);
  connect(m_btn_add_action, SIGNAL(clicked()), this, SLOT(add_action()));
  QFont btn_font = m_btn_add_action->font();
  btn_font.setPointSize((btn_font.pointSize()*8)/10);
  m_btn_add_action->setFont(btn_font);

  m_btn_edit_action = new QPushButton(tr("Modify"));
  btn_row_actions->addWidget(m_btn_edit_action);
  connect(m_btn_edit_action, SIGNAL(clicked()), this, SLOT(edit_action()));
  m_btn_edit_action->setFont(btn_font);

  m_btn_remove_action = new QPushButton(tr("Remove"));
  btn_row_actions->addWidget(m_btn_remove_action);
  connect(m_btn_remove_action, SIGNAL(clicked()), this, SLOT(remove_action()));
  m_btn_remove_action->setFont(btn_font);

  top_layout->addLayout(btn_row_actions);

  QHBoxLayout* box_actions = new QHBoxLayout();
  top_layout->addLayout(box_actions, 3);
  

  QVBoxLayout* action_arrows_box = new QVBoxLayout();
  m_btn_up_action = new QPushButton(UI_ICON(ICON16_GO_UP), "");
  m_btn_up_action->setToolTip(tr("Move one line up"));
  m_btn_down_action = new QPushButton(UI_ICON(ICON16_GO_DOWN), "");
  m_btn_down_action->setToolTip(tr("Move one line down"));
  connect(m_btn_up_action, SIGNAL(clicked()), this, SLOT(move_action_up()));
  connect(m_btn_down_action, SIGNAL(clicked()), this, SLOT(move_action_down()));

  action_arrows_box->addWidget(m_btn_up_action);
  action_arrows_box->addWidget(m_btn_down_action);
  box_actions->addLayout(action_arrows_box);

  lv_actions = new action_listview();
  box_actions->addWidget(lv_actions);

  lv_actions->setHeaderLabels(QStringList(tr("Actions list")));
  lv_actions->setSortingEnabled(false);	// should be kept sorted by the order of actions
  connect(lv_actions, SIGNAL(key_del()), this, SLOT(remove_action()));
  connect(lv_actions, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	  this, SLOT(modify_action(QTreeWidgetItem*,int)));
  connect(lv_actions, SIGNAL(itemSelectionChanged()),
	  this, SLOT(actions_selection_changed()));

  // Help OK Cancel
  QHBoxLayout* hbox_valid = new QHBoxLayout();
  hbox_valid->setMargin(5);
  hbox_valid->setSpacing(5);
  top_layout->addLayout(hbox_valid);

  hbox_valid->addStretch(1);	// space at left
  QPushButton* whelp = new QPushButton(tr("Help"));
  hbox_valid->addWidget(whelp, 0);
  QPushButton* wok = new QPushButton(tr("OK"));
  hbox_valid->addWidget(wok, 0);
  wok->setDefault(true);
  QPushButton* wcancel = new QPushButton(tr("Cancel"));
  hbox_valid->addWidget(wcancel);

  connect(wok, SIGNAL(clicked()), this, SLOT(ok()));
  connect(wcancel, SIGNAL(clicked()), this, SLOT(cancel()));
  connect(whelp, SIGNAL(clicked()), this, SLOT(help()));
  clear_expr();
  disable_all_expr();
  untie_actions();
  load();

  lv_expr->header()->setSortIndicatorShown(true);
  lv_expr->sortByColumn(icol_number, Qt::AscendingOrder);
  lv_expr->setSortingEnabled(true);
  actions_selection_changed();
}

filter_edit::~filter_edit()
{
  if (m_hd)
    delete m_hd;
}


void
filter_edit::expr_sort_order_changed(int col_index, Qt::SortOrder order)
{
  enable_up_down_buttons(col_index==0 && order==Qt::AscendingOrder &&
			 !lv_expr->selectedItems().empty());
}

expr_lvitem*
filter_edit::selected_item() const
{
  return lv_expr->selectedItems().isEmpty() ? NULL:
    static_cast<expr_lvitem*>(lv_expr->selectedItems().at(0));
}

/* show the filter expressions that contain 'substring' and hide those that don't */
void
filter_edit::filter_out_exprs(const QString& substring)
{
  QTreeWidgetItemIterator iter(lv_expr);
  while (*iter) {
    if (substring.isEmpty() ||
	((*iter)->text(icol_name).contains(substring, Qt::CaseInsensitive) ||
	 (*iter)->text(icol_expr).contains(substring, Qt::CaseInsensitive)))
      {
	(*iter)->setHidden(false);
      }
    else {
      (*iter)->setHidden(true);
    }
    ++iter;
  }
  if (substring.isEmpty()) {
    expr_lvitem* item = selected_item();
    if (item)
      lv_expr->scrollToItem(item);
  }
}

/*
 direction: 1=>up, 2=>top, 3=>down, 4=>bottom
*/
void
filter_edit::move_filter_item(int direction)
{
  expr_lvitem* item = dynamic_cast<expr_lvitem*>(lv_expr->currentItem());
  if (!item)
    return;

  int index = lv_expr->indexOfTopLevelItem(item);
  int new_index;

  switch(direction) {
  case 1:
    new_index = index-1;
    break;
  case 2:
    new_index=0;
    break;
  case 3:
    new_index = index+1;
    break;
  case 4:
    new_index = lv_expr->topLevelItemCount()-1;
    break;
  default:
    DBG_PRINTF(1, "invalid direction");
    return;
  }

  if (new_index<0 || index == new_index || new_index>=lv_expr->topLevelItemCount())
    return;

  /* We take out the item at 'index' and reinsert it above
     'new_index'.  The code below should work with any possible
     relationship between 'index' and 'new_index', even though the
     current UI limits 'new_index' to be near 'index' or at the top or
     bottom of the list. */

  lv_expr->setCurrentItem(NULL);

  item = dynamic_cast<expr_lvitem*>(lv_expr->takeTopLevelItem(index));
  /* reinsert item between item0 and item1. If item0 is nul, the
     insert is at the top of the list. If item1 is nul, it's at the
     bottom of the list. */
  expr_lvitem* item0 = (new_index-1>=0) ? dynamic_cast<expr_lvitem*>(lv_expr->topLevelItem(new_index-1)) : NULL;
  expr_lvitem* item1 = (new_index < lv_expr->topLevelItemCount()) ? dynamic_cast<expr_lvitem*>(lv_expr->topLevelItem(new_index)) : NULL;

  if (item1 && item0) {
    // moved between two existing elements
    item->m_expr->m_apply_order = item0->m_expr->m_apply_order + ( item1->m_expr->m_apply_order - item0->m_expr->m_apply_order) / 2;
  }
  else if (!item0 && item1) {
    // moved at the top of the list
    item->m_expr->m_apply_order = item1->m_expr->m_apply_order/2;
  }
  else if (item0 && !item1) {
    // moved at the end of the list
    item->m_expr->m_apply_order = item0->m_expr->m_apply_order+1;
  }

  item->m_expr->m_dirty = true;

  lv_expr->setSortingEnabled(false);
  lv_expr->insertTopLevelItem(new_index, item);
  renumber_items();
  lv_expr->setSortingEnabled(true);
  lv_expr->setCurrentItem(item);
}

void
filter_edit::move_filter_top()
{
  move_filter_item(2);
}

void
filter_edit::move_filter_up()
{
  move_filter_item(1);
}

void
filter_edit::move_filter_bottom()
{
  move_filter_item(4);
}

void
filter_edit::move_filter_down()
{
  move_filter_item(3);
}

void
filter_edit::direction_changed(int id)
{
  Q_UNUSED(id);
  if (m_current_expr) {
    m_current_expr->m_dirty=true;
    dlg_fields_to_filter_expr(m_current_expr);
  }
}

void
filter_edit::show_eval_message()
{
  validate_expression();
}

void
filter_edit::zoom_on_expr()
{
  if (!m_current_expr)
    return;
  filter_expr_text_editor * w = new filter_expr_text_editor(this);
  QString initial_txt = ql_expr_full->text();
  w->set_text(initial_txt);
  int ret=w->exec();
  QString ntxt = w->get_text();
  if (ret && ntxt != initial_txt) {
    ql_expr_full->setText(ntxt);
    current_expr_edited(ntxt);
  }
  w->close();
}

#if 0
void
filter_edit::set_sel_list(const std::list<unsigned int>& l)
{
  m_sel_list = l;
  m_suggest_btn->setEnabled(true);
}
#endif

void
filter_edit::current_name_edited(const QString& new_name)
{
  QList<QTreeWidgetItem*> sel = lv_expr->selectedItems();
  if (sel.size()==1) {
    expr_lvitem* item = static_cast<expr_lvitem*>(sel.at(0));
    item->setText(icol_name, new_name);
  }
  if (m_current_expr) {
    m_current_expr->m_expr_name = new_name;
    m_current_expr->m_dirty = true;
  }
}

void
filter_edit::current_expr_edited(const QString& new_expr)
{
  QList<QTreeWidgetItem*> sel = lv_expr->selectedItems();
  if (sel.size()==1) {
    expr_lvitem* item = static_cast<expr_lvitem*>(sel.at(0));
    item->set_expression_text(new_expr);
  }
  if (m_current_expr) {
    m_current_expr->m_expr_text = new_expr;
    m_current_expr->m_dirty=true;
    ql_expr_full->show_button(0);
    
    // Cancel the timer if it's already running.
    if (m_eval_timer)
      delete m_eval_timer;
    m_eval_timer = new QTimer(this);
    m_eval_timer->setSingleShot(true);
    m_eval_timer->setInterval(500);
    connect(m_eval_timer, SIGNAL(timeout()),
	    this, SLOT(display_expression_validity()));
    m_eval_timer->start();
  }
}

/*
  Disable entirely the actions panel.
  To be called when no expression is selected.
*/
void
filter_edit::untie_actions()
{
  m_lrules->setEnabled(false);
  m_lrules->setText(tr("Actions:"));

  // uncheck all and disable each action widget
  reset_actions();

  // disable the actions list
  lv_actions->setEnabled(false);

#if 0 // REMOVE
  // also disable the radio buttons
  for (int i=0; i<action_line::idx_max; i++) {
    w_actions[i]->m_rb->setEnabled(false);
  }
#endif
}

void
filter_edit::closeEvent(QCloseEvent* event)
{
  if (m_confirm_close && m_expr_list.needs_save()) {
    QString msg = tr("There are unsaved changes.\nSave now?");
    int rep = QMessageBox::question(this, tr("Confirmation"), msg, QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);

    if (rep==QMessageBox::Cancel) {
      m_confirm_close=true;
      event->ignore();
      return;
    }
    if (rep==QMessageBox::Save) {
      event->ignore();
      QTimer::singleShot(0, this, SLOT(ok()));
    }
    else {
      m_confirm_close=false;
      event->accept();
    }
  }
  else
    event->accept();
}

void
filter_edit::ok()
{
  ql_expr_full->validate();
  ql_expr_name->validate();
  //  accept();
  if (m_expr_list.update_db()) {
    m_confirm_close=false;
    close();
  }
}

void
filter_edit::cancel()
{
  //  accept();
  m_confirm_close=false;
  close();
}

bool
filter_edit::load()
{
  expr_list* l = &m_expr_list;
  if (!l->fetch()) return false;
  std::list<filter_expr>::iterator it = l->begin();
  int number=1;
  for (; it != l->end(); ++it) {
    expr_lvitem* item = new expr_lvitem(lv_expr);
    item->setText(icol_name, (*it).m_expr_name);
    item->setText(icol_number, QString::number(number++));
    item->set_expression_text((*it).m_expr_text);
    item->setText(icol_last_hit, (*it).m_last_hit.OutputHM(m_date_format));
    item->m_expr = &(*it);
    item->m_db=true;
  }
  return true;
}

void
filter_edit::renumber_items()
{
  QTreeWidgetItemIterator iter(lv_expr);
  int num=1;
  while (*iter) {
    (*iter)->setText(icol_number, QString::number(num++));
    ++iter;
  }
}

void
filter_edit::clear_expr()
{
  ql_expr_name->setText(QString());
  ql_expr_full->setText(QString());
  m_current_expr=NULL;
  lv_expr->clearSelection();
  untie_actions();
}

#if 0
void
filter_edit::suggest_filter()
{
  if (!m_hd) {
    m_hd = new headers_groupview();
    m_hd->set_threshold(100);
    m_hd->init(m_sel_list);
    connect(m_hd->m_trview, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	    this, SLOT(expr_from_header(QTreeWidgetItem*)));
    connect(m_hd, SIGNAL(close()), this, SLOT(close_headers_groupview()));
  }
  m_hd->show();
}
#endif

void
filter_edit::expr_from_header(QTreeWidgetItem* item)
{
  if (!item) return;
  const QString h = item->text(1);
  int sep_pos = h.indexOf(":");
  if (sep_pos>0) {
    QString header_name = h.left(sep_pos);
    QString header_value = h.mid(sep_pos+1);
    header_value = header_value.trimmed();
    new_expr();
    QString expr = "header(\""+ header_name + "\") eq \"" + header_value + "\"";
    ql_expr_full->setText(expr);
  }
}

void
filter_edit::close_headers_groupview()
{
}

void
filter_edit::test_expr()
{
  if (!m_current_expr)
    return;
  m_test_msgs_filter = new msgs_filter();
  msgs_filter* f = m_test_msgs_filter;

  /* Messages to test are fetched by batches of 1000, from newer to
     older */
  f->m_max_results = 1000;
  f->set_date_order(-1);
  f->m_include_trash = true;

  m_fthread = new fetch_thread();
  m_nb_filter_test_match = 0;
  m_nb_filter_tested_msgs = 0;
  int r = f->asynchronous_fetch(m_fthread);

  if (r==1) {
    if (m_test_window_results==NULL) {
      m_test_window_results = new filter_results_window;
      m_test_window_results->setAttribute(Qt::WA_DeleteOnClose);
      connect(m_test_window_results, SIGNAL(destroyed()), this, SLOT(close_results_window()));
      connect(m_test_window_results, SIGNAL(stop_run()), this, SLOT(end_test_requested()));
      m_test_window_results->show();
    }
    else {
      m_test_window_results->clear();
    }
    QString win_title;
    if (m_current_expr->m_expr_name.isEmpty())
      win_title = tr("Filter matches");
    else
      win_title = tr("Filter matches for '%1'").arg(m_current_expr->m_expr_name);
    m_test_window_results->setWindowTitle(win_title);
    m_test_window_results->show_filter_expression(m_current_expr->m_expr_text);

    m_test_window_results->show_progressbar();
    m_filter_run_stopped = false;
    m_ftimer = new QTimer(this);
    m_tested_expr = m_current_expr->m_expr_text;
    connect(m_ftimer, SIGNAL(timeout()), this, SLOT(timer_done()));
    m_waiting_for_results = true;
    m_ftimer->start(100);		// check results every 1/10s
  }
}

void
filter_edit::timer_done()
{
  if (m_waiting_for_results && m_fthread->isFinished()) {
    m_waiting_for_results=false;

    m_test_msgs_filter->postprocess_fetch(*m_fthread);

    bool finished=false;
    if (m_test_window_results != NULL && !m_filter_run_stopped) {
      std::list<mail_result>::iterator it;
      for (it=m_fthread->m_results->begin(); it!=m_fthread->m_results->end(); ++it) {
	filter_evaluator filter_eval;
	m_nb_filter_tested_msgs++;
	filter_eval_result res = filter_eval.evaluate(m_tested_expr, m_expr_list, it->m_id);
	if (res.result) {
	  m_nb_filter_test_match++;
	  if (m_test_window_results && !m_filter_run_stopped)
	    m_test_window_results->incorporate_message(*it);
	  else
	    break;
	}
	else {
	  if (!res.errstr.isEmpty()) {
	    // Stop the test on any evaluation error
	    m_ftimer->stop();
	    m_test_window_results->hide_progressbar();
	    QMessageBox::critical(this, tr("Filter error"), tr("Error near character %1:\n%2").arg(res.evp+1).arg(res.errstr));
	    if (m_test_window_results->nb_results()==0)
	      delete m_test_window_results;
	    finished = true;
	    break;
	  }
	}
	QApplication::processEvents();
      }
    }

    if (!m_test_msgs_filter->has_more_results() || !m_test_window_results || m_filter_run_stopped || finished) {
      finished=true;
      if (m_test_window_results) {
	m_test_window_results->hide_progressbar();
	m_test_window_results->show_status_message(tr("%1 match(es) found in %2 message(s). Filter test completed.").arg(m_nb_filter_test_match).arg(m_nb_filter_tested_msgs));
      }
    }
    else {
      m_test_window_results->show_status_message(tr("%1 match(es) found. Testing more...").arg(m_nb_filter_test_match));
      m_waiting_for_results = true;
      int r = m_test_msgs_filter->asynchronous_fetch(m_fthread, +1);
      if (r!=1) // error
	finished=true;
    }
    if (finished) {
      delete m_ftimer;
      m_fthread->release();
      delete m_fthread;
      delete m_test_msgs_filter;
    }
  }
}

void
filter_edit::close_results_window()
{
  if (m_test_window_results) {
    m_test_window_results = NULL;
  }
}

void
filter_edit::end_test_requested()
{
  m_filter_run_stopped=true;
  if (m_waiting_for_results) {
    m_waiting_for_results = false;
    m_fthread->cancel();
    m_fthread->release();
  }
}

void
filter_edit::delete_expr()
{
  if (!m_current_expr) {
    return;
  }
  int r=QMessageBox::warning(this, tr("Please confirm"), tr("Delete filter?"), tr("OK"), tr("Cancel"), QString());
  if (r==0) {
    expr_lvitem* item=dynamic_cast<expr_lvitem*>(lv_expr->currentItem());
    if (!item) {
      DBG_PRINTF(1, "Couldn't find selected item");
      return;
    }
    item->m_expr->m_delete=true;
    delete item;
    clear_expr();
    enable_up_down_buttons(false);
    renumber_items();
  }
}

void
filter_edit::enable_up_down_buttons(bool b)
{
  m_btn_up->setEnabled(b);
  m_btn_down->setEnabled(b);
  m_btn_bottom->setEnabled(b);
  m_btn_top->setEnabled(b);
}

void
filter_edit::selection_changed()
{
  if (lv_expr->selectedItems().empty()) {
    // selection cleared
    clear_expr();
    enable_up_down_buttons(false);
    expr_btn_delete->setEnabled(false);
    enable_expr_edit(false);
  }
  else {
    // single selection
    expr_lvitem*  item = static_cast<expr_lvitem*>(lv_expr->selectedItems().at(0));
    enable_up_down_buttons(lv_expr->header()->sortIndicatorSection()==0 &&
			   lv_expr->header()->sortIndicatorOrder()==Qt::AscendingOrder);

    filter_expr* e = item->m_expr;
    m_current_expr = e;
    expr_btn_delete->setEnabled(item->m_id!=0);
    enable_expr_edit(true);
    filter_expr_to_dlg(e);
    reset_actions();
    display_actions();
    if (m_eval_timer) {
      delete m_eval_timer;
      m_eval_timer=NULL;
    }
    display_expression_validity();
  }
}

void
filter_edit::display_expression_validity()
{
  if (m_current_expr) {
    filter_evaluator filter_eval;
    filter_eval_result res = filter_eval.evaluate(m_current_expr->m_expr_text,
						  m_expr_list, 0);
    if (!res.errstr.isEmpty()) {
      ql_expr_full->show_button(1);
    }
    else {
      ql_expr_full->show_button(2);
    }
  }
}

/*
  update the 'Actions' panel with the actions connected to the
  currently selected expression
*/
void
filter_edit::display_actions()
{
  m_lrules->setEnabled(true);
  lv_actions->clear();
  lv_actions->setEnabled(true);
  m_current_action=NULL;
  filter_expr* e = m_current_expr;
  if (!e) {
    DBG_PRINTF(1, "ERR: no current expression");
    return;
  }

  if (e->m_expr_name.isEmpty())
    m_lrules->setText(tr("Actions connected to current condition"));
  else
    m_lrules->setText(tr("Actions connected to condition '%1':").arg(e->m_expr_name));

  QList<filter_action>::iterator iter;
  action_lvitem* after=NULL;
  for (iter=e->m_actions.begin(); iter!=e->m_actions.end(); ++iter) {
    after = new action_lvitem(lv_actions, after);
    CHECK_PTR(after);
    after->m_act_ptr = &(*iter);
    after->setText(0, iter->ui_text());
  }
  create_null_action();
  if (lv_actions->topLevelItem(0)) {
    // pre-select the first element
    lv_actions->setCurrentItem(lv_actions->topLevelItem(0));
    m_current_action=static_cast<action_lvitem*>(lv_actions->topLevelItem(0));
  }
}

// Add the (New action) entry
void
filter_edit::create_null_action()
{
#if 0
  int count=lv_actions->topLevelItemCount();
  action_lvitem* last=NULL;
  if (count>0)
    last = static_cast<action_lvitem*>(lv_actions->topLevelItem(count-1));
  action_lvitem* item = new action_lvitem(lv_actions, last);
  CHECK_PTR(item);
  item->setText(0, tr("(New action)"));
  item->m_act_ptr = NULL;	// no real associated action
#endif
}

/*
  Reset the 'Actions' panel
*/
void
filter_edit::reset_actions()
{
  lv_actions->clear();
}


void
filter_edit::enable_expr_edit(bool enable)
{
  m_expr_container->setEnabled(enable);
  if (!enable) {
    ql_expr_full->show_button(0);
  }
}

void
filter_edit::validate_expression()
{
  if (!m_current_expr)
    return;
  filter_evaluator filter_eval;
  filter_eval_result res = filter_eval.evaluate(m_current_expr->m_expr_text,
						m_expr_list, 0);
  if (!res.errstr.isEmpty()) {
    QMessageBox::critical(this, tr("Filter error"), tr("Error at character %1:\n%2").arg(res.evp+1).arg(res.errstr));
  }
  else {
    QMessageBox::information(this, tr("Filter validity"), tr("The expression syntax is correct."));
  }
}

void
filter_edit::new_expr()
{
  clear_expr();
  enable_expr_edit(true);

  expr_lvitem* item = new expr_lvitem();
  int number = 1+lv_expr->topLevelItemCount();
  item->setText(icol_number, QString::number(number));
  lv_expr->addTopLevelItem(item);

  filter_expr ne;
  ne.m_new=true;
  ne.m_dirty=true;
  ne.m_expr_id=0;
  ne.m_apply_order = 1 + m_expr_list.max_apply_order();
  m_expr_list.push_back(ne);
  item->m_expr = &(m_expr_list.back());
  m_current_expr = item->m_expr;

  filter_expr_to_dlg(item->m_expr);
  lv_expr->setCurrentItem(item);
  ql_expr_name->setFocus();
}

bool
filter_edit::expr_fields_filled() const
{
  return !ql_expr_full->text().isEmpty();
}

void
filter_edit::dlg_fields_to_filter_expr(filter_expr *e)
{
  e->m_expr_text = ql_expr_full->text();
  e->m_expr_name = ql_expr_name->text();

  const char dir[]={'I','O','B','N'};
  int dir_idx=m_dir->selected_id();
  if (dir_idx>=0 && dir_idx<4)
    e->m_direction = dir[m_dir->selected_id()];
  else
    e->m_direction = 'N';
}

void
filter_edit::filter_expr_to_dlg(filter_expr* e)
{
  ql_expr_name->setText(e->m_expr_name);
  ql_expr_full->setText(e->m_expr_text);
  ql_expr_full->setCursorPosition(0);
  switch(e->m_direction) {
  case 'I':
    m_dir->setButton(0);
    break;
  case 'O':
    m_dir->setButton(1);
    break;
  case 'B':
    m_dir->setButton(2);
    break;
  case 'N':
    m_dir->setButton(3);
    break;
  }
}

void
filter_edit::expr_update()
{
  bool select_item=false;
  // change the text in the expressions listview

  QList<QTreeWidgetItem*> list_selected = lv_expr->selectedItems();
  expr_lvitem* item = list_selected.count()==1 ? 
    static_cast<expr_lvitem*>(list_selected.first()) : NULL;

  if (!item) {
    item = new expr_lvitem(lv_expr);
    CHECK_PTR(item);
    int number = 1+lv_expr->topLevelItemCount();
    item->setText(icol_number, QString::number(number));
    item->setText(icol_name, ql_expr_name->text());
    select_item=true;
    filter_expr new_expr;
    m_expr_list.push_back(new_expr);
    item->m_expr = &(m_expr_list.back());
    item->m_expr->m_new=true;
    item->m_expr->m_expr_id=0;
    m_current_expr = item->m_expr;
  }
  item->setText(icol_name, ql_expr_name->text());
  item->set_expression_text(ql_expr_full->text());

  filter_expr* e = item->m_expr; // should be m_current_expr
  e->m_expr_name = ql_expr_name->text();
  e->m_expr_text = ql_expr_full->text();
  dlg_fields_to_filter_expr(e);
  e->m_dirty=true;
  DBG_PRINTF(6, "expr_id %d is marked dirty", e->m_expr_id);
  if (select_item)
    lv_expr->setCurrentItem(item);
}

void
filter_edit::disable_all_expr()
{
  enable_expr_edit(false);
  expr_btn_delete->setEnabled(false);
}

void
filter_edit::actions_selection_changed()
{
  QList<QTreeWidgetItem*> sel = lv_actions->selectedItems();
  bool has_sel = !sel.isEmpty();
  m_btn_edit_action->setEnabled(has_sel);
  m_btn_remove_action->setEnabled(has_sel);
  m_btn_up_action->setEnabled(has_sel);
  m_btn_down_action->setEnabled(has_sel);
}

/*
  Delete the current action
*/
void
filter_edit::remove_action()
{
  action_lvitem* a = lv_actions->selected_action();
  if (!a || !a->m_act_ptr)
    return;

  filter_expr* e=m_current_expr;
  QList<filter_action>::iterator iter;
  for (iter=e->m_actions.begin(); iter!=e->m_actions.end(); ++iter) {
    if (a->m_act_ptr==&(*iter)) {
      e->m_actions.erase(iter);
      e->m_dirty=true;
      break;
    }
  }
  delete a;
}

void
filter_edit::add_action()
{
  if (!m_current_expr)
    return;

  filter_action_editor editor(this);
  editor.setWindowTitle(tr("Add a filter action"));
  int res=editor.exec();
  if (res==QDialog::Accepted) {
    filter_action a = editor.get_action();
    action_lvitem* p = new action_lvitem;
    lv_actions->addTopLevelItem(p);
    m_current_expr->m_actions.push_back(a);
    p->m_act_ptr=&(m_current_expr->m_actions.back());
    p->setText(0, p->m_act_ptr->ui_text());
    m_current_expr->m_dirty=true;
  }
}


// called on double click on lv_actions
void
filter_edit::modify_action(QTreeWidgetItem* item, int column)
{
  Q_UNUSED(item);
  Q_UNUSED(column);
  edit_action();
}

void
filter_edit::edit_action()
{
  action_lvitem* lva = lv_actions->selected_action();
  if (!lva)
    return;
  filter_action_editor editor(this);
  editor.setWindowTitle(tr("Edit a filter action"));
  editor.display_action(lva->m_act_ptr);
  int res=editor.exec();
  if (res==QDialog::Accepted) {
    filter_action a = editor.get_action();
    *(lva->m_act_ptr) = a;
    lva->setText(0, a.ui_text());
    m_current_expr->m_dirty=true;
  }

}

void
filter_edit::help()
{
  helper::show_help("filters");
}

void
filter_edit::move_action_up()
{
  if (!m_current_expr)
    return;
  if (lv_expr->header()->sortIndicatorSection()!=0) {
    QMessageBox::critical(this, tr("Error"), tr("Please sort by filter order (first column) to enable moving filters"));
    return;
  }
  lv_actions->move_filter_action(-1, m_current_expr);
}

void
filter_edit::move_action_down()
{
  if (!m_current_expr)
    return;
  lv_actions->move_filter_action(1, m_current_expr);
}

//
// focus_line_edit
//
focus_line_edit::focus_line_edit(QWidget* parent, const QString& name, 
				 filter_edit* form) :
  QLineEdit(parent)
{
  m_form=form;
  m_name=name;
  connect(this, SIGNAL(returnPressed()), this, SLOT(validate()));
}

void
focus_line_edit::validate()
{
  if (text()!=m_last_value && m_form->expr_fields_filled()) {
    m_form->expr_update();
    m_last_value=text();
  }
}

void
focus_line_edit::setText(const QString& t)
{
  m_last_value=t;
  QLineEdit::setText(t);
  setCursorPosition(0);
}

void
focus_line_edit::focusOutEvent(QFocusEvent * e)
{
  validate();
  QLineEdit::focusOutEvent(e);
}

action_lvitem*
action_listview::selected_action()
{
  return selectedItems().isEmpty() ? NULL:
    static_cast<action_lvitem*>(selectedItems().at(0));  
}

void
action_listview::keyPressEvent(QKeyEvent* e)
{
  if (e->modifiers()!=0 || e->key()!=Qt::Key_Delete) {
    QTreeWidget::keyPressEvent(e);
  }
  else
    emit key_del();
}

void
action_listview::move_filter_action(int direction, filter_expr* filter)
{
  action_lvitem* item = selected_action();
  if (!item)
    return;

  int index = indexOfTopLevelItem(item);
  int new_index;
  switch(direction) {
  case -1:
    new_index = index-1;
    break;
  case 1:
    new_index = index+1;
    break;
  default:
    return; // shouldn't happen
  }

  if (new_index<0 || index == new_index || new_index>=topLevelItemCount())
    return;

  /* We take out the item at 'index' and reinsert it above
     'new_index'.  The code below should work with any possible
     relationship between 'index' and 'new_index', even though the
     current UI limits 'new_index' to be near 'index' or at the top or
     bottom of the list. */

  item = dynamic_cast<action_lvitem*>(takeTopLevelItem(index));
  insertTopLevelItem(new_index, item);
  setCurrentItem(item);
  filter_action action = filter->m_actions.takeAt(index);
  filter->m_actions.insert(new_index, action);
  filter->m_dirty=true;
}

expr_line_edit::expr_line_edit(QWidget* parent) : QLineEdit(parent)
{
  m_button = new QToolButton(this);
  QPixmap pixmap = FT_MAKE_ICON(ICON16_IMPORTANT);
  m_button->setIcon(QIcon(pixmap));
  m_button->setIconSize(pixmap.size());
  m_button->setCursor(Qt::ArrowCursor);
  m_button->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  m_button->setEnabled(false);
  connect(m_button, SIGNAL(clicked()), this, SLOT(button_clicked()));
  connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(update_button(const QString&)));
  
}

void
expr_line_edit::resizeEvent(QResizeEvent *)
{
  QSize sz = m_button->sizeHint();
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  m_button->move(rect().right() - frameWidth - sz.width(),
		 (rect().bottom() + 2 - sz.height())/2);
}

void
expr_line_edit::update_button(const QString& text)
{
  m_button->setEnabled(!text.isEmpty());
}

void
expr_line_edit::validate()
{
}

/*
  show= 0:not visible, 1:warning icon, 2:ok icon
*/
void
expr_line_edit::show_button(int show)
{
  if (show) {
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(m_button->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), m_button->sizeHint().width() + frameWidth * 2 + 2),
		 qMax(msz.height(), m_button->sizeHint().height() + frameWidth * 2 + 2));
    QPixmap pixmap;
    if (show==1) {
      pixmap = FT_MAKE_ICON(ICON16_IMPORTANT);
      m_button->setToolTip(tr("Error while evaluating expression. Click to read the error message."));
    }
    else {
      pixmap = FT_MAKE_ICON(ICON16_DIALOG_OK);
      m_button->setToolTip(tr("No error found in expression."));
    }
    m_button->setIcon(QIcon(pixmap));
    m_button->setIconSize(pixmap.size());
    m_button->show();
  }
  else {
    m_button->setToolTip("");
    m_button->hide();
  }
}

void
expr_line_edit::button_clicked()
{
  emit toolbutton_clicked();
}

