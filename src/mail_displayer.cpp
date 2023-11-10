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

#include "mail_displayer.h"
#include "main.h"
#include "app_config.h"
#include "db.h"
#include "body_view.h"
#include "message_view.h"
#include "tags.h"
#include "xface/xface.h"
#include <QRegExp>
#include <QTextCodec>
#include "filter_log.h"
#include <list>

mail_displayer::mail_displayer(message_view* w)
{
  msg_widget = w;
}

mail_displayer::~mail_displayer()
{
}

void
mail_displayer::find_urls(const QString& s, std::list<std::pair<int,int> >* matches)
{
  int pos=0;
  int len;
  QRegExp url("(https?://|www\\.)[\\w\\-]+(\\.[\\w\\-]+)*([^>\"\\s\\[\\]\\)])*");
  while ((pos=url.indexIn(s, pos))>=0) {
    len=url.matchedLength();
    QChar lastchar=s.at(pos+len-1);
    if (lastchar==',' || lastchar=='.'|| lastchar==';')
      len--;
    matches->push_back(std::pair<int,int>(pos, len));
    pos+=len;
  }
}

/*
  Takes a single line as input (\n included)
  Translate tabs to spaces; translate the characters that are not
  allowed in a QTextEdit; highlight searched text;
  optionally put URLs inside href tags
*/
QString
mail_displayer::expand_body_line(const QString& line,
				 const display_prefs& prefs)
{
  int len = line.length();
  const int tabsize=8;
  QString exp_s;
  static const QChar tab = QChar('\t');
  static const QChar lbracket = QChar('<');
  static const QChar rbracket = QChar('>');
  static const QChar equal_sign = QChar('=');
  static const QChar amp = QChar('&');
  static const QChar ctrl_92 = QChar((ushort)0x92);
  int col = 0;			// current column
  int incr = 0;			// increment in source to next character
  int pos = 0; 			// position in the source
  QChar last_src_char;
  // positions of the occurrences of a searched text within the original string
  std::list<uint> hilight_list;

  std::pair<int,int> cur_url;
  std::list<std::pair<int,int> > urls;
  if (prefs.m_clickable_urls) {
    find_urls(line, &urls);
  }
  if (!urls.empty()) {
    cur_url = urls.front();
    urls.pop_front();
  }
  else
    cur_url = std::pair<int,int>(-1,-1);

  while (pos < len) {
    incr = 1;			// default increment in the source

    // TODO: accelerate the most common case by processing first [a-zA-Z0-9] chars

    if (pos == cur_url.first) {
      /* TODO: adjust cur_url.second for the delta of sequences that expand
	 or shrink between source and destination */
      exp_s.append(QString("<a href=\"%1\">").arg(line.mid(pos, cur_url.second)));
    }
    // contents
    const QChar c = line.at(pos);

    if (c == ' ') {
      // repeated spaces are ignored as in html, so we use &nbsp; instead
      // (but only in the case of repeated spaces to limit the overhead)
      if (last_src_char==QChar(' ') || last_src_char==tab) {
	exp_s.append("&nbsp;");
      }
      else {
	exp_s.append(c);
      }
      col++;
    }
    else if (c=='\n') {
      exp_s.append("<br>");
      col=0;
    }
    else if (c == tab) {
      int j;
      for (j=0; j<(int)(tabsize-(col%tabsize)); j++)
	exp_s.append("&nbsp;");
      col += j;
    }
    else if (c == lbracket) {
      exp_s.append("&lt;");
      col++;
    }
    else if (c == rbracket) {
      exp_s.append("&gt;");
      col++;
    }
    else if (c == amp) {
      exp_s.append("&amp;");
      col++;
    }
    else if (c == ctrl_92) {
      // Unicode 0x92 is not displayed by QTextBrowser but is produced
      // by Outlook so we replace it by a basic simple quote
      exp_s.append(QChar(0x27));
      col++;
    }
    else if (c.unicode()>=(ushort)0x80 && c.unicode()<=(ushort)0x9F) {
      /* Unicode characters from the "other, Control category" and
	 whose codes are higher than 0x80 are expressed as HTML codes
	 because QTextBrowser doesn't render them correctly */
      exp_s.append(QString("&#x%1;").arg(c.unicode(), 0, 16));
    }
    else {
      // check for quoted printable =XY sequence
      if (prefs.m_decode_qp && (c == equal_sign)) {
	QString decoded_qp = consume_qp(line, &pos, len, prefs);
	if (!decoded_qp.isEmpty()) {
	  exp_s.append(decoded_qp);
	  col += decoded_qp.length();
	  incr = 0;		// pos has already been adjusted by consume_qp
	}
      }
      else {
	// default case
	exp_s.append(c);
	col++;
      }
    }

    if (pos == (cur_url.first+cur_url.second-1)) {
      exp_s.append("</a>");
      if (!urls.empty()) {
	cur_url = urls.front();	// next URL
	urls.pop_front();
      }
      else
	cur_url = std::pair<int,int>(-1,-1);
    }
    last_src_char = c;
    pos += incr;
  }
  return exp_s;
}

