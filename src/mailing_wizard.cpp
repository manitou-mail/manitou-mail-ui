/* Copyright (C) 2004-2015 Daniel Verite

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
#include "mailing.h"
#include "mailing_wizard.h"
#include "mail_template.h"
#include "addresses.h"
#include "identities.h"
#include "text_merger.h"
#include "mailing_viewer.h"

#include <QDebug>
#include <QComboBox>
#include <QBoxLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFont>
#include <QCheckBox>
#include <QTextCodec>
#include <QProgressBar>
#include <QWebPage>
#include <QWebFrame>
#include <QGroupBox>
#include <QButtonGroup>

void
mailing_wizard_page::cleanupPage()
{
  /* called on pressing the Back button.
     The base function resets the fields to the values they had before initializePage()
     We don't want that so we don't call it
  */
}

mailing_wizard_page_title::mailing_wizard_page_title()
{
  setTitle(tr("Basic mailing information"));
  setSubTitle(tr("Choose the title, subject and sender's identity"));

  int row=0;
  QGridLayout* layout = new QGridLayout(this);
  m_title_widget = new QLineEdit;
  registerField("mailing_title*", m_title_widget);
  layout->addWidget(new QLabel(tr("Title:")), row, 0);
  layout->addWidget(m_title_widget, row, 1);

  QLabel* title_subtext = new QLabel(tr("The title is used internally to refer to the mailing. It does not appear in the messages sent."));
  connect(m_title_widget, SIGNAL(textChanged(const QString&)), this, SIGNAL(completeChanged()));
  QFont mini_font = title_subtext->font();
  mini_font.setPointSize((mini_font.pointSize()*8)/10);
  title_subtext->setFont(mini_font);
  row++;
  layout->addWidget(title_subtext, row, 1);

  row++;
  layout->addWidget(new QLabel(tr("Sender:")), row, 0);
  m_cb_sender = new QComboBox;
  layout->addWidget(m_cb_sender, row, 1);
  QLabel* sender_subtext = new QLabel(tr("These addresses are defined in Preferences->Identities."));
  sender_subtext->setFont(mini_font);
  row++;
  layout->addWidget(sender_subtext, row, 1);
  if (m_identities.fetch()) {
    identities::iterator iter;
    for (iter = m_identities.begin(); iter != m_identities.end(); ++iter) {
      mail_identity* p = &iter->second;
      mail_address addr;
      addr.set_name(p->m_name);
      addr.set_email(p->m_email_addr);
      m_cb_sender->addItem(addr.email_and_name(), QVariant(p->m_email_addr));
    }
  }

  row++;
  layout->addWidget(new QLabel(tr("Subject:")), row, 0);
  QLineEdit* subject = new QLineEdit;
  connect(subject, SIGNAL(textChanged(const QString&)), this, SIGNAL(completeChanged()));
  layout->addWidget(subject, row, 1);
  registerField("subject", subject);
  QLabel* subject_subtext = new QLabel(tr("Placeholders written as {{variable-name}} may be used for variable text."));
  subject_subtext->setFont(mini_font);
  row++;
  layout->addWidget(subject_subtext, row, 1);
}

void
mailing_wizard_page_title::initializePage()
{
  //  m_title_widget->setText(get_options()->title);
}

bool
mailing_wizard_page_title::isComplete() const
{
  if (field("mailing_title").toString().isEmpty())
    return false;
  if (m_cb_sender->currentIndex() < 0)
    return false;
  if (field("subject").toString().isEmpty())
    return false;
  return true;
}


mail_address
mailing_wizard_page_title::sender_address()
{
  mail_address addr;
  identities::iterator iter;

  int idx = m_cb_sender->currentIndex();
  if (idx >= 0) {
    QString email = m_cb_sender->itemData(idx).toString();
    for (iter = m_identities.begin(); iter != m_identities.end(); ++iter) {
      mail_identity* p = &iter->second;
      if (email == p->m_email_addr) {
	addr.set_name(p->m_name);
	addr.set_email(p->m_email_addr);
	return addr;
      }
    }
  }
  return addr;
}

mailing_wizard_page_import_data::mailing_wizard_page_import_data()
{
  setTitle(tr("Recipient email addresses and merge data"));
  setSubTitle(tr("Choose how you want to submit the data"));
  m_rb[0] = new QRadioButton(tr("Import the list of recipients email addresses from a text file"));
  m_rb[0]->setChecked(true);
  m_rb[1] = new QRadioButton(tr("Import multi-column data in CSV format with column headers"));
  m_rb[2] = new QRadioButton(tr("Insert the list of recipients email addresses into a text area in the next page"));
  QVBoxLayout* layout = new QVBoxLayout(this);
  for (int i=0; i<3; i++) {
    layout->addWidget(m_rb[i]);
  }

  registerField("addresses_from_file", m_rb[0]);
  registerField("mailmerge_from_file", m_rb[1]);
  registerField("addresses_from_textarea", m_rb[2]);

  // label for the list of template variables
  m_label_variables = new QLabel();
  m_label_variables->setWordWrap(true);
  layout->addWidget(m_label_variables);
}


