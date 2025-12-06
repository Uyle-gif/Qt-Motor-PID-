import serial
import time
import random

PORT_NAME = 'COM9' 
BAUDRATE = 9600

# 5ms = 0.005 giây -> 200Hz
SEND_INTERVAL = 0.005 

motor_state = {
    "target": 0.0,
    "current": 0.0,
    "running": False,
    "dir": 1
}

def init_serial():
    try:
        ser = serial.Serial(PORT_NAME, BAUDRATE, timeout=0.01)
        print(f"Connected: {PORT_NAME} | Speed: {1/SEND_INTERVAL:.0f} Hz (5ms)")
        return ser
    except Exception as e:
        print(f"Error: {e}")
        exit()

def process_cmd(ser):
    if ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith("SPID"):
                parts = line.split()
                if len(parts) >= 5:
                    motor_state["target"] = float(parts[4])
                    motor_state["running"] = True
                    print(f"Start: {motor_state['target']}")
            elif line.startswith("STOP"):
                motor_state["running"] = False
                motor_state["target"] = 0.0
                print("Stop")
            elif line.startswith("MDIR"):
                motor_state["dir"] *= -1
                print("Reverse")
        except: pass

def main():
    ser = init_serial()
    last_time = time.perf_counter() 

    while True:
        try:
            process_cmd(ser)

            target = motor_state["target"] if motor_state["running"] else 0.0
            current = abs(motor_state["current"])
            
            inertia = 0.02 if motor_state["running"] else 0.01
            new_val = current + (target - current) * inertia
            motor_state["current"] = new_val * motor_state["dir"]

            now = time.perf_counter()
            if now - last_time >= SEND_INTERVAL:
                val_out = motor_state["current"] + random.uniform(-1, 1) if motor_state["running"] else 0
                
                msg = f"SPD {val_out:.2f}\n"
                ser.write(msg.encode())
                
                last_time = now

            time.sleep(0.0001) 

        except KeyboardInterrupt:
            break

if __name__ == "__main__":
    main()