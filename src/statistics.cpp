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

#include "database.h"
#include "identities.h"
#include "main.h"
#include "sqlstream.h"
#include "statistics.h"
#include "tags.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPrinter>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>

// Global function to avoid callers having to pull the entire class.
void
open_stats_window()
{
  stats_view* w = new stats_view(NULL);
  w->show();
  w->run_query();
}

stats_view::stats_view(QWidget* parent): QMainWindow(parent)
{
  setWindowTitle(tr("Mail statistics"));
  m_webview = new stats_webview;
  m_color = QColor(54, 162, 235);

  QWidget* container = new QWidget;
  QVBoxLayout* top_layout = new QVBoxLayout;

  QHBoxLayout* h_layout = new QHBoxLayout;
  //  QHBoxLayout* h_layout2 = new QHBoxLayout;

  QFormLayout* f_layout = new QFormLayout;

  top_layout->addLayout(h_layout);
  //  top_layout->addLayout(h_layout2);

  m_query_sel = new QComboBox;
  m_query_sel->addItem(tr("All messages"), "nb_msgs");
  m_query_sel->addItem(tr("Incoming messages"), "nb_incoming_msgs");
  m_query_sel->addItem(tr("Outgoing messages"), "nb_outgoing_msgs");
  f_layout->addRow(tr("Counted items:"), m_query_sel);

  m_date1 = new QDateEdit;
  m_date1->setCalendarPopup(true);
  m_date1->setDate(QDate::currentDate().addMonths(-1));
  f_layout->addRow(tr("From:"), m_date1);

  m_date2 = new QDateEdit;
  m_date2->setCalendarPopup(true);
  m_date2->setDate(QDate::currentDate());
  f_layout->addRow(tr("Until:"), m_date2);

  m_tag = new tag_line_edit_selector;

  connect(m_tag, &tag_line_edit_selector::returnPressed, [=]() {
      if (m_tag->text().isEmpty())
	m_tag->show_all_completions();
    });

  f_layout->addRow(tr("Tag:"), m_tag);

  {
    QHBoxLayout* color_layout = new QHBoxLayout;
    m_color_label = new QLabel("  ");
    set_color_label();
    QPushButton* btn_color = new QPushButton(tr("Change"));
    color_layout->addWidget(m_color_label);
    color_layout->addWidget(btn_color);
    connect(btn_color, SIGNAL(clicked()), this, SLOT(change_color()));
    f_layout->addRow(tr("Color:"), color_layout);
  }
  h_layout->addLayout(f_layout);

  h_layout->addWidget(new QLabel(tr("Identities:")));
  m_list_idents = new QListWidget();
  h_layout->addWidget(m_list_idents);
  h_layout->setStretchFactor(m_list_idents, 0);

  QHBoxLayout *btn_layout = new QHBoxLayout;
  QPushButton* btnc = new QPushButton(tr("Clear"));
  connect(btnc, SIGNAL(clicked()), this, SLOT(clear_graph()));
  QPushButton* btn0 = new QPushButton(tr("Add"));
  connect(btn0, SIGNAL(clicked()), this, SLOT(run_query()));
  QPushButton* btn1 = new QPushButton(tr("Export PDF"));
  QPushButton* btn2 = new QPushButton(tr("Export CSV"));
  connect(btn1, SIGNAL(clicked()), this, SLOT(pdf_export()));
  connect(btn2, SIGNAL(clicked()), this, SLOT(csv_export()));
  btn_layout->addWidget(btn0);
  btn_layout->addWidget(btnc);
  btn_layout->addWidget(btn1);
  btn_layout->addWidget(btn2);
  btn_layout->addStretch(1);

  init_identities();

  h_layout->addStretch(1);
  top_layout->addLayout(btn_layout);
  top_layout->addWidget(m_webview);
  top_layout->setStretchFactor(h_layout, 0);
  top_layout->setStretchFactor(m_webview, 10);
  container->setLayout(top_layout);
  setCentralWidget(container);

}

