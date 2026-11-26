#define MICROPY_HW_BOARD_NAME               "LetuRobot ESP32-S3"
#define MICROPY_HW_MCU_NAME                 "ESP32S3"

// REPL 走 UART0（GPIO43/44），方便和你的 UART 通信共用一根线
#define MICROPY_HW_ENABLE_UART_REPL         (1)

// I2C0 默认引脚（OLED / MPU6050 你确认实际引脚后改这里）
#define MICROPY_HW_I2C0_SCL                 (9)
#define MICROPY_HW_I2C0_SDA                 (8)