void
mailing_wizard_page_import_data::initializePage()
{
  mailing_options* options = get_options();

  QSet<QString> variables;
  text_merger::extract_variables(options->text_part, variables);
  text_merger::extract_variables(options->html_part, variables);
  text_merger::extract_variables(field("subject").toString(), variables);

  if (!variables.isEmpty()) {
    QString txt = "<b>"+tr("Variables found in template(s):")+"</b>";
    QStringList list = variables.toList();
    qSort(list);
    txt.append("<br>");
    txt.append(list.join(", "));
/*
    for (it=list.constBegin(); it!=list.constEnd(); ++it) {
      txt.append(QString(" %1").arg(*it));
    }
*/
    m_label_variables->setText(txt);
    m_rb[1]->setChecked(true);
  }
  else
    m_label_variables->setText("");
}

int
mailing_wizard_page_import_data::nextId() const
{
  if (m_rb[0]->isChecked() || m_rb[1]->isChecked()) {
    return mailing_wizard::page_data_file;
  }
  else
    return mailing_wizard::page_paste_address_list;
}

mailing_wizard_page_paste_address_list::mailing_wizard_page_paste_address_list()
{
  setTitle(tr("Insert the recipients email addresses"));
  setSubTitle(tr("Each line in the list will be taken as the To: field of a new independant message."));
  QVBoxLayout* layout = new QVBoxLayout(this);
  m_textarea = new QPlainTextEdit;
  layout->addWidget(m_textarea);
}

QStringList
mailing_wizard_page_paste_address_list::get_addresses()
{
  return m_textarea->toPlainText().split("\n", QString::KeepEmptyParts);
}

int
mailing_wizard_page_paste_address_list::nextId() const
{
  return mailing_wizard::page_parse_data;
}

mailing_wizard_page_data_file::mailing_wizard_page_data_file()
{
  setTitle(tr("Data file"));
  QVBoxLayout* layout = new QVBoxLayout(this);
  QHBoxLayout* ll = new QHBoxLayout;
  QLabel* lfile = new QLabel(tr("Filename:"));
  ll->addWidget(lfile);
  m_filename = new QLineEdit;
  ll->addWidget(m_filename);
  registerField("addresses_filename*", m_filename);
  QPushButton* btn = new QPushButton(tr("Browse"));
  ll->addWidget(btn);
  layout->addLayout(ll);
  connect(btn, SIGNAL(clicked()), this, SLOT(browse()));

  QString csv_txt = tr("<b>Rules:</b>");
  csv_txt.append("<br>");
  csv_txt.append(tr("The first record of the file must contain the column names."));
  csv_txt.append("<br>");
  csv_txt.append(tr("The recipient email address must be placed at the first column."));
  csv_txt.append("<br>");
  csv_txt.append(tr("The separator character between fields can be a tab, a comma or a semicolon."));
  csv_txt.append("<br>");
  csv_txt.append(tr("The file must be encoded in the current locale."));
  m_label_csv_rules = new QLabel(csv_txt);
  layout->addWidget(m_label_csv_rules);
  m_label_csv_rules->hide();

}

void
mailing_wizard_page_data_file::initializePage()
{
  if (field("mailmerge_from_file").toBool()) {
    setSubTitle(tr("Please specify the location of the data file in CSV format"));
    m_label_csv_rules->show();
  }
  else {
    setSubTitle(tr("Please specify the location of the data file containing the recipient email addresses"));
    m_label_csv_rules->hide();
  }
}

void
mailing_wizard_page_data_file::browse()
{
  QString s = QFileDialog::getOpenFileName(this);
  if (!s.isEmpty())
    m_filename->setText(s);
}

bool
mailing_wizard_page_data_file::validatePage()
{
  QString path = m_filename->text();
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString errmsg = QIODevice::tr(f.errorString().toLatin1().constData());
    QMessageBox::critical(NULL, tr("Error"), tr("Can't open file '%2': %1").arg(errmsg).arg(path));
    return false;
  }
  return true;
}

int
mailing_wizard_page_data_file::nextId() const
{
  return mailing_wizard::page_parse_data;
}

mailing_wizard_page_parse_data::mailing_wizard_page_parse_data()
{
  m_nb_lines=0;
  m_nb_addresses=0;
  m_nb_errors=0;

  QVBoxLayout* layout = new QVBoxLayout(this);

  m_result = new QLabel;
  m_result->setTextFormat(Qt::RichText);
  layout->addWidget(m_result);

  layout->addStretch(1);
  m_errors_label =  new QLabel(tr("Parse errors:"));
  layout->addWidget(m_errors_label);

  m_errors_textarea = new QPlainTextEdit;
  QFont font;
  font.setStyleHint(QFont::TypeWriter);
  font.setPointSize(10);
  m_errors_textarea->setFont(font);
  m_errors_textarea->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_errors_textarea->setReadOnly(true);
  
  layout->addWidget(m_errors_textarea);

  m_stop_text = new QLabel;
  layout->addWidget(m_stop_text);

  m_ignore_errors_checkbox = new QCheckBox();
  connect(m_ignore_errors_checkbox, SIGNAL(stateChanged(int)), this, SLOT(ignore_error_state_changed(int)));
  m_ignore_errors_text = new QLabel(tr("Discard the invalid lines and continue"));
  QHBoxLayout* hb = new QHBoxLayout();
  layout->addLayout(hb);
  hb->setMargin(2);
  hb->addWidget(m_ignore_errors_checkbox);
  hb->addWidget(m_ignore_errors_text);
  hb->addStretch(1);

  m_ignore_errors_checkbox->setVisible(false);
  m_ignore_errors_text->setVisible(false);
}

