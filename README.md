# letu-fw —— LetuRobot ESP32-S3 的 MicroPython 固件仓库

这个仓库只做一件事：用 GitHub Actions 把 `letu` 这个 **C 用户模块（usermod）** 编进 MicroPython 固件，
产出一个能直接烧进 N8R2 板子的 `Letu_S3_lib.bin`。编好之后，Python 侧 `import letu` 就能直接控制电机。

## 目录结构

```
letu-fw/
├─ .github/workflows/build.yml       # 云编译入口（唯一触发器）
├─ board/LETU_S3/                    # 板卡定义，会被 cp 进 ports/esp32/boards/
│  ├─ mpconfigboard.cmake            # esp32s3 + sdkconfig 片段顺序
│  ├─ mpconfigboard.h                # 板名、UART REPL、I2C 引脚
│  ├─ sdkconfig.board                # 8MB flash + 自定义分区表
│  ├─ partitions-letu-8MiB.csv       # 分区表（app / font / vfs）
│  └─ board.json                     # 元信息，不参与编译
├─ usermod/letu/
│  ├─ micropython.cmake              # 3 行，把 mod_letu.c 挂到 usermod 目标
│  └─ mod_letu.c                     # 整个 letu 模块
└─ examples/main.py                  # 板端验收例程
```

`build.yml` 里硬编码的三个路径就是：`board/LETU_S3`、`board/LETU_S3/partitions-letu-8MiB.csv`、
`usermod/letu/micropython.cmake`。动结构必须同步改工作流。

## 版本配对（不能随意换）

| 组件 | 版本 | 说明 |
|---|---|---|
| MicroPython | v1.27.0 | 板子实际跑的版本 |
| ESP-IDF | v5.5.1 | 1.27 依赖 `esp_driver_touch_sens` 等 5.5 组件，5.2 编不过 |
| 工具链 | esp-14.2.0 | 随 IDF 5.5.1 安装 |

## 分区布局（8MB）

| 分区 | 类型 | 偏移 | 大小 | 用途 |
|---|---|---|---|---|
| nvs | data/nvs | 0x9000 | 0x6000 | 标定参数（预留） |
| phy_init | data/phy | 0xf000 | 0x1000 | RF 校准 |
| factory | app | 0x10000 | 0x390000 | 固件本体（到 0x3A0000 止） |
| font | data/spiffs | 0x3a0000 | 0x60000 | Mixly 的 HZK12.bin 落在这里 |
| vfs | data/fat | 0x400000 | 0x400000 | 文件系统 |

`font` 分区必须显式声明：MicroPython 启动时会先找名为 `vfs` 的分区，找不到才按剩余空间自建，
不声明就会被文件系统盖掉 0x3A0000 那 384KB 汉字库。

## 编好之后怎么接进 Mixly 4.0

1. `Letu_S3_lib.bin` 放进 `mixly\boards\default\micropython_esp32s3\build\`
2. 改同目录 `config.json`，新增一项板卡（key 形如 `micropython:esp32s3:letu`），
   烧录命令指向 `./build/Letu_S3_lib.bin`（0x0）+ `../micropython/build/HZK12.bin`（0x3A0000）
3. 重启 Mixly，下拉选这块板，点“初始化固件”
4. 串口看到 `>>>` 后执行 `import letu` 验证

## 当前 API（阶段A）

| 调用 | 说明 |
|---|---|
| `letu.blink(pin, count, period_ms)` | 闪灯，pin 是参数（你的板子是 42） |
| `letu.motor(no, speed)` | 单路电机，`no` 1~4，`speed` -100~100，**0 = 两路全高刹车** |
| `letu.run12(left, right)` | M1 / M2 同时驱动 |
| `letu.stop()` | 四路占空比清零（滑行），不是刹车 |
| `letu.dir(no, forward)` | 运行时翻转某路方向（1 正向，0 反转），不需要重编固件 |
| `letu.M1..M4` | 常量 1~4 |

引脚与你原 `LetuMotorRun.h` 完全一致：M1=5/6、M2=7/38、M3=21/10、M4=11/12，PWM 1000Hz / 8bit。

## 往 LetuRobot_esp32s3 完整库演进

阶段A 只证明「C 模块 → 固件 → import → 轮子转」这条链路。后续按原库逐块加进 `mod_letu.c`：

1. **编码器**：4 路 PCNT（{13,14}{15,16}{36,35}{40,39}，swapAB={1,0,1,0}），暴露 `letu.enc(n)` 或 `letu.speed(n)`
2. **巡线**：A 组 ADC1 1/2/3/4，B 组 ADC2 17/18/19/20（注意 19/20 是 USB D-/D+），暴露 `letu.adc(ch)` / `letu.line()`
3. **闭环**：把原 `LetuFollower` 的 PID 放进来，`letu.go(dist, speed)`、`letu.turn(angle)`
4. **标定**：nvs 分区已有，写 `letu.calib(...)`，掉电保存
5. **I2C 外设**：MPU6050 与 SH1106 OLED 的实际引脚确认后，在 `mpconfigboard.h` 改 `MICROPY_HW_I2C0_SDA/SCL`
6. **对象化**：内部再包一层 `letu.Car()`，方法式调用（`car.go()` / `car.stop()`），API 更贴近你原来的类

每次只改 `mod_letu.c`，push 上去 Actions 自动重编，下载 artifact 换 bin 即可，不用动板卡和分区表。
