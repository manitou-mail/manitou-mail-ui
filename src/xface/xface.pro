TEMPLATE=lib
CONFIG+=staticlib release
DESTDIR=.
SOURCES= xface.cpp \
 arith.cpp \
 compress.cpp \
 file.cpp \
 gen.cpp

HEADERS=compface.h \
 data.h \
 vars.h \
 xface.h
