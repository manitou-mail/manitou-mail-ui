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

#include "main.h"
#include "attachment.h"
#include "app_config.h"
#include <fstream>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegExp>
#include <QTextCodec>
#include <QProcess>
#include <QMessageBox>
#include <QTimer>
#include <QUuid>

#include "sha1.h"
#include "unistd.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>
#endif

attachment::attachment() :
  m_Id(0), m_data(NULL)
{
}

attachment::attachment(const attachment& a)
{
  m_data = a.m_data;
  m_Id = a.m_Id;
  m_filename = a.m_filename;
  m_mime_type = a.m_mime_type;
  m_size = a.m_size;
  m_charset = a.m_charset;
  m_mime_content_id = a.m_mime_content_id;
}

/* Duplicate the metadata and ID of src, excluding memory contents */
attachment
attachment::dup_no_data() const
{
  attachment a;

  a.m_data = NULL;
  a.m_Id = m_Id;
  a.m_filename = m_filename;
  a.m_mime_type = m_mime_type;
  a.m_size = m_size;
  a.m_charset = m_charset;
  a.m_mime_content_id = m_mime_content_id;
  return a;
}

attachment::~attachment()
{
  free_data();
}

bool
attachment::is_binary()
{
  const unsigned char* contents = (unsigned char*)get_contents();
  for (uint i=0; i<m_size; i++) {
    if (contents[i]!='\t' && contents[i]!='\r' && contents[i]!='\n' &&
	(contents[i]<0x20 || contents[i]>=0x7f))
      return true;
  }
  return false;
}

QString
attachment::default_os_application()
{
#ifdef Q_OS_WIN
  QString ext = extension(filename());
  if (!ext.isEmpty()) {
    ext.prepend(".");
    DWORD dwSize = 255;
    TCHAR sBuffer[MAX_PATH] = {0};
    wchar_t* exta = new wchar_t[ext.length()+1];
    ext.toWCharArray(exta);
    exta[ext.length()]='\0';
    HRESULT hr = AssocQueryString((ASSOCF)0, ASSOCSTR_FRIENDLYAPPNAME, exta, L"Open", sBuffer, &dwSize);
    delete exta;
    return QString::fromWCharArray(sBuffer);
  }
#endif
  return QString();
}

QString
attachment::sha1_to_base64(unsigned int digest[5])
{
  QString res;
  const char* alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned int mask;
  unsigned int idx;
 // for each 6 bits block, i being the highest bit number of the block
  for (int i=32*5-1; i>0; i-=6) {
    if (i%32 >= 5) {
      // 6 bits block doesn't cross word boundary
      mask = 0x3f << ((i%32)-5);
      idx = (digest[4-i/32] & mask) >> ((i%32)-5);
    }
    else {
      // 6 bits block crosses word boundary
      unsigned int nbd=5-(i%32);
      mask = 0x3f >> nbd;
      idx = (digest[4-i/32] & mask) << nbd;
      if (i/32>0) {  // if we're not at the rightmost word
	mask = ((1<<(nbd+1))-1) << (32-nbd);
	idx += (digest[1+4-i/32] & mask) >> (32-nbd);
      }
    }
    res.append(alpha[idx]);
  }
  return res;
}

void
attachment::compute_sha1_fp()
{
  if (m_filename.length()>0) {
    get_size_from_file();
  }
  QFile f(m_filename);
  if (f.open(QIODevice::ReadOnly)) {
    SHA1 sha1;
    sha1.Reset();
    while (!f.atEnd() && !f.error()) {
      unsigned char buf[8192];
      qint64 n_read;
      while ((n_read=f.read((char*)buf, sizeof(buf))) >0) {
	sha1.Input(buf, (unsigned int)n_read);	
      }
    }
    if (!f.error()) {
      unsigned int digest[5];
      sha1.Result(digest);
      m_sha1_b64 = sha1_to_base64(digest);
    }
    f.close();
  }
}

QString
attachment::application() const
{
  db_cnx db;
  try {
    sql_stream s("SELECT program_name FROM programs WHERE content_type=':p1' AND conf_name=:p2", db);
    s << m_mime_type << get_config().name();
    QString prog;
    if (!s.eof()) {
      s >> prog;
    }
    else {
      sql_stream s2("SELECT program_name FROM programs WHERE content_type=':p1' AND conf_name is null", db);
      s2 << m_mime_type;
      if (!s2.eof()) {
	s2 >> prog;
      }
    }
    return prog;
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return QString();
  }
}

