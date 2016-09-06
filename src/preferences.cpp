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

#include "preferences.h"
#include "helper.h"
#include "identities.h"
#include "app_config.h"
#include "tags.h"

#include <QTabWidget>
#include <QComboBox>
#include <QLabel>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QDialog>
#include <QApplication>
#include <QPushButton>
#include <QValidator>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTreeWidget>
#include <QButtonGroup>
#include <QStyleFactory>

#include "db.h"
#include "ui_controls.h"
#include "icons.h"

class mt_dialog : public QDialog
{
public:
  mt_dialog();
  ~mt_dialog();
};

mt_dialog::mt_dialog() : QDialog(NULL)
{
  
}

mt_dialog::~mt_dialog()
{
}

struct prefs_dialog::preferences_widgets {
  // identities tab
  QLineEdit* w_email;
  QLineEdit* w_name;
  tag_line_edit_selector* w_root_tag;
  QLineEdit* w_xface;
  QPlainTextEdit* w_signature;
  QComboBox* w_ident_cb;
  QPushButton* w_del_ident;
  QCheckBox* w_restricted;
  QCheckBox* w_default_identity;
  /* The email address that corresponds to the default identity. The
     value is overwritten when the "use as default" checkbox is
     checked for an identity, so that the final value should be the one for the
     'default_identity' configuration entry, no matter the final values of the
     m_default fields */
  QString m_default_email;

  // display tab
  button_group* w_show_tags;
  button_group* w_messages_order;
  button_group* w_display_threaded;
  button_group* w_display_sender;
  button_group* w_clickable_urls;
  button_group* w_preferred_format;
  button_group* w_dynamic_sender_column;
  QComboBox* w_date_format;
  button_group* w_notifications;
  QComboBox* w_style;

  // directories tab
  QLineEdit* w_attachments_dir;
  QLineEdit* w_xpm_dir;
  QLineEdit* w_help_dir;
  QLineEdit* w_browser_path;

  // fetching tab
  QLineEdit* w_initial_fetch;
  QLineEdit* w_fetch_ahead;
  QLineEdit* w_max_connections;
  QSpinBox* w_auto_check;
  QSpinBox* w_auto_refresh;
  QCheckBox* w_auto_incorporate;
  

  // mime types tab
  std::map<QString,QString> suffix_map;	// mime_type=>suffix
  std::map<QLineEdit*,QLineEdit*> ql_map; // mime_type=>suffix for widgets
  //QWidget* mimetypes_container;
  QTreeWidget* mt_lv;		// mimetypes listview

  // composer tab
  button_group* w_composer_format_new_mail;
  button_group* w_composer_format_replies;
  button_group* w_composer_address_check;

  // search tab
  button_group* w_search_accents;
};

viewer_edit_dialog::viewer_edit_dialog(QWidget*parent, const QString& mt, const QString& prog)
  : QDialog(parent)
{
  setWindowTitle(tr("Viewer program"));
  QVBoxLayout* tl = new QVBoxLayout(this);
  QGridLayout* layout = new QGridLayout();
  tl->addLayout(layout);
  layout->setMargin(5);
  int row=0;
  QLabel* l1 = new QLabel(tr("Mime Type"), this);
  m_mimetype = new QLineEdit(this);
  m_mimetype->setText(mt);
  layout->addWidget(l1, row, 0);
  layout->addWidget(m_mimetype, row, 1);

  row++;
  QLabel* l2 = new QLabel(tr("Program"), this);
  m_viewer = new QLineEdit(this);
  m_viewer->setText(prog);
  layout->addWidget(l2, row, 0);
  layout->addWidget(m_viewer, row, 1);
  tl->addStretch(1);

  row++;
  QHBoxLayout* hb = new QHBoxLayout();
  hb->setMargin(5);
  hb->setSpacing(5);
  QPushButton* ok = new QPushButton(tr("OK"));
  hb->addWidget(ok);
  hb->addStretch(10); // spacer
  QPushButton* cancel = new QPushButton(tr("Cancel"));
  hb->addWidget(cancel);
  connect(ok, SIGNAL(clicked()), this, SLOT(ok()));
  hb->setStretchFactor(ok, 2);
  connect(cancel, SIGNAL(clicked()), this, SLOT(cancel()));
  hb->setStretchFactor(cancel, 2);

  tl->addLayout(hb);
}

viewer_edit_dialog::~viewer_edit_dialog()
{
}

QString viewer_edit_dialog::mimetype() const
{
  return m_mimetype->text();
}

QString viewer_edit_dialog::viewer() const
{
  return m_viewer->text();
}

void
viewer_edit_dialog::ok()
{
  accept();
}

void
viewer_edit_dialog::cancel()
{
  reject();
}

void
prefs_dialog::edit_mimetype()
{
  QTreeWidget* lv = m_widgets->mt_lv;
  QTreeWidgetItem* item = lv->currentItem();
  if (!item)
    return;
  viewer_edit_dialog dlg(this, item->text(0), item->text(1));
  if (dlg.exec()==QDialog::Accepted) {
    item->setText(0, dlg.mimetype());
    item->setText(1, dlg.viewer());
    m_viewers_updated=true;
  }
}

void
prefs_dialog::new_mimetype()
{
  viewer_edit_dialog dlg(this, QString::null, QString::null);
  if (dlg.exec()==QDialog::Accepted && !dlg.mimetype().isEmpty()) {
    QTreeWidget* lv = m_widgets->mt_lv;
    QTreeWidgetItem* item=new QTreeWidgetItem(lv);
    item->setText(0, dlg.mimetype());
    item->setText(1, dlg.viewer());
    lv->setCurrentItem(item);
    m_viewers_updated=true;
  }
}

