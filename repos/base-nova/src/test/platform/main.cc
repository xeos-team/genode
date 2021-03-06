/*
 * \brief  Some platform tests for the base-nova
 * \author Alexander Boettcher
 * \date   2015-01-02
 *
 */

/*
 * Copyright (C) 2015 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/thread.h>
#include <base/printf.h>
#include <base/snprintf.h>

#include <util/touch.h>
#include <rm_session/connection.h>
#include <region_map/client.h>

#include <os/attached_rom_dataspace.h>
#include <os/config.h>

#include <trace/timestamp.h>
#include <nova/native_thread.h>

#include "server.h"

static unsigned failed = 0;

static unsigned check_pat = 1;

using namespace Genode;

void test_pat()
{
	/* read out the tsc frequenzy once */
	Genode::Attached_rom_dataspace _ds("hypervisor_info_page");
	Nova::Hip * const hip = _ds.local_addr<Nova::Hip>();

	enum { DS_ORDER = 12, PAGE_4K = 12 };

	Ram_dataspace_capability ds = env()->ram_session()->alloc (1 << (DS_ORDER + PAGE_4K), WRITE_COMBINED);
	addr_t map_addr = env()->rm_session()->attach(ds);

	enum { STACK_SIZE = 4096 };

	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "rpc_ep");

	Test::Component  component;
	Test::Capability session_cap = ep.manage(&component);
	Test::Client     client(session_cap);

	Genode::Rm_connection rm;
	Genode::Region_map_client rm_free_area(rm.create(1 << (DS_ORDER + PAGE_4K)));
	addr_t remap_addr = Genode::env()->rm_session()->attach(rm_free_area.dataspace());

	/* trigger mapping of whole area */
	for (addr_t i = map_addr; i < map_addr + (1 << (DS_ORDER + PAGE_4K)); i += (1 << PAGE_4K))
		touch_read(reinterpret_cast<unsigned char *>(map_addr));

	/*
	 * Manipulate entrypoint
	 */
	Nova::Rights all(true, true, true);
	Genode::addr_t  utcb_ep_addr_t = client.leak_utcb_address();
	Nova::Utcb *utcb_ep = reinterpret_cast<Nova::Utcb *>(utcb_ep_addr_t);
	/* overwrite receive window of entrypoint */
	utcb_ep->crd_rcv = Nova::Mem_crd(remap_addr >> PAGE_4K, DS_ORDER, all);

	/*
	 * Set-up current (client) thread to delegate write-combined memory
	 */
	Nova::Mem_crd snd_crd(map_addr >> PAGE_4K, DS_ORDER, all);

	Nova::Utcb *utcb = reinterpret_cast<Nova::Utcb *>(Thread_base::myself()->utcb());
	enum {
		HOTSPOT = 0, USER_PD = false, HOST_PGT = false, SOLELY_MAP = false,
		NO_DMA = false, EVILLY_DONT_WRITE_COMBINE = false
	};

	Nova::Crd old = utcb->crd_rcv;

	utcb->set_msg_word(0);
	bool ok = utcb->append_item(snd_crd, HOTSPOT, USER_PD, HOST_PGT,
	                            SOLELY_MAP, NO_DMA, EVILLY_DONT_WRITE_COMBINE);
	(void)ok;

	uint8_t res = Nova::call(session_cap.local_name());
	(void)res;

	utcb->crd_rcv = old;

	/* sanity check - touch re-mapped area */
	for (addr_t i = remap_addr; i < remap_addr + (1 << (DS_ORDER + PAGE_4K)); i += (1 << PAGE_4K))
		touch_read(reinterpret_cast<unsigned char *>(remap_addr));

	/*
	 * measure time to write to the memory
	 */
	memset(reinterpret_cast<void *>(map_addr), 0, 1 << (DS_ORDER + PAGE_4K));
	Trace::Timestamp map_start = Trace::timestamp();
	memset(reinterpret_cast<void *>(map_addr), 0, 1 << (DS_ORDER + PAGE_4K));
	Trace::Timestamp map_end = Trace::timestamp();

	memset(reinterpret_cast<void *>(remap_addr), 0, 1 << (DS_ORDER + PAGE_4K));
	Trace::Timestamp remap_start = Trace::timestamp();
	memset(reinterpret_cast<void *>(remap_addr), 0, 1 << (DS_ORDER + PAGE_4K));
	Trace::Timestamp remap_end = Trace::timestamp();

	Trace::Timestamp map_run   = map_end - map_start;
	Trace::Timestamp remap_run = remap_end - remap_start;

	Trace::Timestamp diff_run = map_run > remap_run ? map_run - remap_run : remap_run - map_run;

	if (check_pat && diff_run * 100 / hip->tsc_freq) {
		failed ++;

		PERR("map=%llx remap=%llx --> diff=%llx freq_tsc=%u %llu us",
		     map_run, remap_run, diff_run, hip->tsc_freq,
		     diff_run * 1000 / hip->tsc_freq);
	}

	Nova::revoke(Nova::Mem_crd(remap_addr >> PAGE_4K, DS_ORDER, all));

	/*
	 * note: server entrypoint died because of unexpected receive window
	 *       state - that is expected
	 */
}

void test_server_oom()
{
	using namespace Genode;

	enum { STACK_SIZE = 4096 };

	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "rpc_ep");

	Test::Component  component;
	Test::Capability session_cap = ep.manage(&component);
	Test::Client     client(session_cap);

	/* case that during reply we get oom */
	for (unsigned i = 0; i < 20000; i++) {
		Genode::Native_capability got_cap = client.void_cap();

		if (!got_cap.valid()) {
			PERR("%u cap id %lx invalid", i, got_cap.local_name());
			failed ++;
			break;
		}

		/* be evil and keep this cap by manually incrementing the ref count */
		Cap_index idx(cap_map()->find(got_cap.local_name()));
		idx.inc();

		if (i % 5000 == 4999)
			PINF("received %u. cap", i);
	}

	/* XXX this code does does no longer work since the removal of 'solely_map' */