void
mailing_wizard_page_parse_data::ignore_error_state_changed(int state)
{
  Q_UNUSED(state);
  emit completeChanged();
}

void
mailing_wizard_page_parse_data::initializePage()
{
  if (field("addresses_from_textarea").toBool()) {
    setTitle(tr("Addresses syntax check"));
  }
  else {
    setTitle(tr("Data file check"));
  }
  setSubTitle(tr("Result of the analysis of the data"));

  m_nb_lines=0;
  m_nb_addresses=0;
  m_nb_errors=0;
  m_error_lines.clear();

  if (field("addresses_from_file").toBool() || field("mailmerge_from_file").toBool()) {
    QString path = field("addresses_filename").toString();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QMessageBox::critical(NULL, tr("Error"), tr("Can't open file '%2': error %1").arg(f.error()).arg(path));
      m_file_checked=false;
    }
    else {

      if (field("addresses_from_file").toBool()) {
	check_addresses_file(f);
      }
      else if (field("mailmerge_from_file").toBool()) {
	check_csv_file(f);
      }
      m_file_checked=true;
      f.close();
    }
  }

  else if (field("addresses_from_textarea").toBool()) {
    mailing_wizard* wz = static_cast<mailing_wizard*>(wizard());
    QStringList alist = wz->get_pasted_addresses();
    check_pasted_addresses(alist);
    m_file_checked=true;
  }

  QString result = tr("Number of lines: %1").arg(m_nb_lines) + "<br>" + tr("Number of email addresses: %1").arg(m_nb_addresses) + "<br>";
  if (m_recipient_list.isEmpty())
    result.append(tr("<b>Error:</b> no valid address has been found"));
  else {
    if (m_nb_errors>0)
      result.append(tr("<b>Number of errors:</b>&nbsp;<font color=red>%1</font>").arg(m_nb_errors));
    else
      result.append(tr("Number of errors: %1").arg(m_nb_errors));
  }

  if (field("mailmerge_from_file").toBool()) {
    // for CSV file, append the names of columns
    result.append("<p><b>" + tr("Column names:") + "</b><br>");
    mailing_options* opt = get_options();
    result.append(" ");
    result.append(opt->field_names.join("<br> "));
  }
  m_result->setText("<p>"+result);

  m_errors_textarea->clear();
  if (m_error_lines.size()>0) {
    for (int i=0; i<m_error_lines.size(); i++) {
      m_errors_textarea->appendPlainText(m_error_lines.at(i));
    }
    m_errors_textarea->show();
    m_errors_textarea->verticalScrollBar()->setValue(0);
    m_errors_label->show();
  }
  else {
    m_errors_textarea->hide();
    m_errors_label->hide();
  }

  if (m_nb_errors>0) {
    m_stop_text->setVisible(true);
    if (field("addresses_from_textarea").toBool())
      m_stop_text->setText(tr("The list of addresses is rejected. Please go back to the previous page and fix the errors."));
    else
      m_stop_text->setText(tr("The data file is rejected. You may go back to the previous page and submit a fixed data file."));
    if (m_nb_addresses > 0) {
      m_ignore_errors_checkbox->setVisible(true);
      m_ignore_errors_text->setVisible(true);
    }
  }
  else {
    m_stop_text->setVisible(false);
    m_ignore_errors_checkbox->setVisible(false);
    m_ignore_errors_text->setVisible(false);
  }

  emit completeChanged();
}

void
mailing_wizard_page_parse_data::check_csv_file(QFile& f)
{
  m_nb_lines=0;
  m_nb_addresses=0;
  m_nb_errors=0;
  m_error_lines.clear();
  m_recipient_list.clear();

  mailing_options *opt = get_options();
  opt->csv_data.clear();

  text_merger tm;
  try {
    tm.init(&f);
  }
  catch(QString error) {
    m_error_lines.append(error);
  }

  QString email_field_name = tm.column_name(0);
  // TODO check if email_field_name is empty?
  QString tmpl = QString("{%1}").arg(email_field_name);
  QString email;
  opt->field_names = tm.column_names();

  while (!f.atEnd()) {
    try {
      QStringList fields_data = tm.collect_data(&f);
      if (fields_data.isEmpty())
	continue;
      // discard empty lines (LF or CRLF alone). This is mostly useful
      // if encountered at the end
      if (fields_data.size()==1 && fields_data.at(0).isEmpty())
	continue;

      if (fields_data.size() != tm.nb_columns()) {
	throw QObject::tr("line %3: %1 columns found where %2 are expected").arg(fields_data.size()).arg(tm.nb_columns()).arg(tm.nb_lines());
      }

      email = fields_data.at(0);
      fields_data.removeFirst();

      // The first element of contents is the email address
      QStringList recipients; // may be filled with several addresses
      bool res=parse_line(email, recipients);
      if (!res || recipients.size()==0) {
	m_nb_errors++;
	m_error_lines << tr("line %1: %2").arg(tm.nb_lines()).arg(email);
      }
      else {
	m_recipient_list.append(recipients.join(","));
	m_nb_addresses += recipients.size();
	opt->csv_data.append(fields_data);
      }
    }
    catch(QString error) {
      m_error_lines.append(error);
      m_nb_errors++;
    }
  }
  m_nb_lines = tm.nb_lines();
}