/* Convert quoted-printable section into a QString.
   Returns the decode string and advance *ppos by the number of chars
   consumed in the line, possibly zero.
   As input, line[ppos] is at the equal sign potentially starting a section.
*/
QString
mail_displayer::consume_qp(const QString& line,
			   int* ppos,
			   int len,
			   const display_prefs& prefs)
{
  int pos = *ppos;
  QByteArray bytes;

  if (pos+1 < len && line.at(pos+1) == '\n') {
    /* soft line break: ignore it. RFC2045 says: An equal sign as the
       last character on a encoded line indicates such a
       non-significant ("soft") line break in the encoded text */
    pos += 2; 		// consume LF in addition to equal
  }
  else if (pos+2 < len && line.at(pos+1) == '\r' && line.at(pos+2) == '\n') {
    pos += 3;		// ignore CR,LF soft line break
  }
  else if (pos+2 < len) {
    /* Consume successive encoded bytes until it ends */
    int hex;
    do {
      hex = hex_code_qp(line.at(pos+1), line.at(pos+2)); // returns -1 if invalid hexadecimal
      if (hex >= 0) {
	bytes.append((char)hex);
	pos += 3;		// consume equal sign and hex code
      }
      // continue until we find something that is not QP
    } while (hex >= 0 && (pos+2 < len) && line.at(pos) == '=');
  }
  *ppos = pos;
  // Convert the bytes into a string, according to the source encoding
  return prefs.m_codec->toUnicode(bytes);
}


int
mail_displayer::hex_code_qp(QChar c1, QChar c2)
{
  int n = 0;
  if (c1>='0' && c1<='9')
    n = c1.toLatin1()-'0';
  else if (c1>='A' && c1<='F')
    n = c1.toLatin1()-'A'+10;
  else
    return -1;

  if (c2>='0' && c2<='9')
    n = (n*16) + c2.toLatin1()-'0';
  else if (c2>='A' && c2<='F')
    n = (n*16) + (c2.toLatin1()-'A')+10;
  else
    return -1;
  return n;
}

