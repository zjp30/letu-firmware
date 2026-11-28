# examples/main.py —— 阶段A 验收例程
# 上传方式：Mixly 文本模式点“上传”（ampy 会存成 main.py 并运行），
# 或 PC 上 ampy -p COM5 put examples/main.py main.py
import time
import letu

LED = 42          # 你的板子实际 LED 引脚

# 若某侧轮子转向相反，取消对应注释即可（1=正向，0=反转）
# letu.dir(letu.M1, 0)
# letu.dir(letu.M3, 0)

def demo():
    # 1) 闪灯：确认固件、Python、GPIO 三层都通
    letu.blink(LED, 3, 200)

    # 2) 四轮逐个点动：25% 占空比 0.6 秒，看哪个轮子转、转向对不对
    for n in (letu.M1, letu.M2, letu.M3, letu.M4):
        letu.motor(n, 25)
        time.sleep(0.6)
        letu.motor(n, 0)      # 0 = 两路全高刹车，和原 Arduino 库语义一致
        time.sleep(0.2)

    # 3) 两轮同转：speed>0 前进，<0 后退（麦轮会原地转）
    letu.run12(30, 30)
    time.sleep(1)
    letu.run12(-30, -30)
    time.sleep(1)

    # 4) 松手
    letu.stop()

if __name__ == "__main__":
    demo()
