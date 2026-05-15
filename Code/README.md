# Firmware workspace

The embedded software is split into two STM32CubeMX/Keil projects.

| Target | MCU | Project file | Responsibility |
|---|---|---|---|
| Robot body | STM32F407VET6 | [`RollingRobot/RollingRobot/MDK-ARM/RollingRobot.uvprojx`](RollingRobot/RollingRobot/MDK-ARM/RollingRobot.uvprojx) | Sensing, pressure regulation, gait control, servo actuation, and telemetry |
| Handheld controller | STM32F103C8T6 | [`RollingRobot/Controller/Controller/MDK-ARM/Controller.uvprojx`](RollingRobot/Controller/Controller/MDK-ARM/Controller.uvprojx) | User input, status display, and wireless command transmission |

## Robot application layer

The project-specific FreeRTOS code lives in [`RollingRobot/RollingRobot/Task/`](RollingRobot/RollingRobot/Task/):

| Source | Role |
|---|---|
| `wit_imu.c` | Parses IMU data and estimates pose, supporting vertices, direction, and speed |
| `nrf_recv.c` | Receives remote commands and publishes motion/pressure set points |
| `pump_charge.c` | Samples pressure and drives the pump with PID-assisted soft PWM |
| `servo_move.c` | Maps the requested gait to 12 servo channels and interpolates motion |
| `nrf_send.c` | Packages pressure, motion, and IMU telemetry for the controller |

Device drivers are under each target's `Hardware/` directory. STM32 HAL, CMSIS, and FreeRTOS middleware are generated or vendor-provided dependencies and remain alongside the project for reproducibility.

## Build hygiene

The `MDK-ARM/` directories contain only portable project inputs. Keil outputs (`.o`, `.axf`, `.map`, reports, and dependency files) and user-specific `.uvguix.*` settings are ignored. Verified prebuilt HEX files from the original project snapshot are kept separately in [`../firmware/`](../firmware/).
