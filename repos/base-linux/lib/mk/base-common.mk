#
# \brief  Portions of base library shared by core and non-core processes
# \author Norman Feske
# \date   2013-02-14
#

LIBS += syscall

SRC_CC += ipc/ipc.cc
SRC_CC += avl_tree/avl_tree.cc
SRC_CC += allocator/slab.cc
SRC_CC += allocator/allocator_avl.cc
SRC_CC += heap/heap.cc heap/sliced_heap.cc
SRC_CC += console/console.cc
SRC_CC += child/child.cc
SRC_CC += child/process.cc
SRC_CC += elf/elf_binary.cc
SRC_CC += lock/lock.cc
SRC_CC += env/region_map_mmap.cc env/debug.cc
SRC_CC += signal/signal.cc signal/common.cc signal/platform.cc
SRC_CC += server/server.cc server/common.cc
SRC_CC += thread/trace.cc thread/thread_env.cc thread/stack_allocator.cc
SRC_CC += irq/platform.cc
SRC_CC += sleep.cc
SRC_CC += region_map_client.cc
SRC_CC += rm_session_client.cc
SRC_CC += entrypoint/entrypoint.cc
SRC_CC += component/component.cc

INC_DIR += $(REP_DIR)/src/include $(BASE_DIR)/src/include

vpath %.cc $(REP_DIR)/src/base
vpath %.cc $(BASE_DIR)/src/base
