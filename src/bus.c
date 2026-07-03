#include "bus.h"
#include <stdlib.h>

u8 bus_read(NES *nes, u16 addr)
{
    if (addr < 0x2000)
        return nes->ram[addr & 0x7FF];
    if (addr < 0x4000)
        return ppu_read_reg(&nes->ppu, addr & 7);
    if (addr == 0x4016)
    {
        if (nes->ctrl[0].strobe)
            nes->ctrl[0].shift = nes->ctrl[0].state;
        u8 val = nes->ctrl[0].shift & 1;
        if (!nes->ctrl[0].strobe)
            nes->ctrl[0].shift >>= 1;
        return 0x40 | val;
    }
    if (addr == 0x4017)
    {
        if (nes->ctrl[1].strobe)
            nes->ctrl[1].shift = nes->ctrl[1].state;
        u8 val = nes->ctrl[1].shift & 1;
        if (!nes->ctrl[1].strobe)
            nes->ctrl[1].shift >>= 1;
        return 0x40 | val;
    }
    if (addr < 0x4018)
        return 0;
    if (addr >= 0x6000)
        return cart_prg_read(nes->cart, addr);
    return 0xFF;
}

void bus_write(NES *nes, u16 addr, u8 val)
{
    if (addr < 0x2000)
    {
        nes->ram[addr & 0x7FF] = val;
        return;
    }
    if (addr < 0x4000)
    {
        ppu_write_reg(&nes->ppu, addr & 7, val);
        return;
    }

    if (addr == 0x4014)
    {
        u16 base = (u16)val << 8;
        u8 buf[256];
        for (int i = 0; i < 256; i++)
            buf[i] = bus_read(nes, base + i);
        ppu_oam_dma(&nes->ppu, buf);
        nes->cpu.cycles += 513 + (nes->cpu.cycles & 1);
        return;
    }
    if (addr == 0x4016)
    {
        bool strobe = (val & 1);
        for (int p = 0; p < 2; p++)
        {
            if (strobe || (nes->ctrl[p].strobe && !strobe))
                nes->ctrl[p].shift = nes->ctrl[p].state;
            nes->ctrl[p].strobe = strobe;
        }
        return;
    }
    if (addr >= 0x6000)
        cart_prg_write(nes->cart, addr, val);
}

NES *nes_create(const char *rom_path)
{
    Cartridge *cart = cart_load(rom_path);
    if (!cart)
        return NULL;
    NES *nes = calloc(1, sizeof(NES));
    nes->cart = cart;
    cpu_init(&nes->cpu, nes);
    ppu_init(&nes->ppu, nes);
    nes_reset(nes);
    nes->running = true;
    return nes;
}

void nes_destroy(NES *nes)
{
    if (!nes)
        return;
    cart_free(nes->cart);
    free(nes);
}

void nes_reset(NES *nes)
{
    cpu_reset(&nes->cpu);
    ppu_reset(&nes->ppu);
    memset(&nes->apu, 0, sizeof(nes->apu));
}

void nes_run_frame(NES *nes)
{
    nes->frame_ready = false;

    do
    {
        int cpu_cyc = cpu_step(&nes->cpu);

        for (int i = 0; i < cpu_cyc * 3; i++)
        {
            int prev_sl = nes->ppu.scanline;
            int prev_dot = nes->ppu.dot;

            ppu_step(&nes->ppu);

            /* NMI */
            if (nes->ppu.nmi_edge)
            {
                nes->ppu.nmi_edge = false;
                nes->cpu.nmi_pending = true;
            }
            if (nes->cart->mapper == 4 && (nes->ppu.mask & 0x18))
            {
                if (prev_dot == 260 && prev_sl < 240)
                {
                    if (nes->cart->counter == 0)
                    {
                        nes->cart->counter = nes->cart->reload;
                        if (nes->cart->irq_enable)
                            nes->cpu.irq_pending = true;
                    }
                    else
                    {
                        nes->cart->counter--;
                        if (nes->cart->counter == 0 && nes->cart->irq_enable)
                            nes->cpu.irq_pending = true;
                    }
                }
            }

            if (nes->ppu.scanline == 0 && nes->ppu.dot == 0 &&
                (prev_sl == 261 || prev_sl == 260))
            {
                nes->frame_ready = true;
            }
        }
    } while (!nes->frame_ready);
}

void nes_set_button(NES *nes, int player, int button, bool pressed)
{
    if (player < 0 || player > 1)
        return;
    if (pressed)
        nes->ctrl[player].state |= (u8)button;
    else
        nes->ctrl[player].state &= ~(u8)button;
}
