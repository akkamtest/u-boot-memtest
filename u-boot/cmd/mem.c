// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <console.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <hash.h>
#include <inttypes.h>
#include <mapmem.h>
#include <watchdog.h>
#include <asm/io.h>
#include <linux/compiler.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_SYS_MEMTEST_SCRATCH
#define CONFIG_SYS_MEMTEST_SCRATCH 0
#endif

#ifdef CONFIG_CMD_MEMTEST
#include "memtest.h"
#endif

static int mod_mem(cmd_tbl_t *, int, int, int, char * const []);

/* Display values from last command.
 * Memory modify remembered values are different from display memory.
 */
static ulong	dp_last_addr, dp_last_size;
static ulong	dp_last_length = 0x40;
static ulong	mm_last_addr, mm_last_size;

static	ulong	base_address = 0;

/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l, .q} {addr} {len}
 */
#define DISP_LINE_LEN	16
static int do_mem_md(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr, length, bytes;
	const void *buf;
	int	size;
	int rc = 0;

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = dp_last_addr;
	size = dp_last_size;
	length = dp_last_length;

	if (argc < 2)
		return CMD_RET_USAGE;

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/* New command specified.  Check for a size specification.
		 * Defaults to long if no or incorrect specification.
		 */
		if ((size = cmd_get_data_size(argv[0], 4)) < 0)
			return 1;

		/* Address is specified since argc > 1
		*/
		addr = simple_strtoul(argv[1], NULL, 16);
		addr += base_address;

		/* If another parameter, it is the length to display.
		 * Length is the number of objects, not number of bytes.
		 */
		if (argc > 2)
			length = simple_strtoul(argv[2], NULL, 16);
	}

	bytes = size * length;
	buf = map_sysmem(addr, bytes);

	/* Print the lines. */
	print_buffer(addr, buf, size, length, DISP_LINE_LEN / size);
	addr += bytes;
	unmap_sysmem(buf);

	dp_last_addr = addr;
	dp_last_length = length;
	dp_last_size = size;
	return (rc);
}

static int do_mem_mm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return mod_mem (cmdtp, 1, flag, argc, argv);
}
static int do_mem_nm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return mod_mem (cmdtp, 0, flag, argc, argv);
}

static int do_mem_mw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	u64 writeval;
#else
	ulong writeval;
#endif
	ulong	addr, count;
	int	size;
	void *buf, *start;
	ulong bytes;

	if ((argc < 3) || (argc > 4))
		return CMD_RET_USAGE;

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

	/* Address is specified since argc > 1
	*/
	addr = simple_strtoul(argv[1], NULL, 16);
	addr += base_address;

	/* Get the value to write.
	*/
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	writeval = simple_strtoull(argv[2], NULL, 16);
#else
	writeval = simple_strtoul(argv[2], NULL, 16);
#endif

	/* Count ? */
	if (argc == 4) {
		count = simple_strtoul(argv[3], NULL, 16);
	} else {
		count = 1;
	}

	bytes = size * count;
	start = map_sysmem(addr, bytes);
	buf = start;
	while (count-- > 0) {
		if (size == 4)
			*((u32 *)buf) = (u32)writeval;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		else if (size == 8)
			*((u64 *)buf) = (u64)writeval;
#endif
		else if (size == 2)
			*((u16 *)buf) = (u16)writeval;
		else
			*((u8 *)buf) = (u8)writeval;
		buf += size;
	}
	unmap_sysmem(start);
	return 0;
}

#ifdef CONFIG_MX_CYCLIC
static int do_mem_mdc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	ulong count;

	if (argc < 4)
		return CMD_RET_USAGE;

	count = simple_strtoul(argv[3], NULL, 10);

	for (;;) {
		do_mem_md (NULL, 0, 3, argv);

		/* delay for <count> ms... */
		for (i=0; i<count; i++)
			udelay (1000);

		/* check for ctrl-c to abort... */
		if (ctrlc()) {
			puts("Abort\n");
			return 0;
		}
	}

	return 0;
}

