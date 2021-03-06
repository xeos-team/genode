SRC_CC += console/log_console.cc
SRC_CC += cpu/cache.cc
SRC_CC += env/env.cc env/stack_area.cc env/reinitialize.cc
SRC_CC += thread/thread_start.cc
SRC_CC += irq/platform.cc
SRC_CC += server/rpc_cap_alloc.cc

vpath %.cc  $(REP_DIR)/src/base
vpath %.cc $(BASE_DIR)/src/base

INC_DIR += $(REP_DIR)/src/include $(BASE_DIR)/src/include

LIBS += base-common
