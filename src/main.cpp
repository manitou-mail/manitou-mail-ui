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

#include "main.h"

#include <QFileInfo>
#include <QLibraryInfo>
#include <QStyle>
#include <QMessageBox>
#include <QTextCodec>
#include <QApplication>
#include <QTranslator>
#include <QHostInfo>
#include <QSettings>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QUuid>

#include "selectmail.h"
#include "login.h"
#include "users.h"
#include "helper.h"
#include "msg_list_window.h"
#include "newmailwidget.h"
#include "message_port.h"
#include "msg_status_cache.h"
#include "app_config.h"
#include "log_window.h"
#include "icons.h"

#include <stdarg.h>
#include <time.h>

#if HAVE_DECL_GETOPT_LONG
// Use system getopt_long
 #include <getopt.h>
 typedef struct option optstruct;
 #define OPTIND optind
 #define GETOPT_LONG getopt_long
 #define OPTARG optarg
#else
// Use our getopt_long living in the getoptns namespace
 #include "mygetopt.h"
 typedef struct getoptns::option optstruct;
 #define OPTIND getoptns::optind
 #define GETOPT_LONG getoptns::getopt_long
 #define OPTARG getoptns::optarg
#endif

#ifndef Q_OS_WIN
#include <sys/utsname.h>
#endif


static app_config global_conf;

/* Debugging level, set with --debug-level command like option.
   DBG_PRINTF() macro calls cause a display if their
   level (first argument) is lower or equal than global_debug_level.
   Normally the values should be:
   1: signal something that reveals an inconsistency inside our code

   2: signal something that shouldn't normally happen but may happen under certain circumstances (for example severe violations of RFC in the data).

   3 and higher: used for actual printf-like debugging. Use higher
   values for the more mundane messages or those that cause a lot of output
*/
int global_debug_level;
int global_debug_window;

manitou_application * gl_pApplication;

QString gl_xpm_path; // used by FT_MAKE_ICON (icons.h)
QString gl_help_path;

void
debug_printf(int level, const char* file, int line, const char *fmt, ...)
{
  if (level<=global_debug_level) {
    if (global_debug_window) {
      QString str;
      va_list ap;
      va_start(ap,fmt);
      str.vsprintf(fmt, ap);
      va_end(ap);
      str.prepend(QString("%1, line %2:").arg(file).arg(line));
      log_window::log(str);
    }
    else {
      fprintf(stderr, "%s, line %d: ", file, line);
      va_list ap;
      va_start(ap,fmt);
      vfprintf(stderr, fmt, ap);
      va_end(ap);
      fprintf(stderr, "\n");
      fflush(stderr);
    }
  }

}

void
err_printf(const char* file, int line, const char *fmt, ...)
{
  fprintf(stderr, "%s, line %d: ERR: ", file, line);
  va_list ap;
  va_start(ap,fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  fflush(stderr);
}

app_config&
get_config()
{
  return global_conf;
}

void
usage(const char* progname)
{
#ifdef Q_OS_WIN
  QMessageBox::critical(NULL, "Arguments error", "The possible arguments are --dbcnx followed by a connection string and --config followed by a configuration name.\nExample, --dbcnx \"dbname=mail user=mailadmin host=pgserver\" --config myconf");
#else
  fprintf(stderr, "Usage: %s [--debug-output=level] [--config=confname] \"connect string\".\nThe connect string looks like, for example, \"dbname=mail user=mailadmin host=pgserver\".\n",
	  progname);
#endif
  exit(1);
}

manitou_application::manitou_application(int& argc, char** argv)
  : QApplication(argc,argv)
{
  default_style_name = style()->objectName();
  m_tray_icon = NULL;
}

manitou_application::~manitou_application()
{
}

void
manitou_application::set_style(const QString style)
{
  // if style is empty we're going back to the default style
  if (style.isEmpty())
    this->setStyle(default_style_name);
  else
    this->setStyle(style);
}

void
manitou_application::start_new_mail(const mail_header& header)
{
  mail_msg msg;
  msg.set_header(header);

  new_mail_widget* w = new new_mail_widget(&msg, 0);
  if (w->errmsg().isEmpty()) {
    w->show();
    w->insert_signature();
    w->start_edit();
  }
  else {
    delete w;
  }
}

void
manitou_application::setup_desktop_tray_icon()
{
  m_tray_icon=NULL;

  if (get_config().get_string("display/notifications/new_mail")=="system") {
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      QIcon icon(UI_ICON(ICON16_TRAYICON));
      m_tray_icon = new QSystemTrayIcon(icon);
      m_tray_icon->show();
    }
  }
}

void
manitou_application::desktop_notify(const QString title, const QString message)
{
  if (m_tray_icon) {
    m_tray_icon->showMessage(title, message);
  }
}

void
manitou_application::cleanup()
{
  DBG_PRINTF(2, "cleanup");
  if (m_tray_icon)
    delete m_tray_icon;
  helper::close();
}