void
prefs_dialog::delete_mimetype()
{
  QTreeWidget* lv = m_widgets->mt_lv;
  QTreeWidgetItem* item = lv->currentItem();
  if (!item)
    return;
  int rep=QMessageBox::question(NULL, tr("Confirmation"), 
				tr("Are you sure you want to delete this entry?"),
				QMessageBox::Yes, QMessageBox::No);
  if (rep==QMessageBox::No)
    return;
  delete item;
  m_viewers_updated=true;
}

QWidget*
prefs_dialog::mimetypes_widget()
{
  QWidget* w1=new QWidget(this);
  CHECK_PTR(w1);
  QVBoxLayout* top_layout = new QVBoxLayout(w1);
  top_layout->setMargin(3);

  QHBoxLayout* hbl = new QHBoxLayout();
  top_layout->addLayout(hbl);
  hbl->addStretch(1);

  QPushButton* new_entry = new QPushButton(tr("New"), w1);
  hbl->addWidget(new_entry);
  connect(new_entry, SIGNAL(clicked()), this, SLOT(new_mimetype()));

  QPushButton* edit_entry = new QPushButton(tr("Edit"), w1);
  hbl->addWidget(edit_entry);
  connect(edit_entry, SIGNAL(clicked()), this, SLOT(edit_mimetype()));

  QPushButton* del_entry = new QPushButton(tr("Delete"), w1);
  hbl->addWidget(del_entry);
  connect(del_entry, SIGNAL(clicked()), this, SLOT(delete_mimetype()));
  hbl->addStretch(1);

  QTreeWidget* lv = new QTreeWidget(w1);
  m_widgets->mt_lv = lv;
  top_layout->addWidget(lv);
  QStringList headers;
  headers << tr("MIME type") << tr("Viewer command");
  lv->setHeaderLabels(headers);
  connect(lv, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	  this, SLOT(edit_mimetype()));

  attch_viewer_list lmt;
  lmt.fetch(get_config().name());
  attch_viewer_list::iterator iter = lmt.begin();
  for (; iter != lmt.end(); ++iter) {
    QTreeWidgetItem* lvi = new QTreeWidgetItem(lv);
    lvi->setText(0, iter->m_mime_type);
    lvi->setText(1, iter->m_program);
  }
  return w1;
}

QWidget*
prefs_dialog::fetching_widget()
{
  QWidget* w=new QWidget(this);
  CHECK_PTR(w);
  QVBoxLayout* top_layout = new QVBoxLayout(w);
  QGridLayout* grid = new QGridLayout();
  top_layout->addLayout(grid);
  int row=0;
  {
    QLabel* l=new QLabel(tr("Initial fetch"), w);
    grid->addWidget(l, row, 0);
    QLineEdit* le = new QLineEdit(w);
    m_widgets->w_initial_fetch = le;
    le->setMaxLength(5);
    le->setMaximumWidth(50);
    grid->addWidget(le, row, 1);
    QIntValidator *v1 = new QIntValidator(this);
    v1->setBottom(0);
    le->setValidator(v1);
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Max fetch ahead"), w);
    grid->addWidget(l, row, 0);
    QLineEdit* le = new QLineEdit(w);
    m_widgets->w_fetch_ahead = le;
    le->setMaxLength(5);
    le->setMaximumWidth(50);
    grid->addWidget(le, row, 1);
    QIntValidator *v1 = new QIntValidator(this);
    v1->setBottom(0);
    le->setValidator(v1);
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Max database connections"), w);
    grid->addWidget(l, row, 0);
    QLineEdit* le = new QLineEdit(w);
    m_widgets->w_max_connections = le;
    le->setMaxLength(3);
    le->setMaximumWidth(50);
    grid->addWidget(le, row, 1);
    le->setValidator(new QIntValidator(2, 10, this));
  }

  row++;
  {
    QLabel* l=new QLabel(tr("Automatically refresh results every"), w);
    QHBoxLayout* hb = new QHBoxLayout;
    hb->setSpacing(3);
    QSpinBox* sp = new QSpinBox();
    hb->addWidget(sp);
    sp->setMinimum(0);
    hb->addWidget(new QLabel(tr("mn")));
    grid->addWidget(l, row, 0);
    grid->addLayout(hb, row, 1);
    m_widgets->w_auto_refresh=sp;
  }

  row++;
  {
    QLabel* l=new QLabel(tr("Auto-incorporate new messages into lists"), w);
    QCheckBox* sp = new QCheckBox();
    grid->addWidget(l, row, 0);
    grid->addWidget(sp, row, 1);
    m_widgets->w_auto_incorporate=sp;
  }


  top_layout->addStretch(1);
  return w;
}

