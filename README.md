# NESEmu - Emulador NES em C

Emulador de Nintendo Entertainment System (NES) escrito em C com SDL2.
## Fotos




## Funcionalidades

- CPU 6502 completa com todos os modos de endereçamento
- PPU com renderização de fundo e sprites
- Suporte a mappers 0, 1, 2, 3 e 4 (MMC1, MMC3)
- Controle via teclado e gamepad

## Como compilar

### Pré-requisitos

- GCC ou qualquer compilador C
- SDL2

### Linux / macOS

```bash
make
```

### Windows (MSYS2 / MinGW)

```bash
make
```

## Como usar

```bash
./nes_emulator caminho/para/rom.nes
```

## Controles

### Jogador 1

| Tecla | Ação |
|-------|------|
| Z | A |
| X | B |
| Enter | Start |
| Shift Direito | Select |
| Setas | D-Pad |

### Jogador 2

| Tecla | Ação |
|-------|------|
| Q | A |
| W | B |
| 1 | Start |
| 2 | Select |
| T/G/F/H | Cima/Baixo/Esquerda/Direita |

### Geral

| Tecla | Ação |
|-------|------|
| R | Reset |
| P | Screenshot |
| ESC | Sair |

## Estrutura do projeto

```
nes_emulator/
├── include/        # Headers (.h)
│   ├── bus.h
│   ├── cartridge.h
│   ├── cpu.h
│   ├── nes.h
│   └── ppu.h
├── src/            # Código fonte (.c)
│   ├── bus.c
│   ├── cartridge.c
│   ├── cpu.c
│   ├── main.c
│   ├── palette.c
│   └── ppu.c
├── Makefile
└── README.md
```
