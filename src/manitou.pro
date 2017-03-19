TEMPLATE = app
TARGET = manitou
DEPENDPATH += . xface
CONFIG += release c++11
INCLUDEPATH += . .. xface /usr/local/pgsql/include
QT += network webkit webkitwidgets gui widgets printsupport
DEFINES += WITH_PGSQL
LIBS += -L/usr/local/pgsql/lib -lpq
ICON = ../icons/manitou.icns
RESOURCES += manitou.qrc
#TRANSLATIONS = ../translations/manitou_fr.ts
TRANS_QM_FILES = ../translations/manitou_fr.qm ../translations/manitou_de.qm ../translations/manitou_es.qm

mac {
    HelpFiles.files = ../help 
    TranslationFiles.files = $${TRANS_QM_FILES}
    HelpFiles.path = Contents/MacOS
    TranslationFiles.path = Contents/MacOS/translations
    QMAKE_BUNDLE_DATA += HelpFiles TranslationFiles
# try commenting out the following line for Qt5.5 with macOS Sierra/XCode
# if you have compilation errors (also use macx-clang for QMAKESPEC)
#    QMAKE_MAC_SDK = macosx10.12
}

# Input
HEADERS += about.h \
           addressbook.h \
           addresses.h \
           app_config.h \
           attachment.h \
           attachment_listview.h \
           body_edit.h \
           body_view.h \
           browser.h \
           composer_widgets.h \
           database.h \
           date.h \
           db.h \
           db_error_dialog.h \
           db_listener.h \
           dbtypes.h \
           dragdrop.h \
           edit_address_widget.h \
           edit_rules.h \
           export_window.h \
           errors.h \
           fetch_thread.h \
           filter_log.h \
           filter_rules.h \
           headers_groupview.h \
           helper.h \
           html_editor.h \
           icons.h \
           identities.h \
           import_window.h \
           line_edit_autocomplete.h \
           login.h \
           mail_displayer.h \
           mail_listview.h \
           mail_template.h \
           mailheader.h \
           mailing.h \
           mailing_viewer.h \
           mailing_window.h \
           mailing_wizard.h \
           main.h \
           mbox_file.h \
           mbox_import_wizard.h \
           message.h \
           message_port.h \
           message_view.h \
           mime_msg_viewer.h \
           msg_list_window.h \
           msg_properties.h \
           msg_status_cache.h \
           msgs_page_list.h \
           mygetopt.h \
           newmailwidget.h \
           notepad.h \
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
           log_window.h \
           tagsdialog.h \
           text_merger.h \
           ui_controls.h \
           ui_feedback.h \
           users_dialog.h \
           user_queries.h \
           users.h \
           filter_results_window.h \
           filter_action_editor.h \
           filter_expr_editor.h \
           words.h \
           filter_eval.h \
           xface/compface.h \
           xface/data.h \
           xface/vars.h \
           xface/xface.h \
           statistics.h
SOURCES += about.cpp \
           filter_eval.cpp \
           filter_results_window.cpp \
           filter_action_editor.cpp \
           filter_expr_editor.cpp \
           addressbook.cpp \
           addresses.cpp \
           app_config.cpp \
           attachment.cpp \
           attachment_listview.cpp \
           body_edit.cpp \
           body_view.cpp \
           browser.cpp \
           composer_widgets.cpp \
           date.cpp \
           db.cpp \
           db_listener.cpp \
           db_error_dialog.cpp \
           edit_address_widget.cpp \
           edit_rules.cpp \
           export_window.cpp \
           errors.cpp \
           fetch_thread.cpp \
           filter_log.cpp \
           filter_rules.cpp \
           getopt.cpp \
           getopt1.cpp \
           headers_groupview.cpp \
           helper.cpp \
           html_editor.cpp \
           identities.cpp \
           import_window.cpp \ 
           line_edit_autocomplete.cpp \
           login.cpp \
           log_window.cpp \
           mail_displayer.cpp \
           mail_listview.cpp \
           mail_template.cpp \
           mailheader.cpp \
           mailing.cpp \
           mailing_viewer.cpp \
           mailing_window.cpp \
           mailing_wizard.cpp \
           main.cpp \
           mbox_file.cpp \
           mbox_import_wizard.cpp \
           message.cpp \
           message_port.cpp \
           message_view.cpp \
           mime_msg_viewer.cpp \
           msg_list_window.cpp \
           msg_list_window_pages.cpp \
           msg_properties.cpp \
           msg_status_cache.cpp \
           msgs_page_list.cpp \
           newmailwidget.cpp \
           notepad.cpp \
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
           statistics.cpp \
           tags.cpp \
           tagsbox.cpp \
           tagsdialog.cpp \
           text_merger.cpp \
           ui_feedback.cpp \
           users_dialog.cpp \
           user_queries.cpp \
           users.cpp \
           words.cpp \
           xface/arith.cpp \
           xface/compress.cpp \
           xface/file.cpp \
           xface/gen.cpp \
           xface/xface.cpp