QString
mail_displayer::sprint_headers(int show_headers_level, mail_msg* msg)
{
  static const char *hListe[] = {
    "date:", "from:", "to:", "cc:", "reply-to:", "bcc:", "subject:"
  };

  if (show_headers_level==0)
    return QString();	// don't show any header

  const QString& h=msg->get_headers();
  uint nPos=0;
  uint nEnd=h.length();
  QString sOutput;
  QString s_xface, s_face;
  uint face_offset=0;
  uint xface_offset=0;

  sOutput.reserve(8192);	// avoid many small reallocs

  if(show_headers_level==2) {	// show all headers
    while (nPos<nEnd) {
      int nPosEh=0;		// end of header name
      // position of end of current line
      int nPosLf=h.indexOf('\n',nPos);
      if (nPosLf==-1)
	nPosLf=nEnd;
      if (h.mid(nPos,1)!=" " && h.mid(nPos,1)!="\t" && h.mid(nPos,5)!="From "
	  && (nPosEh=h.indexOf(':',nPos))>0)
      {
	  QString sContents = h.mid(nPosEh+1, nPosLf-nPosEh-1);
	  sContents.replace(QRegExp("<"), "&lt;");
	  sContents.replace(QRegExp(">"), "&gt;");
	  sContents.replace(QRegExp ("\t"), " ");
	  sOutput.append(QString("<font color=\"blue\">%1</font>%2<br>").
			 arg(h.mid(nPos, nPosEh-nPos+1)).arg(sContents));
      }
      else {
	sOutput.append(h.mid(nPos,nPosLf-nPos+1));
	sOutput.append("<br>");
      }
      nPos=nPosLf+1;
    }
  }
  else if (show_headers_level==1) {
    // Show only headers from hListe
    const int nH=sizeof(hListe)/sizeof(hListe[0]);
    QString sLines[nH];

    while (nPos<nEnd) {
      // position of end of current line
      int nPosLf=h.indexOf('\n',nPos);
      if (nPosLf==-1)
	nPosLf=nEnd;

      if (h.mid(nPos,7) == "X-Face:" && s_face.isEmpty()) {
	xface_offset = nPos+7;
      }
      if (h.mid(nPos,5) == "Face:") {
	s_face = h.mid(nPos+5, nPosLf-(nPos+5));
	face_offset = nPos+5;
	xface_offset = 0;
      }

      // see if we have to display this header
      for (int i=0; i<nH; i++) {
	uint nHlen=strlen(hListe[i]);
	if (h.mid(nPos,nHlen).toLower() == hListe[i]) {
	  QString sContents=h.mid(nPos+nHlen, nPosLf-(nPos+nHlen));
	  sContents.replace("<", "&lt;");
	  sContents.replace(">", "&gt;");
	  sContents.replace("\t", " ");
	  sLines[i] = QString("<font color=\"blue\">") + h.mid(nPos,nHlen) +
	    "</font>" + sContents + "<br>";
	  break;
	}
      }
      nPos=nPosLf+1;
    }
    // order the headers as in hListe and not as they appear in the
    // original message
    for (int j=0; j<nH; j++) {
      if (sLines[j].length()>0)
	sOutput+=sLines[j];
    }
    QString hface;
    QString hface_url;
    if (face_offset) {
      hface = "Face:";
      hface_url = QString("face?id=%1&o=%2").arg(msg->get_id()).arg(face_offset);
    }
    else if (xface_offset) {
      hface = "<nobr>X-Face:</nobr>";
      hface_url = QString("xface?id=%1&o=%2").arg(msg->get_id()).arg(xface_offset);
    }
    if (!hface.isEmpty()) {
      QString st = "<table><tr><td width=50 valign=top><font color=\"blue\">"+hface+"</font><br>";
      st += "<img src=\"manitou://"+hface_url+"\">";
      st += "</td>";
      sOutput = st + "<td>" + sOutput + "</td></tr></table>";
    }
  }
  else if (show_headers_level==3) {
    // show raw headers
    if (msg->header().fetch_raw()) {
      sOutput=msg->header().m_raw;
      sOutput.replace("<", "&lt;");
      sOutput.replace(">", "&gt;");
      sOutput.replace("\n", "<br>");
      sOutput.replace(" ", "&nbsp;");
    }
  }
  else if (show_headers_level==4) {
    // show decoded headers from raw source
    if (msg->header().fetch_raw()) {
      QStringList list=msg->header().m_raw.split(QRegExp("\n\\s*"));
      QStringList::Iterator it = list.begin();
      while (it != list.end()) {
	QString sline=*it;
	mail_header::decode_line(sline);
	sline.replace("<", "&lt;");
	sline.replace(">", "&gt;");
	sline.replace(" ", "&nbsp;");
	sOutput.append(sline);
	sOutput.append("<br>");
	++it;
      }
    }
  }
  else {
    msg->header().format(/*show_headers_level,*/ sOutput, h);
  }
  // remove the last <br>
  if (sOutput.endsWith("<br>"))
    sOutput.truncate(sOutput.length()-4);


  return sOutput;
}

