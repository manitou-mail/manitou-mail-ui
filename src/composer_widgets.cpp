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

/* Various widgets used by the main composer class (new_mail_widget) */

#include "main.h"
#include "app_config.h"
#include "composer_widgets.h"
#include "icons.h"

#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QTimer>
#include <QDebug>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>

/***********************/
/* header_field_editor */
/***********************/
header_field_editor::header_field_editor(QWidget* parent)
{
  static const char* defaults[] = {
    // the contents must be aligned with enum header_index
    QT_TR_NOOP("To"),
    QT_TR_NOOP("Cc"),
    QT_TR_NOOP("Bcc"),
    QT_TR_NOOP("ReplyTo"),
    QT_TR_NOOP("Add..."),
    QT_TR_NOOP("Remove...")
  };

  m_cb = new QComboBox(parent);
  for (uint i=0; i<sizeof(defaults)/sizeof(defaults[0]); i++) {
    m_cb->addItem(tr(defaults[i]));
  }
  m_old_index=0;
  connect(m_cb, SIGNAL(currentIndexChanged(int)), this, SLOT(cb_changed(int)));
  m_lineedit = new edit_address_widget(parent);
  m_more_button = new QPushButton(FT_MAKE_ICON(FT_ICON16_ADDRESSBOOK), "", parent);
  connect(m_more_button, SIGNAL(clicked()), this, SLOT(more_addresses()));
}

header_field_editor::~header_field_editor()
{
  /* The widgets have to be deleted explictly since our parent won't
     do it when this happens by the 'remove' combobox action */
  delete m_lineedit;
  delete m_more_button;
  delete m_cb;
}

/*
  Slot. Called on combobox index change
 */
void
header_field_editor::cb_changed(int index)
{
  if (index == index_header_remove) {
    emit remove();    
  }
  else if (index == index_header_add) {
    // When "Add..." gets selected, execute the action but put back
    // the previous index
    m_cb->blockSignals(true);
    m_cb->setCurrentIndex(m_old_index);
    m_cb->blockSignals(false);    
    emit add();
  }
  else
    m_old_index=index;
}

void
header_field_editor::set_type(header_index type)
{
  m_cb->blockSignals(true);
  m_cb->setCurrentIndex((int)type);
  m_cb->blockSignals(false);
}

QString
header_field_editor::get_value() const
{
  return m_lineedit->text();
}

QString
header_field_editor::get_field_name() const
{
  if (!m_lineedit->text().isEmpty()) {
    switch(m_cb->currentIndex()) {
    case index_header_to:
      return "To";
    case index_header_cc:
      return "Cc";
    case index_header_bcc:
      return "Bcc";
    case index_header_replyto:
      return "ReplyTo";
    }
  }
  return QString::null;
}

void
header_field_editor::set_value(const QString val)
{
  m_lineedit->setText(val);
}

void
header_field_editor::grid_insert(QGridLayout* layout, int row, int column)
{
  layout->addWidget(m_cb, row, column++);
  layout->addWidget(m_lineedit, row, column++);
  layout->addWidget(m_more_button, row, column++);
}

void
header_field_editor::more_addresses()
{
  input_addresses_widget* w = new input_addresses_widget(get_value());
  connect(w, SIGNAL(addresses_available(QString)),
	   this, SLOT(addresses_offered(QString)));
  // try to open the window at the current mouse position
  w->move(QCursor::pos());
  w->show();  
}

/* Slot. Typically called when the user has OK'd the
   input_addresses_widget window. At this point we receive the new
   addresses, possibly reformat them and put them into our lineedit
   control */
void
header_field_editor::addresses_offered(QString addresses)
{
  mail_address::join_address_lines(addresses);
  m_lineedit->setText(addresses);
}