void
stats_view::set_color_label()
{
  int cr,cg,cb;
  m_color.getRgb(&cr, &cg, &cb);
  QString style;
  style.sprintf("QLabel { background: #%02x%02x%02x }", cr, cg, cb);
  m_color_label->setStyleSheet(style);
}

void
stats_view::change_color()
{
  QColorDialog dlg(m_color, NULL);

  if (dlg.exec() == QDialog::Accepted) {
    m_color = dlg.selectedColor();
    set_color_label();
  }
}

void
stats_view::init_identities()
{
  identities idents;
  idents.fetch();

  QListWidgetItem* item = new QListWidgetItem(tr("[All]"));
  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, 0);
  m_list_idents->addItem(item);

  identities::const_iterator iter1;
  for (iter1 = idents.begin(); iter1 != idents.end(); ++iter1) {
    QListWidgetItem* item = new QListWidgetItem(iter1->second.m_email_addr);
    m_list_idents->addItem(item);
    item->setCheckState(Qt::Unchecked);
    item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
    item->setData(Qt::UserRole, iter1->second.m_identity_id);
  }
  ident_changed(m_list_idents->item(0)); // initialize color/state of all items
  connect(m_list_idents, SIGNAL(itemChanged(QListWidgetItem*)),
	  this, SLOT(ident_changed(QListWidgetItem*)));
}

/**
   Enable or disable entries in the identities list depending on the
   state of the top [All] item. Also visually reflect their state.
 */
void
stats_view::ident_changed(QListWidgetItem* item)
{
  if (item->data(Qt::UserRole).toInt() == 0) {
    // item for all identities
    bool enable = !(item->checkState() == Qt::Checked);
    QBrush brush_on =  m_list_idents->palette().brush(QPalette::Active, QPalette::WindowText);
    QBrush brush_off =  m_list_idents->palette().brush(QPalette::Disabled, QPalette::WindowText);

    m_list_idents->blockSignals(true); // don't recurse into the slot


    for (int row=1; row < m_list_idents->count(); row++) {
      QListWidgetItem* item = m_list_idents->item(row);
      if (enable) {
	item->setFlags(item->flags() | (Qt::ItemIsUserCheckable|Qt::ItemIsSelectable));
	item->setForeground(brush_on);
      }
      else {
	item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable|Qt::ItemIsSelectable));
	item->setForeground(brush_off);
      }

      if (!enable)
	item->setCheckState(Qt::Unchecked);
    }

    m_list_idents->blockSignals(false);
  }
}

QList<int>
stats_view::selected_identities()
{
  QList<int> list;
  for (int row=0; row < m_list_idents->count(); row++) {
    QListWidgetItem* item = m_list_idents->item(row);
    if (item->checkState() == Qt::Checked)
      list.append(item->data(Qt::UserRole).toInt()); // identity_id
  }
  return list;
}

void
stats_view::clear_graph()
{
  m_all_results.clear();
  m_webview->clear();
}

void
stats_view::show_graph(QList<QString>& labels)
{
  QString script =
"    var ctx = document.getElementById('myChart');"
"  var myChart = new Chart(ctx, {"
"    type: 'line',"
"    data: {"
"        labels: {#labels#},"
"        datasets: {#datasets#}"
"    },"
"    options: {"
    "	 showLines: true, responsive: false,"
"        scales: {"
"            yAxes: [{"
"                ticks: {"
"                    beginAtZero:true"
"                }"
"            }]"
"        }"
    "    }"
"});"
    ;

  //  run_script(script);

  QString slabels = "[";
  for (int i=0; i < labels.count(); i++) {
    if (i>0)
      slabels.append(',');
    slabels.append(QString("'%1'").arg(labels.at(i)));
  }
  slabels.append("]");
  script.replace("{#labels#}", slabels);

  QStringList datasets;
  QString str_datasets = "[";

  for (int i_dataset=0; i_dataset < m_all_results.count(); i_dataset++) {
    const struct result_msgcount& res = m_all_results.at(i_dataset);

    // data
    QString scnt;
    for (int i=0; i<res.values.count(); i++) {
      if (i>0)
	scnt.append(',');
      scnt.append(QString("%1").arg(res.values.at(i)));
    }

    if (i_dataset > 0)
      str_datasets.append(",");

    int cr,cg,cb;
    res.color.getRgb(&cr, &cg, &cb);
    str_datasets.append(QString(" { data: [ %1 ],\n").arg(scnt));
    str_datasets.append(QString("label: '#%1',\n").arg(i_dataset+1));
    str_datasets.append("borderWidth: 3,\n");
    str_datasets.append(QString("borderColor: 'rgba(%1,%2,%3, 0.6)',\n").arg(cr).arg(cg).arg(cb));
    str_datasets.append(QString("backgroundColor: 'rgba(%1,%2,%3, 0.2)',\n").arg(cr).arg(cg).arg(cb));
    str_datasets.append("fill: true\n");
    str_datasets.append("}\n");

  } // for each dataset
  str_datasets.append("]");

  script.replace("{#datasets#}", str_datasets);

  // DBG_PRINTF(4, "script=%s", script.toLocal8Bit().constData());

  m_webview->display(script);
}