QString
mail_displayer::text_body_to_html(const QString &b, const display_prefs& prefs)
{
  int startline=0;
  int endline;

  QString b2;
  int quoted_lines=0;
  static const char* quoted_text_font="<font color=\"#5ca730\">";
  static const char* quoted_ellipsis_font="<font color=\"#4d8b28\">";
  // number of lines always shown at the beginning of a quoted part
  const int quoted_text_lines_shown=3;
  bool last_folded_line_was_newline=false;
  do {
    endline = b.indexOf ('\n', startline);
    if (endline<0) {
      endline = b.length();
    }
    if (prefs.m_hide_quoted>0) {
      /* Reduce portions of quoted text.
	 Empty lines are assumed to be part of a quote block */
      if (b.at(startline)=='>' || (quoted_lines>0 && b.at(startline)=='\n')) {
	if (++quoted_lines <= quoted_text_lines_shown) {
	  QString q=quoted_text_font;
	  q.append(expand_body_line(b.mid(startline, endline-startline), prefs));
	  q.append("</font>");
	  q.append("<br>");
	  b2.append(q);
	  last_folded_line_was_newline=false; // we're not folding yet
	}
	else {
	  last_folded_line_was_newline = (b.at(startline)=='\n');
	}
      }
      else if (quoted_lines) {
	if (quoted_lines>quoted_text_lines_shown) {
	  if (last_folded_line_was_newline)
	    quoted_lines--;   // don't count an ending empty line
	  if (quoted_lines-quoted_text_lines_shown>0) {
	    b2.append(quoted_ellipsis_font);
	    b2.append("<b>&gt; ");
	    b2.append(QObject::tr("[Collapsed quoted text (%1 lines)]").arg(quoted_lines-quoted_text_lines_shown));
	    b2.append("</b></font><br>");
	  }
	  if (last_folded_line_was_newline)
	    b2.append("<br>");
	}
	quoted_lines=0;
      }
      // end of quoted text processing
    }
    if (!quoted_lines) {
      b2.append(expand_body_line(b.mid(startline, endline-startline), prefs));
      b2.append("<br>");
    }
    startline = endline+1;
  } while (startline < b.length());
  // FIXME: if a block of quoted text finishes the body, the [Hidden quoted]
  // line is zapped. The right fix is to move the whole expansion of
  // quoted text in a separate function:
  // startline = expand_quoted_text(&body_text, startline, &html_quoted_text);
  return b2;
}

// [static] Return an html-displayable form of the string
QString
mail_displayer::htmlize(QString s)
{
  return s.replace("<", "&lt;").replace(">", "&gt;");
}

/* Format the traces of the filter execution in an HTML string.
   Filters that have been modified or deleted since the time when they've been
   applied to the message are indicated as such.
*/
QString
mail_displayer::format_filters_trace(const filter_log_list& list)
{
  if (list.isEmpty())
    return QString();

  QString r = "<font color=\"brown\">" + QObject::tr("Filtered by:") + " </font>";
  bool show_numbers=(list.count()>1);
  for (int i=0; i<list.count(); i++) {
    const filter_log_entry e = list.at(i);
    if (i>0)
      r.append("<br> &nbsp;");
    if (show_numbers)
      r.append(QString("%1. ").arg(i+1));
    QString name = e.filter_name();
    if (!name.isEmpty()) {
      r.append(QString(" [%1] ").arg(htmlize(name)));
    }
    else {
      r.append(QString(" [#%1] ").arg(e.filter_expr_id()));
      if (e.m_deleted) {
	r.append("<i>" + QObject::tr("(filter deleted)") + "</i>");
      }
    }
    if (!e.m_deleted) {
      r.append("<i>");
      r.append(e.m_modified ? QObject::tr("(filter modified)") : htmlize(e.filter_expression()));
      r.append("</i>");
    }
  }
  return r;
}


QString
mail_displayer::sprint_additional_headers(const display_prefs& prefs,
					  mail_msg* msg)
{
  QString h;

  // Insert tag names
  if (prefs.m_show_tags_in_headers) {
    std::list<uint>& tags = msg->get_tags();
    QStringList tag_names = tags_repository::names_list(tags);
    if (!tag_names.isEmpty()) {
      h.append(QString("<br><font color=\"brown\">" + QObject::tr("Tags:") + " </font>"));
      for (int i=0; i<tag_names.size(); i++) {
	tag_names[i] = "<b>"+ htmlize(tag_names[i]) + "</b>";
      }

      h.append(tag_names.join(" | "));
    }
  }

  // Insert filters log if enabled
  if (prefs.m_show_filters_trace) {
    filter_log_list lli;
    if (lli.fetch(msg->get_id())) {
      QString fh = format_filters_trace(lli);
      if (!fh.isEmpty()) {
	h.append("<br>");
	h.append(fh);
      }
    }
  }

  return h;  
}

void
display_prefs::init()
{
  m_show_headers_level = 1;
  m_show_tags = get_config().get_bool("show_tags");
  m_threaded = get_config().get_bool("display_threads");
  m_hide_quoted = 0;
  m_clickable_urls=get_config().get_bool("body_clickable_urls", true);
  m_show_filters_trace = get_config().get_bool("display/filters_trace");
  m_show_tags_in_headers = get_config().get_bool("display/tags_in_headers", true);
  m_decode_qp = false;
  m_codec = NULL;
}
