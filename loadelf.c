/*
 * ELF loader module
 *
 * (c) 4/2006 Martin Strubel <hackfin@section5.ch>
 *
 * $Id: loadelf.c,v 1.3 2006/05/08 13:53:34 strubi Exp $
 *
 */

#include <libelf/libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bfemu.h"
#include "memcache.h"

typedef struct _elfdesc {
	Elf        *elf;
	int         filedesc;
	Elf_Data   *data;
	Elf32_Sym  *symtab;
	size_t      symtab_sz;
	char        *strtab;
} ElfDesc;


void load_data(CPU cpu, unsigned long addr, Elf_Data *data)
{
	set_memory_queued(cpu, addr, data->d_size, data->d_buf);
}

int check_elf_version(void)
{
	if (elf_version(EV_CURRENT) == EV_NONE) {
		return 0;
	} else {
		return 1;
	}
}

int lookup_symbol(ElfDesc *ed, const char *sym, ADDR *addr)
{
	char *name;
	Elf32_Sym *symtab = ed->symtab;
	size_t symtab_sz = ed->symtab_sz;
	char *strtab = ed->strtab;

	int i;
	if (symtab && strtab) {
		for (i = 0; i < symtab_sz; i++) {
			name = &strtab[symtab[i].st_name];
			if (strcmp(name, sym) == 0) {
				*addr = symtab[i].st_value;
				return 1;
			}
		}
	}
	return 0; // not found
}

#ifdef DEBUG
void decode_shflags(int flag, char *fstr)
{
	memset(fstr, '_', 5);
	if (flag & SHF_WRITE)     fstr[0] = 'W';
	if (flag & SHF_ALLOC)     fstr[1] = 'A';
	if (flag & SHF_EXECINSTR) fstr[2] = 'E';
	if (flag & SHF_MERGE)     fstr[3] = 'M';
	if (flag & SHF_STRINGS)   fstr[4] = 'S';
	fstr[5] = '\0';
}
#endif

int load_elf(int fdes, CPU cpu, ElfDesc **desc)
{
	int ndx;
	Elf32_Ehdr *eh;
	Elf32_Shdr *sh;
	Elf_Scn    *section;
	Elf_Data   *data;
	char *name;
#ifdef DEBUG
	char flagstring[8];
#endif

	ElfDesc *ed;

	ed = (ElfDesc *) malloc(sizeof(ElfDesc));
	if (ed == NULL) return 0;
	*desc = ed;

	ed->filedesc = fdes;
	ed->elf = elf_begin(fdes, ELF_C_READ, NULL);
	eh = elf32_getehdr(ed->elf);

	// Make sure it's an elf exe and check for blackfin arch
	if (elf_kind(ed->elf) != ELF_K_ELF || !eh || eh->e_machine != 0x6a	) {
		printf("Not a Blackfin ELF executable\n");
		return 0;
	}

	ndx = eh->e_shstrndx; // String offset
	section = NULL;
	section = elf_nextscn(ed->elf, section);
	while (section) {
		sh = elf32_getshdr(section);
		name = elf_strptr(ed->elf, ndx, (size_t) sh->sh_name);


		data = elf_getdata(section, NULL);

		if (strcmp(name, ".symtab") == 0) {
			ed->symtab = (Elf32_Sym *) data->d_buf;
			ed->symtab_sz = data->d_size / sizeof (Elf32_Sym);
		} else
		if (strcmp(name, ".strtab") == 0) {
			ed->strtab = (char *) data->d_buf;
		}

		while (data) {
			if (sh->sh_flags & (SHF_WRITE | SHF_EXECINSTR | SHF_ALLOC)
				&& sh->sh_size > 0 && data->d_buf) {
				load_data(cpu, sh->sh_addr, data);

#ifdef DEBUG
				decode_shflags(sh->sh_flags, flagstring);
				printf("T: %x F: %s Name: %16s  Addr: %08x  Off: %08x Sz: %x\n",
					sh->sh_type, 
					flagstring, 
					name,
					sh->sh_addr, sh->sh_offset,
					sh->sh_size);
#endif
			}

			data = elf_rawdata(section, data);
		}
		section = elf_nextscn(ed->elf, section);
	}

	// Flush possibly not yet written data
	memcache_flush(cpu);

	return 1;
}

void free_elf(ElfDesc *ed)
{
	elf_end(ed->elf);
	free(ed);
}


