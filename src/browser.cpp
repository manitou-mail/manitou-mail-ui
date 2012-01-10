/* Copyright (C) 2004-2008 Daniel Vérité

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

#include "browser.h"
#include "main.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>


browser::browser()
{
}

browser::~browser()
{
}

void
browser::open_url(const QString& u)
{
  QString url=u;
  /* cope with client programs that won't default to the http protocol,
     such as kfmclient */
  if (url.indexOf("http://", 0, Qt::CaseInsensitive)!=0 && url.indexOf("https://", 0, Qt::CaseInsensitive)!=0) {
    url.prepend("http://");
  }

  if (!QDesktopServices::openUrl(QUrl(url))) {
    QMessageBox::critical(NULL, tr("Error"), tr("Unable to open URL"));
  }
}

void
browser::open_url(const QUrl& url)
{
  browser::open_url(url.toString());
}