stats_view::~stats_view()
{
}

void
stats_view::pdf_export()
{
  if (m_all_results.isEmpty()) {
    QMessageBox::critical(this, tr("Error"), tr("No data to export."));
    return;
  }

  QString filename = QFileDialog::getSaveFileName(this, tr("PDF file"), m_last_pdf_filename);
  if (filename.isEmpty())
    return;

  QPrinter* m_printer = new QPrinter;
  m_printer->setOutputFormat(QPrinter::PdfFormat);
  m_printer->setPaperSize(QPrinter::A4);

  m_printer->setOutputFileName(filename);
  m_last_pdf_filename = filename;
  m_printer->setOrientation(QPrinter::Portrait);
  m_webview->print(m_printer);
  delete m_printer;
}

void
stats_view::csv_export()
{
  if (m_all_results.isEmpty()) {
    QMessageBox::critical(this, tr("Error"), tr("No data to export."));
    return;
  }

  QString filename = QFileDialog::getSaveFileName(this, tr("CSV file"), m_last_csv_filename);
  if (filename.isEmpty())
    return;

  QFile f(filename);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    QMessageBox::critical(this, tr("Error opening file"), f.errorString());
    return;
  }

  m_last_csv_filename = filename;

  /* If there are several graphs on screen, the leftmost column is its index
     in the serie. */
  const char sep = ';';
  int nb_graphs = m_all_results.count();
  QTextStream out(&f);

  if (nb_graphs > 1)
    out << "Series" << sep;
  out << m_last_results.dates.join(sep) << "\n";
  for (int g=0; g < nb_graphs; g++)  {
    if (nb_graphs > 1) {
      out << QString("#%1").arg(g+1) << sep;
    }
    const QList<int>& vals = m_all_results.at(g).values;
    for (int i=0; i < vals.count(); i++) {
      if (i>0)
	out << sep;
      out << QString("%1").arg(vals.at(i));
    }
    out << "\n";
  }

  f.close();
}

