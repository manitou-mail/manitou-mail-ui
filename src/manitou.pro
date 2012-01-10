TEMPLATE=app
SUBDIRS=xface
TARGET=manitou
QT += assistant qt3support
CONFIG += qt  thread exceptions warn_off rtti assistant
OBJECTS_DIR=o
MOC_DIR=moc

DEFINES += WITH_PGSQL
LIBS += -Lxface -lxface
unix {
  INCLUDEPATH += $$(PG_INCLUDE_DIR) ..
  LIBS += -L$$(PG_LIB_DIR) -lpq
  CONFIG += console
}
win32 {
  TMP=$$(PGSQL_SRC)
  isEmpty(TMP) {
    error(The PGSQL_SRC environment variable must be defined to point to postgresql source code)
  }
  INCLUDEPATH += $$(PGSQL_SRC)/interfaces/libpq $$(PGSQL_SRC)/include ..
  LIBS += -L$$(PGSQL_SRC)/interfaces/libpq -lpq -lwsock32 -lwinmm
  DEFINES += _WINDOWS WIN_FIXME WITH_PGSQL
}

SOURCES=about.cpp \
 addressbook.cpp \
 addresses.cpp \
 attachment.cpp \
 attachment_listview.cpp \
 bitvector.cpp \
 body_edit.cpp \
 body_view.cpp \
 browser.cpp \
 date.cpp \
 db.cpp \
 edit_rules.cpp \
 errors.cpp \
 filter_rules.cpp \
 headers_groupview.cpp \
 helper.cpp \
 identities.cpp \
 login.cpp \
 mailheader.cpp \
 mail_listview.cpp \
 mail_displayer.cpp \
 main.cpp \
 message.cpp \
 message_port.cpp \
 mime_msg_viewer.cpp \
 msg_list_window.cpp \
 msg_list_window_pages.cpp \
 msg_properties.cpp \
 msgs_page_list.cpp \
 newmailwidget.cpp \
 notewidget.cpp \
 preferences.cpp \
 prog_chooser.cpp \
 query_listview.cpp \
 searchbox.cpp \
 selectmail.cpp \
 sha1.cpp \
 sql_editor.cpp \
 sqlquery.cpp \
 sqlstream.cpp \
 tags.cpp \
 tagsbox.cpp \
 tagsdialog.cpp \
# trayicon.cpp \
# trayicon_win.cpp \
 users.cpp \
 user_queries.cpp \
 words.cpp

HEADERS= about.h \
 addressbook.h \
 addresses.h \
 attachment.h \
 attachment_listview.h \
 browser.h \
 bitvector.h \
 body_edit.h \
 body_view.h \
 database.h \
 date.h \
 db.h \
 dbtypes.h \
 dragdrop.h \
 edit_rules.h \
 errors.h \
 filter_rules.h \
 headers_groupview.h \
 helper.h \
 icons.h \
 identities.h \
 login.h \
 mailheader.h \
 mail_displayer.h \
 mail_listview.h \
 main.h \
 message_port.h \
 mime_msg_viewer.h \
 msg_list_window.h \
 msg_properties.h \
 msgs_page_list.h \
 newmailwidget.h \
 notewidget.h \
 preferences.h \
 prog_chooser.h \
 query_listview.h \
 searchbox.h \
 selectmail.h \
 sha1.h \
 sql_editor.h \
 sqlquery.h \
 sqlstream.h \
 tags.h \
 tagsbox.h \
 tagsdialog.h \
# trayicon.h \
 users.h \
 user_queries.h \
 words.h \
 xpm_attached.h \
 xpm_forwarded.h \
 xpm_new.h \
 xpm_processed.h \
 xpm_read.h \
 xpm_replied.h \
 xpm_trash.h


TRANSLATIONS = manitou_fr.ts manitou_es.ts
