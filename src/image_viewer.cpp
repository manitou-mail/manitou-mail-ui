/* Copyright (C) 2017 Daniel Verite

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
#include "image_viewer.h"
#include "attachment.h"

#include <QImageReader>
#include <QScrollArea>
#include <QBoxLayout>
#include <QMessageBox>

image_viewer::image_viewer(QWidget* parent) : QWidget(parent)
{
 // don't keep the image in memory longer than necessary
  setAttribute(Qt::WA_DeleteOnClose);

  QVBoxLayout* layout = new QVBoxLayout(this);
  m_image_label = new image_label;
  m_scroll_area = new QScrollArea;
  m_scale_factor = 1.0;
  m_size_text = new QLabel;
  layout->addWidget(m_size_text);

  m_image_label->setBackgroundRole(QPalette::Base);
  m_image_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  m_image_label->setScaledContents(true);

  layout->addWidget(m_scroll_area);

  m_scroll_area->setBackgroundRole(QPalette::Dark);
  m_scroll_area->setWidget(m_image_label);
  m_scroll_area->setVisible(false);
}

image_viewer::~image_viewer()
{
}

void
image_viewer::show_attachment(attachment* a)
{
  if (!a->filename().isEmpty())
    setWindowTitle(a->filename());
  else
    setWindowTitle(tr("Image view"));

  attachment_iodevice io(a, this);
  QImageReader imgr(&io);
  QString img_format = QString::fromLocal8Bit(imgr.format());
  if (img_format.isEmpty())
    img_format = tr("unknown");
#if QT_VERSION>=0x050500
  imgr.setAutoTransform(true);
#endif
  m_image = imgr.read();
  io.close();			// ends transaction

  if (m_image.isNull()) {
    QMessageBox::critical(this, APP_NAME, tr("Cannot instantiate image from attachment"));
    close();
  }
  else {
    m_size_text->setText(tr("Format: %3 - Size: %1x%2 px").
			 arg(m_image.width()).
			 arg(m_image.height()).
			 arg(img_format));
    m_image_label->setPixmap(QPixmap::fromImage(m_image));
    m_scale_factor = 1.0;

    m_scroll_area->setVisible(true);
    m_scroll_area->setWidgetResizable(true);

    m_image_label->resize(m_scale_factor * m_image_label->pixmap()->size());
    show();
  }
}

image_label::image_label(QWidget* parent) : QLabel(parent)
{
  m_pixmap_width = m_pixmap_height = 0;
}

image_label::~image_label()
{
}

void
image_label::setPixmap(const QPixmap& p)
{
  m_pixmap_width = p.width();
  m_pixmap_height = p.height();
  update_margins();
  QLabel::setPixmap(p);
}

void
image_label::resizeEvent(QResizeEvent* event)
{
    update_margins();
    QLabel::resizeEvent(event);
}

/* Maintain the pixmap width/height ratio by adjusting the contents
   margins. */
void
image_label::update_margins()
{
  if (m_pixmap_width <= 0 || m_pixmap_height <= 0)
    return;

  int w = width();
  int h = height();

  if (w <= 0 || h <= 0)
    return;

  if (w * m_pixmap_height > h * m_pixmap_width) {
    int m = (w - (m_pixmap_width * h / m_pixmap_height)) / 2;
    setContentsMargins(m, 0, m, 0);
  }
  else {
    int m = (h - (m_pixmap_height * w / m_pixmap_width)) / 2;
    setContentsMargins(0, m, 0, m);
  }
}
