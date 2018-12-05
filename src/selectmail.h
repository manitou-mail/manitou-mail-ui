/* Copyright (C) 2004-2018 Daniel Verite

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

#ifndef INC_SELECTMAIL_H
#define INC_SELECTMAIL_H

#include <QDialog>
#include <QKeyEvent>
#include <QStringList>
#include <QDateTime>
#include <QTime>
#include <QMap>

#include <vector>
#include "db.h"
#include "words.h"
#include "sqlquery.h"
#include "mail_listview.h"
#include "edit_address_widget.h"
#include "filter_rules.h"
#include "fetch_thread.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;
class QTimer;
class QToolButton;
class tag_selector;

class tag_line_edit_selector;
class button_group;

// Options from the searchbox
class fts_options
{
public:
  void clear();
  //  QString m_word;
  QStringList m_words;		// full-text search: words to find
  QStringList m_exclude_words;  // full-text search: words to exclude
  QStringList m_substrs;	// full-text search: substrings to find
  QMap<QString,QString> m_operators;
};

/*
  Set of options that complement a fetch filter to indicate
  what to with the results.
*/
class fetch_command
{
public:
  // destination of the results
  enum {
    new_page,
    new_window,
    extend_current_page,
    replace_current_page
  } destination;
};

/*
  Identifies a segment with (date,mail_id) pairs marking the start and
  end of one segment of results for a filter (msgs_filter).  Segments
  are fetched one at a time (as opposed to all results that match the
  filter) to reduce memory consumption, execution time and latency.
  The SQL query uses segments through clauses constraining
  (msg_date,mail_id) withing bounds, and using LIMIT.
*/
class result_segment
{
public:
  /* The bounds are (date_{min,max}, mailid_{min,max}) as a pair.
     mail_id is a tie-breaker for the date */
  QString date_min;		// format: YYYYMMDDHHMISS, with HH being 24-hour
  mail_id_t mail_id_min;
  QString date_max;		// same format as date_min
  mail_id_t mail_id_max;

  /* First segment of the resultset. Either a first fetch for a query
     or a subsequent backward fetch that retrieved less results than
     the limit */
  bool is_first = true;

  /* We know whether there are still results after a segment by asking
     for N+1 results instead of N and getting N+1 results back. */
  bool is_last;

  // whether the other fields are initialized and valid
  bool is_valid = false;

  void print();
};

class msgs_filter_user_param
{
public:
  QString m_name;
  QString m_title;
  QString m_value;
};

class msgs_filter
{
public:
  msgs_filter();
  virtual ~msgs_filter();
  const QString user_query() const {
    return m_user_query;
  }
  void set_user_query(const QString q) {
    m_user_query=q;
  }
  void add_result(mail_result&, mail_listview*);
  int exec_time() const;	// in milliseconds
  typedef std::list<mail_msg*> mlist_t;
  mlist_t m_list_msgs;
  // fetch the selection into a mail list widget
  int fetch(mail_listview*, int direction=0);
  int asynchronous_fetch (fetch_thread* t, int direction=0);
  void make_list(mail_listview*);
  void set_auto_refresh(bool s=true) {
    m_auto_refresh=s;
  }
  bool auto_refresh() const {
    return m_auto_refresh;
  }
  int parse_search_string(QString s, fts_options&);

  // to do some pre-processing before the fetch
  void preprocess_fetch(fetch_thread&);

  // to do some post-processing after the fetch
  void postprocess_fetch(fetch_thread&);

  // returns a valid query that will always fetch an empty list of messages
  static sql_query empty_list_query();
  static const char* select_list_mail_columns;

  // return the list of parameters refered to in the user query
  QList<msgs_filter_user_param>& user_query_parameters() {
    return m_user_query_params;
  }

  //private:

  enum recipient_type {
    rFrom=0,
    rTo,
    rCc,
    rAny
  };
  // search criteria
  int m_nAddrType;
  QString m_sAddress;
  QString m_subject;
  QString m_body_substring;
  QString m_addr_to;
  QString m_sql_stmt;
  QString m_tag_name;
  QDate m_date_min;
  QDate m_date_max;
  uint m_thread_id;

