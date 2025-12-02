# DC Motor PID Control System

A real-time GUI application built with **Qt C++** to monitor, tune, and control DC Motors via UART interface.

## 🚀 Features

* **Real-time Plotting:** High-performance graph for Motor Speed (RPM) and Error using `QCustomPlot`.
* **PID Tuning:** Live adjustment of Kp, Ki, Kd, and Setpoint.
* **Control:** Start, Stop, Reverse direction (`MDIR`), and set PWM Frequency (`SFRE`).
* **Data Export:** Save session data to `.csv` for analysis.
* **Optimized Performance:** Uses queued replotting and data sampling to prevent GUI freezing at high baudrates.

## 🔌 Communication Protocol

The GUI communicates with the MCU (STM32/Arduino) using ASCII strings via UART (Default: 9600/115200 baud).

### 1. GUI ➤ MCU (Commands)
| Command | Format | Example |
| :--- | :--- | :--- |
| **Set PID** | `SPID <Kp> <Ki> <Kd> <SetPoint>` | `SPID 2.5 0.1 0 100` |
| **Stop** | `STOP` | `STOP` |
| **Reverse** | `MDIR` | `MDIR` |
| **Set Freq** | `SFRE <Freq>` | `SFRE 1000` |

### 2. MCU ➤ GUI (Feedback)
The MCU must send the current speed in this exact format for the graph to update:
```text
SPD <Current_Speed>
