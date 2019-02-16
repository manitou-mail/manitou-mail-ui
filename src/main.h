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

#ifndef INC_MAIN_H
#define INC_MAIN_H

#ifdef _MSC_VER
#pragma warning (disable:4786)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

#include "config.h"
#include "dbtypes.h"
#include "message.h"

#ifndef CHECK_PTR
#define CHECK_PTR(p) do { if (!p) abort(); } while(0)
#endif

#include <QString>
#include <QApplication>
#include <QDebug>

class QSystemTrayIcon;
class QStringList;
class mail_header;

#include <map>
//#include <string>

#if defined(__GNUG__) //&& !defined(QT_WS_WIN)
  #define _UNUSED_ __attribute((unused))
  #if __GNUG__>=3
	// standard (gcc3) way for defining a variable number of arguments
	#define DBG_PRINTF(l,...) debug_printf(l,__FILE__,__LINE__, __VA_ARGS__)
	#define ERR_PRINTF(...) err_printf(__FILE__,__LINE__, __VA_ARGS__)
  #else
	// or gcc2's way
	#define DBG_PRINTF(l,args...) debug_printf(l,__FILE__,__LINE__, ## args)
	#define ERR_PRINTF(args...) err_printf(__FILE__,__LINE__, ## args)
  #endif
#else
  // not gcc
  #define DBG_PRINTF {do { } while(0);}
  #define ERR_PRINTF {do { } while(0);}
  #define _UNUSED_
#endif

void debug_printf(int level, const char* file, int line, const char *fmt, ...);
void err_printf(const char* file, int line, const char *fmt, ...);
//void debug_printf1(int level, const char* file, int line, const char *fmt, ...);

#define APP_NAME "Manitou"

class FT
{
public:
  enum searchInEnum {
    searchInBodies=1,
    searchInSubjects=2,
//    searchInAuthors=4,
    searchInHeaders=4,
    searchMax=4
  };
  enum searchOptionsEnum {
    caseInsensitive=1,
    wrapAround=2,
    autoClose=4,
    optionsMax=4
  };
};

class manitou_application : public QApplication
{
  Q_OBJECT
public:
  manitou_application(int& argc, char** argv);
  ~manitou_application();
  void start_new_mail(const mail_header&);
  void set_style(const QString style);
  QString m_dbname;
  void setup_desktop_tray_icon();
  void desktop_notify(const QString,const QString);
  void display_warning(const QString);
  QString chartjs_path();
public slots:
  void cleanup();
private:
  QString default_style_name;
  QSystemTrayIcon* m_tray_icon;
};

/* Application-level exception. Code that opens a database transaction
   should first clean it up before throwing that exception.
   Code that catches these exceptions is generally expected to display
   the corresponding error through the GUI. */
class app_exception
{
public:
  app_exception();
  app_exception(QString errmsg);
  virtual ~app_exception();
  QString m_err_msg;
};

extern manitou_application* gl_pApplication;

#endif // INC_MAIN_H