QString
attachment::get_temp_location()
{
  QString fname;

  if (m_filename.isEmpty()) {
    /* try to find a proper extension for the filename we generate,
       since viewer programs may rely on it.  It's also required for
       the '<winshell>' pseudo-viewer to have a chance to work. */
    QString extension;
    if (!m_mime_type.isEmpty()) {
      QMap<QString,QString> extension_map;
      fetch_filename_suffixes(extension_map); // TODO: cache this
      QMap<QString,QString>::const_iterator it = extension_map.constBegin();
      for (; it != extension_map.constEnd(); ++it) {
	if (it.value() == m_mime_type) {
	  extension = it.key();
	  break;
	}
      }
    }
    fname = QString("attch-%1").arg(m_Id);
    if (!extension.isEmpty()) {
      fname.append('.');
      fname.append(extension);
    }
  }
  else {
    fname = QString("attch-%1-%2").arg(m_Id).arg(m_filename);
  }
  QString dirname=get_config().get_string("attachments_directory");
  QDir dir;
  if (!dirname.isEmpty()) {
    dir = QDir(dirname);
    if (!dir.exists())
      dir = QDir::temp();
  }
  else
    dir = QDir::temp();
  return dir.absoluteFilePath(fname);
}

void
attachment::launch_os_viewer(const QString document_path)
{
#ifdef Q_OS_WIN
  SHELLEXECUTEINFO shellInfo;
  memset(&shellInfo, 0, sizeof(SHELLEXECUTEINFO));
  shellInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  shellInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  shellInfo.lpFile = (LPCTSTR)document_path.utf16();
  shellInfo.lpParameters = NULL;
  shellInfo.nShow = SW_SHOW;
  ::ShellExecuteEx(&shellInfo);
#else
  Q_UNUSED(document_path);
#endif
}

void
attachment::launch_external_viewer(const QString document_path)
{
  QString command_string=application().trimmed();
  QRegExp qr=QRegExp("\\$1");

  if (!command_string.isEmpty()) {
#ifdef Q_OS_WIN
    if (command_string == "<winshell>") {
      /* As a special case for Windows, <winshell> means that the
	 attachment's name itself is passed to ShellExecute. It assumes
	 that it triggers the association at the OS level between the file
	 extension and a program */
      launch_os_viewer(document_path);
      return;
    }
#endif
    QString q_doc_path = document_path;
    if (document_path.indexOf(" ")>=0) {
      // enclose between quotes if the path contains spaces
      q_doc_path.prepend('"');
      q_doc_path.append('"');
    }
    if (command_string.indexOf(qr) >=0) // replace $1 if specified
      command_string.replace(qr, q_doc_path);
    else {
      command_string.append(" ");
      command_string.append(q_doc_path);
    }
    if (!QProcess::startDetached(command_string)) {
      QMessageBox::warning(NULL, "Error", QApplication::tr("Unable to run command:\n%1").arg(command_string));
    }
  }
}

void
attachment::set_contents(const char* contents, uint size)
{
  m_data = (char*) malloc(size+1);
  if (!m_data)
    return;
  memcpy (m_data, contents, size);
  m_data[size] = '\0';
  m_size = size;
}

void
attachment::set_contents_ptr(char* contents, uint size)
{
  m_data = contents;
  m_size = size;
}

void
attachment::append_decoded_contents(QString& body)
{
  char* contents = get_contents();
  if (!contents)
    return;
  if (charset().isEmpty() || charset()==QString("us-ascii")
      || charset()==QString("US-ASCII")) {
    body.append(contents);
  }
  else {
    QTextCodec* codec = QTextCodec::codecForName(charset().toLatin1());
    if (!codec) {
      body.append(contents);
    }
    else {
      if (m_data)
	body.append(codec->toUnicode(m_data, m_size));
    }
  }
}

