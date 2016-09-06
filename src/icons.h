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

#ifndef INC_ICONS_H
#define INC_ICONS_H


extern QString gl_xpm_path;

#define FT_ICON16_ARROW_LEFT "stock_left-16.png"
#define FT_ICON16_ARROW_RIGHT "stock_right-16.png"
#define FT_ICON16_ACTION_TRASH "stock_delete-16.png"
#define FT_ICON16_EDIT "stock_edit-16.png"
#define FT_ICON16_PRINT "stock_print-16.png"
#define FT_ICON16_FIND "stock_search-16.png"
#define FT_ICON16_REPLY_ALL "reply_to_all.xpm"
#define FT_ICON16_REPLY "reply.xpm"
#define FT_ICON16_FORWARD "forward.xpm"
#define FT_ICON16_NEW_QUERY "stock_exec-16.png"
#define FT_ICON16_REPLACE_QUERY "exec2-16.png"
#define FT_ICON16_REFRESH "stock_refresh-16.png"
#define FT_ICON16_SAVE_SQL "stock_data-new-sql-query-16.png"
#define FT_ICON16_GOTO_BOTTOM "stock_bottom-16.png"
//#define FT_ICON16_NEWMAIL "red-newmail.png"
#define FT_ICON16_NEW_MAIL "stock_form-letter-dialog-16.png"		// FIXME
#define FT_ICON16_EDIT_CUT "cut.xpm"
#define FT_ICON16_EDIT_COPY "stock_copy-16.png"
#define FT_ICON16_EDIT_PASTE "stock_paste-16.png"
#define FT_ICON16_PEOPLE "stock_people.png"
#define FT_ICON16_INBOX "inbox.png"

#define FT_ICON16_STATUS_NEW "status_new.png"
#define FT_ICON16_STATUS_READ "status_read.png"
#define FT_ICON16_STATUS_REPLIED "status_replied.png"
#define FT_ICON16_STATUS_FORWARDED "status_forwarded.png"
#define FT_ICON16_STATUS_TRASHED "status_trashed.png"
#define FT_ICON16_STATUS_PROCESSED "status_processed.png"
#define FT_ICON16_STATUS_ATTACHED "status_attached.png"

#define FT_ICON16_COMPOSED "16_edit.png"
#define FT_ICON16_PREFS "16_configure_mail.png"
#define FT_ICON16_PROPERTIES "properties-16.png"
#define FT_ICON16_FILTERS "apply-filters-16.png"
#define FT_ICON16_NEW_WINDOW "open-in-new-window-16.png"
#define FT_ICON16_CLOSE_WINDOW "close.png"
//#define FT_ICON16_UNTRASH "undelete_message.png"
#define FT_ICON16_UNTRASH "stock_undelete-16.png"
#define FT_ICON16_DELETE_MSG "remove.png"
#define FT_ICON16_EDIT_NOTE "evolution-memos.png"
#define FT_ICON16_EDIT_NOTE_GREY "evolution-memos-grey.png"
#define FT_ICON16_GET_NEW_MAIL "stock_form-letter-dialog-16.png"
#define FT_ICON16_TAGS "tag.png"
#define FT_ICON16_GET_UNPROC_MAIL "stock_form-open-in-design-mode-16.png"
#define FT_ICON16_UNDO "stock_undo-16.png"
#define FT_ICON16_REDO "stock_redo-16.png"
#define FT_ICON16_ATTACH "stock_attach-16.png"
#define FT_ICON16_HELP "stock_help-16.png"
#define FT_ICON16_ABOUT "gtk-about.png"
#define FT_ICON16_STOP "stock_stop-16.png"
#define FT_ICON16_SEND "mail-need-reply.xpm"
#define FT_ICON16_FONT "stock_font-16.png"
#define FT_ICON16_ADDRESSBOOK "stock_addressbook.png"
#define FT_ICON16_OUTBOX "stock_outbox.png"
#define FT_ICON16_QUIT "exit.png"
#define FT_ICON16_OPENFILE "stock_open-16.png"
#define FT_ICON16_ZOOM_PAGE "stock_zoom-page-16.png"
#define FT_ICON16_TO_SEND "stock_slide-duplicate-16.png"

#define ICON16_HEADER "header.png"
#define ICON16_CLEAR_TEXT "clear_edit.png"
#define ICON16_CANCEL "gtk-cancel.png"
#define ICON16_NOTEPAD "accessories-text-editor.png"
#define ICON16_DISCONNECT "stock_disconnect.png"
#define ICON16_MAIL_MERGE "stock_mail-merge.png"
#define ICON16_LOAD_TEMPLATE "stock_new-template.png"
#define ICON16_SAVE_TEMPLATE "stock_save-template.png"

#define ICON16_GO_BOTTOM "go-bottom.png"
#define ICON16_GO_TOP "go-top.png"
#define ICON16_GO_UP "go-up.png"
#define ICON16_GO_DOWN "go-down.png"

#define ICON16_FOLDER "folder.png"
#define ICON16_IMPORTANT "emblem-important.png"
#define ICON16_DIALOG_OK "dialog-ok.png"
#define ICON16_IMPORT_MBOX "icon-import-mbox.png"
#define ICON16_EXPORT_MESSAGES "filesaveas.png"
#define ICON16_USERS "config-users.png"
#define ICON16_STATISTICS "statistics.png"

#define ICON16_TRAYICON "mail-trayicon.png"

#define FT_MAKE_ICON(name) QPixmap(gl_xpm_path+"/"+name)
#define UI_ICON(name) QIcon(gl_xpm_path+"/"+name)
#define STATUS_ICON(name) QIcon(gl_xpm_path+"/"+name)
#define HTML_ICON(name) QIcon(gl_xpm_path+"/html/"+name)

#endif
