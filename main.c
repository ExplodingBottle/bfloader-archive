/*
 * Standalone version of bfloader
 *
 * (c) 4/2006 Martin Strubel <hackfin@section5.ch>
 *
 * $Id: main.c,v 1.13 2006/05/17 17:37:12 strubi Exp $
 *
 */


#include <stdio.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include "bfemu.h"
#include "memcache.h"
#include "bfin-registers.h"
#include "bfloader.h"

#define _GNU_SOURCE
#include <getopt.h>

static
unsigned const char s_op_brk[2] = { 0x25, 0x00 };

static const char s_line[] =
"------------------------------------------------------------\n";

static const char
s_version[] = "\nbfloader/flashloader Version: "
			VERSION_STRING
			"    (c) 2006 <hackfin@section5.ch>"
			"\n\n";

static const char s_err_action[] = 
"You can not specify several flash file actions at once!\n";

#define DEFAULT_LOADER "bfloader.dxe"

////////////////////////////////////////////////////////////////////////////
// GLOBALS
//

struct manufacturer_codes {
	int code;
	const char *desc;
} s_manufacturers[] = {
	{ 0x01, "AMD/Spansion" },
	{ 0x20, "ST Microelectronics" },
	{ 0x89, "Intel" },
	{ 0x00, NULL }
};

// Global addresses of symbols

ADDR afp_brk_ready;
ADDR afp_command;
ADDR afp_buffer;
ADDR afp_buffersize;
ADDR afp_offset;
ADDR afp_count;
ADDR afp_error;

// Global options

int o_eraseall = 0;
int o_speed = 1;
int o_info = 0;
int o_noverify = 0;
int o_secstart = 0, o_secn = 0;
int o_offset = 0;
int o_size = 0;

////////////////////////////////////////////////////////////////////////////
// PROTOS
//

struct _elfdesc;

#define ELFDESC struct _elfdesc *

// error.c:
void handle_error(int error);

// loadelf.c:
int check_elf_version(void);
int load_elf(int fdes, CPU cpu, ELFDESC *desc);
int lookup_symbol(ELFDESC ed, const char *sym, ADDR *addr);
int free_elf(ELFDESC ed);

int init_cpu(JTAG_CTRL *controller, CPU *cpu)
{
	int ncpus;
	int error;
	int i;
	unsigned int revision;

	JTAG_CTRL jtag;

	error = jtag_open(0, &jtag);
	if (error < 0) {
		printf("Could not open ICEbear. Is it connected?\n"
		       "Try replugging and make sure no gdbproxy or "
			   "ftio_sio module is claiming it.\n");
		return -1;
	}

	*controller = jtag;

	ncpus = jtag_detect(jtag);
	if (ncpus < 0 || ncpus > 2) {
		printf("Could not detect scan chain on target\n"
		       "Is target powered and connected to ICEbear?\n");
		jtag_close(jtag);
		return -1;
	}
	
	if (ncpus == 2) {
		printf("Dual Core CPU found\n");
	}

	printf("Number of devices in JTAG chain: %d\n", ncpus);

	if (jtag_init(jtag, cpu) < 0) {
		printf("Could not initialize CPU on JTAG\n");
		return -1;
	}

	printf("JTAG wait cycles: %d\n", o_speed);
	jtag_config(jtag, JTAG_CLKSPEED, o_speed);

	for (i = 0; i < ncpus; i++) {
		error = emulation_init(cpu[i]);
		if (error < 0) {
			printf("Could not init emulation - check board connection.\n");
			return -1;
		}

		const char *s;
		printf("Detecting device...\n");
		s = detect_device(cpu[i], &revision);
		if (s) {
			printf("Detected Device: %s rev:%d\n", s, revision);
		} else {
			printf("Could not detect device, JTAG failure ?\n");
			return -1;
		}
	}

	return 0;
}

void exit_cpu(JTAG_CTRL jtag, CPU *cpu)
{
	jtag_exit(jtag, cpu);
	jtag_close(jtag);
}