static int do_mem_mwc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	ulong count;

	if (argc < 4)
		return CMD_RET_USAGE;

	count = simple_strtoul(argv[3], NULL, 10);

	for (;;) {
		do_mem_mw (NULL, 0, 3, argv);

		/* delay for <count> ms... */
		for (i=0; i<count; i++)
			udelay (1000);

		/* check for ctrl-c to abort... */
		if (ctrlc()) {
			puts("Abort\n");
			return 0;
		}
	}

	return 0;
}
#endif /* CONFIG_MX_CYCLIC */

static int do_mem_cmp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr1, addr2, count, ngood, bytes;
	int	size;
	int     rcode = 0;
	const char *type;
	const void *buf1, *buf2, *base;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	u64 word1, word2;
#else
	ulong word1, word2;
#endif

	if (argc != 4)
		return CMD_RET_USAGE;

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;
	type = size == 8 ? "double word" :
	       size == 4 ? "word" :
	       size == 2 ? "halfword" : "byte";

	addr1 = simple_strtoul(argv[1], NULL, 16);
	addr1 += base_address;

	addr2 = simple_strtoul(argv[2], NULL, 16);
	addr2 += base_address;

	count = simple_strtoul(argv[3], NULL, 16);

	bytes = size * count;
	base = buf1 = map_sysmem(addr1, bytes);
	buf2 = map_sysmem(addr2, bytes);
	for (ngood = 0; ngood < count; ++ngood) {
		if (size == 4) {
			word1 = *(u32 *)buf1;
			word2 = *(u32 *)buf2;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		} else if (size == 8) {
			word1 = *(u64 *)buf1;
			word2 = *(u64 *)buf2;
#endif
		} else if (size == 2) {
			word1 = *(u16 *)buf1;
			word2 = *(u16 *)buf2;
		} else {
			word1 = *(u8 *)buf1;
			word2 = *(u8 *)buf2;
		}
		if (word1 != word2) {
			ulong offset = buf1 - base;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
			printf("%s at 0x%p (%#0*"PRIx64") != %s at 0x%p (%#0*"
			       PRIx64 ")\n",
			       type, (void *)(addr1 + offset), size, word1,
			       type, (void *)(addr2 + offset), size, word2);
#else
			printf("%s at 0x%08lx (%#0*lx) != %s at 0x%08lx (%#0*lx)\n",
				type, (ulong)(addr1 + offset), size, word1,
				type, (ulong)(addr2 + offset), size, word2);
#endif
			rcode = 1;
			break;
		}

		buf1 += size;
		buf2 += size;

		/* reset watchdog from time to time */
		if ((ngood % (64 << 10)) == 0)
			WATCHDOG_RESET();
	}
	unmap_sysmem(buf1);
	unmap_sysmem(buf2);

	printf("Total of %ld %s(s) were the same\n", ngood, type);
	return rcode;
}

static int do_mem_cp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr, dest, count;
	int	size;

	if (argc != 4)
		return CMD_RET_USAGE;

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	addr = simple_strtoul(argv[1], NULL, 16);
	addr += base_address;

	dest = simple_strtoul(argv[2], NULL, 16);
	dest += base_address;

	count = simple_strtoul(argv[3], NULL, 16);

	if (count == 0) {
		puts ("Zero length ???\n");
		return 1;
	}

#ifdef CONFIG_MTD_NOR_FLASH
	/* check if we are copying to Flash */
	if (addr2info(dest) != NULL) {
		int rc;

		puts ("Copy to Flash... ");

		rc = flash_write ((char *)addr, dest, count*size);
		if (rc != 0) {
			flash_perror (rc);
			return (1);
		}
		puts ("done\n");
		return 0;
	}
#endif

	memcpy((void *)dest, (void *)addr, count * size);

	return 0;
}