input_addresses_widget::input_addresses_widget(const QString& addresses)
{
  m_accel_type=0;
  QVBoxLayout *topLayout = new QVBoxLayout(this);
  topLayout->setSpacing(5);
  topLayout->setMargin(5);

  // search
  QHBoxLayout* hb = new QHBoxLayout();
  hb->setSpacing(3);
  topLayout->addLayout(hb);
  QLabel* lb1 = new QLabel(tr("Search for:"));
  hb->addWidget(lb1);
  m_wfind = new QLineEdit();
  hb->addWidget(m_wfind);
  connect(m_wfind, SIGNAL(returnPressed()), SLOT(find_contacts()));
  QPushButton* w_find_btn = new QPushButton(tr("Find"));
  hb->addWidget(w_find_btn);
  connect(w_find_btn, SIGNAL(clicked()), SLOT(find_contacts()));

  QLabel* lb = new QLabel(tr("Enter the email addresses (separated by comma or newline) :"), this);
  topLayout->addWidget(lb);

  m_wEdit = new QTextEdit();
  m_wEdit->setText(format_multi_lines(addresses));
  topLayout->addWidget(m_wEdit);

  QHBoxLayout* h1 = new QHBoxLayout();
  QPushButton* w_recent_to= new QPushButton(tr("Recent To:"));
  h1->addWidget(w_recent_to);
  QPushButton* w_recent_from= new QPushButton(tr("Recent From:"));
  h1->addWidget(w_recent_from);
  QPushButton* w_prio = new QPushButton(tr("Prioritized:"));
  h1->addWidget(w_prio);
  connect(w_recent_to, SIGNAL(clicked()), SLOT(show_recent_to()));
  connect(w_recent_from, SIGNAL(clicked()), SLOT(show_recent_from()));
  connect(w_prio, SIGNAL(clicked()), SLOT(show_prio_contacts()));
  topLayout->addLayout(h1);

  m_addr_list = new QTreeWidget();
  m_addr_list->hide();
  m_addr_list->setColumnCount(2);
  QStringList labels;
  labels << tr("Name and email") << tr("Last");
  m_addr_list->setHeaderLabels(labels);
  //m_addr_list->setAllColumnsShowFocus(true);
  m_addr_list->setRootIsDecorated(false);
/*
  // TODO: fix this code to resize section 0 the available width
  QFontMetrics font_metrics(m_addr_list->font());
  int date_width=font_metrics.width("0000/00/00 00:00");
  m_addr_list->header()->resizeSection(1, date_width);
  m_addr_list->header()->resizeSection(0, m_addr_list->header()->length()-date_width);
*/
#if QT_VERSION<0x050000
  m_addr_list->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
  connect(m_addr_list, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
	  this, SLOT(addr_selected(QTreeWidgetItem*,int)));

  topLayout->addWidget(m_addr_list);

  QHBoxLayout* h = new QHBoxLayout();
  h->setSpacing(10);
  h->setMargin(10);
  QPushButton* wOk = new QPushButton(tr("OK"));
  h->addWidget(wOk);
  h->addStretch(3);
  QPushButton* wCancel = new QPushButton(tr("Cancel"));
  h->addWidget(wCancel);
  topLayout->addLayout(h);
  setWindowTitle("Mail addresses");
  connect(wOk, SIGNAL(clicked()), SLOT(ok()));
  connect(wCancel, SIGNAL(clicked()), SLOT(cancel()));
}

void
input_addresses_widget::find_contacts()
{
  QString s = m_wfind->text();
  if (s.isEmpty())
    return;
  
  m_addr_list->clear();

  mail_address_list result_list;
  if (result_list.fetch_from_substring(s)) {
    mail_address_list::const_iterator iter;
    for (iter = result_list.begin(); iter != result_list.end(); iter++) {
      QString name_and_email = iter->name() + " <" + iter->get() + ">";
      date date1 = iter->last_sent();
      date date2 = iter->last_recv();
      QString d;
      d = (date1.FullOutput() > date2.FullOutput()) ? date1.OutputHM(2):
	date2.OutputHM(2);
      QStringList string_items;
      string_items << name_and_email << d;
      (void)new QTreeWidgetItem(m_addr_list, string_items);
    }
  }
  m_addr_list->setMinimumHeight(130); // arbitrary
  //  m_addr_list->setSorting(1, false); // descending order
  //  m_addr_list->setShowSortIndicator(true);
  m_addr_list->show();
}

