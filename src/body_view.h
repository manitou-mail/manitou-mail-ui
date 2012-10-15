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

#ifndef INC_BODY_VIEW_H
#define INC_BODY_VIEW_H

#include <QString>
#include <QWebView>
#include <QBuffer>
#include <QFont>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "searchbox.h"

class QNetworkRequest;
class QPaintEvent;
class QResizeEvent;
class QContextMenuEvent;
class mail_msg;
class QUrl;
//class QWheelEvent;

class network_manager : public QNetworkAccessManager
{
  Q_OBJECT
public:
  network_manager(QObject* parent) : QNetworkAccessManager(parent) {
    m_ext_download_permitted=false;
    m_ext_download_permission_asked=false;
  }
  ~network_manager() {}
  mail_msg* m_pmsg;
  QString m_body_style;
protected:
  QNetworkReply* createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData=0 );
private:
  QNetworkReply* empty_network_reply(Operation op, const QNetworkRequest& req);
public:
  bool m_ext_download_permission_asked;
  bool m_ext_download_permitted;
signals:
  void external_contents_requested();
public slots:
  void download_finished();
  void download_error(QNetworkReply::NetworkError);
  void download_progress(qint64, qint64);  
};

class body_view : public QWebView
{
  Q_OBJECT
public:
  body_view(QWidget* w=NULL);
  virtual ~body_view();
  void set_mail_item(mail_msg* p);
  mail_msg* mail_item() {
    return m_pmsg;
  }
  void set_wrap(bool on);
  static void rich_to_plain(QString&);
  void display(const QString& html);
  void redisplay();
  void clear();
  void copy();
  void highlight_terms(const std::list<searched_text>& );
  void authorize_external_contents(bool b);
  QSize sizeHint() const;
  void set_body_style();
  void clear_selection();
  void set_loaded(bool b) {
    m_loaded=b;
  }
  bool is_loaded() const {
    return m_loaded;
  }
  void prepend_body_fragment(const QString&);
  void set_font(const QFont&);
protected:
  void contextMenuEvent(QContextMenuEvent*);
private:
  mail_msg* m_pmsg;
  static const char* m_menu_entries[];
  bool m_can_move_forward;
  bool m_can_move_back;
  QString m_html_text;
  bool m_loaded;
  bool m_ext_download_permitted;
  bool m_ext_download_permission_asked;
  network_manager* m_netman;
  void force_style_sheet();
  QString m_font_css;
private slots:
  void ask_perm_for_contents();
signals:
  void external_contents_requested();
  void popup_body_context_menu();
};


class internal_style_network_reply : public QNetworkReply
{
  Q_OBJECT
public:
  internal_style_network_reply(const QNetworkRequest& req, const QString& style, QObject* parent);
  qint64 bytesAvailable() const;
  qint64 readData(char* data, qint64 size);
  void abort();
private slots:
  void go();
private:
  QBuffer m_buf;
};

class internal_img_network_reply : public QNetworkReply
{
  Q_OBJECT
public:
  internal_img_network_reply(const QNetworkRequest&, const QString&, int, QObject*);
  qint64 bytesAvailable() const;
  qint64 readData(char* data, qint64 size);
  bool seek(qint64);
  qint64 pos() const { return position; }
  qint64 size() const { return m_buffer.size(); }
  void abort() { }
private slots:
  void go();
private:
  QByteArray m_buffer;
  int position;
};

#endif // INC_BODY_VIEW_H
