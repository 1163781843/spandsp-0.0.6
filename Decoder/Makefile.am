AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I./include

sbin_PROGRAMS = Decoder

Decoder_SOURCES =				\
	src/decoder-spandsp.cpp		\
	src/callerid-spandsp.cpp	\
	src/fax-spandsp.cpp			\
	src/Decoder.cpp

Decoder_CFLAGS  = $(AM_CFLAGS)
Decoder_LDFLAGS = $(AM_LDFLAGS)
Decoder_LDADD   = $(top_srcdir)/src/libspandsp.la $(LIBDIR) -llogger -lsndfile

