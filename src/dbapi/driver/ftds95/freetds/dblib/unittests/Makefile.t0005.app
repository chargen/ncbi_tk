# $Id$

APP = db95_t0005
SRC = t0005 common

CPPFLAGS = -DHAVE_CONFIG_H=1 -DNEED_FREETDS_SRCDIR $(FTDS95_INCLUDE) \
           $(ORIG_CPPFLAGS)
LIB      = sybdb_ftds95$(STATIC) tds_ftds95$(STATIC)
LIBS     = $(FTDS95_CTLIB_LIBS) $(NETWORK_LIBS) $(RT_LIBS) $(C_LIBS)
LINK     = $(C_LINK)

CHECK_CMD  = test-db95.sh --no-auto db95_t0005
CHECK_COPY = test-db95.sh t0005.sql

CHECK_REQUIRES = in-house-resources

WATCHERS = ucko