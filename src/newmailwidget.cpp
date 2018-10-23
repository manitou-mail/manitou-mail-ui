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

#include "newmailwidget.h"
#include "app_config.h"
#include "edit_address_widget.h"
#include "notepad.h"
#include "attachment_listview.h"
#include "main.h"
#include "dragdrop.h"
#include "icons.h"
#include "users.h"
#include "notewidget.h"
#include "tagsbox.h"
#include "sqlstream.h"
#include "html_editor.h"
#include "mail_template.h"


#include <QCloseEvent>
#include <QComboBox>
#include <QCursor>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExp>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>


QString
new_mail_widget::m_last_attch_dir;

void
new_mail_widget::create_actions()
{
  m_action_send_msg = new QAction(FT_MAKE_ICON(FT_ICON16_SEND),
				  tr("Send mail"), this);
  connect(m_action_send_msg, SIGNAL(triggered()), this, SLOT(send_now()));

  m_action_send_later = new QAction(UI_ICON(ICON16_CLOCK),
				  tr("Schedule delivery"), this);
  connect(m_action_send_later, SIGNAL(triggered()), this, SLOT(send_later()));

  m_action_attach_file = new QAction(FT_MAKE_ICON(FT_ICON16_ATTACH),
				     tr("Attach file"), this);
  connect(m_action_attach_file, SIGNAL(triggered()), this, SLOT(attach_files()));

  m_action_insert_file = new QAction(FT_MAKE_ICON(FT_ICON16_OPENFILE),
				     tr("Insert text file"), this);
  connect(m_action_insert_file, SIGNAL(triggered()), this, SLOT(insert_file()));

  m_action_edit_note = new QAction(FT_MAKE_ICON(FT_ICON16_EDIT_NOTE),
				   tr("Edit note"), this);
  connect(m_action_edit_note, SIGNAL(triggered()), this, SLOT(edit_note()));

  m_action_add_header = new QAction(FT_MAKE_ICON(ICON16_HEADER),
				   tr("Add header field"), this);
  connect(m_action_add_header, SIGNAL(triggered()), this, SLOT(add_field_editor()));

  m_action_open_notepad = new QAction(FT_MAKE_ICON(ICON16_NOTEPAD),
				   tr("Open global notepad"), this);
  connect(m_action_open_notepad, SIGNAL(triggered()), this, SLOT(open_global_notepad()));

}

void
new_mail_widget::make_toolbars()
{
  QToolBar* toolbar = addToolBar(tr("Message Operations"));
  toolbar->addAction(m_action_send_msg);
  toolbar->addAction(m_action_attach_file);
  toolbar->addAction(m_action_edit_note);
  toolbar->addAction(m_action_add_header);
  toolbar->addAction(m_action_open_notepad);

  m_toolbar_html1 = m_toolbar_html2 = NULL;

  if (m_html_edit) {
    QList<QToolBar*> toolbars = m_html_edit->create_toolbars();
    for (int i=0; i<toolbars.size(); i++) {
      if (i==0) m_toolbar_html1 = toolbars[i];
      if (i==1) {
	addToolBarBreak();
	m_toolbar_html2 = toolbars[i];
      }
      addToolBar(toolbars[i]);
    }
    // Add our own toggle button to the second HTML toolbar
    if (m_toolbar_html2 != NULL) {
      QToolButton* source = new QToolButton();
      source->setIcon(HTML_ICON("stock_view-html-source.png"));
      source->setToolTip(tr("Toggle between HTML source edit and visual edit"));
      source->setCheckable(true);
      connect(source, SIGNAL(toggled(bool)), this, SLOT(toggle_edit_source(bool)));
      m_toolbar_html2->addWidget(source);
    }
  }
}

void
new_mail_widget::toggle_edit_source(bool checked)
{
  if (!m_html_edit) {
    DBG_PRINTF(1, "ERR: no html editor is instantiated");
    return;
  }
  if (checked) {
    m_html_source_editor = new html_source_editor();
    m_html_source_editor->setPlainText(m_html_edit->html_text());
    m_edit_stack->addWidget(m_html_source_editor);
    m_edit_stack->setCurrentWidget(m_html_source_editor);
    m_html_edit->enable_html_controls(false);
    // disable some actions while we're editing html source
    m_action_send_msg->setEnabled(false);
    m_action_insert_file->setEnabled(false);
  }
  else {
    m_html_edit->set_html_text(m_html_source_editor->toPlainText());
    m_html_edit->enable_html_controls(true);
    delete m_html_source_editor;
    m_html_source_editor = NULL;
    m_action_send_msg->setEnabled(true);
    m_action_insert_file->setEnabled(true);
    m_edit_stack->setCurrentWidget(m_html_edit);
  }
}