/*
  Add to the list an address which the user has selected
*/
void
input_addresses_widget::addr_selected(QTreeWidgetItem* item, int column)
{
  Q_UNUSED(column);
  if (!item) return;
  QString addresses = m_wEdit->toPlainText();
  if (!addresses.isEmpty() && addresses.right(1) != "\n") {
    addresses.append("\n");
  }
  QString new_addr=format_multi_lines(item->text(0));
  addresses.append(new_addr);
  m_wEdit->setText(addresses);
}

/*
  Display email addresses recently used in from or to recipients
  what: 1=to, 2=from, 3=contacts with a positive priority
*/
void
input_addresses_widget::show_recent(int what)
{
  const int fetch_step=10;
  struct accel* accel;
  mail_address_list result_list;
  switch(what) {
  case 1:
    accel=&m_recent_to;
    break;
  case 2:
    accel=&m_recent_from;
    break;
  case 3:
    accel=&m_prioritized;
    break;
  default:
    DBG_PRINTF(5,"ERR: impossible choice");
    return;
  }
  if (what!=m_accel_type) {
    accel->rows_displayed=0;
    m_addr_list->clear();
    //    m_addr_list->setSorting(1, false); // descending order
    //    m_addr_list->setShowSortIndicator(true);
    m_addr_list->setMinimumHeight(130); // arbitrary
  }
  if (result_list.fetch_recent(what, fetch_step, accel->rows_displayed)) {
    mail_address_list::const_iterator iter;
    for (iter = result_list.begin(); iter != result_list.end(); iter++) {
      QString name_and_email = iter->name() + " <" + iter->get() + ">";
      date d = (what==1)?iter->last_sent():iter->last_recv();
      QString sdate = d.OutputHM(2); // US format
      QString col1 = (what==3) ? QString("%1").arg(iter->recv_pri()) : sdate;
      QStringList string_items;
      string_items << name_and_email << col1;
      (void)new QTreeWidgetItem(m_addr_list, string_items);
      accel->rows_displayed++;
    }
    m_addr_list->show();
    m_accel_type=what;
  }
}
void
input_addresses_widget::set_header_col1(const QString& text)
{
  QTreeWidgetItem* item=m_addr_list->headerItem();
  if (item!=NULL) {
    item->setText(1, text);
  }
}


void
input_addresses_widget::show_recent_to()
{
  set_header_col1(tr("When"));
  show_recent(1);
}

void
input_addresses_widget::show_recent_from()
{
  set_header_col1(tr("When"));
  show_recent(2);
}

void
input_addresses_widget::show_prio_contacts()
{
  set_header_col1(tr("Priority"));
  show_recent(3);
}


QString
input_addresses_widget::format_multi_lines(const QString addresses)
{
  QList<QString> emails_list;
  QList<QString> names_list;
  mail_address::ExtractAddresses(addresses, emails_list, names_list);
  QString result;

  QList<QString>::const_iterator iter1,iter2;
  for (iter1 = emails_list.begin(), iter2 = names_list.begin();
       iter1!=emails_list.end() && iter2!=names_list.end();
       ++iter1, ++iter2)
    {
      mail_address addr;
      addr.set_email(*iter1);
      addr.set_name(*iter2);
      result.append(addr.email_and_name());
      result.append("\n");
    }
  return result;
}

void
input_addresses_widget::ok()
{
  emit addresses_available(m_wEdit->toPlainText());
  hide();
}

void
input_addresses_widget::cancel()
{
  hide();
}

body_edit_widget::body_edit_widget(QWidget* p)
  : QTextEdit(p)
{
  setAcceptRichText(false);
  QString fontname=get_config().get_string("newmail/font");
  if (!fontname.isEmpty() && fontname!="xft") {
    QFont f;
    if (fontname.at(0)=='-')
      f.setRawName(fontname);		// for pre-0.9.5 entries
    else
      f.fromString(fontname);
    setFont(f);
  }
  else {
//    setFont(QFont("courier", 12));
  }

  setAcceptDrops(true);
}

