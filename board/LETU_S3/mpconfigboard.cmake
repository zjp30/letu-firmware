set(IDF_TARGET esp32s3)

# 顺序很重要：后面的片段覆盖前面的
# sdkconfig.ble 打开蓝牙；我们的 sdkconfig.board 最后生效，负责 8MB flash 和自定义分区表
set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.ble
    boards/sdkconfig.spiram_sx
    boards/LETU_S3/sdkconfig.board
)
