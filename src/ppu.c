#include "ppu.h"
#include "bus.h"

/* Leitura/escrita na VRAM */
static u8 ppu_rd(PPU *ppu, u16 addr) {
    addr &= 0x3FFF;
    if (addr < 0x2000) return cart_chr_read(ppu->nes->cart, addr);
    if (addr < 0x3F00) {
        int nt = cart_nt_mirror(ppu->nes->cart, addr);
        return ppu->nametable[nt & 1][addr & 0x3FF];
    }
    addr &= 0x1F;
    if (addr == 0x10 || addr == 0x14 || addr == 0x18 || addr == 0x1C) addr &= 0x0F;
    return ppu->palette[addr];
}

static void ppu_wr(PPU *ppu, u16 addr, u8 val) {
    addr &= 0x3FFF;
    if (addr < 0x2000) { cart_chr_write(ppu->nes->cart, addr, val); return; }
    if (addr < 0x3F00) {
        int nt = cart_nt_mirror(ppu->nes->cart, addr);
        ppu->nametable[nt & 1][addr & 0x3FF] = val;
        return;
    }
    addr &= 0x1F;
    if (addr == 0x10 || addr == 0x14 || addr == 0x18 || addr == 0x1C) addr &= 0x0F;
    ppu->palette[addr] = val;
}

void ppu_init(PPU *ppu, struct NES *nes) {
    memset(ppu, 0, sizeof(*ppu));
    ppu->nes = nes;
}

void ppu_reset(PPU *ppu) {
    ppu->ctrl     = 0;
    ppu->mask     = 0;
    ppu->status   = 0xA0;
    ppu->scanline = 261;
    ppu->dot      = 0;
    ppu->w        = false;
    ppu->v = ppu->t = 0;
}

/* Registradores */
u8 ppu_read_reg(PPU *ppu, u8 reg) {
    u8 val = 0;
    switch (reg) {
    case 2:
        val = (ppu->status & 0xE0);
        ppu->status &= ~0x80;
        ppu->w = false;
        break;
    case 4: val = ppu->oam[ppu->oam_addr]; break;
    case 7:
        if ((ppu->v & 0x3FFF) < 0x3F00) {
            val = ppu->buf;
            ppu->buf = ppu_rd(ppu, ppu->v);
        } else {
            val = ppu_rd(ppu, ppu->v);
            ppu->buf = ppu_rd(ppu, ppu->v - 0x1000);
        }
        ppu->v += (ppu->ctrl & 0x04) ? 32 : 1;
        break;
    }
    return val;
}

void ppu_write_reg(PPU *ppu, u8 reg, u8 val) {
    switch (reg) {
    case 0:
        ppu->ctrl = val;
        ppu->t = (ppu->t & 0xF3FF) | ((u16)(val & 3) << 10);
        break;
    case 1: ppu->mask = val; break;
    case 3: ppu->oam_addr = val; break;
    case 4: ppu->oam[ppu->oam_addr++] = val; break;
    case 5:
        if (!ppu->w) {
            ppu->t = (ppu->t & 0xFFE0) | (val >> 3);
            ppu->x = val & 7;
        } else {
            ppu->t = (ppu->t & 0x8C1F)
                   | ((u16)(val & 0xF8) << 2)
                   | ((u16)(val & 0x07) << 12);
        }
        ppu->w = !ppu->w;
        break;
    case 6:
        if (!ppu->w) {
            ppu->t = (ppu->t & 0x00FF) | ((u16)(val & 0x3F) << 8);
        } else {
            ppu->t = (ppu->t & 0xFF00) | val;
            ppu->v = ppu->t;
        }
        ppu->w = !ppu->w;
        break;
    case 7:
        ppu_wr(ppu, ppu->v, val);
        ppu->v += (ppu->ctrl & 0x04) ? 32 : 1;
        break;
    }
}

void ppu_oam_dma(PPU *ppu, u8 *src) {
    memcpy(ppu->oam, src, 256);
}

static void inc_coarse_x(PPU *ppu) {
    if ((ppu->v & 0x001F) == 31) {
        ppu->v &= ~0x001F;
        ppu->v ^= 0x0400;
    } else {
        ppu->v++;
    }
}