void
body_edit_widget::dragEnterEvent(QDragEnterEvent* event)
{
  /* it's necessary on Windows to explicitly accept the event since
     the default implementation doesn't seem to accept it for dragged
     files */
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
  else
    QTextEdit::dragEnterEvent(event);
}

void
body_edit_widget::dragMoveEvent(QDragMoveEvent* event)
{
  
  /* We don't let the base dragMoveEvent handle URLs to avoid it
     taking control over the caret and not releasing it after the drop */
  if (!event->mimeData()->hasUrls())
    QTextEdit::dragMoveEvent(event);
}

void
body_edit_widget::dropEvent(QDropEvent* event)
{
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
    foreach (QUrl url, event->mimeData()->urls())  {
      emit attach_file_request(url);
    }    
  }
  else
    QTextEdit::dropEvent(event);
}

///
/// identity_widget
///
identity_widget::identity_widget(QWidget* parent): QDialog(parent)
{
  setWindowTitle(tr("Other identity"));
  QVBoxLayout *layout = new QVBoxLayout(this);
  QGridLayout* grid = new QGridLayout();
  layout->addLayout(grid);
  w_email = new QLineEdit();
  w_email->setMinimumWidth(200);
  w_name = new QLineEdit();
  int row=0;
  grid->addWidget(new QLabel(tr("Email address")), row, 0);
  grid->addWidget(w_email, row, 1);
  row++;
  grid->addWidget(new QLabel(tr("Name (optional)")), row, 0);
  grid->addWidget(w_name, row, 1);
  
  QHBoxLayout* buttons_layout = new QHBoxLayout();
  layout->addLayout(buttons_layout);
  buttons_layout->addStretch(3);
  QPushButton* wok = new QPushButton(tr("OK"));
  buttons_layout->addStretch(2);
  QPushButton* wcancel = new QPushButton(tr("Cancel"));
  buttons_layout->addStretch(3);
  connect(wok, SIGNAL(clicked()), SLOT(ok()));
  connect(wcancel, SIGNAL(clicked()), SLOT(cancel()));
  buttons_layout->addWidget(wok);
  buttons_layout->addWidget(wcancel);
}

identity_widget::~identity_widget()
{
}

void
identity_widget::set_email_address(const QString email)
{
  w_email->setText(email);
}

void
identity_widget::set_name(const QString name)
{
  w_name->setText(name);
}

QString
identity_widget::name()
{
  return w_name->text();
}

QString
identity_widget::email_address()
{
  return w_email->text();
}

void
identity_widget::cancel()
{
  this->reject();
}

void
identity_widget::ok()
{
  this->accept();
}

//
// html_source_editor
//

html_source_editor::html_source_editor(QWidget* parent) : QPlainTextEdit(parent)
{
  m_sticker = new QLabel(this);
  m_sticker->setTextFormat(Qt::RichText);
  m_sticker->setText(QString(" <font size=+2>%1</font> ").arg(tr("Source editor")));
  m_sticker->setToolTip(tr("Use the toggle button of the toolbar to go back to visual editor"));
  m_sticker->setAutoFillBackground(true);
  QPalette pal = m_sticker->palette();
  pal.setColor(QPalette::WindowText, QColor(255, 10, 20, 128));
  pal.setColor(QPalette::Background, QColor(255, 239, 52, 100));
  m_sticker->setPalette(pal);
  m_sticker->setFrameStyle(QFrame::StyledPanel);
  QTimer::singleShot(0, this, SLOT(position_label()));
}

void
html_source_editor::position_label()
{
  QRect r = viewport()->contentsRect();
  m_sticker->move(r.x()+r.width()-3-m_sticker->width(),
		  r.y()+5);
}

void
html_source_editor::resizeEvent(QResizeEvent* e)
{
  QPlainTextEdit::resizeEvent(e);
  position_label();
}