void
stats_view::run_query()
{
  QString qcode = m_query_sel->currentData().toString();
  QDate d1 = m_date1->date();
  QDate d2 = m_date2->date();
  int tag_id = 0;
  QString query;
  QString criteria;

  if (qcode == "nb_msgs") {
    criteria = "";
  }
  else if (qcode == "nb_incoming_msgs") {
    criteria = "AND status&128=0";
  }
  else if (qcode == "nb_outgoing_msgs") {
    criteria = "AND status&128=128";
  }
  else
    return;

  QString main_table = "mail AS m";

  if (!m_tag->text().trimmed().isEmpty()) {
    tag_id = m_tag->current_tag_id();
    if (tag_id > 0) {
      main_table = QString("(SELECT mail.mail_id,mail.msg_date,mail.status FROM mail JOIN mail_tags USING(mail_id) WHERE tag=%1) AS m").arg(tag_id);
    }
    else {
      QMessageBox::critical(this, tr("Error"), tr("Tag does not exist: %1").arg(m_tag->text().trimmed()));
      return;
    }
  }

  if (m_list_idents->item(0)->checkState() == Qt::Unchecked) {
    // Individual identities
    QStringList ids;
    for (int row=1; row < m_list_idents->count(); row++) {
      if (m_list_idents->item(row)->checkState() == Qt::Checked)
	ids.append(QString("%1").arg(m_list_idents->item(row)->data(Qt::UserRole).toInt()));
    }
    if (!ids.isEmpty()) {
      criteria.append(QString("AND identity_id IN (%1)").arg(ids.join(",")));
    }
  }

  /* if the date range (X-axis) has changed, clear the graph. The alternative would be to
     re-run all previous queries with the new range. */
  if (!m_all_results.isEmpty() && (d1 != m_all_results.at(0).mindate ||
				   d2 != m_all_results.at(0).maxdate))
  {
    clear_graph();
  }

  QCursor cursor(Qt::WaitCursor);
  QApplication::setOverrideCursor(cursor);

  query = QString("SELECT d.x::date as day,cnt FROM (SELECT date_trunc('day', msg_date)::date AS day,count(*) AS cnt FROM %4 WHERE msg_date>='%1'::date AND msg_date<'1 day'::interval+'%2'::date %3 GROUP BY 1) AS s RIGHT JOIN generate_series('%1'::date,'%2'::date,'1 day'::interval) AS d(x) ON(day=d.x) ORDER BY 1").
    arg(d1.toString(Qt::ISODate)).arg(d2.toString(Qt::ISODate)).arg(criteria).arg(main_table);

  db_cnx db;
  try {
    sql_stream s(query, db);
    m_last_results.dates.clear();
    m_last_results.values.clear();
    while (!s.eos()) {
      int count;
      QString day;
      s >> day >> count;
      // DBG_PRINTF(4, "%s:%d", day.toLocal8Bit().constData(), count);
      m_last_results.dates.append(day);
      m_last_results.values.append(count);
    }

    struct result_msgcount res1;
    res1.values = m_last_results.values;
    res1.color = m_color;
    res1.mindate = d1;
    res1.maxdate = d2;
    m_all_results.append(res1);

    show_graph(m_last_results.dates);
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
  }

  QApplication::restoreOverrideCursor();
}

stats_webview::stats_webview(QWidget* parent) : QWebView(parent)
{
  m_x = 600;
  m_y = 400;
  m_queue_redisplay = false;
  setMinimumSize(m_x, m_y);
  //  page()->mainFrame()->setScrollBarPolicy( Qt::Vertical, Qt::ScrollBarAlwaysOff );
  //  page()->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
}

void
stats_webview::resizeEvent(QResizeEvent* event)
{
  //  DBG_PRINTF(4, "resizeevent()");
  int w = (int)(600.0*(event->size().width()/600.0));
  int h = (int)(400.0*(event->size().height()/400.0));
  m_x = w;
  m_y = h;
  if (!m_queue_redisplay) {
    m_queue_redisplay = true;
    QTimer::singleShot(300, this, SLOT(redisplay()));
  }
  QWebView::resizeEvent(event);
}

void
stats_webview::display(QString& script)
{
  m_template = script;
  redisplay();
}

void
stats_webview::redisplay()
{
  QUrl baseref("/");
  QString path = gl_pApplication->chartjs_path();
  QString src_js;
  if (!path.isNull()) {
    src_js = "web/Chart.js/dist/Chart.min.js";
    baseref = QUrl::fromLocalFile(QDir(path).canonicalPath()+"/"); // ending slash required!
  }
  else {
    src_js = "https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.1.6/Chart.min.js";
  }
  QString prologue = QString("<html><head><script src=\"%1\"></script></head>"
			     "<body><canvas id='myChart' width='%2' height='%3'></canvas>").
    arg(src_js).arg(m_x-50).arg(m_y-50);

  QString epilogue = "</body></html>";

  this->setHtml(QString("%1<script>%2</script>%3").arg(prologue).arg(m_template).arg(epilogue),
		     baseref);
  m_queue_redisplay = false;
}

void
stats_webview::clear()
{
  m_template.truncate(0);
  page()->mainFrame()->evaluateJavaScript("myChart.clear();");
}