new_mail_widget::new_mail_widget(mail_msg* msg, QWidget* parent)
  : QMainWindow(parent)
{
  lSubject = NULL;
  m_wSubject = NULL;
  m_progress_bar = NULL;

  setWindowTitle(tr("New message"));
  m_wrap_lines = true;
  m_close_confirm = true;
  m_msg = *msg;

  QWidget* main_hb = new QWidget(this);	// container
  setCentralWidget(main_hb);

  QHBoxLayout *topLayout = new QHBoxLayout(main_hb);
  topLayout->setSpacing(4);
  topLayout->setMargin(4);

  m_wtags=new tags_box_widget(main_hb);

  topLayout->addWidget(m_wtags);
  topLayout->setStretchFactor(m_wtags, 0);

  QSplitter* split=new QSplitter(Qt::Vertical, main_hb);

  m_edit_stack = new QStackedWidget(split);
  m_bodyw = new body_edit_widget();
  connect(m_bodyw, SIGNAL(attach_file_request(const QUrl)),
	  this, SLOT(attach_file(const QUrl)));
  m_html_edit = new html_editor();
  connect(m_html_edit, SIGNAL(attach_file_request(const QUrl)),
	  this, SLOT(attach_file(const QUrl)));
  m_html_edit->set_html_text("<html><body></body></html>");
  m_edit_stack->addWidget(m_bodyw);
  m_edit_stack->addWidget(m_html_edit);
  m_edit_mode = html_mode;
  m_edit_stack->setCurrentWidget(m_html_edit);

  QVBoxLayout *hLayout = new QVBoxLayout();
  topLayout->addLayout(hLayout);
  topLayout->setStretchFactor(hLayout, 1);

  gridLayout = new QGridLayout();
  hLayout->addLayout(gridLayout);
  hLayout->addWidget(split);

  //  QWidget* wcont=main_hb;
  int nRow=0;

  header_field_editor* ed = new header_field_editor(this);
  ed->set_type(header_field_editor::index_header_to);
  ed->grid_insert(gridLayout, nRow, 0);
  ed->set_value(m_msg.header().m_to);
  connect(ed, SIGNAL(remove()), this, SLOT(remove_field_editor()));
  connect(ed, SIGNAL(add()), this, SLOT(add_field_editor()));
  m_header_editors.append(ed);
  nRow++;

  QString initial_cc = m_msg.header().m_cc;
  if (!initial_cc.isEmpty()) {
    header_field_editor* ed1 = new header_field_editor(this);
    ed1->set_type(header_field_editor::index_header_cc);
    ed1->grid_insert(gridLayout, nRow++, 0);
    ed1->set_value(initial_cc);
    connect(ed1, SIGNAL(remove()), this, SLOT(remove_field_editor()));
    connect(ed1, SIGNAL(add()), this, SLOT(add_field_editor()));
    m_header_editors.append(ed1);
  }

  lSubject=new QLabel(tr("Subject:"), this);
  gridLayout->addWidget(lSubject, nRow, 0);
  m_wSubject=new QLineEdit(this);
  m_wSubject->setText(m_msg.header().m_subject);
  gridLayout->addWidget(m_wSubject, nRow, 1);
  nRow++;

  m_qAttch=new attch_listview(split);
  m_qAttch->allow_delete(true);
  m_qAttch->setAcceptDrops(true);

  if (m_msg.attachments().size()==0)
    m_qAttch->hide();
  else {
    std::list<attachment>::iterator it;
    for (it=m_msg.attachments().begin(); it!=m_msg.attachments().end(); ++it) {
      attch_lvitem* item = new attch_lvitem(m_qAttch, &(*it));
      item->set_editable(true);
      item->fill_columns();
    }
    m_qAttch->show();
  }
  connect(m_qAttch, SIGNAL(attach_file_request(const QUrl)),
	  this, SLOT(attach_file(const QUrl)));

  attach_item_editor_delegate* delegate = new attach_item_editor_delegate(m_qAttch);
  m_qAttch->setItemDelegate(delegate);

  resize(900,600);

  QList<int> lSizes;
  lSizes.append(400);
  lSizes.append(20);
  split->setSizes(lSizes);
  set_wrap_mode(); 

  create_actions();

  QMenu* pMsg=new QMenu(tr("&Message"), this);
  CHECK_PTR(pMsg);

  QIcon ico_print(FT_MAKE_ICON(FT_ICON16_PRINT));
  QIcon ico_note(FT_MAKE_ICON(FT_ICON16_EDIT_NOTE));
  QIcon ico_close(FT_MAKE_ICON(FT_ICON16_CLOSE_WINDOW));

  pMsg->addAction(m_action_send_msg);
  pMsg->addAction(m_action_send_later);
  pMsg->addAction(m_action_attach_file);
  pMsg->addAction(m_action_insert_file);
  //  pMsg->addAction(tr("Keep for later"), this, SLOT(keep()));
  pMsg->addAction(ico_note, tr("Edit private note"), this, SLOT(edit_note()));

  pMsg->addAction(UI_ICON(ICON16_LOAD_TEMPLATE), tr("Load template"),
		  this, SLOT(load_template()));
  pMsg->addAction(UI_ICON(ICON16_SAVE_TEMPLATE), tr("Save as template"),
		  this, SLOT(save_template()));

  pMsg->addAction(ico_print, tr("Print"), this, SLOT(print()));
  pMsg->addAction(ico_close, tr("&Cancel"), this, SLOT(cancel()));

  QMenu* pEdit = new QMenu(tr("Edit"), this);

  connect(pEdit->addAction(FT_MAKE_ICON(FT_ICON16_EDIT_CUT),
			   tr("Cut")),
	  &QAction::triggered,
	  this,
	  [this]{ run_edit_action("cut"); });

  connect(pEdit->addAction(FT_MAKE_ICON(FT_ICON16_EDIT_COPY),
			   tr("Copy")),
	  &QAction::triggered,
	  this,
	  [this]{ run_edit_action("copy"); });

  connect(pEdit->addAction(FT_MAKE_ICON(FT_ICON16_EDIT_PASTE),
			   tr("Paste")),
	  &QAction::triggered,
	  this,
	  [this]{ run_edit_action("paste"); });

  connect(pEdit->addAction(FT_MAKE_ICON(FT_ICON16_UNDO),
			   tr("Undo")),
	  &QAction::triggered,
	  this,
	  [this]{ run_edit_action("undo"); });

  connect(pEdit->addAction(FT_MAKE_ICON(FT_ICON16_REDO),
			   tr("Redo")),
	  &QAction::triggered,
	  this,
	  [this]{ run_edit_action("redo"); });
  pEdit->addSeparator();
  m_action_external_editor = pEdit->addAction(tr("External editor"),
					      this,
					      SLOT(launch_external_editor()));

  m_ident_menu = new QMenu(tr("Identity"), this);
  load_identities(m_ident_menu);
  if (m_ids.empty()) {
    m_errmsg=tr("No sender identity is defined.\nUse the File/Preferences menu and fill in an e-mail identity to be able to send messages.");
    QMessageBox::warning(this, tr("Error"), m_errmsg, QMessageBox::Ok, Qt::NoButton);
  }

  m_pMenuFormat = new QMenu(tr("&Format"), this);
  QAction* action_wrap_lines = m_pMenuFormat->addAction(tr("&Wrap lines"), this, SLOT(toggle_wrap(bool)));
  action_wrap_lines->setCheckable(true);
  action_wrap_lines->setChecked(m_wrap_lines);
  QIcon ico_font(FT_MAKE_ICON(FT_ICON16_FONT));
  m_pMenuFormat->addAction(ico_font, tr("Font"), this, SLOT(change_font()));

  QActionGroup* msg_format_group = new QActionGroup(this);
  msg_format_group->setExclusive(true);
  m_action_plain_text = m_pMenuFormat->addAction(tr("Plain text"), this, SLOT(to_format_plain_text()));
  m_action_plain_text->setCheckable(true);
  m_action_html_text = m_pMenuFormat->addAction(tr("HTML"), this, SLOT(to_format_html_text()));
  m_action_html_text->setCheckable(true);
  m_action_plain_text->setChecked(m_edit_mode == plain_mode);
  m_action_html_text->setChecked(m_edit_mode == html_mode);
  msg_format_group->addAction(m_action_plain_text);
  msg_format_group->addAction(m_action_html_text);

  m_pMenuFormat->addAction(tr("Store settings"), this, SLOT(store_settings()));

  QMenu* menu_display = new QMenu(tr("Display"), this);
  QAction* action_display_tags = menu_display->addAction(tr("Tags panel"), this, SLOT(toggle_tags_panel(bool)));
  action_display_tags->setCheckable(true);
  action_display_tags->setChecked(true);

  QMenuBar* menu=menuBar();
  menu->addMenu(pMsg);
  menu->addMenu(pEdit);
  menu->addMenu(m_ident_menu);
  menu->addMenu(m_pMenuFormat);
  menu->addMenu(menu_display);

  //  topLayout->setMenuBar(menu);
  make_toolbars();

  QString body_html = m_msg.get_body_html();
  if (!body_html.isEmpty()) {
    m_html_edit->set_html_text(body_html);
    m_html_edit->finish_load();
    m_html_edit->setFocus();
    format_html_text();
  }
  else {
    set_body_text(m_msg.get_body_text());
    format_plain_text();    
  }
}

