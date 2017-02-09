include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_ws
mod_LTLIBRARIES = mod_ws.la
mod_ws_la_SOURCES  = mod_ws.cpp TcpServer.cpp AsyncIOServer.cpp  WebSocketServer.cpp DataParser.cpp WSClientParser.cpp Rtp2H264VideoTransfer.cpp Arithmetic.cpp aes.c
mod_ws_la_CFLAGS   = $(AM_CFLAGS)
mod_ws_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_ws_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