QWidget*
prefs_dialog::paths_widget()
{
  QWidget* w1=new QWidget(this);
  CHECK_PTR(w1);
  QVBoxLayout* top_layout = new QVBoxLayout(w1);
  QGridLayout* grid = new QGridLayout();
  top_layout->addLayout(grid);
  int row=0;
  {
    QLabel* l=new QLabel(tr("Attachments\n(temporary)"), w1);
    grid->addWidget(l, row, 0);
    m_widgets->w_attachments_dir = new QLineEdit(w1);
    grid->addWidget(m_widgets->w_attachments_dir, row, 1);
    QPushButton* disk1 = new QPushButton(UI_ICON(ICON16_FOLDER), "");
    grid->addWidget(disk1, row, 2);
    connect(disk1, SIGNAL(clicked()), this, SLOT(click_attachments_dir()));
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Bitmaps"), w1);
    grid->addWidget(l, row, 0);
    m_widgets->w_xpm_dir = new QLineEdit(w1);
    grid->addWidget(m_widgets->w_xpm_dir, row, 1);
    QPushButton* disk2 = new QPushButton(UI_ICON(ICON16_FOLDER), "");
    grid->addWidget(disk2, row, 2);
    connect(disk2, SIGNAL(clicked()), this, SLOT(click_bitmaps_dir()));
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Help files"), w1);
    grid->addWidget(l, row, 0);
    m_widgets->w_help_dir = new QLineEdit(w1);
    grid->addWidget(m_widgets->w_help_dir, row, 1);
    QPushButton* disk3 = new QPushButton(UI_ICON(ICON16_FOLDER), "");
    grid->addWidget(disk3, row, 2);
    connect(disk3, SIGNAL(clicked()), this, SLOT(click_help_dir()));
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Browser"), w1);
    grid->addWidget(l, row, 0);
    m_widgets->w_browser_path = new QLineEdit(w1);
    grid->addWidget(m_widgets->w_browser_path, row, 1);
    QPushButton* disk4 = new QPushButton(UI_ICON(ICON16_FOLDER), "");
    grid->addWidget(disk4, row, 2);
    connect(disk4, SIGNAL(clicked()), this, SLOT(click_browser_path()));
  }
  top_layout->addStretch(1);
  grid->setColumnStretch(1, 4);
  grid->setColumnStretch(2, 0);
  return w1;
}

void
prefs_dialog::ask_directory(QLineEdit* line)
{
  QString current = line->text();
  QDir d(current);
  if (!d.exists())
    current = QString();
  
  QString dir = QFileDialog::getExistingDirectory(this, tr("Choose an existing directory"), current);
  if (!dir.isEmpty())
    line->setText(dir);
}

void
prefs_dialog::click_help_dir()
{
  ask_directory(m_widgets->w_help_dir);
}

void
prefs_dialog::click_bitmaps_dir()
{
  ask_directory(m_widgets->w_xpm_dir);
}

void
prefs_dialog::click_attachments_dir()
{
  ask_directory(m_widgets->w_attachments_dir);
}

void
prefs_dialog::click_browser_path()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Choose browser"));
  if (!filename.isEmpty())
    m_widgets->w_browser_path->setText(filename);
}


QWidget*
prefs_dialog::composer_widget()
{
  QWidget* w1 = new QWidget(this);
  CHECK_PTR(w1);
  QVBoxLayout* top_layout = new QVBoxLayout(w1);
  QGridLayout* grid = new QGridLayout();
  top_layout->addLayout(grid);
  int row = 0;
  {
    QLabel* l=new QLabel(tr("Format for new messages"),w1);
    button_group* g = new button_group(QBoxLayout::TopToBottom, w1);
    QRadioButton* b1 = new QRadioButton(tr("Plain text"));
    QRadioButton* b2 = new QRadioButton(tr("HTML"));
    g->addButton(b1, 0, "text/plain");
    g->addButton(b2, 1, "text/html");
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_composer_format_new_mail = g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Format for replies"),w1);
    button_group* g = new button_group(QBoxLayout::TopToBottom, w1);
    QRadioButton* b1 = new QRadioButton(tr("Plain text"));
    QRadioButton* b2 = new QRadioButton(tr("HTML"));
    QRadioButton* b3 = new QRadioButton(tr("Same as sender"));
    g->addButton(b1, 0, "text/plain");
    g->addButton(b2, 1, "text/html");
    g->addButton(b3, 2, "same_as_sender");
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_composer_format_replies = g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Basic syntax check on addresses"),w1);
    button_group* g = new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* b1 = new QRadioButton(tr("Yes"));
    QRadioButton* b2 = new QRadioButton(tr("No"));

    g->addButton(b1, 0, "basic");
    g->addButton(b2, 1, "none");
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_composer_address_check = g;
  }
  row++;
  top_layout->addStretch(1);
  return w1;
}

prefs_dialog::sub_label::sub_label(const QString text)
{
  setTextFormat(Qt::RichText);
  setText(text);
  QFont f = font();
  f.setPointSize((f.pointSize()*8)/10);
  setFont(f);
}

QWidget*
prefs_dialog::search_widget()
{
  QWidget* w1 = new QWidget(this);
  CHECK_PTR(w1);
  QVBoxLayout* top_layout = new QVBoxLayout(w1);
  QGridLayout* grid = new QGridLayout();
  top_layout->addLayout(grid);
  int row = 0;
  {
    QLabel* l=new QLabel(tr("Accents & diacritic marks"),w1);
    button_group* g = new button_group(QBoxLayout::TopToBottom, w1);
    QRadioButton* b1 = new QRadioButton(tr("Search with accents"));
    QRadioButton* b2 = new QRadioButton(tr("Search without accents"));
    g->addButton(b1, 0, "accented");
    g->addButton(b2, 1, "unaccented");
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_search_accents = g;
  }
  row++;
  {
    sub_label* l = new sub_label(tr("This can be overriden with the <i>accents:insensitive</i> or <i>accents:sensitive</i> operators in the search box."));    
    grid->addWidget(l, row, 0, 1, -1);
  }
  top_layout->addStretch(1);
  return w1;
}

