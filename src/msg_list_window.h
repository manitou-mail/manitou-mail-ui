/* Copyright (C) 2004-2016 Daniel Verite

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

#ifndef INC_MSG_LIST_WINDOW_H
#define INC_MSG_LIST_WINDOW_H

#include "main.h"
#include "app_config.h"
#include "db.h"
#include "tagsbox.h"
#include "searchbox.h"
#include <QMainWindow>

#include <QString>
#include <QStringList>
#include <QPoint>
#include <QPushButton>
#include <QCloseEvent>
#include <QList>
#include <list>
#include <vector>

#include "selectmail.h"
#include "msgs_page_list.h"
#include "body_view.h"
#include "mail_displayer.h"
#include "query_listview.h"

class QSplitter;
class QMenuBar;
class QTimer;
class QMenu;
class QPainter;
class QToolBar;
class QProgressBar;
class QAction;

class query_listview;
class mail_listview;
class attch_listview;
class message_view;
class attch_lvitem;
class attachment;

class newmail_button: public QPushButton
{
  Q_OBJECT
public:
  newmail_button(QString txt, QWidget* parent);
  virtual ~newmail_button();
  void enable(bool enable=true);
  void set_number(int n) {
    m_number=n;
  }
  void update_font(QFont f);
private:
  int m_number;
public slots:
  void trayicon_click();
signals:
  void show_new_mail();
};

class msg_list_window;

class msgs_page
{
public:
  msgs_page() : m_query_lvitem_id(0) {}
  msgs_filter* m_page_filter;
  message_view* m_page_msgview;
  mail_msg* m_page_current_item;
  attch_listview* m_page_attach;
  mail_listview* m_page_qlist;
  QSplitter* m_page_splitter;
  int m_query_lvitem_id;
  msg_list_window* m_msgs_window;

  // Refresh the status of a message in the listview. If code=-1,
  // remove the mail from the list
  void refresh_status(mail_msg* msg, int code);
};


class msg_list_window : public QMainWindow
{
  Q_OBJECT
  display_prefs display_vars;

public:
  msg_list_window (const msgs_filter*, display_prefs* dprefs=0, QWidget *parent = 0);
  virtual ~msg_list_window ();

  void propagate_status(mail_msg*, int code=0);

  void incorporate_message(mail_result&);
  const display_prefs& get_display_prefs();

  void show_tags (bool show);
  void add_msgs_page(const msgs_filter*,bool if_results);

  void remove_msg(mail_msg* i, bool auto_select_next=true);
  msgs_filter* current_filter() {
    return m_filter;
  }
  void set_title(const QString title=QString::null);
  void clear_quick_query_selection();

/*
  int nb_messages() const {
    return m_filter->m_list_msgs.size();
  }
*/

public slots:
  void fill_fetch(msgs_filter*);
  void fill_fetch_new_page(msgs_filter*);
  void display_msg_contents();
  void body_menu();
  void global_refresh_status(mail_id_t mail_id);
  void show_status_message(const QString&);
  void blip_status_message(const QString&);
  void body_edited(uint mail_id, const QString*);
  void show_progress(int progress);
  void timer_func();
  void timer_idle();
  void abort_operation();
  void install_progressbar(QString=QString::null);
  void uninstall_progressbar();

  void msg_zoom_in();
  void msg_zoom_out();
  void msg_zoom_zero();

  // buttons
  void move_forward();
  void move_backward();
  void sender_properties();

  void enable_commands();

  // File menu
  void preferences();
  void open_global_notepad();
  void import_mailbox();
  void export_messages();
  void new_window();
  void close_window();
  void edit_tags();
  void edit_filters();
  void start_mailing();

  // Edit menu
  void edit_copy();

  // Selection menu
  void new_list();
  void new_messages();
  void non_processed_messages();
  void sel_refine();
  void sel_refresh();
  void sel_import();
  void sel_trashcan();
  void sel_sent();
  void sel_save_query();
#if 0
  void sel_header_analysis();