QStringList&
mailing_wizard_page_parse_data::recipient_list()
{
  return m_recipient_list;
}

//static
bool
mailing_wizard_page_parse_data::parse_line(const QString ql, QStringList& recipients)
{
  std::list<QString> addresses;
  std::list<QString> comments;
  /* extract the addresses (multiple addresses separated by commas are accepted)
     without much syntax checking */
  int r=mail_address::ExtractAddresses(ql, addresses, comments);
  if (r==0) { // success
    /* do basic syntax checking on each address */
    for (std::list<QString>::iterator it = addresses.begin(); it!=addresses.end(); ++it) {
      if (!mail_address::basic_syntax_check(*it)) {
	r=1;
	break;
      }
      else {
	recipients.append(*it);
      }
    }
  }
  return r==0;
}

void
mailing_wizard_page_parse_data::check_addresses_file(QFile& f)
{
  m_nb_lines=0;
  m_nb_addresses=0;
  m_nb_errors=0;
  m_error_lines.clear();
  m_recipient_list.clear();

  QTextCodec* codec = QTextCodec::codecForLocale();

  while (!f.atEnd()) {
    m_nb_lines++;
    QByteArray line = f.readLine();
    QString ql = codec ? codec->toUnicode(line).trimmed() :
      QString::fromLatin1(line).trimmed();
    if (ql.isEmpty())
      continue;

    QStringList recipients; // may be filled with several addresses
    bool res=parse_line(ql, recipients);
    if (!res || recipients.size()==0) {
      m_nb_errors++;
      m_error_lines << tr("line %1: %2").arg(m_nb_lines).arg(ql);
    }
    else {
      m_recipient_list.append(recipients.join(","));
      m_nb_addresses += recipients.size();
    }
  }
}

void
mailing_wizard_page_parse_data::check_pasted_addresses(const QStringList& alist)
{
  m_nb_lines=0;
  m_nb_addresses=0;
  m_nb_errors=0;
  m_error_lines.clear();
  m_recipient_list.clear();

  for (QStringList::const_iterator iter = alist.constBegin(); iter!=alist.end(); ++iter) {
    QString ql = (*iter).trimmed();
    m_nb_lines++;
    if (ql.isEmpty())
      continue;

    QStringList recipients; // may be filled with several addresses
    bool res=parse_line(ql, recipients);
    if (!res || recipients.size()==0) {
      m_nb_errors++;
      m_error_lines << tr("line %1: %2").arg(m_nb_lines).arg(ql);
    }
    else {
      m_recipient_list.append(recipients.join(","));
      m_nb_addresses += recipients.size();
    }
  }
}

bool
mailing_wizard_page_parse_data::validatePage()
{
  return (m_nb_errors==0 || m_ignore_errors_checkbox->isChecked())
    && m_file_checked && m_recipient_list.size()>0;
}

bool
mailing_wizard_page_parse_data::isComplete() const
{
  return mailing_wizard_page::isComplete() &&
    (m_nb_errors==0 || m_ignore_errors_checkbox->isChecked()) &&
    m_file_checked &&
    m_recipient_list.size()>0;  
}

int
mailing_wizard_page_parse_data::nextId() const
{
  return mailing_wizard::page_final;
}

double_file_input::double_file_input(QWidget* parent) : QWidget(parent)
{
  QGridLayout* grid = new QGridLayout(this);

  m_label_html = new QLabel(tr("File for html part:"));
  grid->addWidget(m_label_html, 0, 0); // row,column
  m_filename_html = new QLineEdit;
  grid->addWidget(m_filename_html, 0, 1);
  m_btn_browse1 = new QPushButton(tr("Browse"));
  grid->addWidget(m_btn_browse1, 0, 2);
  connect(m_btn_browse1, SIGNAL(clicked()), this, SLOT(browse_html_file()));

  m_label_text = new QLabel(tr("File for text part:"));
  grid->addWidget(m_label_text, 1, 0);
  m_filename_text = new QLineEdit;
  grid->addWidget(m_filename_text, 1, 1);
  m_btn_browse2 = new QPushButton(tr("Browse"));
  grid->addWidget(m_btn_browse2, 1, 2);
  m_label_filename_text = new QLabel(tr("Leave the above field blank to auto-generate the text part from the html part"));
  QFont font = m_label_filename_text->font();
  font.setPointSize((font.pointSize()*8)/10);
  m_label_filename_text->setFont(font);
  grid->addWidget(m_label_filename_text, 3, 1);
  connect(m_btn_browse2, SIGNAL(clicked()), this, SLOT(browse_text_file()));

  grid->setColumnStretch(0,0); // label
  grid->setColumnStretch(1,1); // lineedit
  grid->setColumnStretch(2,0); // button
}

/* adjust the visibility of html and/or text file lineedits according to
   the mail format the is currently selected */