static int do_mem_base(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	if (argc > 1) {
		/* Set new base address.
		*/
		base_address = simple_strtoul(argv[1], NULL, 16);
	}
	/* Print the current base address.
	*/
	printf("Base Address: 0x%08lx\n", base_address);
	return 0;
}

static int do_mem_loop(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	ulong	addr, length, i, bytes;
	int	size;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	volatile u64 *llp;
#endif
	volatile u32 *longp;
	volatile u16 *shortp;
	volatile u8 *cp;
	const void *buf;

	if (argc < 3)
		return CMD_RET_USAGE;

	/*
	 * Check for a size specification.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	/* Address is always specified.
	*/
	addr = simple_strtoul(argv[1], NULL, 16);

	/* Length is the number of objects, not number of bytes.
	*/
	length = simple_strtoul(argv[2], NULL, 16);

	bytes = size * length;
	buf = map_sysmem(addr, bytes);

	/* We want to optimize the loops to run as fast as possible.
	 * If we have only one object, just run infinite loops.
	 */
	if (length == 1) {
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		if (size == 8) {
			llp = (u64 *)buf;
			for (;;)
				i = *llp;
		}
#endif
		if (size == 4) {
			longp = (u32 *)buf;
			for (;;)
				i = *longp;
		}
		if (size == 2) {
			shortp = (u16 *)buf;
			for (;;)
				i = *shortp;
		}
		cp = (u8 *)buf;
		for (;;)
			i = *cp;
	}

#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	if (size == 8) {
		for (;;) {
			llp = (u64 *)buf;
			i = length;
			while (i-- > 0)
				*llp++;
		}
	}
#endif
	if (size == 4) {
		for (;;) {
			longp = (u32 *)buf;
			i = length;
			while (i-- > 0)
				*longp++;
		}
	}
	if (size == 2) {
		for (;;) {
			shortp = (u16 *)buf;
			i = length;
			while (i-- > 0)
				*shortp++;
		}
	}
	for (;;) {
		cp = (u8 *)buf;
		i = length;
		while (i-- > 0)
			*cp++;
	}
	unmap_sysmem(buf);

	return 0;
}

#ifdef CONFIG_LOOPW
static int do_mem_loopw(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	ulong	addr, length, i, bytes;
	int	size;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	volatile u64 *llp;
	u64 data;
#else
	ulong	data;
#endif
	volatile u32 *longp;
	volatile u16 *shortp;
	volatile u8 *cp;
	void *buf;

	if (argc < 4)
		return CMD_RET_USAGE;

	/*
	 * Check for a size specification.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	/* Address is always specified.
	*/
	addr = simple_strtoul(argv[1], NULL, 16);

	/* Length is the number of objects, not number of bytes.
	*/
	length = simple_strtoul(argv[2], NULL, 16);

	/* data to write */
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	data = simple_strtoull(argv[3], NULL, 16);
#else
	data = simple_strtoul(argv[3], NULL, 16);
#endif

	bytes = size * length;
	buf = map_sysmem(addr, bytes);

	/* We want to optimize the loops to run as fast as possible.
	 * If we have only one object, just run infinite loops.
	 */
	if (length == 1) {
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		if (size == 8) {
			llp = (u64 *)buf;
			for (;;)
				*llp = data;
		}
#endif
		if (size == 4) {
			longp = (u32 *)buf;
			for (;;)
				*longp = data;
		}
		if (size == 2) {
			shortp = (u16 *)buf;
			for (;;)
				*shortp = data;
		}
		cp = (u8 *)buf;
		for (;;)
			*cp = data;
	}

#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	if (size == 8) {
		for (;;) {
			llp = (u64 *)buf;
			i = length;
			while (i-- > 0)
				*llp++ = data;
		}
	}
#endif
	if (size == 4) {
		for (;;) {
			longp = (u32 *)buf;
			i = length;
			while (i-- > 0)
				*longp++ = data;
		}
	}
	if (size == 2) {
		for (;;) {
			shortp = (u16 *)buf;
			i = length;
			while (i-- > 0)
				*shortp++ = data;
		}
	}
	for (;;) {
		cp = (u8 *)buf;
		i = length;
		while (i-- > 0)
			*cp++ = data;
	}
}
#endif /* CONFIG_LOOPW */

