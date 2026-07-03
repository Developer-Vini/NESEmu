#pragma once
#include "nes.h"
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

/* APU depois mexo nisso, deixa eu focar em outras coisas*/
typedef struct APU {
    u8 regs[0x18];
} APU;

/* Estado do controle */
typedef struct Controller {
    u8 state;
    u8 shift;
    bool strobe;
} Controller;

/* Máquina NES */
typedef struct NES {
    CPU        cpu;
    PPU        ppu;
    APU        apu;
    Cartridge *cart;

    u8  ram[0x800];          /* 2 KB de RAM interna */
    Controller ctrl[2];

    bool running;
    bool frame_ready;
} NES;

u8   bus_read(NES *nes, u16 addr);
void bus_write(NES *nes, u16 addr, u8 val);

NES *nes_create(const char *rom_path);
void nes_destroy(NES *nes);
void nes_reset(NES *nes);
void nes_run_frame(NES *nes);

/* Botões */
void nes_set_button(NES *nes, int player, int button, bool pressed);

#define BTN_A      0x01
#define BTN_B      0x02
#define BTN_SELECT 0x04
#define BTN_START  0x08
#define BTN_UP     0x10
#define BTN_DOWN   0x20
#define BTN_LEFT   0x40
#define BTN_RIGHT  0x80