/* Switch to plain text format initiated by the user */
void
new_mail_widget::to_format_plain_text()
{
  if (m_html_edit->isModified()) {
    int res = QMessageBox::Ok;
    res = QMessageBox::warning(this, APP_NAME, tr("The Conversion to plain text format has the effect of loosing the rich text formating (bold, italic, colors, fonts, ...). Please confirm your choice."), QMessageBox::Ok, QMessageBox::Cancel|QMessageBox::Default, Qt::NoButton);
    if (res == QMessageBox::Ok) {
      set_body_text(m_html_edit->to_plain_text());
      format_plain_text();
    }
    else {
      m_action_html_text->setChecked(true);
    }
  }
  else {
    set_body_text(m_html_edit->to_plain_text());
    format_plain_text();
  }
}

/* Switch to html format initiated by the user */
void
new_mail_widget::to_format_html_text()
{
  QString text = mail_displayer::htmlize(m_bodyw->document()->toPlainText());
  m_html_edit->set_html_text(text.replace("\n", "<br>\n"));
  format_html_text();
}


void
new_mail_widget::format_plain_text()
{
  m_edit_stack->setCurrentWidget(m_bodyw);
  m_edit_mode = plain_mode;
  if (m_toolbar_html1)
    removeToolBar(m_toolbar_html1);
  if (m_toolbar_html2) {
    removeToolBarBreak(m_toolbar_html2);
    removeToolBar(m_toolbar_html2);
  }
  m_action_plain_text->setChecked(true);
}

void
new_mail_widget::format_html_text()
{
  m_edit_stack->setCurrentWidget(m_html_edit);
  m_edit_mode = html_mode;
  if (m_toolbar_html1 && m_toolbar_html1->isHidden()) {
    addToolBar(m_toolbar_html1);
    m_toolbar_html1->show();
  }
  if (m_toolbar_html2 && m_toolbar_html2->isHidden()) {
    addToolBarBreak();
    addToolBar(m_toolbar_html2);
    m_toolbar_html2->show();
  }
  m_action_html_text->setChecked(true);
}

/* Slot. Called when the user chooses to remove a header field editor */
void
new_mail_widget::remove_field_editor()
{
  header_field_editor* ed = dynamic_cast<header_field_editor*>(QObject::sender());
  if (ed != NULL) {
    m_header_editors.removeOne(ed);
    ed->deleteLater();
  }
}

/* Slot. Called when the user chooses to add a header field editor */
void
new_mail_widget::add_field_editor()
{
  // keep the subject at the end of the grid by removing/re-adding
  if (lSubject)
    gridLayout->removeWidget(lSubject);
  if (m_wSubject)
    gridLayout->removeWidget(m_wSubject);

  header_field_editor* ed = new header_field_editor(this);
  ed->set_type(header_field_editor::index_header_to);
  // insert before the last row of the grid, which is always occupied by the subject line
  ed->grid_insert(gridLayout, gridLayout->rowCount(), 0);
  ed->set_value(m_msg.header().m_to);
  connect(ed, SIGNAL(remove()), this, SLOT(remove_field_editor()));
  connect(ed, SIGNAL(add()), this, SLOT(add_field_editor()));
  m_header_editors.append(ed);

  int r=gridLayout->rowCount();
  if (lSubject)
    gridLayout->addWidget(lSubject, r , 0);
  if (m_wSubject)
    gridLayout->addWidget(m_wSubject, r, 1);
}

void
new_mail_widget::set_wrap_mode()
{
  if (m_wrap_lines) {
    m_bodyw->setWordWrapMode(QTextOption::WordWrap);
    m_bodyw->setLineWrapColumnOrWidth(78);
    m_bodyw->setLineWrapMode(QTextEdit::FixedColumnWidth);
  }
  else {
    m_bodyw->setLineWrapMode(QTextEdit::NoWrap);
  }
}

void
new_mail_widget::load_identities(QMenu* m)
{
  m_identities_group = new QActionGroup(this);
  m_identities_group->setExclusive(true);

  /* Auto-select our identity as a sender.
     First, if we're replying to a message, try the identity to which this
     message was sent (envelope_from would have been set up by our caller).
     Otherwise get the default identity from the configuration */
  QString default_email = m_msg.header().m_envelope_from;
  if (default_email.isEmpty())
    default_email = get_config().get_string("default_identity");
  if (m_ids.fetch()) {
    identities::iterator iter;
    for (iter = m_ids.begin(); iter != m_ids.end(); ++iter) {
      mail_identity* p = &iter->second;
      QAction* action = m->addAction(p->m_email_addr, this, SLOT(change_identity()));
      m_identities_group->addAction(action);
      action->setCheckable(true);
      if (!default_email.isEmpty() && p->m_email_addr==default_email) {
	action->setChecked(true);
	m_from = default_email;
      }
      m_identities_actions.insert(action, p);
    }
    // if no identity is still defined, use the first one from the list
    if (default_email.isEmpty() && !m_ids.empty() && !m_identities_group->actions().isEmpty()) {
      iter=m_ids.begin();
      QAction* action = m_identities_group->actions().first();
      action->setChecked(true);
      m_from = iter->first;
    }
    // Add "Other" that lets the user enter an identity that is not defined
    // within the preferences
    m_action_identity_other = m->addAction(tr("Other..."), this, SLOT(other_identity()));
    m_identities_group->addAction(m_action_identity_other);
    m_action_identity_other->setCheckable(true);
    m_identities_actions.insert(m_action_identity_other, NULL);
    m_action_edit_other = m->addAction(tr("Edit other..."), this, SLOT(other_identity()));
    // m_action_edit_other is enabled only when the "Other" identity is selected
    m_action_edit_other->setEnabled(false);
  }
  setWindowTitle(tr("New mail from ")+m_from);
}