char*
attachment::get_contents()
{
  if (m_data)
    return m_data;

  db_cnx db;
  try {
    Oid lobjId;
    if (size()==0)
      return NULL;
    sql_stream s("SELECT content FROM attachment_contents WHERE attachment_id=:p1", db);
    s << m_Id;
    if (!s.eos()) {
      s >> lobjId;
    }
    else
      return NULL;		// oops, no (more) content here

    // we allocate one more byte for a '\0' terminator, so that
    // the attachment content can be used as a C string if needed
    m_data = (char*) malloc(size()+1);
    if (!m_data)
      return NULL;

    db.begin_transaction();
    PGconn* c=db.connection();
    int lobj_fd = lo_open(c, lobjId, INV_READ);
    if (lobj_fd < 0) {
      DBG_PRINTF(2, "failed to open large object %u", (uint)lobjId);
      throw db_excpt("lo_open", db);
    }
    lo_read(c, lobj_fd, m_data, size());
    lo_close(c, lobj_fd);
    db.commit_transaction();

    m_data[size()]='\0';
    return m_data;
  }
  catch(db_excpt& p) {
    if (m_data) {
      free(m_data);
      m_data=NULL;
    }
    db.rollback_transaction();
    DBEXCPT(p);
    return NULL;
  }
}

#if 0
// Unused: see streamout_chunk
void
attachment::streamout_content(std::ofstream& of)
{
  if (m_data) {
    of.write(m_data,size());
    return;
  }
  if (size()==0) {
    of.write("", 0);
    return;
  }

  db_cnx db;
  try {
    sql_stream s("SELECT content FROM attachment_contents WHERE attachment_id=:p1", db);
    s << m_Id;
    Oid lobjId;
    if (!s.eos()) {
      s >> lobjId;
    }
    else
      return;  // nothing to read from

    db.begin_transaction();
    PGconn* c=db.connection();
    int lobj_fd = lo_open(c, lobjId, INV_READ);
    if (lobj_fd < 0) {
      DBG_PRINTF(7, "lo_open returns %d", lobj_fd);
      throw db_excpt("lo_open", db);
    }
    char data[8192];
    unsigned int nread;
    do {
      nread = lo_read(c, lobj_fd, data, sizeof(data));
      of.write(data, nread);
    } while (nread==sizeof(data));
    lo_close(c, lobj_fd);
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
  }
}
#endif

int
attachment::open_lo(struct lo_ctxt* slo)
{
  /* allocate a connection on the heap rather than on the stack
     because we'll need it through all the use of the 'slo' context */
  db_cnx* db = new db_cnx();
  slo->db=db;
  PGconn* c=db->connection();
  try {
    sql_stream s("SELECT content FROM attachment_contents WHERE attachment_id=:p1", *db);
    s << m_Id;
    Oid lobjId;
    if (!s.eos()) {
      s >> lobjId;
    }
    else {
      slo->lfd=-1;
      return 0;
    }
    db->begin_transaction();
    int lobj_fd = lo_open(c, lobjId, INV_READ);
    DBG_PRINTF(7, "lo_open returns %d", lobj_fd);
    if (lobj_fd < 0) {
      slo->lfd=-1;
      throw db_excpt("lo_open", *db);
    }
    slo->eof = false;
    slo->db = db;
    slo->lfd = lobj_fd;
    slo->size=size();
    slo->chunk_size=8192;
    return 1;
  }
  catch(db_excpt& p) {
    db->rollback_transaction();
    DBEXCPT(p);
    return 0;
  }
}

/* Transfer a chunk of data from a already opened large object to an
  ofstream */
int
attachment::streamout_chunk(struct lo_ctxt* slo, QIODevice* of)
{
  char data[8192];
  unsigned int nread;
  PGconn* c = slo->db->connection();
  nread = lo_read(c, slo->lfd, data, sizeof(data));
  of->write(data, (qint64)nread);
  return (nread==sizeof(data));	// true if not done yet
}

/*
  Release a large object context
*/
void
attachment::close_lo(struct lo_ctxt* slo)
{
  if (slo->lfd>=0) {
    PGconn* c = slo->db->connection();
    DBG_PRINTF(7, "lo_close(%d)", slo->lfd);
    lo_close(c, slo->lfd);
    slo->db->commit_transaction();
  }
  delete slo->db;
  slo->db=NULL;
}

void
attachment::free_data()
{
  if (m_data) {
    free (m_data);
    m_data=NULL;
  }
}