#ifdef CONFIG_CMD_MEMTEST

/*
 * Perform a memory test. A more complete alternative test can be
 * configured using CONFIG_SYS_ALT_MEMTEST. The complete test loops until
 * interrupted by ctrl-c or by a failure of one of the sub-tests.
 */
static int do_mem_mtest(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	//unsigned long long int start, end, debug_var;
	ulong start = 0, end = 0, stop = 0;
	
	ulong *buf;
	int i = 0, ret = 0;
	//ulong errs = 0;	/* number of errors, or -1 if interrupted */
	unsigned long test = 0, test_id = 0;

	buf = map_sysmem(start, end - start);
	start = (ulong)buf;
	end = start + end;


	if (argc > 1)
		if (strict_strtoul(argv[1], 16, &test) < 0)
		  {
			printf ("'Error parsing test\n");
			return CMD_RET_USAGE;
		}
	if (argc > 2)
		if (strict_strtoul(argv[2], 16, &stop) < 0)
		{
			printf ("'Error parsing stop\n");
			return CMD_RET_USAGE;
		}				
	
	if (argc > 3)
		if (strict_strtoul(argv[3], 16, &start) < 0)
		{
			printf ("Error parsing start=%08lx \n", start);
			return CMD_RET_USAGE;
		}
	if (argc > 4)
		if (strict_strtoul(argv[4], 16, &end) < 0)
		{
			printf ("Error parsing end=%08lx \n", end);
			return CMD_RET_USAGE;
		}

	if (end < start) {
		printf("Refusing to do empty test\n");
		return -1;
	}

	printf("Testing %08lx ... %08lx:\n", start, end);
	if (test == 0)
	{
		test_id = 0x03FF;
		printf("All tests\n");
	}
	else
	{
		test_id = 0x0001 << (test - 1);
	}
	
	if ((test_id & IS_MEMTEST_1) == IS_MEMTEST_1)
	{
		printf("addr_tst0: stop option = %lx, start = %08lx, end = %08lx\n", stop, start, end);
		for (i = 1; i <= MEMTEST_ITERATION;i++)
		{
			printf("addr_tst0 iter = %d\n", i);
			ret |= addr_tst1(start, end, stop);
		}
	}
	if ((test_id & IS_MEMTEST_2) == IS_MEMTEST_2)
	{
		printf("addr_tst1: stop option = %lx, start = %08lx, end = %08lx\n", stop, start, end);
		for (i = 1; i <= MEMTEST_ITERATION;i++)
		{
			printf("addr_tst1 iter = %d\n", i);
			ret |= addr_tst2(start, end, stop);
		}
	}
	if ((test_id & IS_MEMTEST_3) == IS_MEMTEST_3)
	{
		printf("movinv: stop option = %lx, start = %08lx, end = %08lx, number of iteration = %d\n", stop, start, end, MEMTEST_ITERATION);
		ret |= movinv (MEMTEST_ITERATION, start, end, stop);
	}
	if ((test_id & IS_MEMTEST_4) == IS_MEMTEST_4)
	{
		printf("movinv_8bit: stop option = %lx, start = %08lx, end = %08lx, number of iteration = %d\n", stop, start, end, MEMTEST_ITERATION);
		ret |= movinv_8bit (MEMTEST_ITERATION, start, end, stop);
	}
	if ((test_id & IS_MEMTEST_5) == IS_MEMTEST_5)
	{
		printf("movinvr: stop option = %lx, start = %08lx, end = %08lx, number of iteration = %d\n", stop, start, end, MEMTEST_ITERATION);
		ret |= movinvr (MEMTEST_ITERATION, start, end, stop);
	}
	if ((test_id & IS_MEMTEST_7) == IS_MEMTEST_7)
	{
		printf("movinv64: stop option = %lx, start = %08lx, end = %08lx\n", stop, start, end);
		for (i = 1; i <= MEMTEST_ITERATION;i++)
		{
			printf("movinv64 iter = %d\n", i);
			ret |= movinv64(start, end, stop);
		}
	}
	if ((test_id & IS_MEMTEST_8) == IS_MEMTEST_8)
	{
		printf("rand_seq: stop option = %lx, start = %08lx, end = %08lx\n", stop, start, end);
		for (i = 1; i <= MEMTEST_ITERATION;i++)
		{
			printf("rand_seq iter = %d\n", i);
			ret |= rand_seq(i, start, end, stop);
		}
	}
	return ret;
}
#endif	/* CONFIG_CMD_MEMTEST */

