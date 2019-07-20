# LE2019 - CardioArt Lab

## Structure
- `esp32` firmware ของ ESP32 DOIT_DEVKIT_V1
- `udp_server` Python script สำหรับ server
- `gui` Python script ฝั่ง client

## การทำงาน
- ตั้งค่า ID, SSID, PWD, UDP, PORT ผ่าน AT Command บน ESP32
- ESP32 อ่านสัญญาณจาก ADS1292R ผ่าน SPI
- ESP32 ส่ง UDP packet หา server
- Server จัดเก็บ / เรียกดู Real-time
- GUI แสดงข้อมูล / Export / จัดกลุ่ม Arrhythmia