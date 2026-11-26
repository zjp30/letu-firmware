// usermod/letu/mod_letu.c —— 阶段A 固件里的最小 letu 模块
// 只做三件事：闪灯、单路电机、双路电机。全部自包含（不依赖 letu_motor 组件），
// 目的是先把「烧固件 → import letu → 轮子转」这条链路立起来。
#include "py/runtime.h"
#include "py/obj.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ---------------- 板级参数（和 letu_board.h 保持一致） ---------------- */
#define PWM_FREQ_HZ     1000        // 与 Arduino analogWrite 默认一致
#define PWM_BITS        LEDC_TIMER_8_BIT
#define PWM_MAX         255

static const int M_IN1[4] = {5, 7, 21, 11};
static const int M_IN2[4] = {6, 38, 10, 12};
static bool  s_dir[4]  = {true, true, true, true};
static int8_t s_ch[4][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

static void pwm_init_once(void)
{
    if (s_ch[0][0] >= 0) return;

    ledc_timer_config_t tim = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_BITS,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&tim);

    int ch = 0;
    for (int m = 0; m < 4; m++) {
        for (int k = 0; k < 2; k++) {
            int pin = (k == 0) ? M_IN1[m] : M_IN2[m];
            ledc_channel_config_t ccfg = {
                .gpio_num   = pin,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .channel    = (ledc_channel_t)ch,
                .intr_type  = LEDC_INTR_DISABLE,
                .timer_sel  = LEDC_TIMER_0,
                .duty       = 0,
                .hpoint     = 0,
            };
            ledc_channel_config(&ccfg);
            s_ch[m][k] = (int8_t)ch;
            ch++;
        }
    }
}

static void pwm_write(int ch, int duty)
{
    if (ch < 0) return;
    if (duty < 0) duty = 0;
    if (duty > PWM_MAX) duty = PWM_MAX;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, (uint32_t)duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch);
}

/* ---------------- letu.motor(no, speed)  1..4, -100..100 ---------------- */
static void motor_apply(int no, int speed)
{
    if (no < 1 || no > 4) return;
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;

    const int idx = no - 1;
    const int pwm = (speed < 0 ? -speed : speed) * PWM_MAX / 100;
    const bool fwd = s_dir[idx] ^ (speed > 0);

    if (speed == 0) {                       // 与原版一致：两路全高 = 刹车
        pwm_write(s_ch[idx][0], PWM_MAX);
        pwm_write(s_ch[idx][1], PWM_MAX);
        return;
    }
    if (fwd) {
        pwm_write(s_ch[idx][0], PWM_MAX);
        pwm_write(s_ch[idx][1], PWM_MAX - pwm);
    } else {
        pwm_write(s_ch[idx][0], PWM_MAX - pwm);
        pwm_write(s_ch[idx][1], PWM_MAX);
    }
}

static mp_obj_t letu_motor(mp_obj_t no_in, mp_obj_t spd_in)
{
    pwm_init_once();
    motor_apply((int)mp_obj_get_int(no_in), (int)mp_obj_get_int(spd_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(letu_motor_obj, letu_motor);

/* ---------------- letu.run12(l, r) ---------------- */
static mp_obj_t letu_run12(mp_obj_t l_in, mp_obj_t r_in)
{
    pwm_init_once();
    motor_apply(1, (int)mp_obj_get_int(l_in));
    motor_apply(2, (int)mp_obj_get_int(r_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(letu_run12_obj, letu_run12);

/* ---------------- letu.stop()  四路占空比清零 ---------------- */
static mp_obj_t letu_stop(void)
{
    pwm_init_once();
    for (int m = 0; m < 4; m++) {
        pwm_write(s_ch[m][0], 0);
        pwm_write(s_ch[m][1], 0);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(letu_stop_obj, letu_stop);

/* ---------------- letu.dir(no, forward) ---------------- */
static mp_obj_t letu_dir(mp_obj_t no_in, mp_obj_t fwd_in)
{
    int no = (int)mp_obj_get_int(no_in);
    if (no < 1 || no > 4) mp_raise_ValueError(MP_ERROR_TEXT("motor 1..4"));
    s_dir[no - 1] = mp_obj_get_int(fwd_in) != 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(letu_dir_obj, letu_dir);

/* ---------------- letu.blink(pin, count, period_ms) ---------------- */
static mp_obj_t letu_blink(mp_obj_t pin_in, mp_obj_t cnt_in, mp_obj_t ms_in)
{
    int pin = (int)mp_obj_get_int(pin_in);
    int cnt = (int)mp_obj_get_int(cnt_in);
    int ms  = (int)mp_obj_get_int(ms_in);

    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    for (int i = 0; i < cnt; i++) {
        gpio_set_level(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(ms / 2));
        gpio_set_level(pin, 0);
        vTaskDelay(pdMS_TO_TICKS(ms / 2));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(letu_blink_obj, letu_blink);

/* ---------------- 模块表 ---------------- */
static const mp_rom_map_elem_t letu_globals[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_letu) },
    { MP_ROM_QSTR(MP_QSTR_blink),    MP_ROM_PTR(&letu_blink_obj) },
    { MP_ROM_QSTR(MP_QSTR_motor),    MP_ROM_PTR(&letu_motor_obj) },
    { MP_ROM_QSTR(MP_QSTR_run12),    MP_ROM_PTR(&letu_run12_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),     MP_ROM_PTR(&letu_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_dir),      MP_ROM_PTR(&letu_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_M1),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_M2),       MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_M3),       MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_M4),       MP_ROM_INT(4) },
};
static MP_DEFINE_CONST_DICT(letu_dict, letu_globals);

const mp_obj_module_t mp_module_letu = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&letu_dict,
};

MP_REGISTER_MODULE(MP_QSTR_letu, mp_module_letu);
