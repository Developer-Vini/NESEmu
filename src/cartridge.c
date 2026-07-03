#include "cartridge.h"
#include <errno.h>

#define PRG_BANK_SIZE 0x4000
#define CHR_BANK_SIZE 0x2000

static int prg_bank_count_8k(const Cartridge *c) {
    return c->prg_banks * 2;
}

static int chr_bank_count_1k(const Cartridge *c) {
    return c->has_chr_ram ? 8 : c->chr_banks * 8;
}

static int wrap_bank(int bank, int count) {
    if (count <= 0) return 0;
    bank %= count;
    if (bank < 0) bank += count;
    return bank;
}

static void mapper0_sync(Cartridge *c) {
    int banks = prg_bank_count_8k(c);
    for (int i = 0; i < 4; i++) c->prg_bank[i] = wrap_bank(i, banks);
    if (c->prg_banks == 1) {
        c->prg_bank[2] = 0;
        c->prg_bank[3] = 1;
    }
    for (int i = 0; i < 8; i++) c->chr_bank[i] = i;
}

static void mapper2_sync(Cartridge *c) {
    int banks = prg_bank_count_8k(c);
    int sel = (c->regs[0] & 0x0F) * 2;
    c->prg_bank[0] = wrap_bank(sel, banks);
    c->prg_bank[1] = wrap_bank(sel + 1, banks);
    c->prg_bank[2] = wrap_bank(banks - 2, banks);
    c->prg_bank[3] = wrap_bank(banks - 1, banks);
    for (int i = 0; i < 8; i++) c->chr_bank[i] = i;
}

static void mapper3_sync(Cartridge *c) {
    mapper0_sync(c);
    int bank = (c->regs[0] & 0x03) * 8;
    for (int i = 0; i < 8; i++) c->chr_bank[i] = bank + i;
}

static void mapper1_sync(Cartridge *c) {
    int banks = prg_bank_count_8k(c);
    int chr_count = chr_bank_count_1k(c);
    u8 control = c->regs[0] ? c->regs[0] : 0x0C;
    int prg_mode = (control >> 2) & 3;
    int chr_mode = (control >> 4) & 1;
    int prg = c->regs[3] & 0x0F;

    switch (control & 3) {
    case 0: c->mirror = 3; break; /* uma tela inferior */
    case 1: c->mirror = 4; break; /* uma tela superior */
    case 2: c->mirror = 1; break; /* vertical */
    case 3: c->mirror = 0; break; /* horizontal */
    }

    if (prg_mode <= 1) {
        int bank = (prg & 0x0E) * 2;
        for (int i = 0; i < 4; i++) c->prg_bank[i] = wrap_bank(bank + i, banks);
    } else if (prg_mode == 2) {
        c->prg_bank[0] = 0;
        c->prg_bank[1] = 1;
        c->prg_bank[2] = wrap_bank(prg * 2, banks);
        c->prg_bank[3] = wrap_bank(prg * 2 + 1, banks);
    } else {
        c->prg_bank[0] = wrap_bank(prg * 2, banks);
        c->prg_bank[1] = wrap_bank(prg * 2 + 1, banks);
        c->prg_bank[2] = wrap_bank(banks - 2, banks);
        c->prg_bank[3] = wrap_bank(banks - 1, banks);
    }

    if (chr_mode == 0) {
        int bank = (c->regs[1] & 0x1E) * 4;
        for (int i = 0; i < 8; i++) c->chr_bank[i] = wrap_bank(bank + i, chr_count);
    } else {
        int bank0 = c->regs[1] * 4;
        int bank1 = c->regs[2] * 4;
        for (int i = 0; i < 4; i++) c->chr_bank[i] = wrap_bank(bank0 + i, chr_count);
        for (int i = 0; i < 4; i++) c->chr_bank[4 + i] = wrap_bank(bank1 + i, chr_count);
    }
}