QString
attachment::human_readable_size()
{
  int s = size();
  const int factor = 1000;
  if (s < factor)
    return QString("%1 bytes").arg(s);
  /* display 1 number after the decimal dot */
  else if (s < factor*factor)
    return QString("%L1 kB").arg(s/(double)factor, 0, 'f', 1);
  else
    return QString("%L1 MB").arg(s/((double)factor*factor), 0, 'f', 1);
}

/* Reinsert an identical attachment and assign it to a different mail
   (the contents are not duplicated).
   When called, m_Id is the id of the existing attachment (source of copy)
   If the duplication is successful, m_Id is overwritten by the id of
   the target attachement.
   Can throw db_excpt and app_exception.
 */

bool
attachment::duplicate_existing(mail_id_t mail_id, db_ctxt* dbc)
{
  db_cnx& db = *(dbc->m_db);
  attach_id_t new_id;
  static const char* errtxt = QT_TR_NOOP("The contents from the referenced attachment:\n %1\nare no longer accessible in the database.\nThis attachment must be removed from the mail being composed.");

  sql_stream s("INSERT INTO attachments(attachment_id, mail_id, content_type,"
	       " content_size, filename,mime_content_id)"
	       " SELECT nextval('seq_attachment_id'),"
	       ":mail_id,"
	       "content_type, content_size, filename, mime_content_id"
	       " FROM attachments WHERE attachment_id=:att_id"
	       " RETURNING attachment_id", db);
  s << mail_id << m_Id;
  if (!s.eos()) {
    s >> new_id;
    sql_stream s2("INSERT INTO attachment_contents(attachment_id, content, fingerprint)"
		    " SELECT :target_id, content, fingerprint"
		    " FROM attachment_contents WHERE attachment_id=:source_id", db);
    s2 << new_id << m_Id;
    if (s2.affected_rows()==0) {
      /* Can't duplicate: the source attachment is no longer accessible */
      throw app_exception(QObject::tr(errtxt).arg(m_filename));
    }
    m_Id = new_id;
  }
  else {
    /* Can't duplicate, as above */
    throw app_exception(QObject::tr(errtxt).arg(m_filename));
  }

  return true;
}