#if 0

	/* case that during send we get oom */
	for (unsigned i = 0; i < 20000; i++) {
		/* be evil and switch translation off - server ever uses a new selector */
		Genode::Native_capability send_cap = session_cap;
		send_cap.solely_map();

		if (!client.cap_void(send_cap)) {
			PERR("sending %4u. cap failed", i);
			failed ++;
			break;
		}

		if (i % 5000 == 4999)
			PINF("sent %u. cap", i);
	}
#endif

	ep.dissolve(&component);
}

class Greedy : public Thread<4096> {

	public:

		Greedy()
		:
			Thread<0x1000>("greedy")
		{ }

		void entry()
		{
			PINF("starting");

			enum { SUB_RM_SIZE = 2UL * 1024 * 1024 * 1024 };

			Genode::Rm_connection rm;
			Genode::Region_map_client sub_rm(rm.create(SUB_RM_SIZE));
			addr_t const mem = env()->rm_session()->attach(sub_rm.dataspace());

			Nova::Utcb * nova_utcb = reinterpret_cast<Nova::Utcb *>(utcb());
			Nova::Rights const mapping_rwx(true, true, true);

			addr_t const page_fault_portal = native_thread().exc_pt_sel + 14;

			PERR("cause mappings in range [0x%lx, 0x%lx) %p", mem,
			     mem + SUB_RM_SIZE - 1, &mem);

			for (addr_t map_to = mem; map_to < mem + SUB_RM_SIZE; map_to += 4096) {

				/* setup faked page fault information */
				nova_utcb->items   = ((addr_t)&nova_utcb->qual[2] - (addr_t)nova_utcb->msg) / sizeof(addr_t);
				nova_utcb->ip      = 0xbadaffe;
				nova_utcb->qual[1] = (addr_t)&mem;
				nova_utcb->crd_rcv = Nova::Mem_crd(map_to >> 12, 0, mapping_rwx);

				/* trigger faked page fault */
				Genode::uint8_t res = Nova::call(page_fault_portal);
				if (res != Nova::NOVA_OK) {
					PINF("call result=%u", res);
					failed++;
					return;
				}

				/* check that we really got the mapping */
				touch_read(reinterpret_cast<unsigned char *>(map_to));

				/* print status information in interval of 32M */
				if (!(map_to & (32UL * 1024 * 1024 - 1))) {
					printf("0x%lx\n", map_to);
					/* trigger some work to see quota in kernel decreasing */
//					Nova::Rights rwx(true, true, true);
//					Nova::revoke(Nova::Mem_crd((map_to - 32 * 1024 * 1024) >> 12, 12, rwx));
				}
			}
			printf("still alive - done\n");
		}
};

void check(uint8_t res, const char *format, ...)
{
	static char buf[128];

	va_list list;
	va_start(list, format);

	String_console sc(buf, sizeof(buf));
	sc.vprintf(format, list);

	va_end(list);

	if (res == Nova::NOVA_OK) {
		PERR("res=%u %s - TEST FAILED", res, buf);
		failed++;
	}
	else
		printf("res=%u %s\n", res, buf);
}

int main(int argc, char **argv)
{
	printf("testing base-nova platform\n");

	try {
		Genode::config()->xml_node().attribute("check_pat").value(&check_pat);
	} catch (...) { }

	Thread_base * myself = Thread_base::myself();
	if (!myself)
		return -__LINE__;

	addr_t sel_pd  = cap_map()->insert();
	addr_t sel_ec  = myself->native_thread().ec_sel;
	addr_t sel_cap = cap_map()->insert();
	addr_t handler = 0UL;
	uint8_t    res = 0;

	Nova::Mtd mtd(Nova::Mtd::ALL);

	if (sel_cap == ~0UL || sel_ec == ~0UL || sel_cap == ~0UL)
		return -__LINE__;

	/* negative syscall tests - they should not succeed */
	res = Nova::create_pt(sel_cap, sel_pd, sel_ec, mtd, handler);
	check(res, "create_pt");

	res = Nova::create_sm(sel_cap, sel_pd, 0);
	check(res, "create_sm");

	/* changing the badge of one of the portal must fail */
	for (unsigned i = 0; i < (1U << Nova::NUM_INITIAL_PT_LOG2); i++) {
		addr_t sel_exc = myself->native_thread().exc_pt_sel + i;
		res = Nova::pt_ctrl(sel_exc, 0xbadbad);
		check(res, "pt_ctrl %2u", i);
	}

	/* upgrade available capability indices for this process */
	unsigned index = 512 * 1024;
	static char local[128][sizeof(Cap_range)];

	for (unsigned i = 0; i < sizeof(local) / sizeof (local[0]); i++) {
		Cap_range * range = reinterpret_cast<Cap_range *>(local[i]);
		*range = Cap_range(index);

		cap_map()->insert(range);

		index = range->base() + range->elements();
	};

	/* test PAT kernel feature */
	test_pat();

	/**
	 * Test to provoke out of memory during capability transfer of
	 * server/client.
	 *
	 * Set in hypervisor.ld the memory to a low value of about 1M to let
	 * trigger the test.
	 */
	test_server_oom();

	/* Test to provoke out of memory in kernel during interaction with core */
	static Greedy core_pagefault_oom;
	core_pagefault_oom.start();
	core_pagefault_oom.join();

	if (!failed)
		printf("Test finished\n");

	return -failed;
}