static void mapper4_sync(Cartridge *c) {
    int banks = prg_bank_count_8k(c);
    int chr_count = chr_bank_count_1k(c);
    bool prg_mode = (c->regs[0] & 0x40) != 0;
    bool chr_mode = (c->regs[0] & 0x80) != 0;
    int r[8];
    for (int i = 0; i < 8; i++) r[i] = c->regs[i] & 0xFF;

    if (!prg_mode) {
        c->prg_bank[0] = wrap_bank(r[6], banks);
        c->prg_bank[1] = wrap_bank(r[7], banks);
        c->prg_bank[2] = wrap_bank(banks - 2, banks);
        c->prg_bank[3] = wrap_bank(banks - 1, banks);
    } else {
        c->prg_bank[0] = wrap_bank(banks - 2, banks);
        c->prg_bank[1] = wrap_bank(r[7], banks);
        c->prg_bank[2] = wrap_bank(r[6], banks);
        c->prg_bank[3] = wrap_bank(banks - 1, banks);
    }

    int chr[8] = {
        r[0] & 0xFE, (r[0] & 0xFE) + 1,
        r[1] & 0xFE, (r[1] & 0xFE) + 1,
        r[2], r[3], r[4], r[5]
    };
    if (!chr_mode) {
        for (int i = 0; i < 8; i++) c->chr_bank[i] = wrap_bank(chr[i], chr_count);
    } else {
        for (int i = 0; i < 4; i++) c->chr_bank[i] = wrap_bank(chr[4 + i], chr_count);
        for (int i = 0; i < 4; i++) c->chr_bank[4 + i] = wrap_bank(chr[i], chr_count);
    }
}

static void sync_mapper(Cartridge *c) {
    switch (c->mapper) {
    case 0: mapper0_sync(c); break;
    case 1: mapper1_sync(c); break;
    case 2: mapper2_sync(c); break;
    case 3: mapper3_sync(c); break;
    case 4: mapper4_sync(c); break;
    default: mapper0_sync(c); break;
    }
}

Cartridge *cart_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open ROM '%s': %s\n", path, strerror(errno));
        return NULL;
    }

    iNESHeader h;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) ||
        memcmp(h.magic, "NES\x1A", 4) != 0) {
        fprintf(stderr, "Invalid iNES ROM: %s\n", path);
        fclose(f);
        return NULL;
    }

    Cartridge *c = calloc(1, sizeof(*c));
    if (!c) {
        fclose(f);
        return NULL;
    }

    c->prg_banks = h.prg_banks;
    c->chr_banks = h.chr_banks;
    c->mapper = ((h.flags7 & 0xF0) | (h.flags6 >> 4));
    c->mirror = (h.flags6 & 0x08) ? 2 : (h.flags6 & 1);
    c->has_chr_ram = (h.chr_banks == 0);

    if ((h.flags6 & 0x04) && fseek(f, 512, SEEK_CUR) != 0) {
        fprintf(stderr, "Could not skip trainer in ROM: %s\n", path);
        cart_free(c);
        fclose(f);
        return NULL;
    }

    size_t prg_size = (size_t)c->prg_banks * PRG_BANK_SIZE;
    size_t chr_size = c->has_chr_ram ? CHR_BANK_SIZE : (size_t)c->chr_banks * CHR_BANK_SIZE;
    c->prg_rom = malloc(prg_size ? prg_size : PRG_BANK_SIZE);
    c->chr_rom = c->has_chr_ram ? NULL : malloc(chr_size);

    if (!c->prg_rom || (!c->has_chr_ram && !c->chr_rom)) {
        fprintf(stderr, "Out of memory loading ROM: %s\n", path);
        cart_free(c);
        fclose(f);
        return NULL;
    }

    if (fread(c->prg_rom, 1, prg_size, f) != prg_size) {
        fprintf(stderr, "ROM ended while reading PRG data: %s\n", path);
        cart_free(c);
        fclose(f);
        return NULL;
    }
    if (!c->has_chr_ram && fread(c->chr_rom, 1, chr_size, f) != chr_size) {
        fprintf(stderr, "ROM ended while reading CHR data: %s\n", path);
        cart_free(c);
        fclose(f);
        return NULL;
    }

    c->regs[0] = 0x0C; /* MMC1 padrão: último banco PRG fixo */
    sync_mapper(c);
    fclose(f);

    if (c->mapper < 0 || (c->mapper > 4)) {
        fprintf(stderr, "Warning: mapper %d is not implemented; ROM may not run.\n", c->mapper);
    }
    return c;
}