void
double_file_input::format_requested(mailing_db::template_format format)
{
  switch(format) {
  case mailing_db::format_text_plain:
    m_filename_text->show();
    m_label_text->show();
    m_btn_browse2->show();
    m_filename_html->hide();
    m_label_filename_text->hide();
    m_btn_browse1->hide();
    m_label_html->hide();
    break;
  case mailing_db::format_html_and_text:
    m_filename_text->show();
    m_label_text->show();
    m_btn_browse2->show();
    m_filename_html->show();
    m_label_filename_text->show();
    m_btn_browse1->show();
    m_label_html->show();
    break;
  case mailing_db::format_html_only:
    m_filename_text->hide();
    m_label_text->hide();
    m_btn_browse2->hide();
    m_filename_html->show();
    m_label_filename_text->hide();
    m_btn_browse1->show();
    m_label_html->show();
    break;    
  }
}


/* return values:
0= nothing submitted
1=only filename for html is submitted
2=only filename for text is submitted
3=html and text filenames are both submitted
*/
int
double_file_input::submitted_files()
{
  if (m_filename_html->text().isEmpty())
    return (m_filename_text->text().isEmpty() ? 0 : 2);
  else
    return (m_filename_text->text().isEmpty() ? 1 : 3);
}

/* display an error and return false if one specified file can't be opened
 arg. 'which' is 1=html and text, 2=html only, 3=text only */
bool
double_file_input::check_files(int which)
{
  if ((which==1 || which==2) &&
      !check_file_existence(m_filename_html->text()))
    return false;

  return check_file_existence(m_filename_text->text());
}

bool
double_file_input::check_file_existence(const QString path)
{
  if (!path.isEmpty()) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString errmsg = QIODevice::tr(f.errorString().toLatin1().constData());
      QMessageBox::critical(NULL, tr("Error"), tr("Can't open file '%2': %1").arg(errmsg).arg(path));
      return false;
    }
  }
  return true;
}

void
double_file_input::browse_html_file()
{
  QString filter = tr("Html Files (*.htm *.html);;All Files (*)");
  QString s = QFileDialog::getOpenFileName(this, tr("Locate HTML file"), QString(), filter);
  if (!s.isEmpty())
    m_filename_html->setText(s);
}

void
double_file_input::browse_text_file()
{
  QString filter = tr("Text Files (*.txt);;All Files (*)");
  QString s = QFileDialog::getOpenFileName(this, tr("Locate text file"), QString(), filter);
  if (!s.isEmpty())
    m_filename_text->setText(s);
}

mailing_wizard_page_template::mailing_wizard_page_template()
{
  setTitle(tr("Choose a template"));
  setSubTitle(tr("The template is the body of the message, with optional placeholders for variable text"));
  QVBoxLayout* layout = new QVBoxLayout(this);

  QGroupBox* grb = new QGroupBox(tr("Message format"));
  QButtonGroup* grp = new QButtonGroup;
  QVBoxLayout* blayout = new QVBoxLayout();

  m_rb[0] = new QRadioButton(tr("Html (rich text) and alternate plain text part"));
  m_rb[0]->setChecked(true);
  m_format = mailing_db::format_html_and_text;
  grp->addButton(m_rb[0], (int)mailing_db::format_html_and_text);
  blayout->addWidget(m_rb[0]);

  m_rb[1] = new QRadioButton(tr("Html (rich text) only"));
  grp->addButton(m_rb[1], (int)mailing_db::format_html_only);
  blayout->addWidget(m_rb[1]);

  m_rb[2] = new QRadioButton(tr("Plain text only"));
  grp->addButton(m_rb[2], (int)mailing_db::format_text_plain);
  blayout->addWidget(m_rb[2]);

  grb->setLayout(blayout);
  grp->setExclusive(true);
  connect(grp, SIGNAL(buttonClicked(int)), this, SLOT(format_chosen(int)));
  layout->addWidget(grb);
  layout->setStretchFactor(grb, 0);

  QGroupBox* grb1 = new QGroupBox(tr("Message body (template)"));
  QButtonGroup* grp1 = new QButtonGroup;

  m_rb_file_template = new QRadioButton(tr("From file(s)"));
  m_rb_database_template = new QRadioButton(tr("From a saved message template"));
  m_rb_previous_mailing_template = new QRadioButton(tr("Copy from a previous mailing"));

  grp1->addButton(m_rb_file_template, 1);
  grp1->addButton(m_rb_database_template, 2);
  grp1->addButton(m_rb_previous_mailing_template, 3);

  QVBoxLayout* bl1 = new QVBoxLayout();
  bl1->addWidget(m_rb_file_template);
  bl1->addWidget(m_rb_database_template);
  bl1->addWidget(m_rb_previous_mailing_template);
  grb1->setLayout(bl1);
  grp1->setExclusive(true);
  m_rb_file_template->setChecked(true);
  connect(grp1, SIGNAL(buttonClicked(int)), this, SLOT(template_source_chosen(int)));
  layout->addWidget(grb1);
  layout->setStretchFactor(grb1, 0);


  QWidget* container = new QWidget;
  QVBoxLayout* sub_layout = new QVBoxLayout(container);
  layout->addWidget(container);
  layout->setStretchFactor(container, 1);

  m_wpart_files = new double_file_input;
  sub_layout->addWidget(m_wpart_files);

  // list of template titles with the template ID as user data
  m_label_template = new QLabel(tr("Stored templates"));
  sub_layout->addWidget(m_label_template);
  m_wtmpl = new QListWidget;
  sub_layout->addWidget(m_wtmpl);
  mail_template_list mtl;
  mtl.fetch_titles();
  for (int i=0; i<mtl.size(); i++) {
    m_wtmpl->addItem(mtl.at(i).m_title);
    QListWidgetItem* item = m_wtmpl->item(i);
    item->setData(Qt::UserRole, QVariant(mtl.at(i).m_template_id));
  }
  registerField("db_template", m_wtmpl);

  // list of previous mailings
  m_label_mailings = new QLabel(tr("Previous mailings"));
  sub_layout->addWidget(m_label_mailings);
  m_wmailings = new QListWidget;
  sub_layout->addWidget(m_wmailings);
  mailings mls;
  if (mls.load()) {
    for (int i=0; i<mls.size(); i++) {
      m_wmailings->addItem(mls.at(i).title());
      QListWidgetItem* item = m_wmailings->item(i);
      item->setData(Qt::UserRole, QVariant(mls.at(i).id()));
    }
  }
  registerField("previous_mailing", m_wmailings);
  layout->addStretch(5);

  template_source_chosen(1);
}

