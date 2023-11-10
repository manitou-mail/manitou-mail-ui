/* Copyright (C) 2004-2023 Daniel Verite

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

#ifndef INC_EXPORT_WINDOW_H
#define INC_EXPORT_WINDOW_H

#include <QWidget>
#include "tags.h"
#include "main.h"

class QPushButton;
class QProgressBar;
class QTimer;
class QDateTimeEdit;
class QCheckBox;

class export_window: public QWidget
{
  Q_OBJECT
public:
  export_window();
  ~export_window();
public slots:
  void stop();
  void start();
private:
  void save_raw_file(mail_id_t id, db_cnx& db, QString dirname);
  QProgressBar* m_progress_bar;
  QTimer* m_timer;
  QDateTimeEdit* m_wmin_date;
  QDateTimeEdit* m_wmax_date;
  QCheckBox* m_chk_datemin;
  QCheckBox* m_chk_datemax;

  QPushButton* m_btn_start;
  QPushButton* m_btn_stop;
  tag_selector* m_qtag_sel;
  bool m_running;
};

#endif