QWidget*
prefs_dialog::display_widget()
{
  QWidget* w1=new QWidget(this);
  CHECK_PTR(w1);
  QGridLayout* grid = new QGridLayout(w1);
  int row=0;
  {
    QLabel* l=new QLabel(tr("Messages order"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    g->addButton(new QRadioButton(tr("Newest first")), 1);
    g->addButton(new QRadioButton(tr("Oldest first")), 0);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_messages_order=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Show tags panel"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* yes=new QRadioButton(tr("Yes"));
    QRadioButton* no=new QRadioButton(tr("No"));
    g->addButton(yes, 0);
    g->addButton(no, 1);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_show_tags=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Display sender as"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* hn=new QRadioButton(tr("Email"));
    QRadioButton* hm=new QRadioButton(tr("Name"));
    g->addButton(hn, 0);
    g->addButton(hm, 1);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_display_sender=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Clickable URLs in body"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* hn=new QRadioButton(tr("Yes"));
    QRadioButton* hm=new QRadioButton(tr("No"));
    g->addButton(hn, 0);
    g->addButton(hm, 1);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_clickable_urls=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Preferred display format"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* hn=new QRadioButton(tr("HTML"));
    QRadioButton* hm=new QRadioButton(tr("Text"));
    g->addButton(hn, 0);
    g->addButton(hm, 1);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_preferred_format=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Dynamic recipient column"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* hn=new QRadioButton(tr("Yes"));
    QRadioButton* hm=new QRadioButton(tr("No"));
    g->addButton(hn, 0);
    g->addButton(hm, 1);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_dynamic_sender_column=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Threaded display"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* hn=new QRadioButton(tr("Yes"));
    QRadioButton* hm=new QRadioButton(tr("No"));
    g->addButton(hn, 0);
    g->addButton(hm, 1);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_display_threaded=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Date format"),w1);
    QComboBox* g=new QComboBox(w1);
    g->addItem(tr("Localized [%1]").arg(QLocale::system().name()));
    g->addItem("DD/MM/YYYY HH:MI");
    g->addItem("YYYY/MM/DD HH:MI");
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_date_format=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("New mail notifications"),w1);
    button_group* g=new button_group(QBoxLayout::LeftToRight, w1);
    QRadioButton* hn=new QRadioButton(tr("System"));
    QRadioButton* hm=new QRadioButton(tr("None"));
    g->addButton(hn, 1);
    g->addButton(hm, 0);
    grid->addWidget(l, row, 0);
    grid->addWidget(g, row, 1);
    m_widgets->w_notifications=g;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Style"),w1);
    QComboBox* ln=new QComboBox(w1);
    // list available Qt styles in the combobox
    QStringList list = QStyleFactory::keys();
    list.sort();
    ln->addItem("(default)");
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
      ln->addItem(*it);
    }
    grid->addWidget(l, row, 0);
    grid->addWidget(ln, row, 1);
    m_widgets->w_style=ln;
  }
  row++;
  return w1;
}

