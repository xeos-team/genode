#
# \brief  Portions of base library that are exclusive to non-core processes
# \author Norman Feske
# \date   2013-02-14
#
# The content of this file is used for both native Genode as well as hybrid
# Linux/Genode programs. Hence, it must be void of any thread-related code.
#

LIBS += base-common syscall cxx

SRC_CC += console/log_console.cc
SRC_CC += env/env.cc env/platform_env.cc env/stack_area.cc
SRC_CC += server/rpc_cap_alloc.cc

INC_DIR += $(REP_DIR)/src/include $(BASE_DIR)/src/include

vpath %.cc  $(REP_DIR)/src/base
vpath %.cc $(BASE_DIR)/src/base

# vi:set ft=make :