static void inc_fine_y(PPU *ppu) {
    if ((ppu->v & 0x7000) != 0x7000) {
        ppu->v += 0x1000;
    } else {
        ppu->v &= ~0x7000;
        int y = (ppu->v & 0x03E0) >> 5;
        if      (y == 29) { y = 0; ppu->v ^= 0x0800; }
        else if (y == 31) { y = 0; }
        else              { y++; }
        ppu->v = (ppu->v & ~0x03E0) | (y << 5);
    }
}


static void reload_shifters(PPU *ppu) {
    ppu->bg_sr_lo = (ppu->bg_sr_lo & 0xFF00) | ppu->bg_lo;
    ppu->bg_sr_hi = (ppu->bg_sr_hi & 0xFF00) | ppu->bg_hi;
    ppu->at_latch_lo = (ppu->at_byte & 1);
    ppu->at_latch_hi = (ppu->at_byte >> 1) & 1;
}

static void shift_bg(PPU *ppu) {
    ppu->bg_sr_lo <<= 1;
    ppu->bg_sr_hi <<= 1;
    ppu->at_sr_lo  = (ppu->at_sr_lo << 1) | ppu->at_latch_lo;
    ppu->at_sr_hi  = (ppu->at_sr_hi << 1) | ppu->at_latch_hi;
}

static void fetch_bg_step(PPU *ppu, int dot) {

    switch (dot & 7) {
    case 1: 
        ppu->nt_byte = ppu_rd(ppu, 0x2000 | (ppu->v & 0x0FFF));
        break;
    case 3: { 
        u16 a = 0x23C0 | (ppu->v & 0x0C00)
                        | ((ppu->v >> 4) & 0x38)
                        | ((ppu->v >> 2) & 0x07);
        u8 at = ppu_rd(ppu, a);
        if (ppu->v & 0x0040) at >>= 4;
        if (ppu->v & 0x0002) at >>= 2;
        ppu->at_byte = at & 3;
        break;
    }
    case 5: { 
        u16 base = (ppu->ctrl & 0x10) ? 0x1000 : 0;
        ppu->bg_lo = ppu_rd(ppu, base | ((u16)ppu->nt_byte << 4) | ((ppu->v >> 12) & 7));
        break;
    }
    case 7: { 
        u16 base = (ppu->ctrl & 0x10) ? 0x1000 : 0;
        ppu->bg_hi = ppu_rd(ppu, base | ((u16)ppu->nt_byte << 4) | ((ppu->v >> 12) & 7) | 8);
        break;
    }
    }
}

