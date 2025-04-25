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

#include "prog_chooser.h"
#include "main.h"

#include <QString>
#include <QStringList>
#include <QDir>

const char* pdf_viewers[] = {"acroread", "xpdf", NULL};
const char* img_viewers[] = {"kview", "xv", NULL};

struct progdef {
  const char* mime_type;
  const char* suffix;
  const char** progs;
};

static const struct progdef progs[] = {
  {"application/pdf", "pdf", pdf_viewers},
  {"image/gif", "gif", img_viewers},
  {"image/jpeg", "jpg", img_viewers}
};

prog_chooser::prog_chooser(QWidget* parent) : QDialog(parent)
{
}

prog_chooser::~prog_chooser()
{
}

void
prog_chooser::init(const QString& mime_type)
{  
#ifdef Q_OS_WIN
  QChar path_separator=QChar(';');
#else
  QChar path_separator=QChar(':');
#endif

  QByteArray qba = mime_type.toLatin1();
  const char* mt=qba.constData();
  const char** viewers=NULL;
  for (uint i=0; i<sizeof(progs)/sizeof(progs[0]); i++) {
    if (!strcmp(mt, progs[i].mime_type)) {
      viewers=progs[i].progs;
      break;
    }
  }
  if (viewers) {
    char* path=getenv("PATH");
    if (path) {
      QString qpath=QString(path);
      QStringList list=qpath.split(path_separator);
      while (*viewers) {
	QStringList::Iterator it;
	for (it=list.begin(); it!=list.end(); ++it) {
	  QString fullpath = *it + "/" + *viewers;
	  if (QFile::exists(fullpath)) {
//	    DBG_PRINTF(3, "helper found: %s\n", fullpath.latin1());
	    // TODO: continue
	  }
	}
	viewers++;
      }
    }
  }
}

//static
QString
prog_chooser::find_in_path(const QString& program)
{
#ifdef Q_OS_WIN
  QChar path_separator=QChar(';');
#else
  QChar path_separator=QChar(':');
#endif

  char* path=getenv("PATH");
  if (path) {
    QString qpath=path;
    QStringList list=qpath.split(path_separator);
    QStringList::Iterator it;
    for (it=list.begin(); it!=list.end(); ++it) {
      QString fullpath = *it + QDir::separator() + program;
      if (QFile::exists(fullpath)) {
	return fullpath;
      }
    }
  }
  return QString();
}