  int m_status;			// exact value wanted in mail.status
  int m_status_set;		// bitmask: OR'ed bits that have to be 1 in mail.status
  int m_status_unset;		// bitmask: OR'ed bits that have to be 0 in mail.status

  int m_newer_than;		/* newer than or N days old */

  /* String associated to the combobox in the query dialog
     (see m_date_cb initialization).
     Can be "range" or a negative number of days as a string,
     or "today", "yesterday" ... */
  QString m_date_clause;

  /* Date selectors for the search bar */
  QString m_date_exact_clause;
  QString m_date_before_clause;
  QString m_date_after_clause;

  /* Ordering of msg_date (+1=ASC, -1=DESC) column for the fetch.
     Instantiated from the configuration ("messages_order") when the
     filter is initialized, and not changed afterwards. */
  int m_order;

  int m_min_prio;
  uint m_tag_id;

  fts_options m_fts;

  bool m_in_trash;
  bool m_include_trash;

  /* Most queries can't have a progress bar because the database does
     not report progress. There's an exception for partitioned
     full-text search, where the database may emit a notification per
     partition searched, which gets shown if the config option
     display/wordsearch/progress_bar is set. */
  bool m_has_progress_bar;

  filter_expr m_filter_expression;
  bool m_has_filter_expr;
  expr_list m_filter_expr_list;

  void set_date_order(int o) {
    m_order=o;
  }

  result_segment& segment() {
    return m_segment;
  }
  bool has_more_results() const {
    return m_has_more_results;
  }

  int m_max_results;		// max results per fetch iteration
  bool m_fetched;

  std::list<mail_result>* m_fetch_results;
  int build_query (sql_query&);
  //  mail_msg* in_list(mail_id_t id);
  static int load_result_list(PGresult* res, std::list<mail_result>* l, int max_nb=-1);

  /* User parameters names and values for m_user_query */
  QList<msgs_filter_user_param> parse_user_query_parameters();

  void set_user_query_parameters(const QList<msgs_filter_user_param>& list) {
    m_user_query_params = list;
  }

  QTime m_start_time;
  int m_exec_time;
  QString m_errmsg;

#if 0
  progressive_wordsearch m_psearch;
#endif

private:
  void init();
  enum date_comparator {
    date_equal,
    date_after,
    date_before
  };
  enum status_comparator {
    status_is,
    status_isnot
  };
  enum address_type {
    from_address,
    to_address,
    cc_address
  };

  /* Add criteria to the sql_query that correspond to a comparison
     with date_expr using the date_comparator (equal or after or
     before) */
  void process_date_clause(sql_query& q,
			   date_comparator comp,
			   QString date_expr);

  /* Add criteria to the sql_query that correspond to a status test,
     m.status having or not having certain status bits. */
  void process_status_clause(sql_query& q,
			     status_comparator comp,
			     QList<QString> vals);

  /* Add criteria to the sql_query that correspond to a sender/recipient test */
  void process_address_clause(sql_query& q,
			      address_type atype,
			      QList<QString> vals);

  /* Add criteria to the sql_query for the tag operator */
  void process_tag_clause(sql_query& q, QList<QString> vals);

  /* Add criteria to the sql_query for the filename operator */
  void process_filename_clause(sql_query& q, QList<QString> vals);

  /* Add criteria to the sql_query for the filename operator */
  void process_filesuffix_clause(sql_query& q, QList<QString> vals);

  /* Add criteria to the sql_query for the msgid/messageid operator */
  void process_msgid_clause(sql_query& q, QList<QString> vals);

  /* Add criteria to the sql_query for the msgid/messageid operator */
  void process_mail_id_clause(sql_query& q, QList<QString> vals);

  bool m_auto_refresh;
  int add_address_selection (sql_query& q, const QString email_addr, int addr_type);
  /* number of criteria that needs to match an address from
     the mail_addresses table. For each one of these we have to use
     a different sql alias for the mail_addresses table */
  unsigned int m_addresses_count;