QWidget*
prefs_dialog::ident_widget()
{
  QWidget* w=new QWidget(this);
  CHECK_PTR(w);
  QVBoxLayout* top_layout = new QVBoxLayout(w);

  QHBoxLayout* hbl = new QHBoxLayout();
  top_layout->addLayout(hbl);

  hbl->addStretch(1);
  m_widgets->w_ident_cb = new QComboBox(w);
  QPushButton* new_id = new QPushButton(tr("New"), w);
  hbl->addWidget(new_id);
  hbl->addWidget(m_widgets->w_ident_cb, 5);
  connect(new_id, SIGNAL(clicked()), this, SLOT(new_identity()));
  m_widgets->w_del_ident = new QPushButton(tr("Delete"), w);
  hbl->addWidget(m_widgets->w_del_ident);
  connect(m_widgets->w_del_ident, SIGNAL(clicked()), this, SLOT(delete_identity()));

  hbl->addStretch(1);
  connect(m_widgets->w_ident_cb, SIGNAL(activated(int)), this,
	  SLOT(ident_changed(int)));

  QGridLayout* grid = new QGridLayout();
  top_layout->addLayout(grid);

  int row=0;
  {
    QLabel* l=new QLabel(tr("Email address"),w);
    m_widgets->w_email=new QLineEdit(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(m_widgets->w_email, row, 1);
    connect(m_widgets->w_email, SIGNAL(editingFinished()), this, SLOT(email_edited()));

  }
  row++;
  {
    QLabel* l=new QLabel(tr("Name"),w);
    QLineEdit* ln=new QLineEdit(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(ln, row, 1);
    m_widgets->w_name=ln;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Restrictable"),w);
    QCheckBox* cb=new QCheckBox(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(cb, row, 1);
    m_widgets->w_restricted=cb;
  }
  row++;
  {
    QLabel* l = new QLabel(tr("Root tag"),w);
    tag_line_edit_selector* ln = new tag_line_edit_selector(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(ln, row, 1);
    m_widgets->w_root_tag = ln;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Use as default"),w);
    QCheckBox* cb=new QCheckBox(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(cb, row, 1);
    m_widgets->w_default_identity=cb;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("Signature"),w);
    QPlainTextEdit* ln=new QPlainTextEdit(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(ln, row, 1);
    m_widgets->w_signature=ln;
  }
  row++;
  {
    QLabel* l=new QLabel(tr("X-Face"),w);
    QLineEdit* ln=new QLineEdit(w);
    grid->addWidget(l, row, 0);
    grid->addWidget(ln, row, 1);
    m_widgets->w_xface=ln;
  }

  // fetch identities
  if (m_ids_map.fetch()) {
    identities::iterator iter = m_ids_map.begin();
    while (iter != m_ids_map.end()) {
      mail_identity* p= new mail_identity();
      *p = iter->second;
      m_ids.push_back(p);
      m_widgets->w_ident_cb->addItem(p->m_email_addr);
      if (p->m_is_default)
	m_widgets->m_default_email = p->m_email_addr;
      ++iter;
    }
  }
  m_curr_ident = get_identity(m_widgets->w_ident_cb->currentText());
  if (m_curr_ident==NULL) {
    m_curr_ident = new mail_identity();
    m_ids.push_back(m_curr_ident);
  }
  ident_to_widgets();

  return w;
}

prefs_dialog::prefs_dialog(QWidget* parent): QDialog(parent)
{
  m_widgets=NULL;
  m_viewers_updated=false;

  setWindowTitle(tr("Preferences"));
  app_config& conf=get_config();
  QVBoxLayout* layout = new QVBoxLayout(this);

#if 0
  m_conf_selector = new QComboBox(this);
  QStringList all_confs;
  if (app_config::get_all_conf_names(&all_confs)) {
    m_conf_selector->insertStringList(all_confs);
    connect(m_conf_selector, SIGNAL(activated(const QString&)),
	    this, SLOT(other_conf(const QString&)));
  }
  m_conf_selector->setCurrentText(conf.name());
  layout->addWidget(m_conf_selector);
#endif
  m_widgets = new struct preferences_widgets;

  m_tabw = new QTabWidget(this);
  layout->addWidget(m_tabw);
  m_display_page = display_widget();
  m_tabw->addTab(m_display_page, tr("Display"));

  m_ident_page = ident_widget();
  m_tabw->addTab(m_ident_page, tr("Identities"));

  m_mimetypes_page = mimetypes_widget();
  m_tabw->addTab(m_mimetypes_page, tr("MIME viewers"));

  m_composer_page = composer_widget();
  m_tabw->addTab(m_composer_page, tr("Composer"));

  m_search_page = search_widget();
  m_tabw->addTab(m_search_page, tr("Search"));

  m_paths_page = paths_widget();
  m_tabw->addTab(m_paths_page, tr("Paths"));

  m_fetching_page = fetching_widget();
  m_tabw->addTab(m_fetching_page, tr("Fetching"));


  connect(m_tabw, SIGNAL(currentChanged(int)), this, SLOT(page_changed(int)));

  QHBoxLayout* hb = new QHBoxLayout;
  hb->setMargin(5);
  hb->setSpacing(5);
  layout->addLayout(hb);

  hb->insertStretch(0, 10);
  QPushButton* help = new QPushButton(tr("Help"));
  hb->addWidget(help, 1);
  QPushButton* ok = new QPushButton(tr("OK"));
  hb->addWidget(ok, 1);
  QPushButton* cancel = new QPushButton(tr("Cancel"));
  hb->addWidget(cancel, 1);
  connect(help, SIGNAL(clicked()), this, SLOT(help()));
  connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));

  conf_to_widgets(conf);
}

prefs_dialog::~prefs_dialog()
{
  if (m_widgets)
    delete m_widgets;
}

/*
  Returns the help topic corresponding to a tab
*/
QString
prefs_dialog::help_topic(QWidget* tab)
{
  QString help_topic;
  if (tab == m_display_page)
    help_topic = "preferences/display";
  else if (tab == m_ident_page)
    help_topic = "preferences/identities";
  else if (tab == m_paths_page)
    help_topic = "preferences/paths";
  else if (tab == m_fetching_page)
    help_topic = "preferences/fetching";
  else if (tab == m_mimetypes_page)
    help_topic = "preferences/mimeviewers";
  return help_topic;
}

#if 0
void
prefs_dialog::other_conf(const QString& conf)
{
  DBG_PRINTF(3, "to other conf: %s\n", conf.latin1());
  // TODO: switch to new configuration
  m_conf_selector->setCurrentText(get_config().name());
}
#endif

void
prefs_dialog::help()
{
  QString topic = help_topic(m_tabw->currentWidget());
  if (!topic.isEmpty())
    helper::show_help(topic);
}

void
prefs_dialog::page_changed(int index)
{
  Q_UNUSED(index);
  QString topic = help_topic(m_tabw->currentWidget());
  if (!topic.isEmpty())
    helper::track(topic);
}

void
prefs_dialog::conf_to_widgets(app_config& conf)
{
  int idx;
  QString date_format=conf.get_string("date_format");
  if (date_format=="local")
    m_widgets->w_date_format->setCurrentIndex(0);
  else {
    for (idx=1; idx<m_widgets->w_date_format->count(); idx++) {
      if (m_widgets->w_date_format->itemText(idx)==date_format)
	m_widgets->w_date_format->setCurrentIndex(idx);
    }
  }

  switch(conf.get_number("show_tags")) {
  case 0:
    m_widgets->w_show_tags->setButton(1);
    break;
  case 1:
    m_widgets->w_show_tags->setButton(0);
    break;
  }

  QString mo = conf.get_string("messages_order");
  if (mo=="oldest_first")
    m_widgets->w_messages_order->setButton(0);
  else // newest first
    m_widgets->w_messages_order->setButton(1);

  QString dspl_as=conf.get_string("sender_displayed_as");
  if (dspl_as=="name")
    m_widgets->w_display_sender->setButton(1);
  else
    m_widgets->w_display_sender->setButton(0);

  switch(conf.get_bool("body_clickable_urls")) {
  case true:
    m_widgets->w_clickable_urls->setButton(0);
    break;
  case false:
    m_widgets->w_clickable_urls->setButton(1);
    break;
  }

  QString new_mail_notif=conf.get_string("display/notifications/new_mail");
  if (new_mail_notif=="none") {
    m_widgets->w_notifications->setButton(0);
  }
  else if (new_mail_notif=="system") {
    m_widgets->w_notifications->setButton(1);
  }
  else
    m_widgets->w_notifications->setButton(1); // system is default

  QString preferred_dspl=conf.get_string("display/body/preferred_format");
  if (preferred_dspl.toUpper() == "HTML") {
    m_widgets->w_preferred_format->setButton(0);
  }
  else if (preferred_dspl.toUpper() == "TEXT") {
    m_widgets->w_preferred_format->setButton(1);
  }
  else
    m_widgets->w_preferred_format->setButton(0); // HTML is the default

  switch(conf.get_bool("display/auto_sender_column")) {
  case true:
    m_widgets->w_dynamic_sender_column->setButton(0);
    break;
  case false:
    m_widgets->w_dynamic_sender_column->setButton(1);
    break;
  }

  switch(conf.get_number("display_threads")) {
  case 1:
    m_widgets->w_display_threaded->setButton(0);
    break;
  case 0:
    m_widgets->w_display_threaded->setButton(1);
    break;
  }

  QString style=conf.get_string("ui_style");
  if (!style.isEmpty()) {
    for (int i=0; i<m_widgets->w_style->count(); i++) {
      QString t=m_widgets->w_style->itemText(i);
      if (t==style) {
	m_widgets->w_style->setCurrentIndex(i);
      }
    }
  }


  // paths
  m_widgets->w_attachments_dir->setText(conf.get_string("attachments_directory"));
  m_widgets->w_xpm_dir->setText(conf.get_string("xpm_path"));
  m_widgets->w_help_dir->setText(conf.get_string("help_path"));
  m_widgets->w_browser_path->setText(conf.get_string("browser"));

  // fetching
  m_widgets->w_max_connections->setText(conf.get_string("max_db_connections"));
  m_widgets->w_fetch_ahead->setText(conf.get_string("fetch_ahead_max_msgs"));
  m_widgets->w_initial_fetch->setText(conf.get_string("max_msgs_per_selection"));
  int auto_refresh = conf.get_number("fetch/auto_refresh_messages_list");
  m_widgets->w_auto_refresh->setValue(auto_refresh);
  bool auto_incorp = conf.get_bool("fetch/auto_incorporate_new_results", false);
  m_widgets->w_auto_incorporate->setChecked(auto_incorp);


  // composer
  {
    QString fm_new = conf.get_string("composer/format_for_new_mail");
    QString fm_rep = conf.get_string("composer/format_for_replies");
    if (!fm_new.isEmpty()) {
      int id = m_widgets->w_composer_format_new_mail->code_to_id(fm_new);
      if (id >= 0)
	m_widgets->w_composer_format_new_mail->setButton(id);
    }
    if (!fm_rep.isEmpty()) {
      int id = m_widgets->w_composer_format_replies->code_to_id(fm_rep);
      if (id >= 0)
	m_widgets->w_composer_format_replies->setButton(id);
    }

    QString ac = conf.get_string("composer/address_check");
    if (!ac.isEmpty()) {
      if (ac=="none")
	m_widgets->w_composer_address_check->setButton(1);
      else
	m_widgets->w_composer_address_check->setButton(0);
    }
  }

  // search
  {
    QString a = conf.get_string("search/accents");
    if (!a.isEmpty()) {
      int id = m_widgets->w_search_accents->code_to_id(a);
      if (id >= 0)
	m_widgets->w_search_accents->setButton(id);
    }
  }
}

void
prefs_dialog::widgets_to_conf(app_config& conf)
{
  conf.set_string("default_identity", m_widgets->m_default_email);
  if (m_widgets->w_date_format->currentIndex()==0)
    conf.set_string("date_format", "local");
  else
    conf.set_string("date_format", m_widgets->w_date_format->currentText());


  if (m_widgets->w_messages_order->selected_id()==1)
    conf.set_string("messages_order", "newest_first");
  else if (m_widgets->w_messages_order->selected_id()==0)
    conf.set_string("messages_order", "oldest_first");

  if (m_widgets->w_show_tags->selected_id()==1)
    conf.set_number("show_tags", 0);
  else if (m_widgets->w_show_tags->selected_id()==0)
    conf.set_number("show_tags", 1);

  switch (m_widgets->w_display_sender->selected_id()) {
  case 1:
    conf.set_string("sender_displayed_as", "name");
    break;
  case 0:
    conf.set_string("sender_displayed_as", "email");
    break;
  }

  switch (m_widgets->w_display_threaded->selected_id()) {
  case 1:
    conf.set_number("display_threads", 0);
    break;
  case 0:
    conf.set_number("display_threads", 1);
    break;
  }

  switch (m_widgets->w_dynamic_sender_column->selected_id()) {
  case 0:
    conf.set_bool("display/auto_sender_column", true);;
    break;
  case 1:
    conf.set_bool("display/auto_sender_column", false);
    break;
  }

  switch (m_widgets->w_notifications->selected_id()) {
  case 1:
    conf.set_string("display/notifications/new_mail", "system");
    break;
  case 0:
    conf.set_string("display/notifications/new_mail", "none");
    break;
  }

  switch (m_widgets->w_preferred_format->selected_id()) {
  case 1:
    conf.set_string("display/body/preferred_format", "text");
    break;
  case 0:
    conf.set_string("display/body/preferred_format", "html");
    break;
  }

  switch (m_widgets->w_clickable_urls->selected_id()) {
  case 0:
    conf.set_number("body_clickable_urls", 1);
    break;
  case 1:
    conf.set_number("body_clickable_urls", 0);
    break;
  }

  if (m_widgets->w_style->currentText()!="(default)") {
    conf.set_string("ui_style", m_widgets->w_style->currentText());
  }
  else {
    conf.set_string("ui_style", QString());
  }


#if 0
  bool ok;
  int autocheck = m_widgets->w_auto_check->text().toInt(&ok);
  if (ok) {
    conf.set_number("autocheck_newmail", autocheck);
  }
#endif

  // paths
  conf.set_string("attachments_directory", m_widgets->w_attachments_dir->text());
  conf.set_string("xpm_path", m_widgets->w_xpm_dir->text());
  conf.set_string("help_path", m_widgets->w_help_dir->text());
  conf.set_string("browser", m_widgets->w_browser_path->text());

  // fetching
  conf.set_number("fetch_ahead_max_msgs", m_widgets->w_fetch_ahead->text().toInt());
  conf.set_number("max_msgs_per_selection", m_widgets->w_initial_fetch->text().toInt());
  conf.set_number("max_db_connections", m_widgets->w_max_connections->text().toInt());
  conf.set_bool("fetch/auto_incorporate_new_results", m_widgets->w_auto_incorporate->isChecked());
  conf.set_number("fetch/auto_refresh_messages_list", m_widgets->w_auto_refresh->text().toInt());

  // composer
  if (m_widgets->w_composer_format_new_mail->selected()) {
    conf.set_string("composer/format_for_new_mail",
		    m_widgets->w_composer_format_new_mail->selected_code());
  }
  if (m_widgets->w_composer_format_replies->selected()) {
    conf.set_string("composer/format_for_replies",
		    m_widgets->w_composer_format_replies->selected_code());
  }

  if (m_widgets->w_composer_address_check->selected()) {
    conf.set_string("composer/address_check",
		    m_widgets->w_composer_address_check->selected_code());
  }

  // search
  if (m_widgets->w_search_accents->selected()) {
    conf.set_string("search/accents",
		    m_widgets->w_search_accents->selected_code());
  }
}

void
prefs_dialog::done(int code)
{
  DBG_PRINTF(5,"done(%d)\n", code);
  if (code==1) {
    widgets_to_ident();
    app_config newconf;
    widgets_to_conf(newconf);
    //DBG_PRINTF(5,"new_conf\n");
    //newconf.dump();
    get_config().diff_update(newconf);
    get_config().diff_replace(newconf);
    get_config().apply();
    if (m_viewers_updated && !update_viewers_db())
	return;
    if (!update_identities_db())
      return;
  }
  QDialog::done(code);
}

/*
  Save the current displayed values into memory
*/
void
prefs_dialog::widgets_to_ident()
{
  mail_identity* id = m_curr_ident;
  // update the values
  id->m_name = m_widgets->w_name->text();
  id->m_xface = m_widgets->w_xface->text();
  id->m_email_addr = m_widgets->w_email->text();
  id->m_signature = m_widgets->w_signature->toPlainText();
  id->m_is_default = m_widgets->w_default_identity->isChecked();
  id->m_root_tag_id = m_widgets->w_root_tag->current_tag_id();
  id->m_is_restricted = m_widgets->w_restricted->isChecked();
  if (id->m_is_default) {
    // if id is now the "default" identity,
    // remove that attribute to the previous default unless it's the same
    mail_identity* prev = get_identity(m_widgets->m_default_email);
    if (prev && prev!=id) {
      prev->m_is_default=false;
    }
    m_widgets->m_default_email=id->m_email_addr;
  }
  else {
    // when the "default" property gets unchecked, we reflect that
    // on m_widgets->m_default_email
    if (m_widgets->m_default_email == id->m_email_addr) // if it was the default
      m_widgets->m_default_email = QString::null;
  }
}

mail_identity*
prefs_dialog::get_identity(const QString& email)
{
  QList<mail_identity*>::iterator it;
  for (it = m_ids.begin(); it!=m_ids.end(); ++it) {
    if ((*it)->m_email_addr==email)
      return (*it);
  }
  return NULL;
}

void
prefs_dialog::ident_to_widgets()
{
  mail_identity* id = m_curr_ident;
  if (!id->m_email_addr.isEmpty()) {
    m_widgets->w_name->setText(id->m_name);
    m_widgets->w_xface->setText(id->m_xface);
    m_widgets->w_email->setText(id->m_email_addr);
    m_widgets->w_restricted->setChecked(id->m_is_restricted);
    m_widgets->w_root_tag->setText(tags_repository::hierarchy(id->m_root_tag_id));
    m_widgets->w_signature->setPlainText(id->m_signature);
    m_widgets->w_default_identity->setChecked(id->m_is_default);
  }
  else {
    m_widgets->w_name->setText(QString::null);
    m_widgets->w_xface->setText(QString::null);
    m_widgets->w_email->setText(QString::null);
    m_widgets->w_signature->setPlainText(QString::null);
    m_widgets->w_default_identity->setChecked(false);
    m_widgets->w_restricted->setChecked(false);
    m_widgets->w_root_tag->setText(QString::null);
  }
}

void
prefs_dialog::ident_changed(int id)
{
  DBG_PRINTF(2,"ident_changed(%d)\n", id);

  //    m_widgets->w_del_ident->setEnabled(true);
  // store the displayed values into memory
  widgets_to_ident();

  // display the values for the identity that becomes current
  QString email=m_widgets->w_ident_cb->currentText();
  m_curr_ident=get_identity(email);
  if (m_curr_ident==NULL) {
    DBG_PRINTF(2, "ERR: m_curr_ident=null for email='%s'\n", email.toLatin1().constData());
  }
  else
    ident_to_widgets();
}

void
prefs_dialog::delete_identity()
{
  if (m_ids.isEmpty())
    return;
  QString cur = m_widgets->w_ident_cb->currentText();
  m_ids.removeAll(m_curr_ident);
  m_widgets->w_ident_cb->removeItem(m_widgets->w_ident_cb->currentIndex());
  m_curr_ident = get_identity(m_widgets->w_ident_cb->currentText());
  if (m_curr_ident==NULL) {
    add_new_identity();
  }
  ident_to_widgets();
}

void
prefs_dialog::add_new_identity()
{
  m_curr_ident = new mail_identity();
  m_ids.push_back(m_curr_ident);
  ident_to_widgets();
  m_widgets->w_ident_cb->addItem("");
  m_widgets->w_ident_cb->setCurrentIndex(m_widgets->w_ident_cb->count()-1);
  m_widgets->w_email->setFocus();
}

void
prefs_dialog::new_identity()
{
  // no new identity if the email address of the current identity is not set
  if (m_widgets->w_ident_cb->count() > 0 &&
      m_widgets->w_email->text().isEmpty()) {
    return;
  }

  email_edited();
  widgets_to_ident();

  // if an empty identity is already available in the combobox
  // then we try to use it
  mail_identity* p = get_identity("");
  if (p) {
    QComboBox* cb = m_widgets->w_ident_cb;
    int c=cb->count();
    for (int i=0; i<c; i++) {
      if (cb->itemText(i)=="") {
	cb->setCurrentIndex(i);
	m_curr_ident=p;
	ident_to_widgets();
	return;
      }
    }
  }

  // if no empty identity has been found, let's create one
  add_new_identity();
}

void
prefs_dialog::email_edited()
{
  if (m_widgets->w_email->text() != m_curr_ident->m_email_addr) {
    // change the identity in the combobox as soon as the user
    // has given it a name or changed its name
    m_curr_ident->m_email_addr = m_widgets->w_email->text();
    if (m_widgets->w_ident_cb->count()>0) {
      m_widgets->w_ident_cb->setItemText(m_widgets->w_ident_cb->currentIndex(),
					 m_curr_ident->m_email_addr);
					
    }
    else {
      m_widgets->w_ident_cb->addItem(m_curr_ident->m_email_addr);
    }
  }
}

bool
prefs_dialog::update_identities_db()
{
  db_cnx db;
  bool in_trans=false;

  try {
    /* Identities that have their email changed are completely removed
       using the old email as the key and then reinserted with the new
       email. This is because the email being the primary key, changing
       rows by successive update can fail, for instance if two
       email addresses are to be swapped, we would get a PK violation
       when doing the first update. While doing a first pass with
       updates, we're collecting such identities in 'ri_list' */
    QList<mail_identity*> ri_list;

    QList<mail_identity*>::iterator it;
    mail_identity* p;
    identities::iterator iter;
    // identities (original emails) that have been processed
    std::set<QString> done_set;

    for (it=m_ids.begin(); it!=m_ids.end(); ++it) {
      p=(*it);
      if (p->m_email_addr.isEmpty())
	continue;			// ignore empty identities, shouldn't happen
      done_set.insert(p->m_orig_email_addr);
      if (p->m_orig_email_addr != p->m_email_addr) {
	// when an identity is newly created, its p->m_orig_email_addr is empty
	// ri_list includes those new identities as well as renamed identities
	ri_list.append(p);
      }
      else {
	// update db
	iter=m_ids_map.find(p->m_email_addr);
	if (iter!=m_ids_map.end()) {
	  mail_identity* ps = &(iter->second); // before change
	  if (!ps->compare_fields(*p)) { // have fields been changed?
	    if (!in_trans) {
	      db.begin_transaction();
	      in_trans=true;
	    }
	    p->update_db();
	  }
	}
	else {
	  /* If the identity is not to be found in the list before
	     editing let's process it later by insert. Currently this
	     case shouldn't happen anyway since it's m_orig_email_addr
	     should be empty */
	  ri_list.append(p);
	}
      }
    }

    /* Delete the identities that were present initially but not in the final map */
    sql_stream sd("DELETE FROM identities WHERE email_addr=:p1", db);
    for (iter=m_ids_map.begin(); iter!=m_ids_map.end(); ++iter) {
      if (done_set.find(iter->first) == done_set.end()) {
	if (!in_trans) {
	  db.begin_transaction();
	  in_trans=true;
	}
	sd << iter->second.m_email_addr;
      }
    }

    /* Now process 'ri_list' */
    if (!ri_list.isEmpty()) {
      if (!in_trans) {
	db.begin_transaction();
	in_trans=true;
      }

      for (it=ri_list.begin(); it!=ri_list.end(); ++it) {
	p=*it;
	if (!p->m_orig_email_addr.isEmpty())
	  sd << p->m_email_addr;
      }

      for (it=ri_list.begin(); it!=ri_list.end(); ++it) {
	p=*it;
	/* update_db() will do an insert because the row, if it existed, has
	   been deleted just above */
	p->update_db();
      }
    }

    if (in_trans) {
      db.commit_transaction();
    }
  }
  catch(db_excpt& p) {
    if (in_trans)
      db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
prefs_dialog::update_viewers_db()
{
  db_cnx db;

  try {
    const QString confname=get_config().name();

    db.begin_transaction();
    sql_stream sd("DELETE FROM programs WHERE conf_name=:n", db);
    sd << confname;

    sql_stream si("INSERT INTO programs(program_name,content_type,conf_name) VALUES(:p1,:p2,:p3)", db);
    QTreeWidget* qw = m_widgets->mt_lv;
    for (int index=0; index<qw->topLevelItemCount(); index++) {
      QTreeWidgetItem* item = qw->topLevelItem(index);
      QString mt_name = item->text(0);
      QString viewer = item->text(1);
      si << viewer << mt_name << confname;
    }
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  return true;
}
