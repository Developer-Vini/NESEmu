#include "cpu.h"
// CPU:
// 1 Ler uma instrução da memória
// 2 Descobrir qual instrução é
// 3 Executar
// 4 Atualizar os registradores
// 5 Repetir

#define SET(f) (cpu->p |= (f))
#define CLR(f) (cpu->p &= ~(f))
#define GET(f) (!!(cpu->p & (f)))
#define SETIF(c, f) \
    do              \
    {               \
        if (c)      \
            SET(f); \
        else        \
            CLR(f); \
    } while (0)

static u8 rd(CPU *cpu, u16 a)
{
    return bus_read(cpu->nes, a);
}
static void wr(CPU *cpu, u16 a, u8 v)
{
    bus_write(cpu->nes, a, v);
}

static void push(CPU *cpu, u8 v)
{
    wr(cpu, 0x100 | cpu->sp--, v);
}
static pop(CPU *cpu)
{
    return rd(cpu, 0x100 | ++cpu->sp);
}

static void push16(CPU *cpu, u8 v)
{
    push(cpu, v >> 8);
    push(cpu, v & 0xFF);
}
static u16 pop16(CPU *cpu)
{
    u8 lo = pop(cpu);
    u8 hi = pop(cpu);
    return (hi << 8) | lo;
}
static u16 rd16(CPU *cpu, u16 a)
{
    return rd(cpu, a) | ((u16)rd(cpu, a + 1) << 8);
}

static int page_cross(u16 base, u16 eff)
{
    return (base & 0xFF00) != (eff & 0xFF00) ? 1 : 0;
}

static void set_nz(CPU *cpu, u8 v)
{
    SETIF(
        v == 0,
        FLAG_Z);
    SETIF(
        v & 0x80,
        FLAG_N);
}

typedef struct
{
    u16 addr;
    int extra;
} EffAddr;

static EffAddr eff(CPU *cpu, AddrMode m)
{
    EffAddr e = {
        0,
        0
    };
    u16 base;
    switch (m)
    {
    case IMP:
    case ACC:
        break;
    case IMM:
        e.addr = cpu->pc++;
        break;
    case ZPG:
        e.addr = rd(cpu, cpu->pc++);
        break;
    case ZPX:
        e.addr = (rd(cpu, cpu->pc++) + cpu->x) & 0xFF;
        break;
    case ZPY:
        e.addr = (rd(cpu, cpu->pc++) + cpu->y) & 0xFF;
        break;
    case ABS:
        e.addr = rd16(cpu, cpu->pc);
        cpu->pc += 2;
        break;
    case ABX:
        base = rd16(cpu, cpu->pc);
        cpu->pc += 2;
        e.extra = page_cross(base, e.addr);
        break;
    case ABY:
        base = rd16(cpu, cpu->pc);
        cpu->pc += 2;
        e.addr = base + cpu->y;
        e.extra = page_cross(base, e.addr);
        break;
    case IND:
    {
        u16 ptr = rd16(cpu, cpu->pc);
        cpu->pc += 2;
        u16 lo = rd(cpu, ptr);
        u16 hi = rd(cpu, (ptr & 0xFF00) | ((ptr + 1) & 0xFF));
        e.addr = lo | (hi << 8);
        break;
    }
    case INX:
        u8 zp = (rd(cpu, cpu->pc++) + cpu->x) & 0xFF;
        e.addr = rd(cpu, zp) | ((u16)rd(cpu, (zp + 1) & 0xFF) << 8);
        break;
    case INY:
    {
        u8 zp = rd(cpu, cpu->pc++);
        base = rd(cpu, zp) | ((u16)rd(cpu, (zp + 1) & 0xFF) << 8);
        e.addr = base + cpu->y;
        e.extra = page_cross(base, e.addr);
        break;
    }
    case REL:
    {
        s8 off = (s8)rd(cpu, cpu->pc++);
        e.addr = cpu->pc + off;
        e.extra = page_cross(cpu->pc, e.addr);
    }
    }
    return e;
}

static int branch(CPU *cpu, bool cond, u16 target)
{
    if (!cond)
    {
        return 0;
    }
    int extra = 1 + page_cross(cpu->pc, target);
    cpu->pc = target;
    return extra;
}
static void adc(CPU *cpu, u8 v)
{
    u16 r = cpu->a + v + GET(FLAG_C);
    SETIF((~(cpu->a ^ v)) & (cpu->a ^ r) & 0x80, FLAG_V);
    SETIF(r > 0xFF, FLAG_C);
    cpu->a = r & 0xFF;
    set_nz(cpu, cpu->a);
}

