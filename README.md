<div align="center">

# 🎮 NESEmu

**Emulador de Nintendo Entertainment System (NES) desenvolvido em C**

[![C](https://img.shields.io/badge/Linguagem-C-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![SDL2](https://img.shields.io/badge/SDL2-0078d4?style=for-the-badge&logo=sdl2&logoColor=white)](https://www.libsdl.org/)
[![License](https://img.shields.io/badge/Licença-MIT-green?style=for-the-badge)](#licença)

</div>

---

## 📸 Screenshots

<div align="center">

![Screenshot 1](screenshots/Screenshot_1.png)
![Screenshot 2](screenshots/Screenshot_2.png)
![Screenshot 3](screenshots/Screenshot_3.png)
![Screenshot 4](screenshots/Screenshot_4.png)
![Screenshot 5](screenshots/Screenshot_5.png)
![Screenshot 6](screenshots/Screenshot_6.png)

</div>

---

## ✨ Funcionalidades

- **CPU 6502 Completa** — Todos os modos de endereçamento e instruções implementadas
- **PPU Otimizada** — Renderização de fundo e sprites com suporte a scrolling
- **Mappers** — Suporte a mappers 0, 1, 2, 3 e 4 (MMC1, MMC3)
- **Controle** — Suporte a teclado e gamepad para 2 jogadores
- **Screenshots** — Captura de tela em formato BMP com um toque

---

## 🛠️ Pré-requisitos

| Dependência | Versão |
|-------------|--------|
| GCC / Clang / MSVC | Qualquer versão recente |
| SDL2 | 2.0+ |
| Make (opcional) | GNU Make |

---

## 📦 Instalação

### Linux / macOS

```bash
# Clonar o repositório
git clone https://github.com/Developer-Vini/NESEmu.git
cd NESEmu

# Compilar
make

# Executar
./nes_emulator caminho/para/rom.nes
```

### Windows (MSYS2 / MinGW)

```bash
# Clonar o repositório
git clone https://github.com/Developer-Vini/NESEmu.git
cd NESEmu

# Compilar
make

# Executar
nes_emulator.exe caminho\para\rom.nes
```

### Windows (Visual Studio)

1. Abra o Visual Studio
2. Crie um novo projeto C
3. Adicione os arquivos em `src/` e `include/`
4. Configure o SDL2 nos diretórios de include/library
5. Compile e execute

---

## 🎮 Controles

### Jogador 1

| Tecla | Ação |
|:-----:|:----:|
| `Z` | A |
| `X` | B |
| `Enter` | Start |
| `Shift Direito` | Select |
| `↑ ↓ ← →` | D-Pad |

### Jogador 2

| Tecla | Ação |
|:-----:|:----:|
| `Q` | A |
| `W` | B |
| `1` | Start |
| `2` | Select |
| `T / G / F / H` | ↑ / ↓ / ← / → |

### Comandos Gerais

| Tecla | Ação |
|:-----:|:----:|
| `R` | Reset |
| `P` | Screenshot |
| `ESC` | Sair |

---

## 📁 Estrutura do Projeto

```
NESEmu/
├── include/            # Arquivos de cabeçalho
│   ├── bus.h           # Barramento e estrutura principal NES
│   ├── cartridge.h     # Cartucho e mappers
│   ├── cpu.h           # Processador 6502
│   ├── nes.h           # Tipos e constantes globais
│   └── ppu.h           # Unidade de processamento gráfico
├── src/                # Código fonte
│   ├── bus.c           # Implementação do barramento
│   ├── cartridge.c     # Carregamento e mappers de cartucho
│   ├── cpu.c           # Implementação da CPU 6502
│   ├── main.c          # Ponto de entrada e loop principal
│   ├── palette.c       # Paleta de cores NTSC do NES
│   └── ppu.c           # Implementação da PPU
├── screenshots/        # Capturas de tela do emulador
├── .gitignore
├── Makefile
└── README.md
```

---

## 🧩 Arquitetura

```
┌─────────────────────────────────────────────────┐
│                    Main Loop                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────────────┐  │
│  │  Input   │→│   CPU   │→│       PPU       │  │
│  │ Handler  │  │  6502   │  │  (262 scanlines │  │
│  └─────────┘  └────┬────┘  │   por frame)    │  │
│                     │       └────────┬────────┘  │
│                     ▼                ▼           │
│              ┌──────────────────────────┐        │
│              │         Bus              │        │
│              │  (Address Decoding)      │        │
│              └────────────┬─────────────┘        │
│                           │                      │
│              ┌────────────┴─────────────┐        │
│              │       Cartridge          │        │
│              │  PRG-ROM  │  CHR-ROM     │        │
│              └──────────────────────────┘        │
└─────────────────────────────────────────────────┘
```

---

## 📋 Mappers Suportados

| Mapper | Nome | Jogos Exemplos |
|:------:|:----:|:---------------|
| 0 | NROM | Super Mario Bros, Donkey Kong |
| 1 | MMC1 | Metroid, Legend of Zelda |
| 2 | UxROM | Mega Man, Castlevania |
| 3 | CNROM | Gradius, Arkanoid |
| 4 | MMC3 | Super Mario Bros 3, Kirby's Adventure |

---

## 🔧 Compilação Manual (sem Make)

```bash
gcc -O2 -Wall -Iinclude -o nes_emulator \
    src/main.c src/cpu.c src/ppu.c src/bus.c \
    src/cartridge.c src/palette.c \
    $(sdl2-config --cflags --libs) -lm
```

---

## 📄 Licença

Este projeto é para fins educacionais.

---

<div align="center">

**Desenvolvido com ❤️ por [Vinicius](https://github.com/Developer-Vini)**

</div>