  /* When using aliased tables, use a counter as a suffix to disambiguate
     different aliases to the same table */
  int m_alias_sequence;

  static const int max_possible_prio;

  /* escape % and _ for LIKE clauses and add % at the start and end */
  QString quote_like_arg(const QString&);

  PGresult* res;

  /* the part of the query that's made visible to the user: it
     includes a select-list that retrieves only the mail_id column,
     and also  shouldn't have ORDER BY or LIMIT clauses. Typically,
     it's aimed at being included in a subquery, i.e:

     SELECT <columns> FROM mail WHERE mail_id IN (m_user_query) ORDER BY
     ... LIMIT ... */
  QString m_user_query;

  QList<msgs_filter_user_param> m_user_query_params;

  /* used to fetch another set of results that are older/newer
     (depending on m_order) */
  result_segment m_segment;

  bool m_has_more_results = false;

  /* To move backward/forward the fetch window within the
     results. (0=first fetch, +1=ASC, -1=DESC) for (msg_date,mail_id) pairs.
     Can be changed between fetches.  */
  int m_direction = 0;

#if 0
  /* keep the last part_no we joined against for IWI search. A
     subsequent "fetch more" will start from this part */
  int m_iwi_last_part_no;
#endif
};

// Array of checkboxes for viewing, editing or selecting
// a combination of all possible status
class select_status_box : public QFrame
{
  Q_OBJECT
public:
  select_status_box (bool either, QWidget* parent=0);
  virtual ~select_status_box() {}
  int mask_yes() const;
  int mask_no() const;
  void set_mask(int maskYes, int maskNo);
  int status() const;
public slots:
  void status_changed(int);
private:
  struct st_status {
    const char* name;
    int value;
  };
  static st_status m_status_tab[];
  int m_mask_set;		/* the bits we want to be 1 */
  int m_mask_unset;		/* the bits we want to be 0 */

  // true: the buttons are "yes, no, either"
  // false: the buttons are "yes", no"
  bool m_either;

  std::vector<QButtonGroup*> m_button_groups;
};

// a modal dialog that embeds a select_status_box with OK and Cancel
// buttons
class status_dialog : public QDialog
{
  Q_OBJECT
public:
  status_dialog(QWidget* parent=0);
  select_status_box* m_statusBox;
};

class msg_select_dialog : public QDialog
{
  Q_OBJECT
public:
  msg_select_dialog(bool open_new=true);
  virtual ~msg_select_dialog() {}

  /* fill in the dialog controls with the values provided by the message
     filter */
  void filter_to_dialog (const msgs_filter* cFilter);

public slots:
  void ok();
  void cancel();
  void help();
private slots:
  void more_status();
  void zoom_on_sql();
  void timer_done();
  void date_cb_changed(int);
  void select_date_range(int);

private:
  void set_date_style();

  QString str_status_mask();
  edit_address_widget* m_wcontact;

  QComboBox* m_wAddrType;
  tag_line_edit_selector* m_qtag_sel;
  edit_address_widget* m_wto;
  QLineEdit* m_wSubject;
  QLineEdit* m_wString;
  QLineEdit* m_wSqlStmt;
  QComboBox* m_date_cb;
  QDateTimeEdit* m_wmin_date;
  QDateTimeEdit* m_wmax_date;
  QCheckBox* m_chk_datemin;
  QCheckBox* m_chk_datemax;
  QLineEdit* m_wStatus;
  QLineEdit* m_wMaxResults;
  button_group* m_sort_order;
  QDialogButtonBox* m_btn_box;
  QPushButton* m_wStatusMoreButton;
  QToolButton* m_zoom_button;
  QCheckBox* m_trash_checkbox;
  int m_status_set_mask;
  int m_status_unset_mask;
  void to_filter(msgs_filter*);
  msgs_filter m_filter;
  void enable_inputs (bool enable);
  void enable_date_range();

  fetch_thread m_thread;
  QTimer* m_timer;
  bool m_waiting_for_results;
  bool m_new_selection;
signals:
  void fetch_done(msgs_filter*);
};



#endif // INC_SELECTMAIL_H
