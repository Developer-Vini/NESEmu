#pragma once
#include "nes.h"

typedef struct
{
    u8 magic[4]; // Assinatura iNES: "NES" + 0x1A
    u8 prg_banks;
    u8 chr_banks;
    u8 flags6;
    u8 flags7;
    u8 prg_ram;
    u8 flags0;
    u8 padding[5];
} iNESHeader;

typedef struct Cartridge {
    u8 *prg_rom;
    u8 *chr_rom;
    u8 prg_ram[0x2000];
    u8 chr_ram[0x2000];

    int prg_banks;
    int chr_banks;
    int mapper;
    int mirror;
    bool has_chr_ram;

    u8 regs[8];
    int prg_bank;
    int chr_bank;
    int reload, counter;
    bool irq_enable;
    bool irq_pending;
} Cartridge;


Cartridge *cart_load(const char *path);
void       cart_free(Cartridge *c);

u8   cart_prg_read(Cartridge *c, u16 addr);
void cart_prg_write(Cartridge *c, u16 addr, u8 val);

u8   cart_chr_read(Cartridge *c, u16 addr);
void cart_chr_write(Cartridge *c, u16 addr, u8 val);

int cart_nt_mirror(Cartridge *c, u16 addr);