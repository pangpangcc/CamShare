include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_ws
mod_LTLIBRARIES = mod_ws.la
mod_ws_la_SOURCES  = mod_ws.cpp WebSocketServer.cpp TcpServer.cpp
mod_ws_la_CFLAGS   = $(AM_CFLAGS)
mod_ws_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_ws_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
