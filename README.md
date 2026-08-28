# Simon Says Game — MSP430

An embedded implementation of the classic **Simon Says** memory game, written in bare-metal
C for the **Texas Instruments MSP430G2553** microcontroller and the **Educational BoosterPack MkII**.

University project for the *Microcontrollers* course — Bachelor's Degree in Electronic, Robotics
and Mechatronics Engineering, **Universidad de Sevilla**.

**Author:** Jaime Esparragoso Vides
**Date:** January 2026

📄 Full technical report: [`docs/Proyecto_MSP430_Simon_Says_EN.pdf`](docs/Proyecto_MSP430_Simon_Says_EN.pdf)

---

## Hardware

| Component | Detail |
| --- | --- |
| MCU | MSP430G2553 (MSP‑EXP430G2 / G2ET LaunchPad) |
| Add‑on board | BOOSTXL‑EDUMKII Educational BoosterPack MkII |
| Display | Crystalfontz 128×128 LCD, ST7735 controller (SPI) |
| Inputs | 2‑axis joystick + joystick push‑button, buttons S1 / S2 |
| Output | Piezo buzzer (PWM) |

## Features

- **Finite State Machine** driving the whole game flow:
  `BIENVENIDA → MENSAJE_RONDA → MAQUINA → TURNO_JUGADOR → VICTORIA → VICTORIA_FINAL / FIN`
- **Pseudo‑random sequence** generated with a 16‑bit **LFSR** — no `stdlib.h` `rand()` used.
  The seed is taken from a free‑running tick counter at the exact moment the player presses Start,
  so every game is different.
- **Accessibility / colour‑blind mode:** hold the joystick left while pressing Start to switch
  the four colours for the symbols `#`, `@`, `$`, `%`.
- **Variable timing:** round speed is derived from a base time and shrinks as the game advances.
- **Hard mode:** clearing all 32 rounds unlocks a restart at double speed.
- **Cumulative scoring:** every correct step counts, even if you fail mid‑round.
- Welcome melody, per‑colour tones, victory jingle and a final star animation, all played on the
  buzzer via PWM.

## Peripheral configuration

| Peripheral | Use |
| --- | --- |
| `ADC10` | Joystick axes (channels A0 and A3), with a dead‑zone to avoid phantom moves |
| `Timer0_A` | 25 ms system tick (`LPM0` wake‑up), non‑blocking game timing |
| `Timer1_A` | PWM tone generation for the buzzer (compare mode) |
| SPI | LCD communication (handled by the graphics library) |
| Port 1 / Port 2 IRQ | Buttons S1, S2 and the joystick button |

The main loop keeps the CPU in low‑power mode `LPM0` and only wakes on interrupts.

## Repository layout

```
.
├── Proyecto/        Code Composer Studio project (import this folder)
│   ├── main.c                              game logic + FSM
│   ├── Crystalfontz128x128_ST7735.[ch]     LCD driver
│   ├── HAL_MSP430G2_Crystalfontz128x128_ST7735.[ch]   hardware abstraction layer
│   ├── UARTstdio.[ch]                      UART helper
│   ├── grlib.h / grlib_MKII.lib            TI Graphics Library (prebuilt for the MkII)
│   ├── lnk_msp430g2553.cmd                 linker command file
│   └── .ccsproject / .cproject / .project  CCS project metadata
├── docs/
│   ├── Proyecto_MSP430_Simon_Says_EN.pdf   compiled technical report
│   └── report/                             LaTeX sources (report.tex, main.c, img)
└── media/           Demo videos (added later)
```

## Build & flash

Built and tested with **Code Composer Studio 12.8.1** and the **TI MSP430 compiler v21.6.1.LTS**.

1. Clone the repository:
   ```
   git clone https://github.com/jaimeevv/simon-says-msp430.git
   ```
2. In CCS: **File → Import → C/C++ → CCS Projects**, select the `Proyecto/` folder and finish.
3. Make sure the *TI v21.6.1.LTS* MSP430 compiler is installed
   (**Window → Preferences → Code Composer Studio → Build → Compilers**). Any recent
   MSP430 TI compiler should also work.
4. Connect the LaunchPad, then **Project → Build** and **Run → Debug** (`Ctrl+B`, `F11`).

Everything the linker needs — including the prebuilt `grlib_MKII.lib` — is committed, so the
project builds straight after import with no extra downloads. Build artifacts live in
`Proyecto/Debug/` and are git‑ignored.

## How to play

1. Power on — the welcome screen plays a looping melody.
2. *(Optional)* hold the joystick **left** to enable symbol / colour‑blind mode.
3. Press the **joystick button** (Start).
4. Watch the sequence, then repeat it by tilting the joystick: **up / right / down / left**
   map to the four pads.
5. Each cleared round adds one step. Miss a step or run out of time → **Game Over**.
6. Clear all 32 rounds to trigger the final victory; press Start again for double‑speed hard mode.

## Media

Demo videos of the game running on hardware will be added to [`media/`](media/) later.

## License

The project code (`main.c`) is released under the [MIT License](LICENSE).

The LCD driver files (`Crystalfontz128x128_ST7735.*`, `HAL_MSP430G2_*`, `UARTstdio.*`),
`grlib.h` and `grlib_MKII.lib` are part of the **TI Graphics Library** and remain
© Texas Instruments Incorporated, distributed under TI's BSD‑style license. They are
included here only so the project builds out of the box.