/* Modify memory.
 *
 * Syntax:
 *	mm{.b, .w, .l, .q} {addr}
 *	nm{.b, .w, .l, .q} {addr}
 */
static int
mod_mem(cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char * const argv[])
{
	ulong	addr;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	u64 i;
#else
	ulong i;
#endif
	int	nbytes, size;
	void *ptr = NULL;

	if (argc != 2)
		return CMD_RET_USAGE;

	bootretry_reset_cmd_timeout();	/* got a good command to get here */
	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = mm_last_addr;
	size = mm_last_size;

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/* New command specified.  Check for a size specification.
		 * Defaults to long if no or incorrect specification.
		 */
		if ((size = cmd_get_data_size(argv[0], 4)) < 0)
			return 1;

		/* Address is specified since argc > 1
		*/
		addr = simple_strtoul(argv[1], NULL, 16);
		addr += base_address;
	}

	/* Print the address, followed by value.  Then accept input for
	 * the next value.  A non-converted value exits.
	 */
	do {
		ptr = map_sysmem(addr, size);
		printf("%08lx:", addr);
		if (size == 4)
			printf(" %08x", *((u32 *)ptr));
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		else if (size == 8)
			printf(" %016" PRIx64, *((u64 *)ptr));
#endif
		else if (size == 2)
			printf(" %04x", *((u16 *)ptr));
		else
			printf(" %02x", *((u8 *)ptr));

		nbytes = cli_readline(" ? ");
		if (nbytes == 0 || (nbytes == 1 && console_buffer[0] == '-')) {
			/* <CR> pressed as only input, don't modify current
			 * location and move to next. "-" pressed will go back.
			 */
			if (incrflag)
				addr += nbytes ? -size : size;
			nbytes = 1;
			/* good enough to not time out */
			bootretry_reset_cmd_timeout();
		}
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (nbytes == -2) {
			break;	/* timed out, exit the command	*/
		}
#endif
		else {
			char *endp;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
			i = simple_strtoull(console_buffer, &endp, 16);
#else
			i = simple_strtoul(console_buffer, &endp, 16);
#endif
			nbytes = endp - console_buffer;
			if (nbytes) {
				/* good enough to not time out
				 */
				bootretry_reset_cmd_timeout();
				if (size == 4)
					*((u32 *)ptr) = i;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
				else if (size == 8)
					*((u64 *)ptr) = i;
#endif
				else if (size == 2)
					*((u16 *)ptr) = i;
				else
					*((u8 *)ptr) = i;
				if (incrflag)
					addr += size;
			}
		}
	} while (nbytes);
	if (ptr)
		unmap_sysmem(ptr);

	mm_last_addr = addr;
	mm_last_size = size;
	return 0;
}

#ifdef CONFIG_CMD_CRC32

static int do_mem_crc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int flags = 0;
	int ac;
	char * const *av;

	if (argc < 3)
		return CMD_RET_USAGE;

	av = argv + 1;
	ac = argc - 1;
#ifdef CONFIG_CRC32_VERIFY
	if (strcmp(*av, "-v") == 0) {
		flags |= HASH_FLAG_VERIFY | HASH_FLAG_ENV;
		av++;
		ac--;
	}
#endif

	return hash_command("crc32", flags, cmdtp, flag, ac, av);
}