void
mailing_wizard_page_template::hide_all()
{
  QWidget* widgets[] = {m_wpart_files, m_wtmpl, m_wmailings, m_label_template,
			m_label_mailings };
  for (uint i=0; i<sizeof(widgets)/sizeof(widgets[0]); i++) {
    widgets[i]->setVisible(false);
  }
}

void
mailing_wizard_page_template::format_chosen(int btn_id)
{
  m_format = static_cast<mailing_db::template_format>(btn_id);
  m_wpart_files->format_requested(m_format);
}

void
mailing_wizard_page_template::template_source_chosen(int btn_id)
{
  hide_all();
  switch(btn_id) {
  case 1:
    m_wpart_files->setVisible(true);
    break;
  case 2:
    m_label_template->setVisible(true);
    m_wtmpl->setVisible(true);
    break;
  case 3:
    m_label_mailings->setVisible(true);
    m_wmailings->setVisible(true);
    break;
  }
  m_template_source=(tmpl_template_source)btn_id;
}

int
mailing_wizard_page_template::selected_template_id()
{
  QListWidgetItem* item = m_wtmpl->currentItem();
  return item ? item->data(Qt::UserRole).toInt() : 0;
}

int
mailing_wizard_page_template::selected_mailing_id()
{
  QListWidgetItem* item = m_wmailings->currentItem();
  return item ? item->data(Qt::UserRole).toInt() : 0;
}

mailing_wizard_page_template::tmpl_template_source
mailing_wizard_page_template::template_source()
{
  return m_template_source;
}


QString
mailing_wizard_page_template::html_template_filename()
{
  return m_wpart_files->m_filename_html->text();
}

QString
mailing_wizard_page_template::text_template_filename()
{
  return m_wpart_files->m_filename_text->text();
}


mailing_db::template_format
mailing_wizard_page_template::body_format() const
{
  return m_format;
}


// load the HTML template from a file. In case of error, may throw a
// QString with an error message
void
mailing_wizard_page_template::load_file_templates(const QString filename_text, 
						  const QString filename_html, 
						  QString* text, QString* html)
{
  *text=QString();
  *html=QString();
  if (!filename_text.isEmpty()) {
    QFile f(filename_text);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString errmsg = QIODevice::tr(f.errorString().toLatin1().constData());
      throw errmsg;
    }
  
    QByteArray contents = f.readAll();
    *text = QTextCodec::codecForLocale()->toUnicode(contents);
    f.close();
  }

  if (!filename_html.isEmpty()) {
    QFile f(filename_html);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString errmsg = QIODevice::tr(f.errorString().toLatin1().constData());
      throw errmsg;
    }
  
    QByteArray contents = f.readAll();
    *html = QTextCodec::codecForLocale()->toUnicode(contents);
    f.close();
  }
}

void
mailing_wizard_page_template::load_templates(mailing_options* opt)
{
    mail_template tmpl;
    mailing_wizard_page_template* wzpt = this;

    mailing_db::template_format format = wzpt->body_format();
    mailing_wizard_page_template::tmpl_template_source source = wzpt->template_source();
    if (source==mailing_wizard_page_template::template_source_db_tmpl) {
      // load template from database
      int tmpl_id = wzpt->selected_template_id();
      if (tmpl_id!=0) {
	if (tmpl.load(tmpl_id)) {
	  if (format==mailing_db::format_text_plain || format==mailing_db::format_html_and_text) {
	    opt->text_part = tmpl.text_body();
	  }
	  else
	    opt->text_part.truncate(0);

	  if (format==mailing_db::format_html_and_text || format==mailing_db::format_html_only) {
	    opt->html_part = tmpl.html_body();
	  }
	  else
	    opt->html_part.truncate(0);
	}
	else
	  throw tr("Unable to load template #%1 from database").arg(tmpl_id);
      }
      else
	throw tr("No template selected");
    }
    else if (source==mailing_wizard_page_template::template_source_files) {
      // load template from file
      load_file_templates(wzpt->text_template_filename(),
			  wzpt->html_template_filename(),
			  &opt->text_part,
			  &opt->html_part);
      if (wzpt->body_format()==mailing_db::format_html_and_text && wzpt->text_template_filename().isEmpty()) {
	// generate text part from html
	opt->text_part=generate_text_part(opt->html_part);
      }
    }
    else if (source==mailing_wizard_page_template::template_source_prev_mailing) {
      // load template from previous mailing
      int mlg_id = wzpt->selected_mailing_id();

      if (mlg_id!=0) {
	mailing_db mailing(mlg_id);
	mailing_options mo;
	if (mailing.load_definition(&mo)) {
	  switch(format) {
	  case mailing_db::format_text_plain:
	    opt->text_part = mo.text_part;
	    opt->html_part.truncate(0);
	    break;
	  case mailing_db::format_html_and_text:
	    opt->text_part = mo.text_part;
	    opt->html_part = mo.html_part;
	    if (opt->text_part.isEmpty())
	      opt->text_part=generate_text_part(opt->html_part);
	    break;
	  case mailing_db::format_html_only:
	    opt->html_part = mo.html_part;
	    opt->text_part.truncate(0);
	    break;
	  }
	}
	else
	  throw tr("Unable to load mailing #%1 from database").arg(mlg_id);
      }
      else
	throw tr("No mailing selected");
    }
}

