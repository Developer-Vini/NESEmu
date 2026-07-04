<div align="center">

<img src="screenshots/Screenshot_1.png" width="600">

<br>

# **NESEmu**

<br>

[![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white&labelColor=000000)](https://en.wikipedia.org/wiki/C_(programming_language))
[![SDL2](https://img.shields.io/badge/SDL2-1155EE?style=for-the-badge&logo=sdl2&logoColor=white&labelColor=000000)](https://www.libsdl.org/)

<br>

Emulador de Nintendo Entertainment System desenvolvido em C com SDL2.

</div>


---

## Visão Geral

NESEmu é um emulador de console NES (Nintendo Entertainment System) construído do zero em linguagem C. O projeto implementa a arquitetura completa do hardware original, incluindo o processador MOS 6502, a PPU (Picture Processing Unit) e o sistema de mappers para compatibilidade com diversos cartuchos.

O objetivo principal é educacional — fornecer uma referência clara e funcional de como um emulador de console clássico opera em nível de hardware.

---

## Capturas de Tela

<table>
  <tr>
    <td><img src="screenshots/Screenshot_2.png" width="350"></td>
    <td><img src="screenshots/Screenshot_3.png" width="350"></td>
  </tr>
  <tr>
    <td><img src="screenshots/Screenshot_4.png" width="350"></td>
    <td><img src="screenshots/Screenshot_5.png" width="350"></td>
  </tr>
</table>

---

## Funcionalidades

| Componente | Descrição |
|------------|-----------|
| **CPU 6502** | Implementação completa do processador MOS 6502 com todos os 151 opcodes e 13 modos de endereçamento |
| **PPU** | Unidade de processamento gráfico com renderização de backgrounds, sprites, scrolling e paleta de cores NTSC |
| **Mappers** | Suporte nativo a mappers 0 (NROM), 1 (MMC1), 2 (UxROM), 3 (CNROM) e 4 (MMC3) |
| **Controle** | Entrada via teclado e gamepad para dois jogadores simultâneos |
| **Screenshots** | Captura de tela em formato BMP com uma única tecla |

---

## Instalação

### Requisitos

- Compilador C (GCC, Clang ou MSVC)
- SDL2 >= 2.0
- GNU Make (opcional)

### Compilar e Executar

```bash
git clone https://github.com/Developer-Vini/NESEmu.git
cd NESEmu
make
./nes_emulator <rom.nes>
```

### Windows (MinGW / MSYS2)

```bash
make
nes_emulator.exe <rom.nes>
```

### Compilação Manual

```bash
gcc -O2 -Wall -Iinclude -o nes_emulator \
    src/main.c src/cpu.c src/ppu.c src/bus.c \
    src/cartridge.c src/palette.c \
    $(sdl2-config --cflags --libs) -lm
```

---

## Mapa de Teclado

<details>
<summary><b>Jogador 1</b></summary>

| Tecla | Função |
|:-----:|:------:|
| `Z` | A |
| `X` | B |
| `Enter` | Start |
| `Shift` | Select |
| `↑ ↓ ← →` | D-Pad |

</details>

<details>
<summary><b>Jogador 2</b></summary>

| Tecla | Função |
|:-----:|:------:|
| `Q` | A |
| `W` | B |
| `1` | Start |
| `2` | Select |
| `T / G / F / H` | ↑ / ↓ / ← / → |

</details>

<details>
<summary><b>Comandos do Sistema</b></summary>

| Tecla | Função |
|:-----:|:------:|
| `R` | Reset |
| `P` | Screenshot |
| `Esc` | Sair |

</details>

---

## Arquitetura

```
                    ┌─────────────────────────────┐
                    │           Main Loop          │
                    └──────────────┬──────────────┘
                                   │
              ┌────────────────────┼────────────────────┐
              │                    │                    │
              v                    v                    v
        ┌──────────┐        ┌─────────────┐       ┌──────────┐
        │  Input   │        │     CPU     │       │   PPU    │
        │ Handler  │───────>│  MOS 6502   │──────>│  (NTSC)  │
        └──────────┘        └──────┬──────┘       └────┬─────┘
                                   │                    │
                                   v                    │
                            ┌─────────────┐             │
                            │     Bus     │<────────────┘
                            │ (Decoding)  │
                            └──────┬──────┘
                                   │
                    ┌──────────────┴──────────────┐
                    │                             │
                    v                             v
             ┌─────────────┐             ┌─────────────┐
             │  Cartridge  │             │   RAM/IO    │
             │  PRG + CHR  │             │   2KB RAM   │
             └─────────────┘             └─────────────┘
```

---

## Referências Técnicas

| Recurso | Descrição |
|---------|-----------|
| [NESDev Wiki](https://www.nesdev.org/wiki/Nesdev_Wiki) | Documentação completa do hardware NES |
| [MOS 6502 Reference](http://www.6502.org/) | Referência do processador 6502 |
| [iNES Format](https://www.nesdev.org/wiki/INES) | Especificação do formato de ROM |

---

## Licença

Projeto educacional. Uso livre para fins de estudo e aprendizado.

---

<div align="center">

Feito por [Vinicius](https://github.com/Developer-Vini)

</div>
