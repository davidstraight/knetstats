INCLUDES = $(all_includes)

SUBDIRS = pics

bin_PROGRAMS = knetstats

knetstats_SOURCES = configurebase.ui configure.cpp statisticsbase.ui main.cpp \
	knetstats.cpp knetstatsview.cpp statistics.cpp chart.cpp interface.cpp
knetstats_LDADD = $(LIB_KDEUI)
knetstats_LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = configure.moc knetstats.moc knetstatsview.moc statistics.moc

# these are the headers for your project
noinst_HEADERS = chart.h configure.h knetstats.h knetstatsview.h statistics.h \
	interface.h

messages: rc.cpp
	$(EXTRACTRC) *.ui >> rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/knetstats.pot

KDE_OPTIONS = nofinal