// User-input identity to be used only for this mail
void
new_mail_widget::other_identity()
{
  DBG_PRINTF(3, "other_identity()");
  identity_widget* w = new identity_widget(this);
  if (!m_other_identity_email.isEmpty())
    w->set_email_address(m_other_identity_email);
  if (!m_other_identity_name.isEmpty())
    w->set_name(m_other_identity_name);
  if (w->exec() == QDialog::Accepted) {
    m_other_identity_email = w->email_address();
    m_other_identity_name = w->name();
    m_action_identity_other->setChecked(true);
    m_action_edit_other->setEnabled(true);
    change_identity(); // will update 'm_from'
  }
  else {
    // if the dialog has been cancelled we need to restore the previous identity in
    // the menu through the action group
    QMap<QAction*,mail_identity*>::const_iterator it;
    for (it=m_identities_actions.begin(); it != m_identities_actions.end(); ++it) {
      if (it.value()!= NULL && it.value()->m_email_addr==m_from) {
	//	it.key()->blockSignals(true);
	it.key()->setChecked(true);
	//	it.key()->blockSignals(false);
	m_action_edit_other->setEnabled(false);
	break;
      }
    }
  }
  w->close();
}

void
new_mail_widget::change_identity()
{
  DBG_PRINTF(3, "change_identity()");
  QString old_from=m_from;
  QString old_sig;
  QString new_sig;

  identities::iterator iter;
  for (iter = m_ids.begin(); iter != m_ids.end(); ++iter) {
    mail_identity* p = &iter->second;
    if (old_from == p->m_email_addr) {
      old_sig = expand_signature(p->m_signature, *p);
    }
  }

  QAction* action = m_identities_group->checkedAction();
  // if the identity is the "other one" (user input) which
  // doesn't belong to m_ids
  if (action==m_action_identity_other) {
    new_sig="";
    m_from = m_other_identity_email;
  }
  else {
    // get at the identity by the action associated to it
    QMap<QAction*, mail_identity*>::const_iterator i = m_identities_actions.find(action);
    if (i!=m_identities_actions.constEnd()) {
      new_sig = (*i)->m_signature;
      new_sig = expand_signature(new_sig, **i);
      m_from = (*i)->m_email_addr;
    }
    m_action_edit_other->setEnabled(false);
  }
  if (old_sig != new_sig && !(old_sig.isEmpty() && new_sig.isEmpty())) {
    // try to locate the old signature at the end of the body
    QString body=m_bodyw->toPlainText();
    int idxb=body.lastIndexOf(old_sig);
    if (idxb>=0) {
      // if located, replace it with the signature of the new identity
      body.replace(idxb, old_sig.length(), new_sig);
      m_bodyw->setPlainText(body);
    }
  }
  DBG_PRINTF(3, "Changing from to %s", m_from.toLatin1().constData());
  setWindowTitle(tr("New mail from ")+m_from);
}

// Edit the current message's private note
void
new_mail_widget::edit_note()
{
  note_widget* w=new note_widget(this);
  w->set_note_text(m_note);
  int ret=w->exec();
  if (ret) {
    m_note=w->get_note_text();
    display_note();
  }
  w->close();
}

void
new_mail_widget::open_global_notepad()
{
  notepad* n = notepad::open_unique();
  if (n) {
    n->show();
    n->activateWindow();
    n->raise();
  }
}

void
new_mail_widget::display_note()
{
  attch_lvitem* lvpItem = dynamic_cast<attch_lvitem*>(m_qAttch->topLevelItem(0));
  uint index=0;
  while (lvpItem) {
    if (!lvpItem->get_attachment()) {
      break;			// note found
    }
    index++;
    lvpItem = dynamic_cast<attch_lvitem*>(m_qAttch->topLevelItem(index));
  }
  if (m_note.isEmpty()) {
    if (lvpItem) {		// case 2
      delete lvpItem;		// remove the note from the listview
      lvpItem=NULL;
    }
  }
  else {
    if (!lvpItem) {		// case 3
      lvpItem = new attch_lvitem(m_qAttch, NULL);
    }
    // case 3 & 4
    lvpItem->set_note(m_note);
    lvpItem->fill_columns();
  }

  // don't show the attachments & note's listview when it's empty
  if (m_qAttch->topLevelItem(0))
    m_qAttch->show();
  else
    m_qAttch->hide();
}


void
new_mail_widget::toggle_wrap(bool wrap)
{
  m_wrap_lines = wrap;
  set_wrap_mode();
}

void
new_mail_widget::toggle_tags_panel(bool show_panel)
{
  if (show_panel)
    m_wtags->show();
  else
    m_wtags->hide();
}

void
new_mail_widget::show_tags()
{
  m_wtags->set_tags(m_msg.get_tags());
}

