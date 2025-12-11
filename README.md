# DC Motor PID Control (Qt GUI)

A real-time DC motor speed control system using PID algorithm on STM32F4, monitored via a Qt C++ PC application.

##  Key Features
- **PID Control:** Real-time speed stabilization with adjustable Kp, Ki, Kd parameters.
- **Visual Monitoring:** Live speed plotting, dashboard, and status indicators.
- **Fail-Safe Mechanism:**
  - Independent **Tx/Rx connection check**.
  - **Auto-Stop:** Microcontroller automatically stops the motor if the connection is lost for more than 3 seconds.

##  Communication Protocol
The system uses **UART (ASCII Strings)** for data exchange.

### 1. PC ➔ STM32 (Commands)
| Command | Description | Example Data |
| :--- | :--- | :--- |
| `C_ALV` | Keep-alive signal (sent periodically) | `C_ALV` |
| `M_STR` | Start motor with parameters | `M_STR [Kp] [Ki] [Kd] [SP]` |
| `M_STP` | Stop motor immediately | `M_STP` |
| `M_INV` | Reverse motor direction | `M_INV` |
| `M_FRE` | Set PWM Frequency (Hz) | `M_FRE 1000` |

### 2. STM32 ➔ PC (Feedback)
| Command | Description | Example Data |
| :--- | :--- | :--- |
| `S_ACK` | Connection Acknowledge (Rx OK) | `S_ACK` |
| `E_LST` | **Error:** Connection Lost (Tx Broken) | `E_LST` |
| `M_PLT` | Real-time speed for plotting | `M_PLT 120.5` |

##  How to Use
1. Flash firmware to **STM32** using Keil C.
2. Connect hardware wiring and plug in USB-TTL.
3. Run the **GUI Application** on PC.
4. Select the correct COM port and click **CONNECT**.
5. Enter PID parameters (Kp, Ki, Kd) & Setpoint, then click **START**.
