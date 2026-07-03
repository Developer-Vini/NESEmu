#pragma once
#include "nes.h"

typedef struct PPU {
    /* Registradores ($2000-$2007) */
    u8 ctrl;    /* PPUCTRL   */
    u8 mask;    /* PPUMASK   */
    u8 status;  /* PPUSTATUS */
    u8 oam_addr;

    /* Estado interno */
    u16 v;         /* endereço VRAM atual (15 bits) */
    u16 t;         /* endereço VRAM temporário */
    u8  x;         /* scroll X fino (3 bits) */
    bool w;        /* alternância de escrita */
    u8  buf;       /* buffer de leitura */

    /* Memória */
    u8 nametable[2][0x400];
    u8 palette[32];
    u8 oam[256];
    u8 secondary_oam[32];

    int scanline;
    int dot;
    bool odd_frame;

    u32 fb[NES_WIDTH * NES_HEIGHT];

   
    u16 bg_sr_lo, bg_sr_hi;       
    u8  at_sr_lo, at_sr_hi;
    u8  at_latch_lo, at_latch_hi;

    u8 nt_byte, at_byte, bg_lo, bg_hi;

    /* Renderização de sprites */
    u8 sp_count;
    u8 sp_pattern_lo[8], sp_pattern_hi[8];
    u8 sp_attrib[8];
    u8 sp_x[8];
    u8 sp_index[8];

    bool nmi_occurred;
    bool nmi_output;
    bool nmi_edge;

    struct NES *nes;
} PPU;

void ppu_init(PPU *ppu, struct NES *nes);
void ppu_reset(PPU *ppu);
void ppu_step(PPU *ppu);         

u8   ppu_read_reg(PPU *ppu, u8 reg);
void ppu_write_reg(PPU *ppu, u8 reg, u8 val);
void ppu_oam_dma(PPU *ppu, u8 *src);