#endif

  void sel_filter(const msgs_filter& f);
  void sel_tag(const QString tagname);
  void sel_tag(unsigned int tag_id);
  void sel_tag_status(unsigned int tag_id,int status_set,int status_unset);
  void fetch_more();

  // Display menu
  void toggle_show_tags(bool);
  void toggle_threaded(bool);
  void toggle_fetch_on_demand(bool);
  void toggle_hide_quoted(bool);
  void toggle_show_filters_log(bool);
  void change_font(QAction*);
  void show_headers(QAction*);
  void toggle_include_tags_in_headers(bool);
  void cycle_headers();
  void save_display_settings();

  void search_finished();
  void mail_selected(mail_msg*);
  void display_selection_tags();
  void mails_selected();
  void mail_reply_sender();
  void mail_reply_all();
  void mail_reply_list();
  void mail_reply(int whom_to);
  void action_click_msg_list(const QModelIndex& index);
  void bounce();
  void attch_selected(QTreeWidgetItem*,int);
  void attch_run(attch_lvitem*);
  int attachment_dest(attachment*);
  void find_text();
  void tag_toggled(int id, bool checked);

  // Message menu
  void new_mail();
  void forward();
  void edit_note();
  void save_body();
  void edit_body();
  //  void save_to_mbox();
  void msg_print();
  void msg_trash();
  void msg_untrash();
  void msg_delete();
  void msg_archive();
  void msg_properties();
  void sel_bottom();
  void goto_next_message();
  void goto_previous_message();

  void view_attachment();
  void save_attachment();
  void select_all_text();

  void search_db();
  void search_text_changed(const QString&);

  void search_generic(const QString& text, int where, int options);
  void change_mail_status(int status,mail_msg*);
  void change_multi_mail_status (int statusMask, std::vector<mail_msg*>*);

  // help
  void about();
  void open_help();
  void dynamic_help();
  
  // put the focus on the messages list
  void focus_on_msglist();

  void sel_refresh_list();
  void sel_auto_refresh_list();
  void raise_refresh();
  void quick_query_selection(QTreeWidgetItem * item, int column);

protected:
  virtual void closeEvent(QCloseEvent*);

private:
  void check_new_mail();

  // reflect state of abort button during a progress-controlled operation
  bool progress_aborted();

  /* enable/disable menus and widgets. use to avoid recurse
     into the gui when an action is performed during
     which Qt event loop may be called */
  void enable_interaction(bool b);

  void show_abort_button();
  void hide_abort_button();

  void store_quick_sel(query_lvitem::item_type type, uint tag_id=0);
  void msg_list_postprocess();
  void remove_selected_msgs(int action);	// 0=trash, 1=delete
  void change_page(msgs_page*);
  bool want_new_window() const;
  void remove_msg_page(std::list<msgs_page*>::iterator it,
		       bool and_after=false);

  // find and remove a page to get a place for a new one
  void free_msgs_page();
  void set_pages_font(int element, const QFont&);
  void set_menu_font(const QFont&);

  void init_fonts();

  QString sprint_headers(mail_msg*);
  void display_msg_note();
  void display_body();

  // menu entries
  enum {
    me_File_Quit=0,
    me_File_New_Window,
    me_File_Close_Window,
    me_File_Preferences,
    me_File_Global_Notepad,
    me_File_Import_Mailbox,
    me_File_Export_Messages,
    me_File_Mailing,
    me_Edit_Cut,
    me_Edit_Copy,
    me_Edit_Paste,
    me_Selection_NewMessages,
    me_Selection_CurrentMessages,
    me_Selection_Trashcan,
    me_Selection_Sent,
    me_Selection_New,
    me_Selection_Refine,
    me_Selection_Refresh,
    me_Selection_Store,
    me_Selection_Retrieve,
    me_Selection_Export,
    me_Selection_Save_Query,
    me_Selection_Header_Analysis,
    me_Message_New,
    me_Message_Reply_All,
    me_Message_Reply_Sender,
    me_Message_Forward,
    me_Message_EditNote,
    me_Message_Archive,
    me_Message_Search,
    me_Message_Properties,
    me_Message_Trash,
    me_Message_unTrash,
    me_Message_Delete,
    me_Message_Body_Save,
    me_Message_Body_Edit,
    me_Message_Save_ToMbox,
    me_Message_Attch_View,
    me_Message_Attch_Save,
    me_Message_Print,
    me_Message_Goto_Next,
    me_Message_Goto_Prev,
    me_Display_Tags,
    me_Display_Threaded,
    me_Display_WrapLines,
    me_Display_FastBrowse,
    me_Display_Font_All,
    me_Display_Font_Menus,
    me_Display_Font_Tags,
    me_Display_Font_QuickSel,
    me_Display_Font_Msglist,
    me_Display_Font_Msgbody,
    me_Display_Headers_None,
    me_Display_Headers_Most,
    me_Display_Headers_All,
    me_Display_Headers_Raw,
    me_Display_Headers_RawDec,
    me_Display_Headers_Tags,
    me_Display_Body_ZoomIn,
    me_Display_Body_ZoomOut,
    me_Display_Body_ZoomZero,
    me_Display_Hide_Quoted,
    me_Display_Show_FiltersTrace,
    me_Display_Save_Settings,
    me_Configuration_EditTags,
    me_Configuration_EditFilters,
    me_Help_About,
    me_Help_Open,
    me_Help_Dynamic,

    me_NumberOfEntries		// keep this at the last position
  };
  QAction* m_menu_actions[me_NumberOfEntries];
  QMenuBar* init_menu();
  QToolBar* m_toolbar;
  newmail_button* m_new_mail_btn;
  QPushButton* m_abort_button;
  bool m_abort;

  void create_actions();
  void make_toolbar();
  void make_search_toolbar();

  QString expand_body_line (const QString& line);

  tags_box_widget* m_tags_box;
  query_listview* m_query_lv;

  fetch_thread m_thread;
  QTimer* m_timer;
  QTimer* m_timer_idle;
  msgs_filter* m_loading_filter;
  int m_timer_ticks;		/* in 1/5 seconds */
  bool m_waiting_for_results;

  // current page's widgets and data
  msgs_filter* m_filter;
  message_view* m_msgview;
  mail_msg* m_pCurrentItem;
  attch_listview* m_qAttch;
  mail_listview* m_qlist;

  // Menus. When adding a new QPopupMenu*, also modify set_menu_font()
  QMenu* m_pMenuFile;
  QMenu* m_pMenuEdit;
  QMenu* m_pMenuSelection;
  QMenu* m_pMenuMessage;
  QMenu* m_pMenuDisplay;
