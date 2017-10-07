/* Copyright (C) 2004-2017 Daniel Verite

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

#include "about.h"
#include "main.h"
#include "icons.h"
#include "browser.h"

#include <QPushButton>
#include <QBrush>
#include <QLayout>
#include <QTabWidget>

static const char* gpl_txt = 
"\n"
"		    GNU GENERAL PUBLIC LICENSE\n"
"		       Version 2, June 1991\n"
"\n"
" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n"
"                          675 Mass Ave, Cambridge, MA 02139, USA\n"
" Everyone is permitted to copy and distribute verbatim copies\n"
" of this license document, but changing it is not allowed.\n"
"\n"
"			    Preamble\n"
"\n"
"  The licenses for most software are designed to take away your "
"freedom to share and change it.  By contrast, the GNU General Public "
"License is intended to guarantee your freedom to share and change free "
"software--to make sure the software is free for all its users.  This "
"General Public License applies to most of the Free Software "
"Foundation's software and to any other program whose authors commit to "
"using it.  (Some other Free Software Foundation software is covered by "
"the GNU Library General Public License instead.)  You can apply it to "
"your programs, too.\n"
"\n"
"  When we speak of free software, we are referring to freedom, not "
"price.  Our General Public Licenses are designed to make sure that you "
"have the freedom to distribute copies of free software (and charge for "
"this service if you wish), that you receive source code or can get it "
"if you want it, that you can change the software or use pieces of it "
"in new free programs; and that you know you can do these things.\n"
"\n"
"  To protect your rights, we need to make restrictions that forbid "
"anyone to deny you these rights or to ask you to surrender the rights. "
"These restrictions translate to certain responsibilities for you if you "
"distribute copies of the software, or if you modify it.\n"
"\n"
"  For example, if you distribute copies of such a program, whether "
"gratis or for a fee, you must give the recipients all the rights that "
"you have.  You must make sure that they, too, receive or can get the "
"source code.  And you must show them these terms so they know their "
"rights.\n"
"\n"
"  We protect your rights with two steps: (1) copyright the software, and "
"(2) offer you this license which gives you legal permission to copy, "
"distribute and/or modify the software.\n"
"\n"
"  Also, for each author's protection and ours, we want to make certain "
"that everyone understands that there is no warranty for this free "
"software.  If the software is modified by someone else and passed on, we "
"want its recipients to know that what they have is not the original, so "
"that any problems introduced by others will not reflect on the original "
"authors' reputations.\n"
"\n"
"  Finally, any free program is threatened constantly by software "
"patents.  We wish to avoid the danger that redistributors of a free "
"program will individually obtain patent licenses, in effect making the "
"program proprietary.  To prevent this, we have made it clear that any "
"patent must be licensed for everyone's free use or not licensed at all.\n"
"\n"
"  The precise terms and conditions for copying, distribution and "
"modification follow.\n"
"\n"
"		    GNU GENERAL PUBLIC LICENSE\n"
"   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND "
"MODIFICATION\n"
"\n"
"  0. This License applies to any program or other work which contains "
"a notice placed by the copyright holder saying it may be distributed "
"under the terms of this General Public License.  The \"Program\", below, "
"refers to any such program or work, and a \"work based on the Program\" "
"means either the Program or any derivative work under copyright law: "
"that is to say, a work containing the Program or a portion of it, "
"either verbatim or with modifications and/or translated into another "
"language.  (Hereinafter, translation is included without limitation in "
"the term \"modification\".)  Each licensee is addressed as \"you\".\n"
"\n"
"Activities other than copying, distribution and modification are not "
"covered by this License; they are outside its scope.  The act of "
"running the Program is not restricted, and the output from the Program "
"is covered only if its contents constitute a work based on the "
"Program (independent of having been made by running the Program). "
"Whether that is true depends on what the Program does.\n"
"\n"
"  1. You may copy and distribute verbatim copies of the Program's "
"source code as you receive it, in any medium, provided that you "
"conspicuously and appropriately publish on each copy an appropriate "
"copyright notice and disclaimer of warranty; keep intact all the "
"notices that refer to this License and to the absence of any warranty; "
"and give any other recipients of the Program a copy of this License "
"along with the Program.\n"
"\n"
"You may charge a fee for the physical act of transferring a copy, and "
"you may at your option offer warranty protection in exchange for a fee. "
"\n"
"  2. You may modify your copy or copies of the Program or any portion "
"of it, thus forming a work based on the Program, and copy and "
"distribute such modifications or work under the terms of Section 1 "
"above, provided that you also meet all of these conditions:\n"
"\n"
"    a) You must cause the modified files to carry prominent notices "
"stating that you changed the files and the date of any change. "
"\n"
"    b) You must cause any work that you distribute or publish, that in "
"whole or in part contains or is derived from the Program or any "
"part thereof, to be licensed as a whole at no charge to all third "
"parties under the terms of this License. "
"\n"
"    c) If the modified program normally reads commands interactively "
"when run, you must cause it, when started running for such "
"interactive use in the most ordinary way, to print or display an "
"announcement including an appropriate copyright notice and a "
"notice that there is no warranty (or else, saying that you provide "
"a warranty) and that users may redistribute the program under "
"these conditions, and telling the user how to view a copy of this "
"License.  (Exception: if the Program itself is interactive but "
"does not normally print such an announcement, your work based on "
"the Program is not required to print an announcement.)\n"
"\n"
"These requirements apply to the modified work as a whole.  If "
"identifiable sections of that work are not derived from the Program, "
"and can be reasonably considered independent and separate works in "
"themselves, then this License, and its terms, do not apply to those "
"sections when you distribute them as separate works.  But when you "
"distribute the same sections as part of a whole which is a work based "
"on the Program, the distribution of the whole must be on the terms of "
"this License, whose permissions for other licensees extend to the "
"entire whole, and thus to each and every part regardless of who wrote it.\n"
"\n"
"Thus, it is not the intent of this section to claim rights or contest "
"your rights to work written entirely by you; rather, the intent is to "
"exercise the right to control the distribution of derivative or "
"collective works based on the Program.\n"
"\n"
"In addition, mere aggregation of another work not based on the Program "
"with the Program (or with a work based on the Program) on a volume of "
"a storage or distribution medium does not bring the other work under "
"the scope of this License.\n"
"\n"
"  3. You may copy and distribute the Program (or a work based on it, "
"under Section 2) in object code or executable form under the terms of "
"Sections 1 and 2 above provided that you also do one of the following: "
"\n"
"    a) Accompany it with the complete corresponding machine-readable "
"    source code, which must be distributed under the terms of Sections "
"    1 and 2 above on a medium customarily used for software interchange; or, "
"\n"
"    b) Accompany it with a written offer, valid for at least three "
"    years, to give any third party, for a charge no more than your "
"    cost of physically performing source distribution, a complete "
"    machine-readable copy of the corresponding source code, to be "
"    distributed under the terms of Sections 1 and 2 above on a medium "
"    customarily used for software interchange; or,\n"
"\n"
"    c) Accompany it with the information you received as to the offer "
"    to distribute corresponding source code.  (This alternative is "
"    allowed only for noncommercial distribution and only if you "
"    received the program in object code or executable form with such "
"    an offer, in accord with Subsection b above.)\n"
"\n"
"The source code for a work means the preferred form of the work for "
"making modifications to it.  For an executable work, complete source "
"code means all the source code for all modules it contains, plus any "
"associated interface definition files, plus the scripts used to "
"control compilation and installation of the executable.  However, as a "
"special exception, the source code distributed need not include "
"anything that is normally distributed (in either source or binary "
"form) with the major components (compiler, kernel, and so on) of the "
"operating system on which the executable runs, unless that component "
"itself accompanies the executable.\n"
"\n"
"If distribution of executable or object code is made by offering "
"access to copy from a designated place, then offering equivalent "
"access to copy the source code from the same place counts as "
"distribution of the source code, even though third parties are not "
"compelled to copy the source along with the object code.\n"
"\n"
"  4. You may not copy, modify, sublicense, or distribute the Program "
"except as expressly provided under this License.  Any attempt "
"otherwise to copy, modify, sublicense or distribute the Program is "
"void, and will automatically terminate your rights under this License. "
"However, parties who have received copies, or rights, from you under "
"this License will not have their licenses terminated so long as such "
"parties remain in full compliance.\n"
"\n"
"  5. You are not required to accept this License, since you have not "
"signed it.  However, nothing else grants you permission to modify or "
"distribute the Program or its derivative works.  These actions are "
"prohibited by law if you do not accept this License.  Therefore, by "
"modifying or distributing the Program (or any work based on the "
"Program), you indicate your acceptance of this License to do so, and "
"all its terms and conditions for copying, distributing or modifying "
"the Program or works based on it.\n"
"\n"
"  6. Each time you redistribute the Program (or any work based on the "
"Program), the recipient automatically receives a license from the "
"original licensor to copy, distribute or modify the Program subject to "
"these terms and conditions.  You may not impose any further "
"restrictions on the recipients' exercise of the rights granted herein. "
"You are not responsible for enforcing compliance by third parties to "
"this License.\n"
"\n"
"  7. If, as a consequence of a court judgment or allegation of patent "
"infringement or for any other reason (not limited to patent issues), "
"conditions are imposed on you (whether by court order, agreement or "
"otherwise) that contradict the conditions of this License, they do not "
"excuse you from the conditions of this License.  If you cannot "
"distribute so as to satisfy simultaneously your obligations under this "
"License and any other pertinent obligations, then as a consequence you "
"may not distribute the Program at all.  For example, if a patent "
"license would not permit royalty-free redistribution of the Program by "
"all those who receive copies directly or indirectly through you, then "
"the only way you could satisfy both it and this License would be to "
"refrain entirely from distribution of the Program.\n"
"\n"
"If any portion of this section is held invalid or unenforceable under "
"any particular circumstance, the balance of the section is intended to "
"apply and the section as a whole is intended to apply in other "
"circumstances.\n"
"\n"
"It is not the purpose of this section to induce you to infringe any "
"patents or other property right claims or to contest validity of any "
"such claims; this section has the sole purpose of protecting the "
"integrity of the free software distribution system, which is "
"implemented by public license practices.  Many people have made "
"generous contributions to the wide range of software distributed "
"through that system in reliance on consistent application of that "
"system; it is up to the author/donor to decide if he or she is willing "
"to distribute software through any other system and a licensee cannot "
"impose that choice.\n"
"\n"
"This section is intended to make thoroughly clear what is believed to "
"be a consequence of the rest of this License.\n"
"\n"
"  8. If the distribution and/or use of the Program is restricted in "
"certain countries either by patents or by copyrighted interfaces, the "
"original copyright holder who places the Program under this License "
"may add an explicit geographical distribution limitation excluding "
"those countries, so that distribution is permitted only in or among "
"countries not thus excluded.  In such case, this License incorporates "
"the limitation as if written in the body of this License.\n"
"\n"
"  9. The Free Software Foundation may publish revised and/or new versions "
"of the General Public License from time to time.  Such new versions will "
"be similar in spirit to the present version, but may differ in detail to "
"address new problems or concerns.\n"
"\n"
"Each version is given a distinguishing version number.  If the Program "
"specifies a version number of this License which applies to it and \"any "
"later version\", you have the option of following the terms and conditions "
"either of that version or of any later version published by the Free "
"Software Foundation.  If the Program does not specify a version number of "
"this License, you may choose any version ever published by the Free Software "
"Foundation.\n"
"\n"
"  10. If you wish to incorporate parts of the Program into other free "
"programs whose distribution conditions are different, write to the author "
"to ask for permission.  For software which is copyrighted by the Free "
"Software Foundation, write to the Free Software Foundation; we sometimes "
"make exceptions for this.  Our decision will be guided by the two goals "
"of preserving the free status of all derivatives of our free software and "
"of promoting the sharing and reuse of software generally.\n"
"\n"
"			    NO WARRANTY\n"
"\n"
"11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, "
"THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT "
"PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN OTHERWISE "
"STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER "
"PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY "
"OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, "
"BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF "
"MERCHANTABILITY AND FITNESS FOR A PARTICULAR "
"PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND "
"PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE "
"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL "
"NECESSARY SERVICING, REPAIR OR CORRECTION.\n"
"\n"
"12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR "
"AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR "
"ANY OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE "
"THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR "
"DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL "
"OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR "
"INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT "
"LIMITED TO LOSS OF DATA OR DATA BEING RENDERED "
"INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD "
"PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH "
"ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER "
"PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH "
"DAMAGES.\n"
;


about_box::about_box (QWidget* parent): QDialog(parent)
{
  QVBoxLayout* top_layout = new QVBoxLayout(this);
  top_layout->setSpacing(5);

  setWindowTitle(tr("About Manitou-Mail"));

/*
  m_brush = NULL;
  m_pix = new FT_MAKE_ICON("pattern-about.png");
  if (!m_pix->isNull()) {
    m_brush = new QBrush();
    m_brush->setPixmap(*m_pix);
  }
*/
  m_tabw = new QTabWidget(this);
  top_layout->addWidget(m_tabw);
  init_version_tab();
  init_support_tab();
  init_license_tab();
  m_tabw->addTab(m_version_tab, tr("Version"));
  m_tabw->addTab(m_license_tab, tr("License"));
  m_tabw->addTab(m_support_tab, tr("Support resources"));

  resize(600, 400);

  QHBoxLayout* hbl = new QHBoxLayout;
  top_layout->addLayout(hbl);
  hbl->addStretch(20);
  QPushButton* ok = new QPushButton(tr("OK"), this);
  hbl->addWidget(ok);
  hbl->addStretch(20);

  connect(ok, SIGNAL(clicked()), this, SLOT(close()));
}

