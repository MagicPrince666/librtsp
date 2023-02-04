# OrangePi PC2

## 硬件连接
| 连接设备       | name    | PIN | PIN | name     | 连接设备       |
|---------------|---------|-----|-----|----------|---------------|
| NC            | 3.3v    | 1   | 2   | 5v       | NC            |
| MPU6050 SDA   | SDA     | 3   | 4   | 5v       | NC            |
| MPU6050 SCL   | SCL     | 5   | 6   | GND      | NC            |
| SR04 Echo     | PA6     | 7   | 8   | PC5      | NC            |
| NC            | GND     | 9   | 10  | PC6      | NC            |
| 遥控接收器SBUS  | UART2_RX| 11  | 12  | PD14     | BTS7960 LPWM  |
| NC            | UART2_TX| 13  | 14  | GND      | NC            |
| 舵机PWM        | PA3     | 15  | 16  | PC4      | BTS7960 EN    |
| NC            | 3.3v    | 17  | 18  | PC7      | BTS7960 RPWM  |
| SONY PS2 CMMD | MOSI    | 19  | 20  | GND      | NC            |
| SONY PS2 DAT  | MISO    | 21  | 22  | PA2      | MPU6050 INT   |
| SONY PS2 CLK  | SCLK    | 23  | 24  | CS0      | SONY PS2 ATT  |
| NC            | GND     | 25  | 26  | PA21     | NC            |
| NC            | TWI1-SDA| 27  | 28  | TWI1-SCK | NC            |
| SR04 Trig     | PA7     | 29  | 30  | GND      | NC            |
| 编码器A相      | PA8     | 31  | 32  | PG8      | NC            |
| 编码器B相      | PA9     | 33  | 34  | GND      | NC            |
| BTS7960 R_IS  | PA10    | 35  | 36  | PG9      | NC            |
| BTS7960 L_IS  | PD11    | 37  | 38  | UART1_TX | 调试串口tx      |
| NC            | GND     | 39  | 40  | UART1_RX | 调试串口rx      |

## 编译

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_orangepi_pc2.cmake ..
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_orangepi_pc2.cmake -DCMAKE_BUILD_TYPE=Debug ..
```