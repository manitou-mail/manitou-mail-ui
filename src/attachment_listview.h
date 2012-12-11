/* Copyright (C) 2004-2012 Daniel Verite

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

#ifndef INC_ATTACHMENT_LISTVIEW_H
#define INC_ATTACHMENT_LISTVIEW_H

#include <QPalette>
#include <QTreeWidget>
#include <QDialog>
#include "attachment.h"

class QMimeData;
class QRadioButton;
class QLabel;

class attch_listview: public QTreeWidget
{
  Q_OBJECT
public:
  attch_listview(QWidget* parent);
  virtual ~attch_listview();
  void set_attch_list(attachments_list* l) {
    m_pAttchList=l;
  }
  void progress_report(int);
  void allow_delete(bool b);
  void save_attachments();
protected:
  bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action);
  Qt::DropActions supportedDropActions() const;
  QStringList mimeTypes() const;

private:
  // ptr to the list of attachments of the message object
  attachments_list* m_pAttchList;
  bool m_abort; //  attachment download aborted
signals:
  void progress(int);
  void attach_file_request(const QUrl);
  void init_progress(const QString);
  void finish_progress();
public slots:
  void download_aborted();
private slots:
  void remove_current_attachment();
  void popup_ctxt_menu(const QPoint&);

  // check if fname exists and if it does ask whether it should
  // be overwritten or not
  bool confirm_write(const QString fname);

public:
  static QString m_last_attch_dir;
};

class attch_lvitem : public QTreeWidgetItem
{
public:
  //  attch_lvitem() {}
  attch_lvitem(attch_listview* p, attachment* a) : QTreeWidgetItem(p) {
    m_pAttachment = a;
    m_type = (a?0:1);
    m_listview=p;
  }
  attch_lvitem(attch_listview* p, QTreeWidgetItem* q, attachment* a) :
    QTreeWidgetItem(p,q) {
    m_pAttachment = a;
    m_type = (a?0:1);
    m_listview=p;
  }
  attch_listview* lview() const {
    return m_listview;
  }
  virtual ~attch_lvitem() {}
  void fill_columns();

  // ask for a filename and put the attachment contents to disk if confirmed
  // returns the filename
  QString save_to_disk(const QString filename, bool *abort);

  // download the attachment into 'destfilename'
  bool download(const QString destfilename, bool* abort);

  attachment* get_attachment() const {
    return m_pAttachment;
  }

  void set_note(const QString& note);
  bool is_note() const {
    return (m_type==1);
  }
private:
  int m_type; // 0=attachment, 1=note
  attachment* m_pAttachment;
  QString m_note;
  QPalette m_palette_for_notes;
  attch_listview* m_listview;
};

class attch_dialog_save: public QDialog
{
  Q_OBJECT
public:
  attch_dialog_save();
  void set_app_name(const QString);
  void set_file_name(const QString);
  int choice();
private:
  QRadioButton* m_rb_launch;
  QLabel* m_label_file;
};

#endif