about_box::~about_box()
{
}


void
about_box::init_version_tab()
{
  m_version_tab = new QWidget(this);
  QVBoxLayout* l = new QVBoxLayout(m_version_tab);
  about_panel* wt = new about_panel(m_version_tab);
  l->addWidget(wt);

  QString s=tr("This is <b>Manitou-Mail</b> version %1, (c) 2004-2017 Daniel V&#233;rit&#233; (<i>daniel@manitou-mail.org</i>).").arg(VERSION);
  QString s1=tr("Compiled with Qt %1, running with Qt %2").arg(QT_VERSION_STR).arg(qVersion());
  QString s2=tr(
   "This program is free software; you can redistribute it and/or modify"
   " it under the terms of the GNU General Public License version 2 as"
   " published by the Free Software Foundation."

   "<br>This program is distributed in the hope that it will be useful,"
   " but WITHOUT ANY WARRANTY; without even the implied warranty of"
   " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
   " GNU General Public License for more details.");

  QString build_date=tr("Build date: ") + __DATE__;
  wt->setText(QString("<html><body style=\"background-image:url(/pattern-about.png)\"><table><tr><td><img src=\"/manitou.png\"></td><td valign=\"center\">")+s+"\n<br><p>"+build_date+"</td></tr></table>"+"<p>"+s1+"<p>"+s2+"</body></html>");
}

