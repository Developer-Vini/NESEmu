#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;

// Dimensions
#define NES_WIDTH   256
#define NES_HEIGHT  240

// CPU Clock 
#define CPU_FREQ    1789773  //1.789.773Hz -> 1,79MHz

// IMM (Immediate) -> LDA #10 
// LDA (Zero Page) -> LDA $20 Busca na memoria entre $0000 - $00FF. Mais rapido!
// ABS (Absolute)  -> LDA $1234 Usa endereço completo
// ABX (AbsoluteX) -> LDA $1234,X Soma o registrador X.
// ABY (AbsoluteY) -> LDA $1234,Y Soma o registrador Y.
// IND (Indirect)  -> JMP ($1234) Busca um endereço dentro de outro endereço
// INX             -> LDA($20,X) Indexado por X
// INY             -> LDA ($20),Y Indexado por Y.
// REL             -> Usado para: BEQ, BNE e BMI. Saltos relativos
// IMP             -> CLC Não precisa de operador
// ACC             -> ASL A Opera diretamente no acumulador

typedef enum {
    IMP, ACC, IMM, ZPG, ZPX, ZPY, ABS, ABX, ABY, IND, INX, INY, REL
} AddrMode;

struct NES;
struct CPU;
struct PPU;
struct Cartridge;


extern const u32 NES_PALLETE[64];