/*
  Steps for external edit:
   Save text in temp file.
   Instantiate external program.
   Block text input in window until external program has quit.
   Reload text from temp file.
*/
void
new_mail_widget::launch_external_editor()
{
  QString app = get_config().get_string("composer/external_editor");
  if (app.isEmpty()) {
    QMessageBox::critical(this, APP_NAME, tr("No external editor is defined."));
    return;
  }

  /* Create temp file */
  QString tmpl_fname = QDir::tempPath()+"/manitou-edit-XXXXXX";
  tmpl_fname.append(m_edit_mode == html_mode ? ".html" : ".txt");
  m_external_edit.tmpf.setFileTemplate(tmpl_fname);
  m_external_edit.tmpf.setAutoRemove(false);
  QTextStream out(&m_external_edit.tmpf);
  if (m_external_edit.tmpf.open()) {
    if (m_edit_mode == html_mode) {
      out << m_html_edit->html_text();
    }
    else if (m_edit_mode == plain_mode) {
      out << m_bodyw->document()->toPlainText();
    }
    m_external_edit.tmpf.close();
  }
  else {
    QMessageBox::critical(this, APP_NAME, tr("Unable to open temporary file for editor:\n%1")
			  .arg(m_external_edit.tmpf.errorString()));
    return;
  }

  /* Run process */
  m_external_edit.process = new QProcess(this);
  m_external_edit.active = true;

  m_external_edit.m_abort_button = new QPushButton(tr("Abort editor"), this);
  m_external_edit.m_abort_button->setIcon(UI_ICON(ICON16_CANCEL));
  statusBar()->addPermanentWidget(m_external_edit.m_abort_button);
  connect(m_external_edit.m_abort_button, SIGNAL(clicked()), this, SLOT(abort_external_editor()));

  /* Avoid these actions until the external edition is finished:
     - change text or html body
     - switch between text and html formats
     - launch another external editor */
  m_bodyw->setEnabled(false);
  m_html_edit->page()->setContentEditable(false);
  m_html_edit->setEnabled(false);

  m_action_plain_text->setEnabled(false);
  m_action_html_text->setEnabled(false);
  m_action_external_editor->setEnabled(false);

  QStringList args;
  args << m_external_edit.tmpf.fileName();
  m_external_edit.process->start(app, args);
  connect(m_external_edit.process, SIGNAL(finished(int,QProcess::ExitStatus)),
	  this, SLOT(external_editor_finished(int,QProcess::ExitStatus)));
  connect(m_external_edit.process, SIGNAL(error(QProcess::ProcessError)),
	  this, SLOT(external_editor_error(QProcess::ProcessError)));

}

void
new_mail_widget::abort_external_editor()
{
  disconnect(m_external_edit.process, SIGNAL(finished(int,QProcess::ExitStatus)),
	     this, SLOT(external_editor_finished(int,QProcess::ExitStatus)));
  disconnect(m_external_edit.process, SIGNAL(error(QProcess::ProcessError)),
	     this, SLOT(external_editor_error(QProcess::ProcessError)));

  m_external_edit.process->terminate();
  external_editor_cleanup();
}

void
new_mail_widget::external_editor_finished(int code, QProcess::ExitStatus exit_status)
{
  Q_UNUSED(code);
  Q_UNUSED(exit_status);
  QTextStream in(&m_external_edit.tmpf);
  if (m_external_edit.tmpf.open()) {
    if (m_edit_mode == plain_mode) {
      m_bodyw->setPlainText(in.readAll());
      m_bodyw->document()->setModified(true);
    }
    else if (m_edit_mode == html_mode) {
      QString contents = in.readAll();
      m_html_edit->set_html_text(contents);
      /* there is no html_editor::setModified, but
	 m_external_edit.inserted will do */
    }
    m_external_edit.inserted = true;
  }
  else {
    QMessageBox::critical(this, APP_NAME,
			  tr("Unable to open temporary file after edit:\n%1")
			  .arg(m_external_edit.tmpf.errorString()));
  }

  external_editor_cleanup();
}

void
new_mail_widget::external_editor_error(QProcess::ProcessError error)
{
  Q_UNUSED(error);
  QMessageBox::critical(this, APP_NAME, tr("Failed to run external editor:\n%1")
			.arg(m_external_edit.process->errorString()));
  external_editor_cleanup();
}

void
new_mail_widget::external_editor_cleanup()
{
  /* do not delete m_external_edit.process, its parent object will do it */
  m_bodyw->setEnabled(true);
  m_external_edit.tmpf.close();
  m_external_edit.tmpf.remove();
  /* re-enable actions disabled by launch_external_editor() */
  m_html_edit->setEnabled(true);
  m_action_plain_text->setEnabled(true);
  m_action_html_text->setEnabled(true);
  m_external_edit.active = false;
  m_action_external_editor->setEnabled(true);

  if (m_external_edit.m_abort_button) {
    delete m_external_edit.m_abort_button;
    m_external_edit.m_abort_button = NULL;
  }
}

void
new_mail_widget::closeEvent(QCloseEvent* e)
{
  int res=QMessageBox::Ok;
  if (m_close_confirm && (m_bodyw->document()->isModified() ||
			  m_html_edit->isModified() ||
			  m_external_edit.inserted))
  {
    res = QMessageBox::warning(this, APP_NAME,
			     tr("The message you are composing in this window will be lost if you confirm."),
			     QMessageBox::Ok,
			     QMessageBox::Cancel|QMessageBox::Default,
			     Qt::NoButton);
  }
  if (res == QMessageBox::Ok)
    e->accept();
  else
    e->ignore();
}

void
new_mail_widget::cancel()
{
  close();
}

void
new_mail_widget::keep()
{
  // TODO
}

bool
new_mail_widget::check_validity()
{
  // collect the addresses from the header field editors
  QMap<QString,QString> addresses;
  QListIterator<header_field_editor*> iter(m_header_editors);
  QStringList bad_addresses;

  while (iter.hasNext()) {
    header_field_editor* ed = iter.next();
    QString field_name = ed->get_field_name();
    bool errparse;
    QString value = check_addresses(ed->get_value(), bad_addresses, &errparse);
    if (errparse) {
      QMessageBox::critical(NULL, tr("Syntax error"), tr("Invalid syntax in email addresses:")+"<br><i>"+ed->get_value()+"</i>", QMessageBox::Ok, Qt::NoButton);
      return false;
    }
    if (!field_name.isEmpty() && !value.isEmpty()) {
      if (addresses.contains(field_name)) {
	// append the value
	addresses[field_name].append(QString(", %1").arg(value));
      }
      else {
	addresses.insert(field_name, value);
      }
    }
  }

  if (!bad_addresses.isEmpty()) {
    QString msg;
    if (bad_addresses.count()>1)
      msg = tr("The following email addresses are invalid:")+"<br><i>"+bad_addresses.join("<br>")+"</i>";
    else
      msg = tr("Invalid email address:")+"<br><i>"+bad_addresses.at(0)+"</i>";
    QMessageBox::critical(this, tr("Address error"), msg,
			  QMessageBox::Ok, Qt::NoButton);
    return false;
  }

  if (!addresses.contains("To") && !addresses.contains("Bcc")) {
    QMessageBox::critical(this, tr("Error"), tr("The To: and Bcc: field cannot be both empty"), QMessageBox::Ok, Qt::NoButton);
    return false;
  }

  if (m_ids.empty()) {
    QMessageBox::critical(this, tr("Error"), tr("The message has no sender (create an e-mail identity in the Preferences)"), QMessageBox::Ok, Qt::NoButton);
    return false;
  }

  if (m_wSubject->text().isEmpty()) {
    int res=QMessageBox::warning(this, tr("Warning"), tr("The message has no subject.\nSend nonetheless?"), QMessageBox::Ok, QMessageBox::Cancel|QMessageBox::Default, Qt::NoButton);

    if (res!=QMessageBox::Ok)
      return false;
  }

  return true;
}