//  QPopupMenu* m_pMenuConfig;
  QMenu* m_pMenuHelp;
  // submenus
  QMenu* m_pPopupHeaders;
  QMenu* m_pPopupFonts;
  QMenu* m_popup_display_body;
  QMenu* m_popup_body;
  QMenu* m_pPopupAttach;

  QString m_lastSubjectSearch;
  std::list<searched_text> m_highlighted_text;
  bool m_highlightedCaseSensitive;
  search_box* m_wSearch;

  bool m_fetch_on_demand;
  bool m_ignore_selection_change;

  QAction* m_action_move_forward;
  QAction* m_action_move_backward;
  QAction* m_action_reply_sender;
  QAction* m_action_reply_all;
  QAction* m_action_reply_list;
  QAction* m_action_msg_archive;
  QAction* m_action_msg_delete;
  QAction* m_action_msg_trash;
  QAction* m_action_msg_untrash;
  QAction* m_action_msg_forward;
  QAction* m_action_msg_print;
  QAction* m_action_find_text;
  QAction* m_action_msg_sender_details;
  QAction* m_action_cycle_headers;
  QAction* m_action_search;
  QAction* m_action_new_mail;
  QAction* m_action_new_selection;
  QAction* m_action_refresh_results;
  QAction* m_action_goto_last_msg;
  QAction* m_action_msgview_select_all;

  void enable_forward_backward();

  search_edit* m_ql_search;
  QProgressBar* m_progress_bar;

public:
  void apply_conf(app_config& conf);
  static void apply_conf_all_windows();

private:
  std::list<mail_result> m_auto_refresh_results;
  QList<bool> m_widgets_enable_state;
  QList<bool> m_actions_enable_state;
  /* Each window has pages that consist of a splitter and its
     descendants, mostly a listview and a webview for the message
     body */
  msgs_page_list* m_pages;
signals:
  void mail_chg_status(int, mail_msg*);
  void mail_multi_chg_status(int, std::vector<mail_msg*>*);
  void progress(int);
  void abort_progress();
};

#endif // INC_MSG_LIST_WINDOW_H
