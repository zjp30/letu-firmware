set(IDF_TARGET esp32s3)

# 与 ESP32_GENERIC_S3 的差别：去掉 BLE、去掉 USB-CDC，只留 WiFi + UART REPL
# （编得快，且这版不需要蓝牙；USB 引脚 19/20 在你的板子上给光电用了）
set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    ${SDKCONFIG_IDF_VERSION_SPECIFIC}
    boards/sdkconfig.spiram_sx
    boards/LETU_S3/sdkconfig.board
)