int
main(int argc, char **argv)
{
  manitou_application app(argc,argv);
  gl_pApplication=&app;

#if defined(Q_OS_UNIX)  && !defined(Q_OS_MAC) 
  // The location of the data files depends on the prefix choosen at configure
  // time except under windows.
  QString s = QString(MANITOU_DATADIR);
  QString manitou_tr_path = s + "/translations";
  QString qt_tr_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
  gl_xpm_path = s + "/icons";
  gl_help_path = s + "/help";
#elif defined(Q_OS_WIN)
  // under windows, the data directories are expected to be at the same level
  // than the executable file
  QString s = QApplication::applicationDirPath();
  QString qt_tr_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
  QString manitou_tr_path = s + "/translations";
  gl_xpm_path = s + "/icons";
  gl_help_path = s + "/help";
#elif defined(Q_OS_MAC)
  // we use embedded resources on Mac
  QString qt_tr_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
  gl_xpm_path = ":/images";
  QString s = QApplication::applicationDirPath();
  gl_help_path = s + "/help";
  QString manitou_tr_path = s + "/translations";
#endif

  const char* cnx_string=NULL;
  QString conf_name;
  bool explicit_config=false;

  while(1) {
    int option_index = 0;
    static optstruct long_options[] = {
      {"debug-output", 1, 0, 'l'},
      {"debug-window", 1, 0, 'w'},
      {"config", 1, 0, 'c'},
      {"dbcnx", 1, 0, 'd'},
      {"help", 1, 0, 'h'},
      {0, 0, 0, 0}
    };

    int c = GETOPT_LONG(argc, argv, "d:c:h:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 0:
      printf ("option %s", long_options[option_index].name);
      if (OPTARG)
	printf (" with arg %s", OPTARG);
      printf ("\n");
      break;
    case '?':
      usage(argv[0]);
      break;
    case 'h':
      usage(argv[0]);
      break;
    case 'c':
      conf_name = OPTARG;
      explicit_config=true;
      break;
    case 'l':
      global_debug_level = OPTARG?atoi(OPTARG):10;
      break;
    case 'w':
      global_debug_level = OPTARG?atoi(OPTARG):10;
      global_debug_window = 1;
      break;
    case 'd':
      cnx_string = OPTARG;
      break;
    }
  }
  if (OPTIND < argc && !cnx_string) {
    /* compatibility with earlier versions: if there are non-named
       arguments on the command line, they will be interpreted as a
       connection string */
    cnx_string = argv[OPTIND];
  }

  QTranslator translator;
  QTranslator translator_qt;
  QLocale locale = QLocale::system();
  // search for a translation file, except for the C locale
  if (locale.name() != QLocale::c().name()) {
    if (translator_qt.load(QString("qt_")+locale.name(), qt_tr_path))
      app.installTranslator(&translator_qt);
    if (translator.load(QString("manitou_")+locale.name(), manitou_tr_path)) {
      app.installTranslator(&translator);
    }
    else {
      QString tq = QString("manitou_")+locale.name();
      DBG_PRINTF(3, "Failed to load translation file %s", tq.toLocal8Bit().constData());
    }
  }

  if (conf_name.isEmpty()) {
    // then configuration is hostname-OSname
    QString hostname=QHostInfo::localHostName().toLower();
    if (hostname.isEmpty())
      hostname="unknown";
    QString uname;
#ifndef Q_OS_WIN
    struct utsname u;
    if (::uname(&u)==0 && u.sysname[0]) {
      uname = QString(u.sysname).toLower();
    }
#else
    uname="windows";
#endif
    conf_name = hostname + "-" + uname;
  }

  /* Call createUuid() before creating any window as a workaround
     against QTBUG-11080 or QTBUG-11213 in Qt-4.6.2 on Linux.
     Otherwise, createUuid() may return a UUID that is always the same
     when creating a new Message-Id later on. The fix for this Qt bug
     has not been backported in Ubuntu-10.04 LTS at this time
     (2012-10-08). */
  (void)QUuid::createUuid();

  if (cnx_string==NULL) {
    login_dialog dlg;
    if (dlg.exec() == QDialog::Rejected)
      return 0;
  }
  else {
    QString errstr;
    if (!ConnectDb(cnx_string, &errstr)) {
      QMessageBox::critical(NULL, QObject::tr("Fatal database error"), QObject::tr("Error while connecting to the database:\n")+errstr);
      return 1;
    }
  }

  if (!explicit_config) {
    printf("Configuration used: %s\n", conf_name.toLocal8Bit().constData());
  }

  global_conf.set_name(conf_name);
  global_conf.init();
  global_conf.apply();

  msgs_filter filter;
  filter.m_sql_stmt="0";

#ifdef Q_OS_UNIX
  gl_pApplication->setAttribute(Qt::AA_DontShowIconsInMenus, false);
#endif

  /* Instantiate the user in the database if necessary. Ideally we
     should ask for a fullname if not already known, but there's no
     GUI support for that yet */
  user::create_if_missing(QString::null);

  users_repository::fetch();
  message_port::init();
  msg_status_cache::init_db();
  app.setup_desktop_tray_icon();

  msg_list_window* w = new msg_list_window(&filter,0);
  w->show();

  app.connect(&app, SIGNAL(lastWindowClosed()), SLOT(quit()));
  app.connect(&app, SIGNAL(aboutToQuit()), SLOT(cleanup()));
  app.exec();
  DisconnectDb();
  return 0;
}