/* Bound to the "Send" action in the menu */
void
new_mail_widget::send_now()
{
  if (!check_validity())
    return;
  m_send_datetime = QDateTime();
  this->send();
}


void
new_mail_widget::send()
{
  if (m_wtags)
    m_wtags->get_selected(m_msg.get_tags());

  // collect the addresses from the header field editors
  QMap<QString,QString> addresses;
  QListIterator<header_field_editor*> iter(m_header_editors);
  QStringList bad_addresses;

  while (iter.hasNext()) {
    header_field_editor* ed = iter.next();
    QString field_name = ed->get_field_name();
    bool errparse;
    QString value = check_addresses(ed->get_value(), bad_addresses, &errparse);
    /* errpase should be always false, since addresses must be tested
       by check_validity() */
    if (errparse)
      return;

    if (!field_name.isEmpty() && !value.isEmpty()) {
      if (addresses.contains(field_name)) {
	// append the value
	addresses[field_name].append(QString(", %1").arg(value));
      }
      else {
	addresses.insert(field_name, value);
      }
    }
  }

  /* Collect attachments from the listview */
  m_msg.attachments().clear();
  for (int i=0; i<m_qAttch->topLevelItemCount(); i++) {
    attch_lvitem* item = dynamic_cast<attch_lvitem*>(m_qAttch->topLevelItem(i));
    attachment* a = item->get_attachment();
    if (a) {
      m_msg.attachments().push_back(*a);
    }
  }

  if (m_edit_mode == html_mode) {
    // Collect the attachments refered to by the HTML contents, and generated
    // MIME content-id's for them.
    QStringList local_names = m_html_edit->collect_local_references();
    QMap<QString,QString> map_local_cid;
    attachments_list& alist = m_msg.attachments();
    QStringListIterator it(local_names);

    // For each reference to a distinct local file, we create a new attachment
    while (it.hasNext()) {
      QString filename = it.next();

      // Unicity test, because we don't want to create different attachments
      // for several references to the same file. That may happen for
      // smiley pictures, for example

      if (!map_local_cid.contains(filename)) {
	attachment attch;
	QString external_filename = filename;
	if (external_filename.startsWith("file://", Qt::CaseInsensitive))
	  external_filename = external_filename.mid(strlen("file://")); // remove the scheme
	// TODO: what if we have file:///home/file and /home/file in local_names?
#ifdef Q_OS_WIN
	if (external_filename.startsWith("/")) {
	  external_filename = external_filename.mid(1);
	}
#endif
	attch.set_filename(external_filename);
	attch.get_size_from_file();
	attch.set_mime_type(attachment::guess_mime_type(filename));
	attch.create_mime_content_id();
	alist.push_back(attch);
	map_local_cid[filename] = attch.mime_content_id();
      }
    }

    // Finally, translate the local names to the CIDs references in
    // the HTML document
    if (!map_local_cid.empty()) {
      m_html_edit->replace_local_references(map_local_cid);
    }
  }

  try {
    make_message(addresses);
  }
  catch(QString error_msg) {
    QMessageBox::critical(this, tr("Error"), error_msg);
    return;
  }

  ui_feedback* ui = new ui_feedback(this);
  statusBar()->showMessage(tr("Saving message"));

  if (!m_progress_bar) {
    m_progress_bar = new QProgressBar(this);
    statusBar()->addPermanentWidget(m_progress_bar);
  }

  connect(ui, SIGNAL(set_max(int)), m_progress_bar, SLOT(setMaximum(int)));
  connect(ui, SIGNAL(set_val(int)), m_progress_bar, SLOT(setValue(int)));
  connect(ui, SIGNAL(set_text(const QString&)), statusBar(), SLOT(showMessage(const QString&)));

  try {
    if (m_msg.store(ui)) {
      /* If this message was a reply, then tell to the originator mailitem
	 to update it's status */
      if (m_msg.inReplyTo() != 0) {
	DBG_PRINTF(5, "refresh_request for %d", m_msg.inReplyTo());
	emit refresh_request(m_msg.inReplyTo());
      }
      else if (m_msg.forwardOf().size() != 0) {
	const std::vector<mail_id_t>& v = m_msg.forwardOf();
	for (uint i=0; i < v.size(); i++) {
	  emit refresh_request(v[i]);
	}
      }
      if (m_msg.m_tags_transitions.size() > 0)
	emit tags_counters_changed(m_msg.m_tags_transitions);
      m_close_confirm=false;
      close();
    }
    else {
      QMessageBox::critical(this, tr("Error"), "Error while saving the message");
      statusBar()->showMessage(tr("Send failed."), 3000);
    }
  }
  catch(app_exception& exc) {
    QMessageBox::critical(this, tr("Error"), exc.m_err_msg);
    statusBar()->showMessage(tr("Send failed."), 3000);
  }
  if (m_progress_bar) {
    delete m_progress_bar;
    m_progress_bar=NULL;
  }
}

void
new_mail_widget::send_later()
{
  if (!check_validity())
    return;

  schedule_delivery_dialog dlg;
  int r = dlg.exec();
  if (r == QDialog::Accepted) {
    m_send_datetime = dlg.m_send_datetime->dateTime();
    this->send();
  }
  else
    m_send_datetime = QDateTime();
}


void
new_mail_widget::attach_files()
{
  if (m_last_attch_dir.isEmpty())
    m_last_attch_dir = get_config().get_string("attachments_directory");

  QStringList l=QFileDialog::getOpenFileNames(this, tr("Select files to attach"), m_last_attch_dir);

  QStringList::Iterator it;
  for (it=l.begin(); it!=l.end(); it++) {
    attachment attch;
    attch.set_filename((*it));
    m_last_attch_dir = QFileInfo(*it).canonicalPath();
    attch.get_size_from_file();
    attch.set_mime_type(attachment::guess_mime_type(*it));

    // Insert the attachment's listviewitem at the end of the listview
    attch_lvitem* pItem = new attch_lvitem(m_qAttch, &attch);
    pItem->set_editable(true);
    pItem->fill_columns();
  }
  m_qAttch->show();
}

