/* Copyright (C) 2004-2019 Daniel Verite

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
#include "app_config.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>


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
  browser::open_url(QUrl(url));
}

void
browser::open_url(const QUrl& url)
{
  QString browser = get_config().get_string("browser");
  if (browser.isEmpty()) {
    DBG_PRINTF(4, "openUrl: %s", url.toString().toLocal8Bit().constData());
    if (!QDesktopServices::openUrl(url)) {
      QMessageBox::critical(NULL, tr("Error"), tr("Unable to open URL"));
    }
  }
  else {
    // a specific browser has been set in the preferences
    QStringList args;
    QString appname;
    /* Copy the url into a QByteArray and back to a QString instead of calling url.toString().
       The reason is QUrl::toString() would convert any percent-encoded chars to "human-readable"
       which we don't want */
    QByteArray b_url = url.toEncoded();
    if (b_url.isEmpty()) {
      QMessageBox::critical(NULL, tr("Error"), tr("Unable to parse URL"));
      return;
    }
    QString s_url = QString::fromLatin1(b_url.constData());

    int pos = browser.indexOf("$1");

    if (pos>=0) {
      browser.replace("$1", s_url);
      args = browser.split(" ", QString::SkipEmptyParts);
      appname = args.takeFirst();
    }
    else {
      args << s_url;
      appname=browser;
    }
    QProcess* p = new QProcess;
    DBG_PRINTF(4, "launch %s args=%s", appname.toLocal8Bit().constData(), args.join("\n").toLocal8Bit().constData());
    p->start(appname, args);
  }
}