int init_loader(ELFDESC *ed, CPU cpu, const char *fname)
{
	int fdes;

	if (!check_elf_version()) {
		printf("Wrong ELF version\n");
	}

	fdes = open(fname, O_RDONLY);
	if (fdes < 0) {
		printf("Could not open file %s\n", fname);
		return fdes;
	}
	if (load_elf(fdes, cpu, ed) == 0) {
		printf("Error loading ELF file\n");
		return -1;
	}
	
	return fdes;
}

void exit_loader(int fdes, ELFDESC ed)
{
	close(fdes);
	free_elf(ed);
}

void set_break(CPU cpu, ADDR addr, unsigned char *opcode)
{
	get_memory_cached(cpu, addr, 2, opcode);
	set_memory_queued(cpu, addr, 2, s_op_brk);
}

void restore_break(CPU cpu, ADDR addr, unsigned char *opcode)
{
	set_memory_queued(cpu, addr, 2, opcode);
}

void handle_emuexcpt(CPU cpu, int cause)
{
	BFIN_REGISTER pc;

	if (cause == EMUCAUSE_EMUEXCPT) {

		get_cpuregister(cpu, REG_RETE, &pc);
		pc -= 2;
		set_cpuregister(cpu, REG_RETE, pc);

	} else {
		printf("Not a breakpoint exception\n");
	}
}

 
int wait(CPU cpu)
{
	int state, cause;

	do {
		state = emulation_state(cpu, &cause);
	} while (state == CPUSTATE_RUNNING);

	if (state == CPUSTATE_EMULATION) {
		handle_emuexcpt(cpu, cause);
	} else
	if (state == CPUSTATE_RUNNING) {
		emulation_enter(cpu);
	}

	return 0;
}

int set_int(CPU cpu, ADDR addr, int i)
{
	unsigned char buf[4];
	buf[0] = i; buf[1] = i >> 8; buf[2] = i >> 16; buf[3] = i >> 24;
	return set_memory_queued(cpu, addr, 4, buf);
}

int get_short(CPU cpu, ADDR addr, short *integer)
{
	unsigned char buf[4];
	int i;
	int error;

	error = get_memory_cached(cpu, addr, 2, buf);
	i = buf[1];
	i = i << 8 | buf[0];
	*integer = i;
	return error;
}

int get_int(CPU cpu, ADDR addr, int *integer)
{
	unsigned char buf[4];
	int i;
	int error;

	error = get_memory_cached(cpu, addr, 4, buf);
	i = buf[3];
	i = i << 8 | buf[2];
	i = i << 8 | buf[1];
	i = i << 8 | buf[0];
	*integer = i;
	return error;
}

int exec_command(CPU cpu, int cmd)
{
	unsigned char opcode[4];
	int error;
	
	set_int(cpu, afp_command, cmd);
	set_break(cpu, afp_brk_ready, opcode);
	memcache_flush(cpu);
	emulation_leave(cpu);
	wait(cpu);
	get_int(cpu, afp_error, &error);
	restore_break(cpu, afp_brk_ready, opcode);
	memcache_flush(cpu);
	emulation_enter_singlestep(cpu);
	emulation_go(cpu);
	emulation_leave_singlestep(cpu);

	return error;
}

////////////////////////////////////////////////////////////////////////////
// Memory compare auxiliaries

void memory_dump(unsigned char *buf, unsigned long n)
{
	int i = 0;
	int c = 0;

	while (i < n) {
		printf("%02x ", buf[i]);
		c++;
		if (c == 16) { c = 0; printf("\n"); }
		i++;
	}
	if (c)
		printf("\n");
}


