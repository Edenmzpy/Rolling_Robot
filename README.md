# Rolling Robot — 全向无系留气腱耦合翻滚机器人

Omnidirectional Untethered Pneumatic–Tendon Coupled Rolling Robot

This repository contains the graduation project (B.Eng.) of **Zeng Yufeng (曾宇烽)** at **South China University of Technology (华南理工大学)**. The project designs and implements a fully untethered, omnidirectional rolling robot driven by a synergistic pneumatic–tendon coupling mechanism.

---

## Overview

Existing soft rolling robots typically rely on external air sources and tethered cables, which severely limit their range of motion and deployment flexibility. Most also adopt a single driving mode, restricting motion robustness.

This work addresses these issues by:

- **Regular icosahedron frame** — 12 soft legs mounted on 12 faces enable omnidirectional rolling through full geometric symmetry.
- **Pneumatic–tendon coupled soft leg** — PVC corrugated inner tube + three-section conical PLA sleeve. Tendons control contraction; compressed air regulates stiffness and output force.
- **Interconnected pneumatic circuit** — Three adjacent legs share an air circuit, allowing gas to flow from contracting legs into the extending leg. This reduces tendon tension in contracting legs (by ~34%) and boosts output force in the extending leg (nearly 3×).
- **"Two-contract-one-extend" gait** — An optimized strategy derived from the coupled mechanical model.
- **STM32 + FreeRTOS embedded system** — Dual-layer distributed architecture (controller ↔ robot body) with 5 concurrent tasks decoupled via message queues.
- **IMU-based state perception & motion decision** — Real-time ground-contact vertex identification, end-effector orientation, and velocity estimation.

---

## Repository Structure

```
Rolling_Robot/
├── Code/
│   └── RollingRobot/
│       ├── Controller/              # Handheld controller (STM32F103C8T6)
│       │   └── Controller/
│       │       ├── Core/            # HAL drivers, main.c
│       │       ├── Hardware/        # OLED, NRF24L01 drivers
│       │       └── MDK-ARM/         # Keil project files
│       └── RollingRobot/            # Robot body (STM32F407VET6 + FreeRTOS)
│           ├── Core/                # HAL drivers, FreeRTOS config, main.c
│           ├── Hardware/            # NRF24L01, OLED, IMU (WIT), PWM drivers
│           ├── Task/                # FreeRTOS tasks:
│           │   ├── nrf_recv.c/h     #   Wireless command reception
│           │   ├── nrf_send.c/h     #   Status telemetry
│           │   ├── pump_charge.c/h  #   Pressure closed-loop control (PID+soft PWM)
│           │   ├── servo_move.c/h   #   Gait execution & servo interpolation
│           │   └── wit_imu.c/h      #   IMU attitude estimation & motion decision
│           └── MDK-ARM/             # Keil project files
└── Mechanical_Design/
    └── Model/                       # SolidWorks 3D models (.SLDPRT, .SLDASM)
                                     # + STL/3MF files for 3D printing
```

---

## Key Specifications

| Parameter | Value |
|-----------|-------|
| Frame type | Regular icosahedron |
| Circumscribed sphere radius | 150 mm |
| Number of soft legs | 12 |
| Soft leg travel | 75 mm |
| Actuation | Tendon-driven (servo) + pneumatic |
| Main MCU | STM32F407VET6 (Cortex-M4, 168 MHz) |
| Controller MCU | STM32F103C8T6 (Cortex-M3) |
| RTOS | FreeRTOS |
| IMU | WIT 9-axis (quaternion output via AHRS) |
| Wireless | NRF24L01 (2.4 GHz, 2 Mbps) |
| Gait cycle | ~3 s per step |
| Max speed (flat, 15 kPa) | 0.267 m/s (peak window avg) |

---

## Performance Highlights

- **15 kPa** on flat ground → peak speed **0.267 m/s**, time-averaged speed **0.053 m/s**
- **20 kPa** → peak speed **0.267 m/s** (diminishing returns beyond 15 kPa)
- **1 kg payload** → continuous rolling maintained, 43% speed reduction
- **Terrain adaptability**: demonstrated on flat ground, gravel, and grass
- **34% reduction** in tendon tension via interconnected pneumatic circuit
- **3× boost** in output force for the extending leg

---

## How to Build

### Firmware

1. Open the corresponding Keil MDK project:
   - Robot body: `Code/RollingRobot/RollingRobot/MDK-ARM/`
   - Controller: `Code/RollingRobot/Controller/Controller/MDK-ARM/`
2. Build and flash via ST-Link.

### Mechanical Parts

3MF and STL files for 3D printing are in `Mechanical_Design/Model/`. SolidWorks source files (`.SLDPRT`, `.SLDASM`) are also provided for customization.

---

## Acknowledgments

**Advisor**: Prof. Li Yunquan (李云泉)

**Thesis committee**: South China University of Technology, School of Mechanical and Automotive Engineering

This work builds upon prior research by Fu et al. on untethered soft rolling robots with pneumatic–tendon coupled actuation (IEEE RA-L, 2024).

---

## License

This project is made available for academic and research purposes.
