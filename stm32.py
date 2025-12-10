import serial
import time
import random
import re

# ================= CẤU HÌNH =================
# Nếu dùng Windows và com0com, hãy set port này là đầu kia của cặp COM
# Ví dụ: Qt nối COM1 thì Python nối COM2
SERIAL_PORT = 'COM9'  
BAUD_RATE = 115200

# Mô phỏng vật lý động cơ
TARGET_SPEED = 0.0
CURRENT_SPEED = 0.0
NOISE_LEVEL = 0.5  # Độ nhiễu tín hiệu

def parse_start_command(data_str):
    """
    Parse lệnh gửi từ Qt: M_STR + formatData("kp ki kd sp")
    Chuỗi nhận được sẽ có dạng: "M_STR10 0.5 0 100       ..."
    """
    try:
        # Bỏ header M_STR
        payload = data_str.replace("M_STR", "").strip()
        parts = payload.split()
        if len(parts) >= 4:
            kp = float(parts[0])
            ki = float(parts[1])
            kd = float(parts[2])
            sp = float(parts[3])
            return sp
    except Exception as e:
        print(f"Parse Error: {e}")
    return 0.0

def main():
    global TARGET_SPEED, CURRENT_SPEED
    
    print(f"--- VIRTUAL STM32 MOTOR STARTED ON {SERIAL_PORT} ---")
    print("Waiting for commands from Qt GUI...")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05)
    except serial.SerialException:
        print(f"Lỗi: Không thể mở cổng {SERIAL_PORT}. Hãy kiểm tra lại phần mềm COM ảo.")
        return

    last_time = time.time()

    try:
        while True:
            # 1. ĐỌC DỮ LIỆU TỪ GUI (PC -> STM32)
            if ser.in_waiting > 0:
                # Qt gửi chuỗi có padding space, nên đọc chunk lớn
                raw_data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                print(f"Received: {raw_data}")

                if "M_STR" in raw_data:
                    # Lệnh START kèm tham số PID và Setpoint
                    new_sp = parse_start_command(raw_data)
                    TARGET_SPEED = new_sp
                    print(f"-> COMMAND START: Setpoint = {TARGET_SPEED}")
                    
                elif "M_STP" in raw_data:
                    # Lệnh STOP
                    TARGET_SPEED = 0.0
                    print("-> COMMAND STOP")

                elif "M_INV" in raw_data:
                    print("-> COMMAND INVERT")

            # 2. MÔ PHỎNG VẬT LÝ (PID RESPONSE)
            # Dùng công thức quán tính bậc 1 để tạo đường cong tăng tốc
            # v = v + alpha * (target - v)
            dt = time.time() - last_time
            if dt > 0.02: # Update mỗi 20ms (50Hz)
                last_time = time.time()
                
                # Logic tăng tốc mềm (Inertia)
                step = (TARGET_SPEED - CURRENT_SPEED) * 0.1
                CURRENT_SPEED += step

                # Thêm nhiễu giả lập (Noise) để giống thật
                simulated_speed = CURRENT_SPEED + random.uniform(-NOISE_LEVEL, NOISE_LEVEL)

                # 3. GỬI DỮ LIỆU VỀ GUI (STM32 -> PC)
                # Format đúng như C++ yêu cầu: "M_PLT <float>\n"
                msg = f"M_PLT {simulated_speed:.2f}\n"
                ser.write(msg.encode('utf-8'))
                
                # In ra terminal python để debug chơi
                # print(f"Sent: {msg.strip()}")

            time.sleep(0.0005) # Sleep nhỏ để giảm tải CPU

    except KeyboardInterrupt:
        print("\nExiting...")
        ser.close()

if __name__ == "__main__":
    main()