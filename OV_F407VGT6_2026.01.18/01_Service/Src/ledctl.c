#include "ledctl_port.h"
#include "ledctl.h"
#include <stddef.h>
#include "stdint.h"

#if LEDCTL_PWM_STEPS < 2U || LEDCTL_PWM_STEPS > 256U
    #error LEDCTL_PWM_STEPS_must_be_2_to_256
#endif
#define TICKS_PER_MS (1000U / LEDCTL_TICK_US)
#define MAX_MS       (UINT32_MAX / TICKS_PER_MS)
/* Gamma=2.2: D=b^2.2。表值由 round((i/256)^2.2*65535) 离线生成。
 * G在编译期换算为PWM档位，支持2..256级；运行时无浮点/pow。
 * 四舍五入量化，不强制最小亮度；暗部会有多个输入对应0占空比。
 */
#define G(v) (((v) * LEDCTL_PWM_STEPS + 32767U) / 65535U)
static const uint16_t s_gamma22[257] = {
    G(0U),     G(0U),     G(2U),     G(4U),     G(7U),     G(11U),    G(17U),    G(24U),    G(32U),    G(41U),
    G(52U),    G(64U),    G(78U),    G(93U),    G(110U),   G(128U),   G(147U),   G(168U),   G(191U),   G(215U),
    G(240U),   G(267U),   G(296U),   G(327U),   G(359U),   G(392U),   G(428U),   G(465U),   G(504U),   G(544U),
    G(586U),   G(630U),   G(676U),   G(723U),   G(772U),   G(823U),   G(875U),   G(930U),   G(986U),   G(1044U),
    G(1104U),  G(1165U),  G(1229U),  G(1294U),  G(1361U),  G(1430U),  G(1501U),  G(1574U),  G(1648U),  G(1725U),
    G(1803U),  G(1884U),  G(1966U),  G(2050U),  G(2136U),  G(2224U),  G(2314U),  G(2406U),  G(2500U),  G(2595U),
    G(2693U),  G(2793U),  G(2895U),  G(2998U),  G(3104U),  G(3212U),  G(3322U),  G(3433U),  G(3547U),  G(3663U),
    G(3781U),  G(3900U),  G(4022U),  G(4146U),  G(4272U),  G(4400U),  G(4530U),  G(4663U),  G(4797U),  G(4933U),
    G(5072U),  G(5212U),  G(5355U),  G(5499U),  G(5646U),  G(5795U),  G(5946U),  G(6099U),  G(6255U),  G(6412U),
    G(6572U),  G(6733U),  G(6897U),  G(7063U),  G(7231U),  G(7402U),  G(7574U),  G(7749U),  G(7926U),  G(8105U),
    G(8286U),  G(8469U),  G(8655U),  G(8843U),  G(9033U),  G(9225U),  G(9419U),  G(9616U),  G(9815U),  G(10016U),
    G(10219U), G(10425U), G(10632U), G(10842U), G(11054U), G(11269U), G(11486U), G(11705U), G(11926U), G(12149U),
    G(12375U), G(12603U), G(12833U), G(13066U), G(13301U), G(13538U), G(13777U), G(14019U), G(14263U), G(14509U),
    G(14758U), G(15009U), G(15262U), G(15517U), G(15775U), G(16035U), G(16298U), G(16563U), G(16830U), G(17099U),
    G(17371U), G(17645U), G(17922U), G(18201U), G(18482U), G(18765U), G(19051U), G(19339U), G(19630U), G(19923U),
    G(20218U), G(20516U), G(20816U), G(21119U), G(21424U), G(21731U), G(22040U), G(22352U), G(22667U), G(22984U),
    G(23303U), G(23624U), G(23949U), G(24275U), G(24604U), G(24935U), G(25269U), G(25605U), G(25943U), G(26284U),
    G(26628U), G(26973U), G(27322U), G(27672U), G(28026U), G(28381U), G(28739U), G(29100U), G(29462U), G(29828U),
    G(30196U), G(30566U), G(30939U), G(31314U), G(31692U), G(32072U), G(32454U), G(32840U), G(33227U), G(33617U),
    G(34010U), G(34405U), G(34802U), G(35202U), G(35605U), G(36010U), G(36417U), G(36827U), G(37240U), G(37655U),
    G(38072U), G(38493U), G(38915U), G(39340U), G(39768U), G(40198U), G(40631U), G(41066U), G(41503U), G(41944U),
    G(42387U), G(42832U), G(43280U), G(43730U), G(44183U), G(44639U), G(45097U), G(45557U), G(46020U), G(46486U),
    G(46954U), G(47425U), G(47899U), G(48374U), G(48853U), G(49334U), G(49818U), G(50304U), G(50793U), G(51284U),
    G(51778U), G(52275U), G(52774U), G(53276U), G(53780U), G(54287U), G(54796U), G(55308U), G(55823U), G(56341U),
    G(56860U), G(57383U), G(57908U), G(58436U), G(58966U), G(59499U), G(60035U), G(60573U), G(61114U), G(61657U),
    G(62203U), G(62752U), G(63303U), G(63857U), G(64414U), G(64973U), G(65535U)};
#undef G
static uint16_t GammaDuty(uint16_t brightness) {
    uint32_t index = ((uint32_t)brightness * 256U + LEDCTL_PWM_STEPS / 2U) / LEDCTL_PWM_STEPS;
    return s_gamma22[index];
}

