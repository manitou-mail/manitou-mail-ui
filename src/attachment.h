/* Copyright (C) 2004-2025 Daniel Verite

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

#ifndef INC_ATTACHMENT_H
#define INC_ATTACHMENT_H

#include <QMap>
#include <fstream>
#include "dbtypes.h"
#include "database.h"
#include "sqlstream.h"
#include "ui_feedback.h"
#include <QStringList>
#include <QNetworkReply>
#include <QNetworkRequest>

class attachment_network_reply;

class attachment
{
public:
  struct lo_ctxt {
    db_cnx* db;
    int lfd;
    unsigned int size;
    unsigned int chunk_size;
    bool eof;
  };
  attachment();
  attachment(const attachment&);
  virtual ~attachment();

  /* Duplicate the metadata and ID of src, excluding contents cached in memory */
  attachment dup_no_data() const;

  attach_id_t getId() const { return m_Id; }
  void setId(attach_id_t id) { m_Id=id; }

  void compute_sha1_fp();

  /* Allocate m_data if needed and fetch contents into m_data */
  char* get_contents();

  void get_size_from_file();

  int open_lo(struct lo_ctxt*);
  void close_lo(struct lo_ctxt*);
  int streamout_chunk(struct lo_ctxt* slo, QIODevice* of);

#if 0
  void streamout_content(std::ofstream&);
#endif
  bool store(mail_id_t mail_id, ui_feedback* ui=NULL);

  /* Reinsert an identical attachment and assign it to a different mail
     (the contents are not duplicated) */
  bool duplicate_existing(mail_id_t mail_id, db_ctxt* dbc);

  /* Insert the contents of a file into the ATTACHMENT_CONTENTS table
     members updated: m_size, m_Id */
  bool import_file_content(ui_feedback* ui=NULL);

  static QString guess_mime_type(const QString);
  static QString extension(const QString);

  const QString filename() { return m_filename; }
  QString display_filename() const;
  QString mime_type() { return m_mime_type; }
  QString charset() { return m_charset; }
  uint size() { return m_size; }
  /* rounded size with kB,Mb units */
  QString human_readable_size();

  QString application() const;
  QString default_os_application();
  void launch_external_viewer(const QString document_path);
  void launch_os_viewer(const QString document_path);

  attachment_network_reply* network_reply(const QNetworkRequest& req, QObject* parent);

  // returns the full path and name suitable to store a temporary
  // copy of the attachment 
  QString get_temp_location();

  void setAll(attach_id_t id, uint size, const QString filename, const QString mime_type,
	      const QString charset=QString()) {
    m_Id = id;
    m_filename = filename;
    m_size = size;
    m_mime_type = mime_type;
    m_charset = charset;
  }
  void set_mime_content_id(const QString id) {
    m_mime_content_id=id;
  }
  const QString mime_content_id() const {
    return m_mime_content_id;
  }
  void create_mime_content_id();

  void set_filename(const QString filename) {
    m_filename=filename;
  }
  void set_mime_type(const QString mimetype) {
    m_mime_type = mimetype;
  }
  void set_final_filename(const QString name) {
    m_final_filename = name;
  }

  // allocate and copy the contents
  void set_contents(const char* contents, uint size);

  // append to 'body' the contents of the attachment. May include a conversion
  // of the attachment contents to unicode.
  void append_decoded_contents(QString& body);

  // don't allocate or copy the contents
  void set_contents_ptr(char* contents, uint size);

  // returns true if the contents include non-ASCII characters
  bool is_binary();


  // functions for streaming contents when driven by attachment_network_reply
  // instantiated by webkit
  bool open();
  qint64 read(qint64 size, char* buf);
  bool eof() const;
  void close();

  static bool fetch_filename_suffixes(QMap<QString,QString>&);

  // check for unsafe characters in filenames
  static bool check_filename(const QString name, QString& error);
  static void fixup_filename(QString& input);

protected:
  // attachments.attachment_id in database, 0 when not in database
  attach_id_t m_Id = 0;

  char *m_data;			// with malloc() and free()
  QString m_filename;
  QString m_final_filename;	// to insert in db with a different name than on filesystem
  QString m_charset;
  QString m_mime_type;
  QString m_mime_content_id;
  uint m_size;
  QString m_sha1_b64;
private:
  QString sha1_to_base64(unsigned int digest[5]);
  // dummy
  void free_data();
  struct lo_ctxt m_lo;
  // characters to avoid in attachment filenames
  static const QChar avoid_chars[9];
};

class attachment_network_reply : public QNetworkReply
{
  Q_OBJECT
public:
  attachment_network_reply(const QNetworkRequest &req, attachment* a,
			   QObject* parent);
  virtual ~attachment_network_reply();
  qint64 readData(char* data, qint64 size);
  void abort();
  qint64 bytesAvailable() const;
  bool atEnd() const;
  bool isSequential() const;
private:
  attachment* m_a;
  int already_read;
private slots:
  void go();
  void go_ready_read();
  void go_finished();
};

class attachment_iodevice: public QIODevice
{
  Q_OBJECT
public:
  attachment_iodevice(attachment* a, QObject* parent);
  virtual ~attachment_iodevice();
  qint64 readData(char* data, qint64 size);
  qint64 writeData(const char* data, qint64 maxsize);
  void abort();
  void close();
  qint64 bytesAvailable() const;
  bool atEnd() const;
  bool isSequential() const;
private:
  attachment* m_a = NULL;
  int already_read;
private slots:
  void go();
  void go_ready_read();
  void go_finished();
};

class attachments_list : public std::list<attachment>
{
public:
  attachments_list();
  virtual ~attachments_list();
  bool fetch();
  bool store(db_ctxt* dbc, ui_feedback* ui=NULL);
  void setMailId(mail_id_t id) { m_mailId=id; }
  attachment* get_by_content_id(const QString mime_content_id);

private:
  mail_id_t m_mailId;
  bool m_bFetched;
};

class attachment_viewer {
public:
  attachment_viewer();
  virtual ~attachment_viewer();
  QString m_mime_type;
  QStringList m_suffixes;	// not used yet
  QString m_program;
};

class attch_viewer_list : public std::list<attachment_viewer>
{
public:
  attch_viewer_list();
  virtual ~attch_viewer_list();
  bool fetch(const QString& conf_name, bool force=false);
private:
  bool m_fetched;
};

#endif