static void sbc(CPU *cpu, u8 v)
{
    adc(cpu, v ^ 0xFF);
}

static u8 asl(CPU *cpu, u8 v)
{
    SETIF(v & 0x80, FLAG_C);
    v <<= 1;
    set_nz(cpu, v);
    return v;
}
static u8 lsr(CPU *cpu, u8 v)
{
    SETIF(v & 0x01, FLAG_C);
    v >>= 1;
    set_nz(cpu, v);
    return v;
}
static u8 rol(CPU *cpu, u8 v)
{
    u8 r = (v << 1) | GET(FLAG_C);
    SETIF(v & 0x80, FLAG_C);
    set_nz(cpu, r);
    return r;
}
static u8 ror(CPU *cpu, u8 v)
{
    u8 r = (v >> 1) | (GET(FLAG_C) << 7);
    SETIF(v & 0x01, FLAG_C);
    set_nz(cpu, r);
    return r;
}

typedef struct
{
    AddrMode mode;
    int base_cycles;
} OpInfo;

static const OpInfo OPTBL[256] = {
/*  0        1        2       3         4        5       6       7  */
{IMP,7}, {INX,6}, {IMP,0}, {INX,8}, {ZPG,3}, {ZPG,3}, {ZPG,5}, {ZPG,5},
{IMP,3}, {IMM,2}, {ACC,2}, {IMM,2}, {ABS,4}, {ABS,4}, {ABS,6}, {ABS,6},
{REL,2}, {INY,5}, {IMP,0}, {INY,8}, {ZPX,4}, {ZPX,4}, {ZPX,6}, {ZPX,6},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,7}, {ABX,4}, {ABX,4}, {ABX,7}, {ABX,7},
{ABS,6}, {INX,6}, {IMP,0}, {INX,8}, {ZPG,3}, {ZPG,3}, {ZPG,5}, {ZPG,5},
{IMP,4}, {IMM,2}, {ACC,2}, {IMM,2}, {ABS,4}, {ABS,4}, {ABS,6}, {ABS,6},
{REL,2}, {INY,5}, {IMP,0}, {INY,8}, {ZPX,4}, {ZPX,4}, {ZPX,6}, {ZPX,6},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,7}, {ABX,4}, {ABX,4}, {ABX,7}, {ABX,7},
{IMP,6}, {INX,6}, {IMP,0}, {INX,8}, {ZPG,3}, {ZPG,3}, {ZPG,5}, {ZPG,5},
{IMP,3}, {IMM,2}, {ACC,2}, {IMM,2}, {ABS,3}, {ABS,4}, {ABS,6}, {ABS,6},
{REL,2}, {INY,5}, {IMP,0}, {INY,8}, {ZPX,4}, {ZPX,4}, {ZPX,6}, {ZPX,6},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,7}, {ABX,4}, {ABX,4}, {ABX,7}, {ABX,7},
{IMP,6}, {INX,6}, {IMP,0}, {INX,8}, {ZPG,3}, {ZPG,3}, {ZPG,5}, {ZPG,5},
{IMP,4}, {IMM,2}, {ACC,2}, {IMM,2}, {IND,5}, {ABS,4}, {ABS,6}, {ABS,6},
{REL,2}, {INY,5}, {IMP,0}, {INY,8}, {ZPX,4}, {ZPX,4}, {ZPX,6}, {ZPX,6},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,7}, {ABX,4}, {ABX,4}, {ABX,7}, {ABX,7},
{IMM,2}, {INX,6}, {IMM,2}, {INX,6}, {ZPG,3}, {ZPG,3}, {ZPG,3}, {ZPG,3},
{IMP,2}, {IMM,2}, {IMP,2}, {IMM,2}, {ABS,4}, {ABS,4}, {ABS,4}, {ABS,4},
{REL,2}, {INY,6}, {IMP,0}, {INY,6}, {ZPX,4}, {ZPX,4}, {ZPY,4}, {ZPY,4},
{IMP,2}, {ABY,5}, {IMP,2}, {ABY,5}, {ABX,5}, {ABX,5}, {ABY,5}, {ABY,5},
{IMM,2}, {INX,6}, {IMM,2}, {INX,6}, {ZPG,3}, {ZPG,3}, {ZPG,3}, {ZPG,3},
{IMP,2}, {IMM,2}, {IMP,2}, {IMM,2}, {ABS,4}, {ABS,4}, {ABS,4}, {ABS,4},
{REL,2}, {INY,5}, {IMP,0}, {INY,5}, {ZPX,4}, {ZPX,4}, {ZPY,4}, {ZPY,4},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,4}, {ABX,4}, {ABX,4}, {ABY,4}, {ABY,4},
{IMM,2}, {INX,6}, {IMM,2}, {INX,8}, {ZPG,3}, {ZPG,3}, {ZPG,5}, {ZPG,5},
{IMP,2}, {IMM,2}, {IMP,2}, {IMM,2}, {ABS,4}, {ABS,4}, {ABS,6}, {ABS,6},
{REL,2}, {INY,5}, {IMP,0}, {INY,8}, {ZPX,4}, {ZPX,4}, {ZPX,6}, {ZPX,6},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,7}, {ABX,4}, {ABX,4}, {ABX,7}, {ABX,7},
{IMM,2}, {INX,6}, {IMM,2}, {INX,8}, {ZPG,3}, {ZPG,3}, {ZPG,5}, {ZPG,5},
{IMP,2}, {IMM,2}, {IMP,2}, {IMM,2}, {ABS,4}, {ABS,4}, {ABS,6}, {ABS,6},
{REL,2}, {INY,5}, {IMP,0}, {INY,8}, {ZPX,4}, {ZPX,4}, {ZPX,6}, {ZPX,6},
{IMP,2}, {ABY,4}, {IMP,2}, {ABY,7}, {ABX,4}, {ABX,4}, {ABX,7}, {ABX,7},
};