bool
attachment::store(mail_id_t mail_id, ui_feedback* ui)
{
  db_cnx db;
  try {
    db.begin_transaction();

    if (!db.next_seq_val("seq_attachment_id", &m_Id))
      return false;

    if (m_filename.length()>0) {
      get_size_from_file();
      compute_sha1_fp();
    }

    sql_stream s("INSERT INTO attachments(attachment_id, mail_id, content_type, content_size, filename,mime_content_id) VALUES (:p1, :p2, ':p3', :p4, ':p5', :cid)", db);
    QFileInfo fi(m_filename);
    s << m_Id << mail_id << m_mime_type << m_size << fi.fileName();

    if (!m_mime_content_id.isEmpty())
      s << m_mime_content_id;
    else
      s << sql_null();

    if (!import_file_content(ui)) {
      DBG_PRINTF(2, "Error while importing file contents: %s", m_filename.toLocal8Bit().constData());
      db.rollback_transaction();
      return false;
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

QString  // [static]
attachment::guess_mime_type(const QString filename)
{
  typedef QMap<QString,QString> mt;
  static mt extension_map;
  if (extension_map.isEmpty()) {
    fetch_filename_suffixes(extension_map);
  }

  QString ext = extension(filename);
  if (!ext.isEmpty()) {
    mt::const_iterator it = extension_map.constFind(ext);
    if (it != extension_map.constEnd())
      return it.value();
  }
  return QString("application/octet-stream");	// nothing found
}

// static
QString
attachment::extension(const QString filename)
{
  int dotpos = filename.lastIndexOf('.');
  if (dotpos >=0 && dotpos+1 < filename.length())
    return filename.mid(dotpos+1);
  else
    return QString();
}

void
attachment::get_size_from_file()
{
  if (QFile::exists(m_filename)) {
    QFile f(m_filename);
    m_size=(uint)f.size();
  }
  else
    m_size=0;
}

//static
bool
attachment::fetch_filename_suffixes(QMap<QString,QString>& m)
{
  db_cnx db;
  try {
    sql_stream s("SELECT suffix, mime_type FROM mime_types", db);
    QString suffix;
    QString mime_type;
    while (!s.eos()) {
      s >> suffix >> mime_type;
      m[suffix]=mime_type;
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

void
attachment::create_mime_content_id()
{
  QUuid uid = QUuid::createUuid();
  QString str = uid.toString();
  str.replace(QChar('{'), "").replace(QChar('}'), "");
  m_mime_content_id = str + "@mm";
}

/*
  Insert the contents of a file into the ATTACHMENT_CONTENTS table
  members updated: m_size, m_Id
*/
bool
attachment::import_file_content(ui_feedback* ui)
{
  db_cnx db;

  try {
    db.begin_transaction();
    Oid lobj_id=0;

    /* Look up the attachment by its fingerprint.
       If it's already found in the db, we create a new reference to its large-object id
       without importing a new copy of the contents */
    if (!m_sha1_b64.isEmpty()) {
      sql_stream sfp("SELECT content FROM attachment_contents WHERE fingerprint=:p1", db);
      sfp << m_sha1_b64;
      if (!sfp.eos()) {
	sfp >> lobj_id;
      }
    }

    if (m_filename.length()>0) {
      if (lobj_id==0) {
	QByteArray qb_fname = QFile::encodeName(m_filename);
	if (!ui) {
	  // if not reporting progress, import in one go
	  lobj_id = lo_import(db.connection(), qb_fname.constData());
	  if (lobj_id==0) {
	    throw QString("Error lo_import filename=%1").arg(qb_fname.constData());
	  }
	}
	else {
	  QFileInfo fi(m_filename);
	  int size=0;
	  const int max_size = (1U<<31)-1; // 2GB: max size of a PG large object
	  if (fi.exists()) {
	    if (fi.size() > max_size)
	      size = max_size;
	    else
	      size = (int)fi.size();
	  }
	  else {
	    DBG_PRINTF(2, "Error: file '%s' does not exist", qb_fname.constData());
	    db.rollback_transaction();
	    return false;
	  }
	  const int chunk_size = 65536/8;
	  ui->set_maximum((size+chunk_size-1)/chunk_size);
	  ui->set_status_text(QObject::tr("Uploading file %1").arg(fi.fileName()));
	  PGconn* cnx=db.connection();
	  lobj_id = lo_creat(cnx, INV_READ | INV_WRITE);
	  if (lobj_id<=0)
	    throw QString("lo_creat failed");

	  int fd = lo_open(cnx, lobj_id, INV_WRITE);
	  if (fd<0)
	    throw QString("Error lo_open lobj_id=%1").arg(lobj_id);

	  QFile file(m_filename);
	  char buffer[chunk_size];
	  qint64 rd=0;
	  qint64 written=0;
	  int i=0;
	  if (!file.open(QIODevice::ReadOnly))
	    throw QString("Error opening file");

	  while (rd>=0 && !file.atEnd() && !file.error() && written<max_size) {
	    rd=file.read(buffer, chunk_size);
	    if (rd>0) {
	      int nb = lo_write(cnx, fd, buffer, rd); // returns negative on failure
	      if (nb>=0)
		written += nb;
	      else {
		throw QString("Error lo_write lobj_id=%1").arg(lobj_id);
	      }
	    }
	    ui->set_value(++i);
	  }
	  lo_close(cnx, fd);
	}
      }
    }
    else if (m_size>0 && m_data!=NULL) {
      if (lobj_id==0) {
	lobj_id = lo_creat(db.connection(), INV_READ | INV_WRITE);
	int lobj_fd = lo_open (db.connection(), lobj_id, INV_WRITE);
	lo_write(db.connection(), lobj_fd, m_data, m_size);
	lo_close(db.connection(), lobj_fd);
      }
    }
    else {
      DBG_PRINTF(2,"no filename and no size or no data");
      db.rollback_transaction();
      return true;		// nothing to store
    }

    sql_stream s("INSERT INTO attachment_contents(attachment_id, content, fingerprint) VALUES (:p1,:p2,:p3)", db);
    s << m_Id << (unsigned long)lobj_id;
    if (!m_sha1_b64.isEmpty())
      s << m_sha1_b64;
    else
      s << sql_null();
    db.commit_transaction();
  }
  catch(db_excpt& p) {
    db.rollback_transaction();
    DBEXCPT(p);
    return false;
  }
  catch(QString err) {
    DBG_PRINTF(2, err.toLocal8Bit().constData());
    db.rollback_transaction();
    return false;
  }
  return true;
}

attachments_list::attachments_list() : m_mailId(0), m_bFetched(false)
{
}

attachments_list::~attachments_list()
{
}

bool
attachments_list::fetch()
{
  if (m_bFetched)
    return true;
  try {
    db_cnx db;
    sql_stream s("SELECT attachment_id,content_type,content_size,filename,charset,mime_content_id FROM attachments WHERE mail_id=:p1 ORDER BY attachment_id", db);
    s << m_mailId;
    while (!s.eos()) {
      attachment attch;
      int size;
      attach_id_t id;
      QString filename, content_type, charset, mime_content_id;
      s >> id >> content_type >> size >> filename >> charset >> mime_content_id;
      attch.setAll(id, size, filename, content_type, charset);
      attch.set_mime_content_id(mime_content_id);
      push_back(attch);
    }
  }
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  m_bFetched=true;
  return true;
}

attachment*
attachments_list::get_by_content_id(const QString mime_content_id)
{
  if (mime_content_id.isEmpty())
    return NULL;
  std::list<attachment>::iterator it;
  for (it=begin(); it!=end(); it++) {
    DBG_PRINTF(7, "attch %d,mime_content_id=%s", (*it).getId(), (*it).mime_content_id().toLocal8Bit().constData());
    if ((*it).mime_content_id() == mime_content_id)
      return &(*it);
  }
  DBG_PRINTF(2,"attachment not found by content_id");
  return NULL;
}

bool
attachments_list::store(db_ctxt* dbc, ui_feedback* ui)
{
  std::list<attachment>::iterator it;
  for (it=begin(); it!=end(); ++it) {
    if ((*it).getId() != 0) {
      /* When the attachment already exists in the database for another mail,
	 create a new reference to it. It might fail if the attachment has
         been deleted after we picked it up, in which an app_exception
         will be thrown */
      if (!(*it).duplicate_existing(m_mailId, dbc))
	return false;
    }
    else if (!(*it).store(m_mailId, ui))
      return false;
  }
  return true;
}

attachment_viewer::attachment_viewer()
{
}

attachment_viewer::~attachment_viewer()
{
}

attch_viewer_list::attch_viewer_list() : m_fetched(false)
{
}

attch_viewer_list::~attch_viewer_list()
{
}

bool
attch_viewer_list::fetch(const QString& conf_name, bool force)
{
  if (m_fetched && !force)
    return true;
  try {
    db_cnx db;
    sql_stream s("SELECT program_name,content_type FROM programs WHERE conf_name=:p2 OR conf_name IS NULL", db);
    s << conf_name;
    while (!s.eos()) {
      attachment_viewer v;
      s >> v.m_program >> v.m_mime_type;
      push_back(v);
    }
    m_fetched=true;
  }  
  catch(db_excpt& p) {
    DBEXCPT(p);
    return false;
  }
  return true;
}

bool
attachment::open()
{
  if (open_lo(&m_lo))
    return true;
  else
    return false;
}

qint64
attachment::read(qint64 size, char* buf)
{
  if (size==0) return 0;
  PGconn* c = m_lo.db->connection();
  int r = lo_read(c, m_lo.lfd, buf, (size_t)size);
  DBG_PRINTF(7, "attachment::read requested %d bytes, read %d bytes on lfd=%d",
	     (size_t)size, r, m_lo.lfd);
  if (r<size || r==0) {
    m_lo.eof=true;
    return r;
  }
  else {
    return (qint64)r;
  }
}

bool
attachment::eof() const
{
  return m_lo.eof;
}

void
attachment::close()
{
  close_lo(&m_lo);
}

attachment_network_reply*
attachment::network_reply(const QNetworkRequest& req, QObject* parent)
{
  return new attachment_network_reply(req, this, parent);
}

attachment_network_reply::attachment_network_reply(const QNetworkRequest &req, attachment* a, QObject* parent) : QNetworkReply(parent)
{
  // created with all the data at once
  m_a=a;
  if (!a->open()) {
    setError(ContentNotFoundError, "Content not found in database");
    DBG_PRINTF(2, "error in opening attachment (attachment_id=%d)", a->getId());
  }
  else {
    DBG_PRINTF(7, "attachment opened (attachment_id=%d)", a->getId());
    setRequest(req);
    setOperation(QNetworkAccessManager::GetOperation);
    setReadBufferSize(0);
    open(QIODevice::ReadOnly/* | QIODevice::Unbuffered*/);
    setError(NoError, "No Error");
  }
  QTimer::singleShot(0, this, SLOT(go()));
}

attachment_network_reply::~attachment_network_reply()
{
  DBG_PRINTF(7, "~attachment_network_reply()");
}

qint64
attachment_network_reply::readData(char* data, qint64 size)
{
  DBG_PRINTF(7, "readData size=%ld", size);
  if (m_a->eof()) {
    DBG_PRINTF(7, "eof on attachment");
    return -1;
  }
  qint64 res=m_a->read(size, data);
  DBG_PRINTF(7, "read %ld bytes", res);
  if (!m_a->eof() && res==size) {
    QTimer::singleShot(0, this, SLOT(go_ready_read())); // emit readyRead();
  }
  if (m_a->eof()) {
    QTimer::singleShot(0, this, SLOT(go_finished())); //emit finished();
  }
  return res;
}

bool
attachment_network_reply::atEnd() const
{
  bool b=m_a->eof();
  DBG_PRINTF(7, "atEnd returns %d", b);
  return m_a->eof();
}

/* The base QIODevice::isSequential() returns false. */
bool
attachment_network_reply::isSequential() const
{
  return true;
}

void
attachment_network_reply::abort()
{
  DBG_PRINTF(7, "abort");
  m_a->close();
}


qint64
attachment_network_reply::bytesAvailable() const
{
  qint64 ret=(!m_a->eof()?32768:0) + QNetworkReply::bytesAvailable();
  DBG_PRINTF(7, "bytesAvailable returning %lld", ret);
  return ret;
}

void attachment_network_reply::go()
{
  setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200); 
  setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, "OK"); 
  emit metaDataChanged(); 
  NetworkError err = error(); 
  if (err != NoError) {
    /* We no longer emit error(err) here otherwise the load of the document
       containing the attachment apparently never finishes.
       This happens in particular for empty attachments */
    DBG_PRINTF(7, "emit finished");
    emit finished();
  }
  else {
    DBG_PRINTF(7, "emit readyRead");
    emit readyRead(); 
  } 
}

void
attachment_network_reply::go_finished()
{
  DBG_PRINTF(7, "go_finished");
  emit finished();
}

void
attachment_network_reply::go_ready_read()
{
  DBG_PRINTF(7, "go_ready_read");
  emit readyRead(); 
}

attachment_iodevice::attachment_iodevice(attachment* a, QObject* parent) : QIODevice(parent)
{
  m_a = a;
  if (!a->open()) {
    DBG_PRINTF(2, "error in opening attachment (attachment_id=%d)", a->getId());
  }
  else {
    open(QIODevice::ReadOnly/* | QIODevice::Unbuffered*/);
  }
  QTimer::singleShot(0, this, SLOT(go()));
}

attachment_iodevice::~attachment_iodevice()
{
}

void
attachment_iodevice::close()
{
  if (m_a != NULL)
    m_a->close();		// ends database transaction
  QIODevice::close();
}

qint64
attachment_iodevice::readData(char* data, qint64 size)
{
  if (m_a->eof()) {
    return -1;
  }
  qint64 res=m_a->read(size, data);
  if (!m_a->eof() && res==size) {
    QTimer::singleShot(0, this, SLOT(go_ready_read())); // emit readyRead();
  }
  if (m_a->eof()) {
    QTimer::singleShot(0, this, SLOT(go_finished())); //emit finished();
  }
  return res;
}

qint64
attachment_iodevice::writeData(const char* data, qint64 maxsize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxsize);
  // not implemented, read-only
  return 0;
}

bool
attachment_iodevice::atEnd() const
{
  return m_a->eof();
}

/* The base QIODevice::isSequential() returns false. */
bool
attachment_iodevice::isSequential() const
{
  return true;
}

void
attachment_iodevice::abort()
{
  if (m_a != NULL)
  m_a->close();
}


qint64
attachment_iodevice::bytesAvailable() const
{
  return (!m_a->eof()?32768:0) + QIODevice::bytesAvailable();
}

void attachment_iodevice::go()
{
  emit readyRead();
}

void
attachment_iodevice::go_finished()
{
  emit readChannelFinished();
}

void
attachment_iodevice::go_ready_read()
{
  emit readyRead();
}
