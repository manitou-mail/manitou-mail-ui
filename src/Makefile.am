SUBDIRS = xface
XFACE = xface/libxface.a
bin_PROGRAMS=manitou
AM_CPPFLAGS = -DMANITOU_DATADIR=\"$(pkgdatadir)\"
manitou_CXXFLAGS = $(QT_CXXFLAGS) $(AM_CXXFLAGS)
manitou_CPPFLAGS = $(QT_CPPFLAGS) $(AM_CPPFLAGS)
manitou_LDFLAGS  = $(QT_LDFLAGS) $(LDFLAGS)
manitou_LDADD = $(MOC_OBJS) $(QT_LIBS) $(XFACE) @PG_LIBS@ @EXTRAOBJ@

manitou_SOURCES = main.cpp main.h msg_list_window.cpp db.cpp \
 newmailwidget.cpp tagsbox.cpp searchbox.cpp selectmail.cpp \
 date.cpp sqlquery.cpp addresses.cpp notewidget.cpp sqlstream.cpp \
 tagsdialog.cpp mailheader.cpp message.cpp message.h mailheader.h \
 attachment.h attachment.cpp msg_list_window.h dbtypes.h tagsdialog.h \
 addresses.h searchbox.h database.h newmailwidget.h selectmail.h \
 date.h notewidget.h sqlstream.h db.h tagsbox.h sqlquery.h \
 addressbook.h addressbook.cpp \
 tags.h tags.cpp msg_properties.h msg_properties.cpp users.h users.cpp \
 mail_listview.cpp mail_listview.h dragdrop.h \
 preferences.cpp preferences.h icons.h msg_list_window_pages.cpp \
 about.h about.cpp login.h login.cpp filter_rules.h filter_rules.cpp \
 edit_rules.cpp edit_rules.h helper.cpp helper.h body_view.h body_view.cpp \
 query_listview.h query_listview.cpp identities.h identities.cpp \
 words.cpp words.h user_queries.cpp user_queries.h \
 headers_groupview.cpp headers_groupview.h \
 body_edit.h body_edit.cpp browser.h browser.cpp prog_chooser.cpp \
 prog_chooser.h sql_editor.h sql_editor.cpp errors.h errors.cpp \
 mime_msg_viewer.h mime_msg_viewer.cpp mail_displayer.h \
 mail_displayer.cpp bitvector.h bitvector.cpp message_port.cpp \
 message_port.h msgs_page_list.h msgs_page_list.cpp sha1.cpp sha1.h \
 attachment_listview.h attachment_listview.cpp filter_log.cpp filter_log.h \
 message_view.h message_view.cpp \
 app_config.cpp app_config.h ui_controls.h edit_address_widget.cpp edit_address_widget.h \
 html_editor.h html_editor.cpp notepad.h notepad.cpp db_listener.h db_listener.cpp \
 msg_status_cache.h msg_status_cache.cpp mailing_wizard.cpp mailing_wizard.h \
 mailing.cpp mailing.h mail_template.cpp mail_template.h composer_widgets.cpp composer_widgets.h \
 text_merger.cpp text_merger.h mailing_window.cpp mailing_window.h \
 mailing_viewer.h mailing_viewer.cpp filter_action_editor.h filter_action_editor.cpp \
 filter_expr_editor.cpp filter_expr_editor.h filter_eval.cpp filter_eval.h \
 filter_results_window.cpp filter_results_window.h log_window.h log_window.cpp \
 mbox_file.cpp mbox_file.h mbox_import_wizard.h mbox_import_wizard.cpp \
 import_window.cpp import_window.h ui_feedback.h ui_feedback.cpp \
 fetch_thread.cpp fetch_thread.h export_window.cpp export_window.h \
 users_dialog.cpp users_dialog.h pivot_table.cpp pivot_table.h \
 line_edit_autocomplete.h line_edit_autocomplete.cpp \
 statistics.cpp statistics.h

EXTRA_manitou_SOURCES = getopt.cpp mygetopt.h getopt1.cpp

MOC_OBJS = main.moc.o msg_list_window.moc.o newmailwidget.moc.o tagsbox.moc.o \
	searchbox.moc.o helper.moc.o query_listview.moc.o \
	notewidget.moc.o selectmail.moc.o tagsdialog.moc.o \
	addressbook.moc.o msg_properties.moc.o mail_listview.moc.o \
	preferences.moc.o about.moc.o login.moc.o edit_rules.moc.o \
	user_queries.moc.o headers_groupview.moc.o body_edit.moc.o \
	sql_editor.moc.o browser.moc.o message_port.moc.o \
	body_view.moc.o attachment_listview.moc.o \
	message_view.moc.o ui_controls.moc.o attachment.moc.o \
	html_editor.moc.o notepad.moc.o \
	db_listener.moc.o msg_status_cache.moc.o mime_msg_viewer.moc.o \
	mailing_wizard.moc.o mail_template.moc.o composer_widgets.moc.o \
	mailing_window.moc.o mailing_viewer.moc.o filter_action_editor.moc.o \
	filter_expr_editor.moc.o filter_results_window.moc.o mbox_file.moc.o \
	database.moc.o mbox_import_wizard.moc.o import_window.moc.o \
	log_window.moc.o ui_feedback.moc.o fetch_thread.moc.o export_window.moc.o \
	users_dialog.moc.o pivot_table.moc.o line_edit_autocomplete.moc.o \
	statistics.moc.o

manitou_DEPENDENCIES = @EXTRAOBJ@ $(MOC_OBJS) $(XFACE)

%.moc.o: %.moc.cpp
	$(CXXCOMPILE) $(QT_CPPFLAGS) $(QT_CXXFLAGS) -c $< -o $@

%.moc.cpp: %.h
	$(MOC) $< -o $@