void cpu_init(CPU *cpu, struct NES *nes)
{
    memset(cpu, 0, sizeof(*cpu));
    cpu->nes = nes;
}
void cpu_reset(CPU *cpu)
{
    cpu->sp = 0xFD;
    cpu->p = FLAG_U | FLAG_I;
    cpu->pc = rd16(cpu, 0xFFFC);
    cpu->cycles = 7;
}
void cpu_nmi(CPU *cpu)
{
    push16(cpu, cpu->pc);
    push(cpu, (cpu->p & ~FLAG_B) | FLAG_U);
    SET(FLAG_I);
    cpu->pc = rd16(cpu, 0xFFFA);
    cpu->cycles += 7;
}
void cpu_irq(CPU *cpu)
{
    if (GET(FLAG_I))
    {
        return;
    }
    push16(cpu, cpu->pc);
    push(cpu, (cpu->p & ~FLAG_B) | FLAG_U);
    SET(FLAG_I);
    cpu->pc = rd16(cpu, 0xFFFE);
    cpu->cycles += 7;
}

int cpu_step(CPU *cpu)
{
    if (cpu->nmi_pending)
    {
        cpu->nmi_pending = false;
        cpu_nmi(cpu);
        return 7;
    }
    if (cpu->irq_pending)
    {
        cpu->irq_pending = false;
        cpu_irq(cpu);
        return 7;
    }

    u8 op = rd(cpu, cpu->pc++);
    OpInfo inf = OPTBL[op];
    EffAddr ea = eff(cpu, inf.mode);
    int cyc = inf.base_cycles;


    switch (op)
    {
    /* ── LDA ── */
    case 0xA9:
    case 0xA5:
    case 0xB5:
    case 0xAD:
    case 0xBD:
    case 0xB9:
    case 0xA1:
    case 0xB1:
        cpu->a = rd(cpu, ea.addr);
        set_nz(cpu, cpu->a);
        cyc += ea.extra;
        break;
    /* ── LDX ── */
    case 0xA2:
    case 0xA6:
    case 0xB6:
    case 0xAE:
    case 0xBE:
        cpu->x = rd(cpu, ea.addr);
        set_nz(cpu, cpu->x);
        cyc += ea.extra;
        break;
    /* ── LDY ── */
    case 0xA0:
    case 0xA4:
    case 0xB4:
    case 0xAC:
    case 0xBC:
        cpu->y = rd(cpu, ea.addr);
        set_nz(cpu, cpu->y);
        cyc += ea.extra;
        break;
    /* ── STA ── */
    case 0x85:
    case 0x95:
    case 0x8D:
    case 0x9D:
    case 0x99:
    case 0x81:
    case 0x91:
        wr(cpu, ea.addr, cpu->a);
        break;
    /* ── STX ── */
    case 0x86:
    case 0x96:
    case 0x8E:
        wr(cpu, ea.addr, cpu->x);
        break;
    /* ── STY ── */
    case 0x84:
    case 0x94:
    case 0x8C:
        wr(cpu, ea.addr, cpu->y);
        break;
    /* ── ADC ── */
    case 0x69:
    case 0x65:
    case 0x75:
    case 0x6D:
    case 0x7D:
    case 0x79:
    case 0x61:
    case 0x71:
        adc(cpu, rd(cpu, ea.addr));
        cyc += ea.extra;
        break;
    /* ── SBC ── */
    case 0xE9:
    case 0xE5:
    case 0xF5:
    case 0xED:
    case 0xFD:
    case 0xF9:
    case 0xE1:
    case 0xF1:
        sbc(cpu, rd(cpu, ea.addr));
        cyc += ea.extra;
        break;
    /* ── AND ── */
    case 0x29:
    case 0x25:
    case 0x35:
    case 0x2D:
    case 0x3D:
    case 0x39:
    case 0x21:
    case 0x31:
        cpu->a &= rd(cpu, ea.addr);
        set_nz(cpu, cpu->a);
        cyc += ea.extra;
        break;
    /* ── ORA ── */
    case 0x09:
    case 0x05:
    case 0x15:
    case 0x0D:
    case 0x1D:
    case 0x19:
    case 0x01:
    case 0x11:
        cpu->a |= rd(cpu, ea.addr);
        set_nz(cpu, cpu->a);
        cyc += ea.extra;
        break;
    /* ── EOR ── */
    case 0x49:
    case 0x45:
    case 0x55:
    case 0x4D:
    case 0x5D:
    case 0x59:
    case 0x41:
    case 0x51:
        cpu->a ^= rd(cpu, ea.addr);
        set_nz(cpu, cpu->a);
        cyc += ea.extra;
        break;
    /* ── CMP ── */
    case 0xC9:
    case 0xC5:
    case 0xD5:
    case 0xCD:
    case 0xDD:
    case 0xD9:
    case 0xC1:
    case 0xD1:
    {
        u8 v = rd(cpu, ea.addr);
        SETIF(cpu->a >= v, FLAG_C);
        set_nz(cpu, cpu->a - v);
        cyc += ea.extra;
        break;
    }
    /* ── CPX ── */
    case 0xE0:
    case 0xE4:
    case 0xEC:
    {
        u8 v = rd(cpu, ea.addr);
        SETIF(cpu->x >= v, FLAG_C);
        set_nz(cpu, cpu->x - v);
        break;
    }
    /* ── CPY ── */
    case 0xC0:
    case 0xC4:
    case 0xCC:
    {
        u8 v = rd(cpu, ea.addr);
        SETIF(cpu->y >= v, FLAG_C);
        set_nz(cpu, cpu->y - v);
        break;
    }
    /* ── BIT ── */
    case 0x24:
    case 0x2C:
    {
        u8 v = rd(cpu, ea.addr);
        SETIF((cpu->a & v) == 0, FLAG_Z);
        SETIF(v & 0x40, FLAG_V);
        SETIF(v & 0x80, FLAG_N);
        break;
    }
    /* ── ASL ── */
    case 0x0A:
        cpu->a = asl(cpu, cpu->a);
        break;
    case 0x06:
    case 0x16:
    case 0x0E:
    case 0x1E:
        wr(cpu, ea.addr, asl(cpu, rd(cpu, ea.addr)));
        break;
    /* ── LSR ── */
    case 0x4A:
        cpu->a = lsr(cpu, cpu->a);
        break;
    case 0x46:
    case 0x56:
    case 0x4E:
    case 0x5E:
        wr(cpu, ea.addr, lsr(cpu, rd(cpu, ea.addr)));
        break;
    /* ── ROL ── */
    case 0x2A:
        cpu->a = rol(cpu, cpu->a);
        break;
    case 0x26:
    case 0x36:
    case 0x2E:
    case 0x3E:
        wr(cpu, ea.addr, rol(cpu, rd(cpu, ea.addr)));
        break;
    /* ── ROR ── */
    case 0x6A:
        cpu->a = ror(cpu, cpu->a);
        break;
    case 0x66:
    case 0x76:
    case 0x6E:
    case 0x7E:
        wr(cpu, ea.addr, ror(cpu, rd(cpu, ea.addr)));
        break;
    /* ── INC ── */
    case 0xE6:
    case 0xF6:
    case 0xEE:
    case 0xFE:
    {
        u8 v = rd(cpu, ea.addr) + 1;
        wr(cpu, ea.addr, v);
        set_nz(cpu, v);
        break;
    }
    /* ── DEC ── */
    case 0xC6:
    case 0xD6:
    case 0xCE:
    case 0xDE:
    {
        u8 v = rd(cpu, ea.addr) - 1;
        wr(cpu, ea.addr, v);
        set_nz(cpu, v);
        break;
    }
    /* ── INX/INY/DEX/DEY ── */
    case 0xE8:
        cpu->x++;
        set_nz(cpu, cpu->x);
        break;
    case 0xC8:
        cpu->y++;
        set_nz(cpu, cpu->y);
        break;
    case 0xCA:
        cpu->x--;
        set_nz(cpu, cpu->x);
        break;
    case 0x88:
        cpu->y--;
        set_nz(cpu, cpu->y);
        break;
    /* ── Transferências ── */
    case 0xAA:
        cpu->x = cpu->a;
        set_nz(cpu, cpu->x);
        break;
    case 0xA8:
        cpu->y = cpu->a;
        set_nz(cpu, cpu->y);
        break;
    case 0x8A:
        cpu->a = cpu->x;
        set_nz(cpu, cpu->a);
        break;
    case 0x98:
        cpu->a = cpu->y;
        set_nz(cpu, cpu->a);
        break;
    case 0xBA:
        cpu->x = cpu->sp;
        set_nz(cpu, cpu->x);
        break;
    case 0x9A:
        cpu->sp = cpu->x;
        break;
    /* ── Pilha ── */
    case 0x48:
        push(cpu, cpu->a);
        break;
    case 0x08:
        push(cpu, cpu->p | FLAG_B | FLAG_U);
        break;
    case 0x68:
        cpu->a = pop(cpu);
        set_nz(cpu, cpu->a);
        break;
    case 0x28:
        cpu->p = (pop(cpu) | FLAG_U) & ~FLAG_B;
        break;
    /* ── Saltos ── */
    case 0x4C:
    case 0x6C:
        cpu->pc = ea.addr;
        break;
    case 0x20:
        push16(cpu, cpu->pc - 1);
        cpu->pc = ea.addr;
        break;
    case 0x60:
        cpu->pc = pop16(cpu) + 1;
        break;
    case 0x40:
        cpu->p = pop(cpu) | FLAG_U;
        cpu->pc = pop16(cpu);
        break;
    case 0x00:
        cpu->pc++;
        push16(cpu, cpu->pc);
        push(cpu, cpu->p | FLAG_B | FLAG_U);
        SET(FLAG_I);
        cpu->pc = rd16(cpu, 0xFFFE);
        break;
    /* ── Desvios condicionais ── */
    case 0x90:
        cyc += branch(cpu, !GET(FLAG_C), ea.addr);
        break;
    case 0xB0:
        cyc += branch(cpu, GET(FLAG_C), ea.addr);
        break;
    case 0xF0:
        cyc += branch(cpu, GET(FLAG_Z), ea.addr);
        break;
    case 0xD0:
        cyc += branch(cpu, !GET(FLAG_Z), ea.addr);
        break;
    case 0x10:
        cyc += branch(cpu, !GET(FLAG_N), ea.addr);
        break;
    case 0x30:
        cyc += branch(cpu, GET(FLAG_N), ea.addr);
        break;
    case 0x50:
        cyc += branch(cpu, !GET(FLAG_V), ea.addr);
        break;
    case 0x70:
        cyc += branch(cpu, GET(FLAG_V), ea.addr);
        break;
    /* ── Operações de flags ── */
    case 0x18:
        CLR(FLAG_C);
        break;
    case 0x38:
        SET(FLAG_C);
        break;
    case 0x58:
        CLR(FLAG_I);
        break;
    case 0x78:
        SET(FLAG_I);
        break;
    case 0xB8:
        CLR(FLAG_V);
        break;
    case 0xD8:
        CLR(FLAG_D);
        break;
    case 0xF8:
        SET(FLAG_D);
        break;
    /* ── NOP ── */
    case 0xEA:
        break;
    case 0x1A:
    case 0x3A:
    case 0x5A:
    case 0x7A:
    case 0xDA:
    case 0xFA:
        break;
    default:
        break;
    }

    cpu->cycles += cyc;
    return cyc;
}