void
about_box::init_support_tab()
{
  m_support_tab = new QWidget(this);
  QVBoxLayout* l = new QVBoxLayout(m_support_tab);
  about_panel* txt_w = new about_panel(m_support_tab);
  l->addWidget(txt_w);

  QString git_url="https://github.com/manitou-mail/manitou-mail-ui";

  QString htxt="<html><body style=\"background-image:url(/pattern-about.png)\">";
  htxt.append("<p>");
  htxt.append(tr("Home Page: ") + "<br><a href=\"http://www.manitou-mail.org\">http://www.manitou-mail.org</a><br>");
  htxt.append("<p>");
  htxt.append(tr("Public git repository and issue tracker: ") + 
		"<br><a href=\""+ git_url+"\">" + git_url + "</a><br>");
  htxt.append("<p>");
  htxt.append(tr("Email:<br>support%1manitou-mail.org</a>").arg("@"));
  htxt.append("</body></html>");
  txt_w->setHtml(htxt);

  connect(txt_w, SIGNAL(anchorClicked(const QUrl&)),
	  this, SLOT(link_clicked(const QUrl&)));
}

void
about_box::init_license_tab()
{
  m_license_tab = new QWidget(this);
  QVBoxLayout* l = new QVBoxLayout(m_license_tab);
  about_panel* v2 = new about_panel(m_license_tab);
  l->addWidget(v2);
  QString htxt="<html><body style=\"background-image:url(/pattern-about.png)\">";
  QString gpl=gpl_txt;
  gpl.replace("\n", "<br>");
  htxt.append(gpl);
  htxt.append("</body></html>");
  v2->setHtml(htxt);
}

void
about_box::link_clicked(const QUrl& url)
{
  // TODO: make browser::open_url accept a QUrl as argument
  QByteArray link=url.toEncoded();
  if (link.count() != 0) {
    browser::open_url(QString("%1").arg(link.constData())); // arf
  }
  else {
    DBG_PRINTF(1, "Warning: Empty URL\n");
  }
}

void
about_panel::setSource(const QUrl& url _UNUSED_)
{
  /* No new page, the URL is submitted to an external browser through
   about_box::link_clicked() This function to be defined though
   otherwise the QTextBrowser will replace the current page by a blank
   one when a link is clicked */
  return;
}

QVariant
about_panel::loadResource(int type _UNUSED_, const QUrl& name)
{
  if (name.toString()=="/manitou.png") {
    QVariant v(FT_MAKE_ICON("manitou-logo.png"));
    return v;
  }
  else if (name.toString()=="/pattern-about.png") {
    QVariant v(FT_MAKE_ICON("pattern-about.png"));
    return v;
  }
  return QVariant();
}
