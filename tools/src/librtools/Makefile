# $Id: Makefile 1176 2019-06-30 07:16:06Z mueller $
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright 2011-2019 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
#
#  Revision History: 
# Date         Rev Version  Comment
# 2019-06-15  1163   1.1.6  add Rfilefd
# 2019-06-07  1161   1.1.5  add Rfd
# 2019-03-30  1125   1.1.4  add ReventFd,RtimerFd
# 2019-01-02  1100   1.1.3  drop boost includes and libs
# 2014-11-01   600   1.1.2  add -lboost_system (needed in boost 1.54)
# 2013-02-01   479   1.1.1  use checkpath_cpp.mk
# 2011-07-31   401   1.1    rename realclean->distclean
# 2011-03-27   374   1.0.1  removed Rnamed (obsolete now)
# 2011-01-15   357   1.0    Initial version (adopted from CTB...)
#---
#
# Name of the sharable library
#
SONAME     = rtools
SOMAJV     = 1
SOMINV     = 0
#
# Compile and Link search paths
#
include ../checkpath_cpp.mk
#
# Object files to be included
#
OBJ_all    = Rbits.o
OBJ_all   += RerrMsg.o
OBJ_all   += ReventFd.o
OBJ_all   += Rexception.o
OBJ_all   += Rfd.o
OBJ_all   += RfileFd.o
OBJ_all   += RiosState.o
OBJ_all   += RlogFile.o RlogFileCatalog.o RlogMsg.o
OBJ_all   += RosFill.o 
OBJ_all   += RosPrintBvi.o RosPrintfBase.o RosPrintfS.o
OBJ_all   += RparseUrl.o
OBJ_all   += Rstats.o
OBJ_all   += Rtime.o
OBJ_all   += RtimerFd.o
OBJ_all   += Rtools.o
#
DEP_all    = $(OBJ_all:.o=.dep)
#
#- generic part ----------------------------------------------------------------
#
SOFILE     = lib$(SONAME).so
SOFILEV    = lib$(SONAME).so.$(SOMAJV)
SOFILEVV   = lib$(SONAME).so.$(SOMAJV).$(SOMINV)
#
include ${RETROBASE}/tools/make/generic_cpp.mk
include ${RETROBASE}/tools/make/generic_dep.mk
include ${RETROBASE}/tools/make/generic_so.mk
include ${RETROBASE}/tools/make/dontincdep.mk
#
# The magic auto-dependency include
#
ifndef DONTINCDEP
include $(DEP_all)
endif
#
# cleanup phonies:
#
.PHONY    : clean cleandep distclean
clean     :
	@ rm -f $(OBJ_all)
	@ echo "Object files removed"
#
cleandep  :
	@ rm -f $(DEP_all)
	@ echo "Dependency files removed"
#
distclean : clean cleandep
	@ rm -f $(SOPATH)/lib$(SONAME).a $(SOPATH)/lib$(SONAME).so*
	@ echo "Libraries removed"
#