bool
mailing_wizard_page_template::validatePage()
{
  if (m_template_source == template_source_db_tmpl) {
    int tmpl_id = selected_template_id();
    if (tmpl_id==0)
      return false;

    // Check that the template has the format that the user requested
    mail_template tmpl;
    if (tmpl.load(tmpl_id)) {
      if (m_format==mailing_db::format_text_plain && tmpl.text_body().isEmpty()) {
	QMessageBox::critical(NULL, tr("Error"), tr("The selected template doesn't have a text part"));
	return false;
      }
      if (m_format==mailing_db::format_html_only && tmpl.html_body().isEmpty()) {
	QMessageBox::critical(NULL, tr("Error"), tr("The selected template doesn't have an html part"));	
	return false;
      }
    }
    else {
      QMessageBox::critical(NULL, tr("Error"), tr("Unable to load the selected template"));
      return false;
    }
  }
  else if (m_template_source == template_source_files) {
    int subm = m_wpart_files->submitted_files();
    QString errmsg;

    int which=3;
    switch(m_format) {
    case mailing_db::format_html_only:
      which=2;
      if (subm!=1 && subm!=3)
	errmsg=tr("Please indicate the location of a file containing the HTML part of the message");
      break;

    case mailing_db::format_html_and_text:
      which=1;
      if (subm!=1 && subm!=3)
	errmsg=tr("Please indicate the location of both files, one containing the plain text part and the other the HTML part of the message");
      // (actually the text part can be left blank but the errmsg is long enough already)
      break;

    case mailing_db::format_text_plain:
      which=3;
      if (subm!=2 && subm!=3)
	errmsg=tr("Please indicate the location of a file containing the plain text part of the message");
      break;
    }
    if (!errmsg.isEmpty()) {
      QMessageBox::critical(NULL, tr("Missing filename"), errmsg);
      return false;
    }
    if (!m_wpart_files->check_files(which))
      return false;
  }
  else if (m_template_source == template_source_prev_mailing) {
    int mlg_id = selected_mailing_id();
    if (mlg_id==0)
      return false;

    // Check that the mailing has the format that the user requested
    mailing_db mailing(mlg_id);
    mailing_options mo;
    if (mailing.load_definition(&mo)) {
      if (m_format==mailing_db::format_text_plain && mo.text_part.isEmpty()) {
	QMessageBox::critical(NULL, tr("Error"), tr("The selected previous mailing doesn't have a text part"));
	return false;
      }
      if ((m_format==mailing_db::format_html_only || m_format==mailing_db::format_html_and_text) && mo.html_part.isEmpty()) {
	QMessageBox::critical(NULL, tr("Error"), tr("The selected previous mailing doesn't have an html part"));	
	return false;
      }
    }
    else {
      QMessageBox::critical(NULL, tr("Error"), tr("Unable to retrieve the selected previous mailing"));
      return false;
    }
  }

  // Clear the filenames that are not needed, so that the rest of the process
  // can use filename.isEmpty() as the test
  switch(m_format) {
  case mailing_db::format_html_only:
    m_wpart_files->m_filename_text->clear();
    break;
  case mailing_db::format_text_plain:
    m_wpart_files->m_filename_html->clear();
    break;
  case mailing_db::format_html_and_text:
    break;
  }

  load_templates(get_options());

  return mailing_wizard_page::validatePage();
}

mailing_wizard_page_final::mailing_wizard_page_final()
{
  setTitle(tr("Finish the mailing setup"));
  setSubTitle(tr("Preview the messages and choose to start the mailing immediately or later."));
  int row=0;
  QGridLayout* layout = new QGridLayout(this);
  layout->setColumnStretch(0,0);
  layout->setColumnStretch(1,1);

  row++;
  QPushButton* view_btn = new QPushButton(tr("Preview"));
  layout->addWidget(view_btn, row, 0);
  connect(view_btn, SIGNAL(clicked()), this, SLOT(preview_mailing()));

  QLabel* preview_text = new QLabel(tr("Final check in a preview window on how each message will appear"));
  QFont mini_font = preview_text->font();
  mini_font.setPointSize((mini_font.pointSize()*8)/10);
  preview_text->setFont(mini_font);
  layout->addWidget(preview_text, row, 1);

  row++;
  QRadioButton* b1 = new QRadioButton(tr("Launch immediately"));
  QRadioButton* b2 = new QRadioButton(tr("Launch later"));
  b2->setChecked(true);
  layout->addWidget(b1, row, 0);
  row++;
  layout->addWidget(b2, row, 0);
  registerField("launch_now", b1);

  row++;
  m_progress_label = new QLabel(tr("Data import"));
  layout->addWidget(m_progress_label, row, 0);
  m_progress_label->setVisible(false);
  m_progress_bar = new QProgressBar();
  layout->addWidget(m_progress_bar, row, 0);
  m_progress_bar->setVisible(false);
}