void
new_mail_widget::attach_file(const QUrl url)
{
  QFileInfo info(url.toLocalFile());
  if (info.isReadable()) {
    attachment attch;
    attch.set_filename(info.absoluteFilePath());
    attch.get_size_from_file();
    attch.set_mime_type(attachment::guess_mime_type(info.absoluteFilePath()));

    // Insert the attachment's listviewitem at the end of the listview
    attch_lvitem* pItem = new attch_lvitem(m_qAttch, &attch);
    pItem->set_editable(true);
    pItem->fill_columns();
    m_qAttch->show();
  }
}

void
new_mail_widget::insert_file()
{
  QString file_contents;
  QString name = QFileDialog::getOpenFileName(this);
  if (name.isEmpty())
    return;

  QFile f(name);
  bool opened=f.open(QIODevice::ReadOnly | QIODevice::Text);
  if (opened) {
    while (!f.atEnd() && !f.error()) {
      char buf[8192+1];
      qint64 n_read;
      while ((n_read=f.read(buf, sizeof(buf)-1)) >0) {
	buf[(uint)n_read] = '\0';
	file_contents += buf;
      }
    }
  }

  if (f.error() || !opened) {
    file_contents = QString();
    QString errstr;
    if (f.error()==QFile::OpenError || !opened) {
      errstr=tr("Unable to open file '%1'").arg(name);
    }
    else {
      errstr=tr("Unable to read file '%1': error #%2").arg(name).arg(f.error());
    }
    QMessageBox::information(this, APP_NAME, errstr);
  }

  if (!file_contents.isEmpty()) {
    m_bodyw->insertPlainText(file_contents);
  }
}


void
new_mail_widget::make_message(const QMap<QString,QString>& user_headers)
{
  mail_header& h = m_msg.header();

  m_msg.set_note(m_note);

  QTextDocument* doc = m_bodyw->document();
  if (m_edit_mode == plain_mode) {
    m_msg.set_body_text(doc->toPlainText());
    m_msg.set_body_html("");
  }
  else {
    m_msg.set_body_html(m_html_edit->html_text());
    m_msg.set_body_text(m_html_edit->to_plain_text());
  }

  mail_address addr;
  identities::iterator iter;
  for (iter = m_ids.begin(); iter != m_ids.end(); ++iter) {
    mail_identity* p = &iter->second;
    if (m_from == p->m_email_addr) {
      addr.set_name(p->m_name);
      addr.set_email(p->m_email_addr);
      h.m_xface = p->m_xface;
      m_msg.set_identity_id(p->m_identity_id);
      break;
    }
  }

  if (iter==m_ids.end()) {
    // if no identity has been found with 'm_ids', it has to mean
    // that the "other" identity has been selected
    if (!m_other_identity_email.isEmpty()) {
      addr.set_email(m_other_identity_email);
      addr.set_name(m_other_identity_name);
      m_msg.set_identity_id(0);
    }
    else {
      throw(tr("Error: no sender can be assigned to the message"));
    }
  }

  h.m_from = addr.email_and_name();
  h.m_sender = addr.email();
  h.m_sender_fullname = addr.name();
  h.m_to = user_headers.value("To");
  /* Remove problematic characters from the subject. Despite being a
     single-line edit, it is apparently possible to paste newlines
     into the text. */
  h.m_subject = m_wSubject->text().trimmed();
  h.m_subject.replace("\n\r", " ");
  h.m_subject.replace("\n", " ");
  h.m_subject.replace("\r", " ");
  h.m_subject.replace("\t", " ");

  h.m_cc = user_headers.value("Cc");
  h.m_replyTo = user_headers.value("ReplyTo");
  h.m_bcc = user_headers.value("Bcc");

  m_msg.set_send_datetime(m_send_datetime);
}

/* Return normalized addresses */
QString
new_mail_widget::check_addresses(const QString addresses,
				 QStringList& bad_addresses,
				 bool* unparsable /*=NULL*/)
{
  if (unparsable)
    *unparsable=false;
  if (addresses.isEmpty())
    return QString("");

  QList<QString> emails_list;
  QList<QString> names_list;
  int err = mail_address::ExtractAddresses(addresses, emails_list, names_list);
  QString result;

  if (err && unparsable) {
    *unparsable=true;
    return result;
  }

  bool do_basic_check=(get_config().get_string("composer/address_check")=="basic");

  QList<QString>::const_iterator iter1,iter2;
  for (iter1 = emails_list.begin(), iter2 = names_list.begin();
       iter1!=emails_list.end() && iter2!=names_list.end();
       ++iter1, ++iter2)
    {
      mail_address addr;
      if (do_basic_check) {
	if (!mail_address::basic_syntax_check(*iter1)) {
	  bad_addresses.append(*iter1);
	}
      }
      addr.set_email(*iter1);
      addr.set_name(*iter2);
      result.append(addr.email_and_name());
      result.append(",");
    }
  // remove the trailing ',' if necessary

  if (!result.isEmpty() && result.at(result.length()-1) == ',')
    result.truncate(result.length()-1);
  return result;
}

void
new_mail_widget::start_edit()
{
  m_bodyw->setFocus();
  m_bodyw->document()->setModified(false);
}

const mail_identity*
new_mail_widget::get_current_identity()
{
  identities::const_iterator iter = m_ids.find(m_from);
  return (iter!=m_ids.end() ? &iter->second : NULL);
}

void
new_mail_widget::insert_signature()
{
  const mail_identity* id = get_current_identity();
  if (id) {
    QString sig = expand_signature(id->m_signature, *id);
    if (sig.length()>0 && sig.at(0)!='\n')
      sig.prepend("\n");
    if (m_edit_mode == plain_mode) {
      // insert the signature and leave the cursor just above
      QTextCursor cursor = m_bodyw->textCursor();
      cursor.movePosition(QTextCursor::End);
      int pos = cursor.position();
      m_bodyw->append(sig);
      cursor.setPosition(pos);
      m_bodyw->setTextCursor(cursor);
    }
    else if (m_edit_mode == html_mode) {
      QString html_sig = mail_displayer::htmlize(sig);
      html_sig = "<p><div class=\"manitou-sig\">" + html_sig + "</div>";
      m_html_edit->append_paragraph(html_sig);
    }
  }
}

