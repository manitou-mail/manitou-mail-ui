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
#include "mailing.h"
#include "mail_template.h"
#include "text_merger.h"
#include "mail_displayer.h"
#include "mailing_viewer.h"
#include "icons.h"

#include <QWebPage>
#include <QWebFrame>
#include <QWebView>
#include <QRadioButton>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QUrl>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QFontMetrics>

mailing_viewer::mailing_viewer(QWidget* parent, Qt::WindowFlags f) : QWidget(parent,f)
{
  m_options=NULL;

  setWindowTitle(tr("Mailing preview"));
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  QHBoxLayout* btn_layout = new QHBoxLayout;

  btn_layout->addWidget(new QLabel(tr("Show part:")));
  m_rb_text = new QRadioButton(tr("Plain text"));
  m_rb_html = new QRadioButton(tr("Rich text (HTML)"));
  connect(m_rb_text, SIGNAL(clicked()), this, SLOT(to_plain_text()));
  connect(m_rb_html, SIGNAL(clicked()), this, SLOT(to_html()));

  m_btn_prev = new QPushButton(UI_ICON(FT_ICON16_ARROW_LEFT), "");
  m_btn_prev->setToolTip(tr("Previous message"));
  m_btn_next = new QPushButton(UI_ICON(FT_ICON16_ARROW_RIGHT), "");
  m_btn_next->setToolTip(tr("Next message"));
  connect(m_btn_prev, SIGNAL(clicked()), this, SLOT(prev_message()));
  connect(m_btn_next, SIGNAL(clicked()), this, SLOT(next_message()));
  
  m_number = new QLineEdit;
  m_number->setToolTip(tr("Enter the message number and press return"));
  connect(m_number, SIGNAL(returnPressed()), SLOT(goto_message()));
  m_label_count = new QLabel;
  btn_layout->addWidget(m_rb_text);
  btn_layout->addWidget(m_rb_html);
  btn_layout->addStretch(1);
  btn_layout->addWidget(m_btn_prev);
  btn_layout->addWidget(m_number);
  btn_layout->addWidget(m_label_count);
  btn_layout->addWidget(m_btn_next);
  
  top_layout->addLayout(btn_layout);

  m_webview = new QWebView();
  connect(m_webview, SIGNAL(loadFinished(bool)), this, SLOT(load_finished(bool)));
  top_layout->addWidget(m_webview);

  QHBoxLayout* btn2_layout = new QHBoxLayout;
  m_btn_close = new QPushButton(tr("Close"));
  btn2_layout->addStretch(1);
  btn2_layout->addWidget(m_btn_close);
  btn2_layout->addStretch(1);
  connect(m_btn_close, SIGNAL(clicked()), this, SLOT(close()));
  top_layout->addLayout(btn2_layout);
}

mailing_viewer::~mailing_viewer()
{
  if (m_options_owned && m_options)
    delete m_options;
}

void
mailing_viewer::load_finished(bool ok)
{
  Q_UNUSED(ok);
  prepend_body_fragment(htmlize_header(m_current_header));  
}

