#################################
# $Id$
# Author:  Justin Foley
#################################

# Build application "preprocess_entry"
#################################

APP = preprocess_entry
SRC = preprocess_entry
LIB = xobjwrite variation_utils $(OBJREAD_LIBS) xalnmgr xobjutil \
      gbseq entrez2cli entrez2 tables $(OBJMGR_LIBS)

LIBS = $(CMPRS_LIBS) $(NETWORK_LIBS) $(DL_LIBS) $(ORIG_LIBS)

REQUIRES = objects -Cygwin


WATCHERS = foleyjp