void
mailing_wizard_page_final::preview_mailing()
{
  emit preview();
}


mailing_wizard_page_error_identities::mailing_wizard_page_error_identities()
{
  setTitle(tr("No sender identity defined"));
  QVBoxLayout* layout = new QVBoxLayout(this);
  QLabel* label = new QLabel(tr("To be able to create a mailing, please go first in the Preferences/Identities and define at least one sender identity with an email address."));
  layout->addWidget(label);
}

bool
mailing_wizard_page_error_identities::isComplete() const
{
  return false;
}

mailing_wizard::mailing_wizard()
{
  setWindowTitle(tr("Mailing setup"));

  identities ids;
  if (!ids.fetch() || ids.empty()) {
    setPage(page_error_identities, new mailing_wizard_page_error_identities);
  }
  else {
    setPage(page_title, new mailing_wizard_page_title);
    setPage(page_template, new mailing_wizard_page_template);
    setPage(page_import_data, new mailing_wizard_page_import_data);
    setPage(page_data_file, new mailing_wizard_page_data_file);
    setPage(page_parse_data, new mailing_wizard_page_parse_data);
    setPage(page_paste_address_list, new mailing_wizard_page_paste_address_list);
    mailing_wizard_page_final* p = new mailing_wizard_page_final;
    setPage(page_final, p);
    connect(p, SIGNAL(preview()), this, SLOT(preview_mailmerge()));
  }
}

void
mailing_wizard::preview_mailmerge()
{
  try {
    gather_options();
  }
  catch(QString errmsg) {
    QMessageBox::critical(NULL, tr("Error"), errmsg);
    return;
  }

  mailing_viewer* w = new mailing_viewer(this);
  w->setWindowModality(Qt::WindowModal);
  w->set_data(m_options);
  w->show_merge();
}


QString
mailing_wizard_page_template::generate_text_part(const QString html)
{
  QWebPage page;
  QWebFrame* frame = page.currentFrame();
  frame->setHtml(html);
  QString t = frame->toPlainText();
  t.replace(QChar(0xa0), QChar(0x20)); // translate &nbsp; to space, not unbreakable space
  return t;
}

// throws QString
void
mailing_wizard::gather_options()
{
  m_options.title=field("mailing_title").toString();
  mailing_wizard_page_title* wzpf = static_cast<mailing_wizard_page_title*>(page(page_title));
  mail_address addr = wzpf->sender_address();
  m_options.sender_email = addr.email();

  QString mail_subject = mailing_db::filter_header_line(field("subject").toString());
  m_options.header = QString("From: %1\nSubject: %2").
    arg(addr.email_and_name()).
    arg(mail_subject);

  mailing_wizard_page_template* wzpt = static_cast<mailing_wizard_page_template*>(page(page_template));
  wzpt->load_templates(&m_options);
  mailing_wizard_page_parse_data* wzpp = static_cast<mailing_wizard_page_parse_data*>(page(page_parse_data));
  m_options.mail_addresses = wzpp->recipient_list();
}


bool
mailing_wizard::launch_mailing()
{
  db_cnx db;
  try {
    db.begin_transaction();
    gather_options();

    // instantiate the job
    mailing_db mailing;
    bool failed=true;
    mailing_wizard_page_final* wzpf = static_cast<mailing_wizard_page_final*>(page(page_final));
    mailing.set_status(field("launch_now").toBool() ?
		       mailing_db::status_running:
		       mailing_db::status_not_started);

    if (mailing.create(m_options)) {
      wzpf->m_progress_label->show();
      wzpf->m_progress_bar->show();
      if (mailing.store_data(m_options.mail_addresses, m_options.csv_data, wzpf->m_progress_bar)) {
	if (field("launch_now").toBool())
	  failed = !mailing.instantiate_job();
	else
	  failed=false;
      }
    }
    if (failed)
      throw tr("The launch of the mailing job has failed.");

    db.commit_transaction();
  }
  catch(QString errmsg) {
    db.rollback_transaction();
    QMessageBox::critical(NULL, tr("Error"), errmsg);
    return false;
  }
  return true;
}

bool
mailing_wizard::validateCurrentPage()
{
  if (!currentPage())
    return false;
  if (currentPage()->validatePage()) {
    if (currentId() == page_final) {
      return launch_mailing();
    }
    return true;
  }
  else
    return false;
}

QStringList
mailing_wizard::get_pasted_addresses()
{
  mailing_wizard_page_paste_address_list* wzp = static_cast<mailing_wizard_page_paste_address_list*>(page(page_paste_address_list));
  return wzp->get_addresses();
}

mailing_options*
mailing_wizard_page::get_options()
{
  mailing_wizard* z = static_cast<mailing_wizard*>(wizard());
  return z ? &z->m_options : NULL;
}