void
mailing_viewer::prepend_body_fragment(const QString& fragment)
{
  QWebPage* page = m_webview->page();
  page->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  QString s = fragment;
  s.replace("'", "\\'");
  QString js = QString("try {var b=document.getElementsByTagName('body')[0]; var p=document.createElement('div'); p.innerHTML='%1'; p.style.marginBottom='12px'; b.insertBefore(p, b.firstChild); 1;} catch(e) { e; }").arg(s);
  QVariant v = page->mainFrame()->evaluateJavaScript(js);
  page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

QString
mailing_viewer::htmlize_header(const QString h)
{
  uint npos=0;
  uint nend=h.length();
  QString output;

  // show all headers
  while (npos<nend) {
    int nposeh=0;		// end of header name
    // position of end of current line
    int nposlf=h.indexOf('\n',npos);
    if (nposlf==-1)
      nposlf=nend;
    if (h.mid(npos,1)!=" " && h.mid(npos,1)!="\t" && (nposeh=h.indexOf(':',npos))>0)
    {
      QString c=h.mid(nposeh+1, nposlf-nposeh-1);
      c.replace("&", "&amp;");
      c.replace("<", "&lt;");
      c.replace(">", "&gt;");
      c.replace ("\t", " ");
      output.append(QString("<font color=\"blue\">%1</font>%2<br>")
		    .arg(h.mid(npos,nposeh-npos+1)).arg(c));
    }
    else {
      output.append(h.mid(npos,nposlf-npos+1));
      output.append("<br>");
    }
    npos=nposlf+1;
  }
  output.append("<hr>");
  return output;
}

/*
mailing_options:
  QString title;
  QString sender_email;
  QStringList mail_addresses;
  QStringList field_names;
  int mail_format;
  //  double throughput; // LATER
  QString header;
  QString text_part;
  QString html_part;
*/  
void
mailing_viewer::show_merge()
{
  bool merged=false;
  int data_idx=m_current_index;

  mailing_options* opt = m_options;

  if (data_idx >= opt->mail_addresses.size()) {
    data_idx = opt->mail_addresses.size()-1;
  }
  if (data_idx < 0)
    return;

  if (!opt->csv_data.isEmpty()) {
    text_merger tm;
    try {
      tm.init(opt->field_names, ',');
      // reinsert the recipient email address as the variable at index 0
      QStringList vars = opt->csv_data.at(data_idx);
      vars.insert(0, opt->mail_addresses.at(data_idx));
      // merge
      m_current_text = tm.merge_template(opt->text_part, vars);
      m_current_html = tm.merge_template(opt->html_part, vars);
      /* filter out cr and lf from values to merge into the header since
	 line breaks are not supported in headers */
      QStringList vars_headers;
      for (int i=0; i<vars.size(); i++) {
	QString v=vars[i].replace("\r\n", "\n");
	v=v.replace('\r', '\n');
	v=v.replace('\n', ' ');
	vars_headers.append(v);
      }
      m_current_header = tm.merge_template(opt->header, vars_headers);
      merged=true;
    }
    catch(...) {
      merged=false;
    }
  }
  if (!merged) {
    m_current_text = opt->text_part;
    m_current_html = opt->html_part;
    m_current_header = opt->header;
  }

  m_current_header.append(QString("\nTo: %1\n").arg(opt->mail_addresses.at(data_idx)));

  if (!m_current_text.isEmpty()) {
    m_current_text.replace("&", "&amp;");
    m_current_text.replace(">", "&gt;");
    m_current_text.replace("<", "&lt;");
    m_current_text.replace("\n", "<br>");
  }

  if (m_rb_html->isChecked()) {
    m_webview->setHtml(m_current_html, QUrl("/"));
  }
  else {
    m_webview->setHtml(m_current_text, QUrl("/"));
  }

  // m_current_header is added dynamically at the start of <body> in
  // load_finished()
  show();
}

void
mailing_viewer::to_html()
{
  m_webview->setHtml(m_current_html, QUrl("/"));
}

void
mailing_viewer::to_plain_text()
{
  m_webview->setHtml(m_current_text, QUrl("/"));
}

void
mailing_viewer::next_message()
{
  if (m_options!=NULL && m_current_index < m_options->mail_addresses.size()-1) {
    m_current_index++;
    show_number(m_current_index+1);
    show_merge();
  }
}

void
mailing_viewer::prev_message()
{
  if (m_options!=NULL && m_current_index>0) {
    m_current_index--;
    show_number(m_current_index+1);
    show_merge();
  }
}

void
mailing_viewer::show_number(int number)
{
  m_number->setText(QString("%1").arg(number));
}

void
mailing_viewer::goto_message()
{
  bool ok;
  int new_index = m_number->text().toInt(&ok);
  if (!ok) {
    QMessageBox::critical(this, tr("Error"), tr("Unrecognized number"));
    show_number(m_current_index+1);
    return;
  }
  new_index--;
  if (m_options!=NULL && (new_index<0 || new_index>=m_options->mail_addresses.size())) {
    QMessageBox::critical(this, tr("Error"), tr("The message number must be between 1 and %1").arg(m_options->mail_addresses.size()));
    return;
  }

  m_current_index = new_index;
  show_merge();
}

void
mailing_viewer::set_data(mailing_options& opt)
{
  m_options = &opt;
  m_options_owned = false;
  init();
}

void
mailing_viewer::init()
{
  QString max_nb = QString("%1").arg(m_options->mail_addresses.size());
  QFontMetrics fm(m_number->font());
  m_number->setMaximumWidth(fm.width(max_nb)+15);
  m_label_count->setText(QString("/%1").arg(m_options->mail_addresses.size()));

  if (!m_options->html_part.isEmpty()) {
    m_rb_html->setChecked(true);
    m_rb_html->setEnabled(true);
  }
  else {
    m_rb_html->setEnabled(false);
    m_rb_text->setChecked(true);
  }

  if (!m_options->text_part.isEmpty()) {
    m_rb_text->setEnabled(true);
  }
  else
    m_rb_text->setEnabled(false);

  m_current_index=0;
  show_number(1);
}


/* View a mailing that is already stored inside the database
   (as opposed to a preview) */
void
mailing_viewer::show_merge_existing(mailing_db* m)
{
  m_options = new mailing_options;
  if (!m_options)
    return;
  m_options_owned = true;
  if (!m->load_definition(m_options))
    return;
  try {
    m->load_data(m_options);
  }
  catch (QString errmsg) {
    QMessageBox::critical(this, tr("Error"), tr("An error has occurred when fetching the mailing data:\n%1").arg(errmsg));
  }
  init();
  show_merge();
}
