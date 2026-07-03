#pragma once 
#include "nes.h"

// Flags do processador: [N] [V] [U] [B] [D] [I] [Z] [C]
#define FLAG_C  0x01  // Carry
#define FLAG_Z  0x02  // Zero
#define FLAG_I  0x04  // Interrupção
#define FLAG_D  0x08  // Decimal (não usado no NES)
#define FLAG_B  0x10  // Break
#define FLAG_U  0x20  // Sem uso
#define FLAG_V  0x40  // Overflow
#define FLAG_N  0x80  // Negativo

typedef struct CPU {
    u16 pc;   // Contador de programa
    u8  sp;   // Ponteiro da pilha
    u8  a;    // Acumulador
    u8  x;    // Registrador X
    u8  y;    // Registrador Y
    u8  p;    // Registrador de status (flags)

    int  cycles;
    bool nmi_pending;
    bool irq_pending;

    struct NES *nes;
} CPU;

void cpu_init (CPU *cpu, struct NES *nes);
void cpu_reset(CPU *cpu);
int  cpu_step (CPU *cpu);
void cpu_nmi  (CPU *cpu);
void cpu_irq  (CPU *cpu); 