#include "compface.h"
#include "vars.h"

#include <QString>

int xface_to_xpm(const char* xface_ascii, QString& xface_xpm)
{
  xface x;
  try {
    x.UnCompAll ((char*)xface_ascii);
    x.UnGenFace();
  }
  catch (int) {
    return 0;
  }

  static const char prologue[] =
    "/* XPM */\n"
    "static char *test[] = {\n"
    "/* width height num_colors chars_per_pixel */\n"
    "\"    48    48        2            1\",\n"
    "/* colors */\n"
    "\"0 c #ffffff\",\n"
    "\"1 c #000000\",\n"
    "/* pixels */\n";

  xface_xpm.truncate(0);
  xface_xpm.append (prologue);

  const char* buf = x.buffer();
  xface_xpm.append ('"');
  for (int i=0; i<PIXELS; i++) {
    if (i%48==0 && i!=0) {
      xface_xpm.append("\",\n\"");
    }
    xface_xpm.append (buf[i]+'0');
  }
  xface_xpm.append ("\"\n};\n");
  return 1;
}