#endif

/**************************************************/
U_BOOT_CMD(
	md,	3,	1,	do_mem_md,
	"memory display",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address [# of objects]"
#else
	"[.b, .w, .l] address [# of objects]"
#endif
);


U_BOOT_CMD(
	mm,	2,	1,	do_mem_mm,
	"memory modify (auto-incrementing address)",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address"
#else
	"[.b, .w, .l] address"
#endif
);


U_BOOT_CMD(
	nm,	2,	1,	do_mem_nm,
	"memory modify (constant address)",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address"
#else
	"[.b, .w, .l] address"
#endif
);

U_BOOT_CMD(
	mw,	4,	1,	do_mem_mw,
	"memory write (fill)",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address value [count]"
#else
	"[.b, .w, .l] address value [count]"
#endif
);

U_BOOT_CMD(
	cp,	4,	1,	do_mem_cp,
	"memory copy",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] source target count"
#else
	"[.b, .w, .l] source target count"
#endif
);

U_BOOT_CMD(
	cmp,	4,	1,	do_mem_cmp,
	"memory compare",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] addr1 addr2 count"
#else
	"[.b, .w, .l] addr1 addr2 count"
#endif
);

#ifdef CONFIG_CMD_CRC32

#ifndef CONFIG_CRC32_VERIFY

U_BOOT_CMD(
	crc32,	4,	1,	do_mem_crc,
	"checksum calculation",
	"address count [addr]\n    - compute CRC32 checksum [save at addr]"
);

#else	/* CONFIG_CRC32_VERIFY */

U_BOOT_CMD(
	crc32,	5,	1,	do_mem_crc,
	"checksum calculation",
	"address count [addr]\n    - compute CRC32 checksum [save at addr]\n"
	"-v address count crc\n    - verify crc of memory area"
);

#endif	/* CONFIG_CRC32_VERIFY */

#endif

#ifdef CONFIG_CMD_MEMINFO
__weak void board_show_dram(phys_size_t size)
{
	puts("DRAM:  ");
	print_size(size, "\n");
}

static int do_mem_info(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	board_show_dram(gd->ram_size);

	return 0;
}
#endif

U_BOOT_CMD(
	base,	2,	1,	do_mem_base,
	"print or set address offset",
	"\n    - print address offset for memory commands\n"
	"base off\n    - set address offset for memory commands to 'off'"
);

U_BOOT_CMD(
	loop,	3,	1,	do_mem_loop,
	"infinite loop on address range",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address number_of_objects"
#else
	"[.b, .w, .l] address number_of_objects"
#endif
);

#ifdef CONFIG_LOOPW
U_BOOT_CMD(
	loopw,	4,	1,	do_mem_loopw,
	"infinite write loop on address range",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address number_of_objects data_to_write"
#else
	"[.b, .w, .l] address number_of_objects data_to_write"
#endif
);
#endif /* CONFIG_LOOPW */

#ifdef CONFIG_CMD_MEMTEST
U_BOOT_CMD(
	mtest,	5,	1,	do_mem_mtest,
	"TEST RAM",
	"[test [stop [start [end]]]"
);
#endif	/* CONFIG_CMD_MEMTEST */

#ifdef CONFIG_MX_CYCLIC
U_BOOT_CMD(
	mdc,	4,	1,	do_mem_mdc,
	"memory display cyclic",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address count delay(ms)"
#else
	"[.b, .w, .l] address count delay(ms)"
#endif
);

U_BOOT_CMD(
	mwc,	4,	1,	do_mem_mwc,
	"memory write cyclic",
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	"[.b, .w, .l, .q] address value delay(ms)"
#else
	"[.b, .w, .l] address value delay(ms)"
#endif
);
#endif /* CONFIG_MX_CYCLIC */

#ifdef CONFIG_CMD_MEMINFO
U_BOOT_CMD(
	meminfo,	3,	1,	do_mem_info,
	"display memory information",
	""
);
#endif
