include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_file_recorder

mod_LTLIBRARIES = mod_file_recorder.la
mod_file_recorder_la_SOURCES  = mod_file_recorder.cpp VideoRecorder.cpp CommonFunc.cpp VideoRecordManager.cpp
mod_file_recorder_la_CFLAGS   = -I.
mod_file_recorder_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_file_recorder_la_LDFLAGS  = -avoid-version -module -no-undefined -shared -lm -lz

$(am_mod_file_recorder_la_OBJECTS):  VideoRecorder.h CommonFunc.h