void
new_mail_widget::prepend_html_paragraph(const QString para)
{
  if (m_edit_mode == html_mode) {
    m_html_edit->prepend_paragraph(para);
  }
}

void
new_mail_widget::append_html_paragraph(const QString para)
{
  if (m_edit_mode == html_mode) {
    m_html_edit->append_paragraph(para);
  }
}

QString
new_mail_widget::expand_signature(const QString sig, const mail_identity& identity)
{
  QString esig=sig;
  bool user_fetched = false;
  user u;
  int pos=0;
  QRegExp rx("\\{(\\w+)\\}");
  while ((pos=rx.indexIn(esig, pos))>=0) {
    const QString field = rx.cap(1);
    if (!user_fetched && field.startsWith("operator")) {
      int user_id = user::current_user_id();
      if (user_id>0)
	u.fetch(user_id);
    }

    QString field_val;
    bool use_field = true;

    if (field=="operator_login") {
      field_val = u.m_login;
    }
    else if (field=="operator_firstname") {
      int bpos = u.m_fullname.indexOf(' ');
      if (bpos>=1) {
	field_val = u.m_fullname.left(bpos);
      }
    }
    else if (field=="operator_fullname") {
      field_val = u.m_fullname;
    }
    else if (field=="operator_email") {
      field_val = u.m_email;
    }
    else if (field=="operator_custom_field1") {
      field_val = u.m_custom_field1;
    }
    else if (field=="operator_custom_field2") {
      field_val = u.m_custom_field2;
    }
    else if (field=="operator_custom_field3") {
      field_val = u.m_custom_field3;
    }
    else if (field=="sender_email") {
      field_val = identity.m_email_addr;
    }
    else if (field=="sender_name") {
      field_val = identity.m_name;
    }
    else
      use_field=false;

    if (use_field) {
      esig.replace(pos, rx.matchedLength(), field_val);
      pos += field_val.length();
    }
    else
      pos += rx.matchedLength();
  }
  return esig;
}

void
new_mail_widget::change_font()
{
  bool ok;
  QFont f=QFontDialog::getFont(&ok, m_bodyw->font());
  if (ok) {
    get_config().set_string("newmail/font", f.toString());
    m_bodyw->setFont(f);
  }
}

void
new_mail_widget::store_settings()
{
  // Save the font
  app_config& conf=get_config();
  if (conf.store("newmail/font")) {
    QString user_msg;
    if (conf.name().isEmpty())
      user_msg = QString(tr("The display settings have been saved in the default configuration."));
    else
      user_msg = QString(tr("The display settings have been saved in the '%1' configuration.")).arg(conf.name());
    QMessageBox::information(this, tr("Confirmation"), user_msg); 
  }
}

void
new_mail_widget::load_template()
{
  template_dialog dialog;
  int r = dialog.exec();
  if (r==QDialog::Accepted) {
    int id = dialog.selected_template_id();
    if (id>0) {
      m_template.load(id);
      // TODO: ask confirmation before loosing the current text if any
      if (m_template.html_body().isEmpty()) {
	set_body_text(m_template.text_body());
	format_plain_text();
      }
      else {
	m_html_edit->set_html_text(m_template.html_body());
	m_html_edit->finish_load();
	m_html_edit->setFocus();
	format_html_text();
      }
    }
  }
}

void
new_mail_widget::save_template()
{
  QString title;
  bool ok;
  if (m_template.m_template_id>0) {
    // a template has been loaded
    title = m_template.m_title;
  }
  bool keep_asking=false;
  do {
    title = QInputDialog::getText(this, tr("Save template"), tr("Title of the template:"),
			QLineEdit::Normal, title, &ok);
    if (title.isEmpty()) {
      // TODO: display an error message
      keep_asking=true;
    }
    // TODO: check if the title is already used in the database
  } while (ok && keep_asking);

  if (ok) {
    try {
      QMap<QString,QString> addresses;  
      make_message(addresses);
    }
    catch(QString error_msg) {
      QMessageBox::critical(this, tr("Error"), error_msg);
      return;
    }

    m_template.m_title = title;
    QString header;
    // TODO: add other header fields?
    header = "Subject: " + m_msg.header().m_subject;
    m_template.set_header(header);

    if (m_edit_mode == plain_mode) {
      QTextDocument* doc = m_bodyw->document();
      m_template.set_text_body(doc->toPlainText());
      m_template.set_html_body(QString());
    }
    else {
      m_template.set_html_body(m_html_edit->html_text());
      QString text=m_html_edit->to_plain_text();
      /* &nbsp; is translated to 0xA0 but we prefer the normal space character
	 for our usage in mail text parts */
      text.replace(QChar(0xa0), QChar(0x20));
      m_template.set_text_body(text);
    }

    if (m_template.m_template_id>0)
      m_template.update();
    else
      m_template.insert();
  }

}

void
new_mail_widget::print()
{
  QTextDocument* doc = m_bodyw->document();
  QPrinter printer;
  QPrintDialog *dialog = new QPrintDialog(&printer, this);
  dialog->setWindowTitle(tr("Print Document"));
  if (dialog->exec() != QDialog::Accepted)
    return;
  doc->print(&printer);
}

void
new_mail_widget::run_edit_action(const char* action_keyword)
{
  if (m_edit_mode == plain_mode) {
    QString action = QString::fromLatin1(action_keyword);
    if (action == "cut")
      m_bodyw->cut();
    else if (action == "copy")
      m_bodyw->copy();
    else if (action == "paste")
      m_bodyw->paste();
    else if (action == "undo")
      m_bodyw->undo();
    else if (action == "redo")
      m_bodyw->redo();
  }
  else if (m_edit_mode == html_mode) {
    m_html_edit->run_edit_action(action_keyword);
  }
}

schedule_delivery_dialog::schedule_delivery_dialog(QWidget* parent) : QDialog(parent)
{
  setWindowTitle(tr("Schedule for later delivery"));
  QVBoxLayout* top = new QVBoxLayout(this);
  QFormLayout* layout = new QFormLayout;
  top->addLayout(layout);
  m_send_datetime = new QDateTimeEdit;
  m_send_datetime->setMinimumDateTime(QDateTime::currentDateTime());
  layout->addRow(tr("Send after:"), m_send_datetime);
  QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok
						| QDialogButtonBox::Cancel);
  connect(btns, SIGNAL(accepted()), this, SLOT(accept()));
  connect(btns, SIGNAL(rejected()), this, SLOT(reject()));
  top->addWidget(btns);
}