int mem_cmp(unsigned char *in, unsigned char *out, unsigned long size)
{
	int i;
	for (i = 0; i < size; i++) {
		if (in[i] != out[i]) {
			printf("Mismatch at byte %d\n", i);
			printf("Should be:\n");
			memory_dump(&in[i], 16);
			printf("Is:\n");
			memory_dump(&out[i], 16);
			return 1;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// Flash auxiliaries


const char *lookup_manufacturer(int code)
{
	struct manufacturer_codes *s;
	s = s_manufacturers;

	while (s->code) {
		if (code == s->code) return s->desc;
		s++;
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////
// Main flash actions


int write_flashimage(CPU cpu, int offset, int n, const char *filename)
{
	int error;
	FILE *file;
	struct stat fs;
	unsigned char *buf;
	int bufsize;
	int size;
	int off;
	ADDR target_buf;

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open file %s\n", filename);
		return -1;
	}

	if (fstat(fileno(file), &fs) < 0) {
		perror("fstat");
		return -1;
	}

	if (n == 0 || n > fs.st_size)
		n = fs.st_size;

	if (n == 0) {
		fprintf(stderr, "File has size 0");
		return -1;
	}

	get_int(cpu, afp_buffersize, &bufsize);

	buf = (unsigned char *) malloc(bufsize);
	if (buf == NULL) {
		fprintf(stderr, "malloc(%d) failed\n", bufsize);
		return -1;
	}

	off = offset;
	n += offset;

	get_int(cpu, afp_buffer, (int *) &target_buf);

	while (off < n) {
		size = fread(buf, 1, bufsize, file);
		printf("Writing %d bytes to flash at offset $%08x\n", size, off);
		set_memory_queued(cpu, target_buf, size, buf);
		set_int(cpu, afp_offset, off);
		set_int(cpu, afp_count, size);
		memcache_flush(cpu);
		error = exec_command(cpu, WRITE);
		if (error < 0) return error;
		off += size;
	}

	fclose(file);
	free(buf);

	return error;
}

int read_flashimage(CPU cpu, int offset, int n, const char *filename)
{
	int error;
	FILE *file;
	unsigned char *buf;
	int bufsize;
	int size, end;
	int off;
	ADDR target_buf;

	file = fopen(filename, "wb");
	if (!file) {
		perror("Opening flash file for writing");
		return -1;
	}

	get_int(cpu, afp_buffersize, &bufsize);

	buf = (unsigned char *) malloc(bufsize);
	if (buf == NULL) {
		fprintf(stderr, "malloc(%d) failed\n", bufsize);
		return -1;
	}

	off = offset;
	end = n + offset;

	get_int(cpu, afp_buffer, (int *) &target_buf);

	while (off < end) {
		if (n > bufsize) size = bufsize;
		else             size = n;

		printf("Reading %d bytes from flash at offset $%08x\n", size, off);

		set_int(cpu, afp_offset, off);
		set_int(cpu, afp_count, size);
		error = exec_command(cpu, READ);
		if (error < 0) return error;
		get_memory_cached(cpu, target_buf, size, buf);
		error = fwrite(buf, 1, size, file);
		if (error != size) {
			printf("Write to file failed!\n");
			error = -1; break;
		}
		off += size;
		n -= size;
	}

	fclose (file);
	free(buf);
	return error;
}

int compare_flashimage(CPU cpu, int offset, int n, const char *filename)
{
	int error;
	FILE *file;
	struct stat fs;
	unsigned char *buf, *data;
	int bufsize;
	int size, end;
	int off;
	ADDR target_buf;


	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open file %s\n", filename);
		return -1;
	}

	if (fstat(fileno(file), &fs) < 0) {
		perror("fstat");
		return -1;
	}

	if (n == 0 || n > fs.st_size)
		n = fs.st_size;

	if (n == 0) {
		fprintf(stderr, "File has size 0");
		return -1;
	}

	get_int(cpu, afp_buffersize, &bufsize);

	buf = (unsigned char *) malloc(2 * bufsize);
	data = &buf[bufsize];

	if (buf == NULL) {
		fprintf(stderr, "malloc(%d) failed\n", bufsize);
		return -1;
	}

	off = offset;
	end = n + offset;

	get_int(cpu, afp_buffer, (int *) &target_buf);

	while (off < end) {
		if (n > bufsize) size = bufsize;
		else             size = n;

		error = fread(data, 1, size, file);
		if (error != size) {
			printf("Read from file failed!\n");
			error = -1; break;
		}

		set_int(cpu, afp_offset, off);
		set_int(cpu, afp_count, size);
		error = exec_command(cpu, READ);
		if (error < 0) return error;
		get_memory_cached(cpu, target_buf, size, buf);
		printf("Comparing %d bytes from flash at offset $%08x\n", size, off);
		if (mem_cmp(data, buf, size)) break;
		off += size;
		n -= size;
	}

	fclose(file);
	free(buf);
	return error;
}
////////////////////////////////////////////////////////////////////////////

enum {
	OPT_SPEED = 1,
	OPT_PROGRAM,
	OPT_DUMP,
	OPT_COMPARE,
	OPT_ERASESECTORS,
	OPT_OFFSET,
	OPT_SIZE,
	OPT_VERSION,
	OPT_BACKEND
};

enum {
	MODE_NOACTION,
	MODE_PROGRAM,
	MODE_DUMP,
	MODE_COMPARE
};

typedef enum {
	T_INT16,
	T_INT32,
	T_STRING
} SymType;

struct syminfo {
	const char *desc;
	const char *name;
	SymType     type;
} s_symbols[] = {
	{ "Driver title",      "_AFP_Title",       T_STRING },
	{ "Description ",      "_AFP_Description", T_STRING },
	{ "Manufacturer Code", "_AFP_ManCode",     T_INT32 },
	{ "Device Code",       "_AFP_DevCode",     T_INT32 },
	{ "Number of sectors", "_AFP_NumSectors",  T_INT32 },
	{ "Sector size",       "_AFP_SectorSize1", T_INT32 },
	{ "Bus width",         "_AFP_FlashWidth",  T_INT32 },
	{ "Buffer size",       "_AFP_Size",        T_INT32 },
	{ NULL, NULL, 0 }
};

int show_value(ELFDESC ed, CPU cpu, const char *sym, SymType type)
{
	ADDR a;
	int vi;
	short vs;

	static unsigned char buf[128];

	if (!lookup_symbol(ed, sym, &a)) {
		printf("Warning: Could not find symbol '%s'\n", sym);
		return 0;
	}

	switch (type) {
		case T_STRING:
			get_int(cpu, a, &vi);
			a = (ADDR) vi;
			get_memory_cached(cpu, a, 127, buf);
			buf[127] = 0;
			printf("%s", buf);
			break;
		case T_INT16:
			get_short(cpu, a, &vs);
			printf("0x%x", vi);
			break;
		case T_INT32:
			get_int(cpu, a, &vi);
			printf("0x%x", vi);
			break;
	}

	return 0;
}



int dump_info(ELFDESC ed, CPU cpu)
{
	int error;
	struct syminfo *s = s_symbols;
	const char *info;
	ADDR a;
	int code;

	if (!lookup_symbol(ed, "_AFP_ManCode", &a)) {
		printf("Fatal error: AFP_ManCode not defined on backend\n");
		return 0;
	}
	printf(s_line);
	get_int(cpu, a, &code);
	info = lookup_manufacturer(code);
	if (info) printf("Manufacturer: %s\n", info);

	printf(s_line);
	while (s->desc) {
		printf("%20s: ", s->desc);
		error = show_value(ed, cpu, s->name, s->type);
		if (error < 0) return error;
		s++;
		printf("\n");
	}
	printf(s_line);
	return error;
}


int check_sectors_valid(int a, int b)
{
	if (b != 0) return 1;
	return 0;
}

int erase_sectors(ELFDESC ed, CPU cpu, int start, int n)
{
	int error;
	ADDR sector;
	int end = start + n;

	if (!lookup_symbol(ed, "_AFP_Sector", &sector)) {
		return -1;
	}

	while (start < end) {
		set_int(cpu, sector, start);
		error = exec_command(cpu, ERASE_SECT);
		printf("Erased sector %d\n", start);
		if (error < 0) return error;
		start++;
	}
	return error;
}


struct symaddr {
	const char *name;
	ADDR *p;
} g_symaddrs[] = {
	{ "AFP_BreakReady", &afp_brk_ready },
	{ "_AFP_Command",   &afp_command },
	{ "_AFP_Offset",    &afp_offset },
	{ "_AFP_Count",     &afp_count },
	{ "_AFP_Size",      &afp_buffersize },
	{ "_AFP_Buffer",    &afp_buffer },
	{ "_AFP_Error",     &afp_error },
	{ NULL, 0 }
};

int init_symaddrs(ELFDESC ed, struct symaddr *s)
{
	while (s->name) {
		if (!lookup_symbol(ed, s->name, s->p)) {
			printf("Symbol '%s' not found\n", s->name);
		}
		s++;
	}
	return 0;
}


int main_action(CPU cpu, const char *flashloader,
		int mode, const char *flashfile)
{
	int error;
	ELFDESC ed;
	int fdes;
	ADDR start;
	ADDR symaddr;
	unsigned char opcode[4];

	fdes = init_loader(&ed, cpu, flashloader);
	if (fdes < 0) {
		printf("Error initializing Flash Loader\n");
		return -1;
	}

	init_symaddrs(ed, g_symaddrs);

	if (!lookup_symbol(ed, "start", &start)) {
		printf("Could not find start entry!\n");
		return -1;
	}

	// Set PC to start from:
	set_cpuregister(cpu, REG_RETE, start);
	set_break(cpu, afp_brk_ready, opcode);
	memcache_flush(cpu);

	// Now let the program run
	emulation_leave(cpu);

	wait(cpu);
	restore_break(cpu, afp_brk_ready, opcode);
	memcache_flush(cpu);
	emulation_enter_singlestep(cpu);
	emulation_go(cpu);
	emulation_leave_singlestep(cpu);

	// Now here is the command sequence

	if (o_info) {
		dump_info(ed, cpu);
	}
	
	// Now check, whether there is an initialization error
	get_int(cpu, afp_error, &error);
	if (error < 0) {
		handle_error(error);
		fprintf(stderr, "Possibly you are using the wrong loader or the chip"
				" is not recognized.\n");
		return -1;
	}

	if (o_noverify) {
		if (lookup_symbol(ed, "_AFP_Verify", &symaddr)) {
			set_int(cpu, symaddr, 0);
			printf("Verify turned off.\n");
		} else {
			printf("Error: AFP_Verify not existing!\n");
		}
	} else {
		printf("Verify is on (default).\n");
	}

	if (o_eraseall) {
		printf(s_line);
		printf("Erasing entire Flash chip, please wait. This can take\n"
		       "minutes to complete. Do not interrupt.\n");
		error = exec_command(cpu, ERASE_ALL);
		if (error < 0) handle_error(error);
		else           printf("Done.\n");
		printf(s_line);
	} else if (check_sectors_valid(o_secstart, o_secn) && !o_eraseall) {
		printf("Erasing %d sectors starting from sector %d\n",
				o_secn, o_secstart);
		erase_sectors(ed, cpu, o_secstart, o_secn);
	}

	switch (mode) {
		case MODE_PROGRAM:
			printf(s_line);
			printf("Writing Flash image from file '%s'\n", flashfile);
			error = write_flashimage(cpu, o_offset, o_size, flashfile);
			if (error < 0) handle_error(error);
			else           printf("Done.\n");
			printf(s_line);
			break;
		case MODE_DUMP:
			if (o_size == 0) {
				fprintf(stderr, "Size not specified\n");
				break;
			}
			printf(s_line);
			printf("Reading %d bytes from Flash at $%08x to file '%s'\n",
					o_size, o_offset, flashfile);
			error = read_flashimage(cpu, o_offset, o_size, flashfile);
			if (error < 0) handle_error(error);
			printf("Done.\n");
			printf(s_line);
			break;
		case MODE_COMPARE:
			printf(s_line);
			printf("Comparing Flash with image in file  '%s'\n", flashfile);
			error = compare_flashimage(cpu, o_offset, o_size, flashfile);
			if (error < 0) handle_error(error);
			printf("Done.\n");
			printf(s_line);
			break;
	}

	error = exec_command(cpu, RESET);
	if (error < 0) handle_error(error);

	exit_loader(fdes, ed);
	return 0;
}


int main(int argc, char **argv)
{
	JTAG_CTRL jtag;
	CPU cpus[2];
	CPU cpu;
	int error;
	char *flashloader = DEFAULT_LOADER;

	char *flashfile = NULL;
	int mode = MODE_NOACTION;

	static struct option long_options[] = {
		{ "speed",        1, 0,           OPT_SPEED },
		{ "program",      1, 0,           OPT_PROGRAM },
		{ "dump",         1, 0,           OPT_DUMP },
		{ "compare",      1, 0,           OPT_COMPARE },
		{ "offset",       1, 0,           OPT_OFFSET },
		{ "size",         1, 0,           OPT_SIZE },
		{ "erasesectors", 1, 0,           OPT_ERASESECTORS },
		{ "eraseall",     0, &o_eraseall, 1 },
		{ "noverify",     0, &o_noverify, 1 },
		{ "info",         0, &o_info,     1 },
		{ "backend",      1, 0,           OPT_BACKEND },
		{ "version",      0, 0,           OPT_VERSION },
		{ NULL,  0, 0, 0}
	};

	if (argc <= optind && optind > 0) {
		printf("\nUsage: %s [options]\n\n"
		"  --info                 Show Flash info\n"
		"  --version              Print version information\n"
		"\n"
		"Flash modes:\n"
		"These modes are exclusive:"
			" [program | dump | compare]\n"
		"  --program=file         Program flash with binary image data\n"
		"  --dump=file            Dump flash to file (specify offset/size)\n"
		"  --compare=file         Compare flash with binary image in file\n"
		"  --eraseall             Erase entire Flash before programming\n"
		"  --erasesectors=off,n   Erase <n> sectors from sector <off>\n"
		"\n"
		"Options:\n"
		"  --offset=<hexaddr>     Set write/read offset to <hexaddr>\n"
		"                         Must have format 0xNNNN.\n"
		"  --size=<hexsize>       Read/Write only up to specified size.\n"
		"                         Must have format 0xNNNN.\n"
		"  --noverify             Turn verify off (faster)\n"
		"  --backend=<bfloader>   Use other than default loader backend\n"
		"  --speed=<value>        JTAG speed setting (0: fastest)\n"
		"\n", argv[0]);
		return -1;
	}

	// Process options:
	
	while (1) {
		int c;
		int option_index;

		c = getopt_long(argc, argv, "+", long_options, &option_index);
		if (c == EOF) break;

		switch (c) {
			case 0:
				break;
			case OPT_SPEED:
				o_speed = atoi(optarg);
				if (o_speed < 0 || o_speed > 255) {
					fprintf(stderr, "Usage: --speed=[0-255]\n");
					return -1;
				}
				break;
			case OPT_ERASESECTORS:
				// Parse start, end
				if (sscanf(optarg, "%d,%d", &o_secstart, &o_secn) != 2) {
					fprintf(stderr, "Usage: --erasesectors=<offset>,<n>\n");
					return -1;
				}
				break;
			case OPT_PROGRAM:
				flashfile = optarg;
				if (mode != MODE_NOACTION) {
					fprintf(stderr, s_err_action);
					return -1;
				}
				mode = MODE_PROGRAM;
				break;
			case OPT_DUMP:
				flashfile = optarg;
				if (mode != MODE_NOACTION) {
					fprintf(stderr, s_err_action);
					return -1;
				}
				mode = MODE_DUMP;
				break;
			case OPT_COMPARE:
				flashfile = optarg;
				if (mode != MODE_NOACTION) {
					fprintf(stderr, s_err_action);
					return -1;
				}
				mode = MODE_COMPARE;
				break;
			case OPT_OFFSET:
				if (sscanf(optarg, "0x%x", &o_offset) != 1) {
					fprintf(stderr, "Usage: --offset=0x<hex number>\n");
					return -1;
				}
				break;
			case OPT_SIZE:
				if (sscanf(optarg, "0x%x", &o_size) != 1) {
					fprintf(stderr, "Usage: --size=0x<hex number>\n");
					return -1;
				}
				break;
			case OPT_BACKEND:
				flashloader = optarg;
				break;
			case OPT_VERSION:
				printf(s_version);
				break;
			default:
				fprintf(stderr, "\n"
	"Run %s with no arguments to get a list of options.\n\n",
	argv[0]);
				return -1;

		}
	}

	if (mode == MODE_NOACTION && o_eraseall == 0 && o_secn == 0 &&
			o_info == 0) {
		return 0;
	}


	if (init_cpu(&jtag, cpus) < 0) return -1;

	cpu = cpus[0];


	error = emulation_enter(cpu);
	if (error < 0) {
		printf("Emulator not ready!\n");
		return -1;
	}

	cpu_reset(cpu, 3);

	error = main_action(cpu, flashloader, mode, flashfile);

	cpu_reset(cpu, 3);
	emulation_leave(cpu);

	exit_cpu(jtag, cpus);
	return error;
}
