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

#ifndef INC_IMAGE_VIEWER_H
#define INC_IMAGE_VIEWER_H

#include <QImage>
#include <QWidget>
#include <QLabel>

class QScrollArea;

class attachment;

class image_label : public QLabel
{
  Q_OBJECT
public:
  image_label(QWidget* parent = NULL);
  ~image_label();

public slots:
  void setPixmap(const QPixmap& pm);

protected:
  void resizeEvent(QResizeEvent* event);

private:
  void update_margins();
  int m_pixmap_width;
  int m_pixmap_height;
};


class image_viewer : public QWidget
{
  Q_OBJECT
public:
  image_viewer(QWidget* parent=NULL);
  ~image_viewer();
  void show_attachment(attachment*);
private:
  QImage m_image;
  image_label* m_image_label;
  QScrollArea* m_scroll_area;
  QLabel* m_size_text;
  double m_scale_factor;
};

#endif