void cart_free(Cartridge *c) {
    if (!c) return;
    free(c->prg_rom);
    free(c->chr_rom);
    free(c);
}

u8 cart_prg_read(Cartridge *c, u16 addr) {
    if (addr >= 0x6000 && addr < 0x8000) return c->prg_ram[addr & 0x1FFF];
    if (addr < 0x8000 || c->prg_banks <= 0) return 0xFF;
    int slot = (addr - 0x8000) / 0x2000;
    int bank = wrap_bank(c->prg_bank[slot], prg_bank_count_8k(c));
    return c->prg_rom[(bank * 0x2000) + (addr & 0x1FFF)];
}

void cart_prg_write(Cartridge *c, u16 addr, u8 val) {
    if (addr >= 0x6000 && addr < 0x8000) {
        c->prg_ram[addr & 0x1FFF] = val;
        return;
    }
    if (addr < 0x8000) return;

    switch (c->mapper) {
    case 1:
        if (val & 0x80) {
            c->regs[4] = 0;
            c->regs[5] = 0;
            c->regs[0] |= 0x0C;
            sync_mapper(c);
            return;
        }
        c->regs[4] = (c->regs[4] >> 1) | ((val & 1) << 4);
        c->regs[5]++;
        if (c->regs[5] == 5) {
            int target = (addr >> 13) & 3;
            c->regs[target] = c->regs[4] & 0x1F;
            c->regs[4] = c->regs[5] = 0;
            sync_mapper(c);
        }
        break;
    case 2:
        c->regs[0] = val;
        sync_mapper(c);
        break;
    case 3:
        c->regs[0] = val;
        sync_mapper(c);
        break;
    case 4:
        switch (addr & 0xE001) {
        case 0x8000:
            c->regs[0] = val;
            sync_mapper(c);
            break;
        case 0x8001:
            c->regs[c->regs[0] & 7] = val;
            sync_mapper(c);
            break;
        case 0xA000:
            c->mirror = (val & 1) ? 0 : 1;
            break;
        case 0xC000:
            c->reload = val;
            break;
        case 0xC001:
            c->counter = 0;
            break;
        case 0xE000:
            c->irq_enable = false;
            c->irq_pending = false;
            break;
        case 0xE001:
            c->irq_enable = true;
            break;
        }
        break;
    default:
        break;
    }
}

u8 cart_chr_read(Cartridge *c, u16 addr) {
    addr &= 0x1FFF;
    int slot = addr / 0x400;
    int bank = wrap_bank(c->chr_bank[slot], chr_bank_count_1k(c));
    int off = (bank * 0x400) + (addr & 0x3FF);
    if (c->has_chr_ram) return c->chr_ram[off & 0x1FFF];
    return c->chr_rom[off % (c->chr_banks * CHR_BANK_SIZE)];
}

void cart_chr_write(Cartridge *c, u16 addr, u8 val) {
    if (!c->has_chr_ram) return;
    addr &= 0x1FFF;
    int slot = addr / 0x400;
    int bank = wrap_bank(c->chr_bank[slot], 8);
    c->chr_ram[(bank * 0x400) + (addr & 0x3FF)] = val;
}

int cart_nt_mirror(Cartridge *c, u16 addr) {
    int nt = ((addr - 0x2000) >> 10) & 3;
    switch (c->mirror) {
    case 0: return (nt >> 1) & 1; /* horizontal */
    case 1: return nt & 1;        /* vertical */
    case 3: return 0;             /* uma tela inferior */
    case 4: return 1;             /* uma tela superior */
    default: return nt & 1;       /* fallback quatro telas */
    }
}