static void eval_sprites(PPU *ppu) {
    ppu->sp_count = 0;
    int h = (ppu->ctrl & 0x20) ? 16 : 8;
    int next_sl = ppu->scanline + 1;

    for (int i = 0; i < 64; i++) {
        int y   = (int)ppu->oam[i * 4] + 1;
        int row = next_sl - y;
        if (row < 0 || row >= h) continue;
        if (ppu->sp_count >= 8) {
            ppu->status |= 0x20;
            continue;
        }

        u8 tile = ppu->oam[i * 4 + 1];
        u8 attr = ppu->oam[i * 4 + 2];
        u8 xpos = ppu->oam[i * 4 + 3];

        if (attr & 0x80) row = h - 1 - row;  /* espelhamento vertical */

        u16 pt_addr;
        if (h == 8) {
            u16 base = (ppu->ctrl & 0x08) ? 0x1000 : 0;
            pt_addr  = base | ((u16)tile << 4) | row;
        } else {
            u16 base = (tile & 1) ? 0x1000 : 0;
            pt_addr  = base | ((tile & 0xFE) << 4) | (row & 7) | ((row >= 8) ? 16 : 0);
        }

        u8 lo = ppu_rd(ppu, pt_addr);
        u8 hi = ppu_rd(ppu, pt_addr + 8);

        /* Espelhamento horizontal: inverte bits */
        if (attr & 0x40) {
            lo = (u8)(((lo * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
            hi = (u8)(((hi * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
        }

        int s = ppu->sp_count++;
        ppu->sp_pattern_lo[s] = lo;
        ppu->sp_pattern_hi[s] = hi;
        ppu->sp_attrib[s]     = attr;
        ppu->sp_x[s]          = xpos;
        ppu->sp_index[s]      = i;
    }
}

/* Renderiza um pixel */
static void render_pixel(PPU *ppu) {
    int x = ppu->dot - 1;
    int y = ppu->scanline;

    /* Pixel de fundo */
    u8 bg_pixel = 0, bg_pal = 0;
    if ((ppu->mask & 0x08) && (x >= 8 || (ppu->mask & 0x02))) {
        u16 mux   = 0x8000 >> ppu->x;
        bg_pixel  = (!!(ppu->bg_sr_hi & mux) << 1) | !!(ppu->bg_sr_lo & mux);
        bg_pal    = (!!(ppu->at_sr_hi & mux) << 1) | !!(ppu->at_sr_lo & mux);
    }

    /* Pixel do sprite */
    u8 sp_pixel = 0, sp_pal = 0;
    bool sp_front = false;
    int sp_slot = -1;
    if ((ppu->mask & 0x10) && (x >= 8 || (ppu->mask & 0x04))) {
        for (int i = 0; i < ppu->sp_count; i++) {
            int off = x - (int)ppu->sp_x[i];
            if (off < 0 || off > 7) continue;
            u8 bit = 0x80 >> off;
            u8 px  = (!!(ppu->sp_pattern_hi[i] & bit) << 1) | !!(ppu->sp_pattern_lo[i] & bit);
            if (!px) continue;
            sp_pixel = px;
            sp_pal   = (ppu->sp_attrib[i] & 3) + 4;
            sp_front = !(ppu->sp_attrib[i] & 0x20);
            sp_slot = i;
            break;
        }
    }

    if (bg_pixel && sp_pixel && sp_slot >= 0 && ppu->sp_index[sp_slot] == 0 &&
        x < 255 && (ppu->mask & 0x18)) {
        ppu->status |= 0x40;
    }

    u8 pal_idx;
    if      (!bg_pixel && !sp_pixel) pal_idx = 0;
    else if (!bg_pixel)              pal_idx = (sp_pal << 2) | sp_pixel;
    else if (!sp_pixel)              pal_idx = (bg_pal << 2) | bg_pixel;
    else                             pal_idx = sp_front
                                               ? ((sp_pal << 2) | sp_pixel)
                                               : ((bg_pal << 2) | bg_pixel);

    u8 color = ppu_rd(ppu, 0x3F00 | (pal_idx & 0x1F)) & 0x3F;
    ppu->fb[y * NES_WIDTH + x] = NES_PALETTE[color];
}


void ppu_step(PPU *ppu) {
    bool rendering = (ppu->mask & 0x18) != 0;
    int  sl  = ppu->scanline;
    int  dot = ppu->dot;

    if (sl < 240 || sl == 261) {

        bool bg_fetch = (dot >= 1 && dot <= 256) || (dot >= 321 && dot <= 336);

        if (rendering) {

            if (sl < 240 && dot >= 1 && dot <= 256)
                shift_bg(ppu);

            if (bg_fetch) {
                fetch_bg_step(ppu, dot);
                if ((dot & 7) == 0) {
                    reload_shifters(ppu);
                    inc_coarse_x(ppu);
                }
            }

            if (dot >= 321 && dot <= 336)
                shift_bg(ppu);

            
            if (dot == 256) inc_fine_y(ppu);

            
            if (dot == 257)
                ppu->v = (ppu->v & ~0x041F) | (ppu->t & 0x041F);

           
            if (sl == 261 && dot >= 280 && dot <= 304)
                ppu->v = (ppu->v & ~0x7BE0) | (ppu->t & 0x7BE0);
        }

        
        if (sl < 240 && dot == 257)
            eval_sprites(ppu);

        if (sl < 240 && dot >= 1 && dot <= 256)
            render_pixel(ppu);
    }

    /* Início do VBlank */
    if (sl == 241 && dot == 1) {
        ppu->status |= 0x80;
        if (ppu->ctrl & 0x80)          /* bit de habilitação NMI */
            ppu->nmi_edge = true;
    }

    if (sl == 261 && dot == 1) {
        ppu->status &= ~0xE0;
    }

    ppu->dot++;
    if (ppu->dot > 340) {
        ppu->dot = 0;
        ppu->scanline++;
        if (ppu->scanline > 261) {
            ppu->scanline  = 0;
            ppu->odd_frame = !ppu->odd_frame;
            if (ppu->odd_frame && rendering) ppu->dot = 1; 
        }
    }
}
