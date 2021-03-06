# $Id: Makefile 1176 2019-06-30 07:16:06Z mueller $
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright 2011-2019 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
#
#  Revision History: 
# Date         Rev Version  Comment
# 2019-01-02  1100   1.1.4  drop boost includes
# 2014-11-08   602   1.1.3  add  TCLLIB/TCLLIBNAME to LDLIBS
# 2013-02-01   479   1.1.2  use checkpath_cpp.mk
# 2012-12-24   464   1.1.1  add TCLINC
# 2011-07-31   401   1.1    rename realclean->distclean
# 2011-07-01   386   1.0.2  add BOOSTINC
# 2011-03-20   372   1.0.1  renamed ..tcl -> ..tpp
# 2011-03-19   371   1.0    Initial version
#---
#
# Name of the sharable library
#
SONAME     = rutiltpp
SOMAJV     = 1
SOMINV     = 0
#
# Compile and Link search paths
#
include ../checkpath_cpp.mk
#
INCLFLAGS  = -I${RETROBASE}/tools/src -I${TCLINC}
LDLIBS     = -lpthread 
LDLIBS    += -L${TCLLIB} -l${TCLLIBNAME}
LDLIBS    += -L${RETROBASE}/tools/lib -lrtcltools
#
# Object files to be included
#
OBJ_all    = Rutiltpp_Init.o RtclBvi.o RtclSystem.o RtclSignalAction.o
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
