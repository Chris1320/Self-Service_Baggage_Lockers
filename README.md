# Baliuag University Library Lockers - An RFID-based Baggage System.

## Modules Used

- 1x LCD1602 Module
- 1x RC522 RFID Module
- 1x Membrane Switch Module
- 1x SG90 Servo Motor

## Third-Party Libraries Included

| Library           | Version | Link                                               |
| ----------------- | ------- | -------------------------------------------------- |
| Keypad            | v3.1.1  | https://playground.arduino.cc/Code/Keypad          |
| LiquidCrystal_I2C | v1.1.2  | https://github.com/marcoschwartz/LiquidCrystal_I2C |
| MFRC522           | v1.4.11 | https://github.com/miguelbalboa/rfid               |
| Servo             | v1.2.1  | https://www.arduino.cc/en/Reference/Servo          |

## Pin Layout

| Pin Number/Code | Description              |
| --------------- | ------------------------ |
|                 |                          |
| A0              | SG90 Servo Motor         |
| 1               | Membrane switch column 4 |
| 2               | Membrane switch column 3 |
| 3               | Membrane switch column 2 |
| 4               | Membrane switch column 1 |
| 5               | RFID RST                 |
| 6               | Membrane switch row 4    |
| 7               | Membrane switch row 3    |
| 8               | Membrane switch row 2    |
| 9               | Membrane switch row 1    |
| 10              | RFID SDA/SS              |
| 11              | RFID MOSI                |
| 12              | RFID MISO                |
| 13              | RFID SCK                 |
| SDA             | LCD1602 SDA              |
| SCL             | LCD1602 I2C SCL          |
