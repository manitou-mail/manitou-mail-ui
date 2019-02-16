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

#ifndef INC_HTML_EDITOR_H
#define INC_HTML_EDITOR_H

#include <QWebView>
#include <QWebPage>
#include <QDialog>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QUrl>

class QToolBar;
class QLineEdit;
class QToolButton;
class QComboBox;
class QDropEvent;
class QDragMoveEvent;
class QDragEnterEvent;

class html_editor : public QWebView
{
  Q_OBJECT
public:
  html_editor(QWidget* parent=NULL);
  virtual ~html_editor();
  QList<QToolBar*> create_toolbars();
  bool eval_jscript(const QString);
  QString html_text() const;
  void set_html_text(const QString&);
  void append_paragraph(const QString& fragment);
  void prepend_paragraph(const QString& fragment);
  void finish_load();
  // Return a list of paths of local files referenced by the HTML doc
  QStringList collect_local_references();
  void replace_local_references(const QMap<QString,QString>&);
  QString to_plain_text() const;
  void enable_html_controls(bool);
protected:
  void dropEvent(QDropEvent*);
  void dragMoveEvent(QDragMoveEvent*);
  void dragEnterEvent(QDragEnterEvent*);
public slots:
  // HTML operations
  void insert_link();
  void insert_image();
  void bold();
  void italic();
  void strikethrough();
  void foreground_color();
  void background_color();
  void underline();
  void superscript();
  void subscript();
  void remove_format();
  void indent_left();
  void indent_right();
  void insert_hr();
  void insert_unordered_list();
  void insert_ordered_list();
  void change_font(const QString&);
  void change_font_size(int);
  void format_header(int);
  void undo();
  void redo();
  void align_left();
  void align_right();
  void center();
  void justify();
  // run actions from the menubar
  void run_edit_action(const char*);

private slots:
  void load_finished(bool);
  void link_clicked(const QUrl&);

private:
  // Actions
  struct action_type {
    const char* name;
    const char* shortcut;
    const char* icon;
    const char* slot;
    QWebPage::WebAction webaction;
  };
  static struct action_type m_action_definitions[];
  QMap<const char*, QAction*> m_actions; // name=>action
  void create_actions();

  // Javascript
  void insert_html(const QString);
  void exec_command(const QString, const QString=QString());
  bool m_load_finished;
  static const char* manitou_files_jscript;

  // Controls
  QComboBox* m_font_chooser;
  QComboBox* m_para_format_chooser;
  QComboBox* m_headers_chooser;

signals:
  void attach_file_request(const QUrl);  
};

class link_editor: public QDialog
{
  Q_OBJECT
public:
  link_editor(QWidget* parent);
  ~link_editor();
  QString url();
  QString text();
private slots:
  void preview();
private:
  QLineEdit* m_link;
  QLineEdit* m_text;
};
#endif // INC_HTML_EDITOR_H