typedef enum { MODE_OFF, MODE_ON, MODE_BLINK, MODE_BREATH } Mode;
typedef struct {
    LED_Handle_t *led;
    Mode mode;
    uint32_t first, second; /* 亮/灭ticks，或完整呼吸周期ticks */
    uint32_t elapsed;
    uint32_t ramp_acc; /* 分数累加器，避免在中断中除法 */
    uint16_t ramp;     /* 0 .. 2*PWM_STEPS-1的三角波相位 */
    uint16_t pwm_phase, duty;
    bool lit;
} Slot;
static Slot s_leds[LEDCTL_COUNT];

/* 保存并恢复原PRIMASK，禁止无条件开启调用者原已关闭的中断。
 * CMSIS内建函数由BSP的STM32 HAL头文件提供。
 */
static uint32_t Lock(void) {
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    __DMB();
    return saved;
}
static void Unlock(uint32_t saved) {
    __DMB();
    __set_PRIMASK(saved);
}
static bool ValidId(LEDCtl_Id_t id) { return (unsigned int)id < (unsigned int)LEDCTL_COUNT; }
static void Write(Slot *s, bool on) {
    if (s->lit != on) {
        if (on)
            LED_On(s->led);
        else
            LED_Off(s->led);
        s->lit = on;
    }
}
bool LEDCtl_Bind(LEDCtl_Id_t id, LED_Handle_t *hled) {
    unsigned int i;
    Slot *s;
    if (!ValidId(id) || hled == NULL || hled->GPIOx == NULL || hled->GPIO_Pin == 0U
        || (hled->GPIO_Pin & (hled->GPIO_Pin - 1U)) != 0U
        || (hled->ActiveLevel != LED_ACTIVE_LOW && hled->ActiveLevel != LED_ACTIVE_HIGH))
        return false;
    s = &s_leds[id];
    if (s->led == hled)
        return true;
    if (s->led != NULL)
        return false;
    for (i = 0; i < LEDCTL_COUNT; ++i) {
        LED_Handle_t *old = s_leds[i].led;
        if (old != NULL && (old == hled || (old->GPIOx == hled->GPIOx && old->GPIO_Pin == hled->GPIO_Pin)))
            return false;
    }
    LED_On(hled);
    s->led = hled;
    return true;
}
static bool Set(LEDCtl_Id_t id, Mode mode, uint32_t a, uint32_t b) {
    uint32_t saved;
    Slot *s;
    if (!ValidId(id))
        return false;
    saved = Lock();
    s     = &s_leds[id];
    if (s->led == NULL) {
        Unlock(saved);
        return false;
    }
    if (s->mode != mode || s->first != a || s->second != b) {
        s->mode      = mode;
        s->first     = a;
        s->second    = b;
        s->elapsed   = 0U;
        s->ramp_acc  = 0U;
        s->ramp      = 0U;
        s->pwm_phase = 0U;
        s->duty      = 0U;
        Write(s, mode == MODE_ON || mode == MODE_BLINK);
    }
    Unlock(saved);
    return true;
}
bool LEDCtl_On(LEDCtl_Id_t id) { return Set(id, MODE_ON, 0U, 0U); }
bool LEDCtl_Off(LEDCtl_Id_t id) { return Set(id, MODE_OFF, 0U, 0U); }
bool LEDCtl_Blink(LEDCtl_Id_t id, uint32_t on_ms, uint32_t off_ms) {
    if (on_ms == 0U || off_ms == 0U || on_ms > MAX_MS || off_ms > MAX_MS)
        return false;
    return Set(id, MODE_BLINK, on_ms * TICKS_PER_MS, off_ms * TICKS_PER_MS);
}
bool LEDCtl_Breath(LEDCtl_Id_t id, uint32_t period_ms) {
    if (period_ms < 100U || period_ms > MAX_MS)
        return false;
    return Set(id, MODE_BREATH, period_ms * TICKS_PER_MS, 0U);
}
void LEDCtl_TickISR(void) {
    unsigned int i;
    uint32_t saved = Lock();
    for (i = 0; i < LEDCTL_COUNT; ++i) {
        Slot *s = &s_leds[i];
        if (s->led == NULL)
            continue;
        if (s->mode == MODE_BLINK) {
            if (++s->elapsed >= (s->lit ? s->first : s->second)) {
                s->elapsed = 0U;
                Write(s, !s->lit);
            }
        } else if (s->mode == MODE_BREATH) {
            /* 无溢出的模加法；最多两步(256级、100ms周期时)。 */
            uint32_t add = 2U * LEDCTL_PWM_STEPS;
            while (add >= s->first - s->ramp_acc) {
                add -= s->first - s->ramp_acc;
                s->ramp_acc = 0U;
                if (++s->ramp == 2U * LEDCTL_PWM_STEPS)
                    s->ramp = 0U;
            }
            s->ramp_acc += add;
            /* 占空比在PWM帧边界锁存，防止一帧内随亮度变化。 */
            if (s->pwm_phase == 0U) {
                uint16_t brightness = s->ramp <= LEDCTL_PWM_STEPS ? s->ramp : 2U * LEDCTL_PWM_STEPS - s->ramp;
                s->duty             = GammaDuty(brightness);
            }
            Write(s, s->pwm_phase < s->duty);
            if (++s->pwm_phase == LEDCTL_PWM_STEPS)
                s->pwm_phase = 0U;
        }
    }
    Unlock(saved);
}
