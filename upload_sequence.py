import serial
import time

# ── Change COM3 to your port (check Arduino IDE → Tools → Port) ──
PORT = "COM4"
BAUD = 115200

commands = [
    "---CLEAR THE SETTING---",
    "CLEAR",
    
    "---SWIPE UP---",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1500",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE 250 800",
    "ADD DELAY 500",
    "ADD PRESS",
    "ADD DELAY 200",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD RELEASE",
    
    "---PENCET HOME 3X---",
    "ADD DELAY 1000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 2000",
    "ADD MOVE 260 860",
    "ADD DELAY 500",
    "ADD CLICK",
    "ADD DELAY 500",
    "ADD CLICK",
    "ADD DELAY 500",
    "ADD CLICK",
    
    "---BUKA APLIKASI PRESENI---",
    "ADD DELAY 1000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE 260 680",
    "ADD DELAY 500",
    "ADD CLICK",
    
    "---BUAT PRESENI---",
    "ADD DELAY 5000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE 260 380",
    "ADD DELAY 500",
    "ADD CLICK",
    
    "---BUAT PRESENSI---",
    "ADD DELAY 5000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1000",
    "ADD MOVE 260 770",
    "ADD DELAY 500",
    "ADD CLICK",
    
    "SAVE",
    "LIST",
]

s = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)  # wait for ESP32 ready

print(f"Connected to {PORT}, sending {len(commands)} commands...\n")

for cmd in commands:
    s.write((cmd + "\n").encode())
    time.sleep(0.3)
    response = s.read_all().decode(errors="ignore").strip()
    if response:
        print(f"  >> {response}")
    print(f"  Sent: {cmd}")

s.close()
print("\nDone! Sequence uploaded and saved.")
