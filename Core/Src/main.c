#include "main.h"
#include "stm32f1xx_hal.h"
#include <string.h>

float temp_setpoint = 150.0f;
float temp_input = 0.0f;
float pwm_duty = 0.0f;
float frequency_hz = 50000.0f;
float f_min = 30000.0f;
float f_max = 70000.0f;

float error = 0.0f;
float integral = 0.0f;
float Kp = 0.6f, Ki = 0.05f;
float integral_max = 200.0f;

float current_voltage = 0.0f;
float current_trip_threshold = 2.5f;

const float softstart_step = 1.0f;
const uint32_t softstart_delay_ms = 20;

typedef enum {HEAT_LOW, HEAT_MED, HEAT_HIGH} HeatMode;
HeatMode mode = HEAT_MED;

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;

void read_temperature_and_current(void);
void update_mode(void);
void pid_control(void);
void set_pwm(float duty, float freq);
void soft_start(float target_duty, float freq);
void emergency_stop(void);
uint8_t button_pressed_debounced(GPIO_TypeDef* port, uint16_t pin);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM1_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    soft_start(0.0f, frequency_hz);

    while (1)
    {
        if (HAL_GPIO_ReadPin(ESTOP_PORT, ESTOP_PIN) == GPIO_PIN_RESET) { emergency_stop(); }

        read_temperature_and_current();
        update_mode();
        pid_control();
        set_pwm(pwm_duty, frequency_hz);

        if (current_voltage > current_trip_threshold) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            HAL_Delay(50);
            while (1) { }
        }

        HAL_Delay(10);
    }
}

/* Implementations (same logic as earlier scaffold) */

void read_temperature_and_current(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = TEMP_ADC_CHANNEL;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
        float voltage = ((float)adc_val) * 3.3f / 4095.0f;
        temp_input = voltage * 100.0f;
    }
    HAL_ADC_Stop(&hadc1);

    sConfig.Channel = CURRENT_ADC_CHANNEL;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
        current_voltage = ((float)adc_val) * 3.3f / 4095.0f;
    }
    HAL_ADC_Stop(&hadc1);
}

uint8_t button_pressed_debounced(GPIO_TypeDef* port, uint16_t pin)
{
    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
            while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) { HAL_Delay(5); }
            return 1;
        }
    }
    return 0;
}

void update_mode(void)
{
    if (button_pressed_debounced(MODE_BTN_PORT, MODE_BTN_LOW)) {
        mode = HEAT_LOW; temp_setpoint = 100.0f; frequency_hz = 35000.0f;
    } else if (button_pressed_debounced(MODE_BTN_PORT, MODE_BTN_MED)) {
        mode = HEAT_MED; temp_setpoint = 150.0f; frequency_hz = 50000.0f;
    } else if (button_pressed_debounced(MODE_BTN_PORT, MODE_BTN_HIGH)) {
        mode = HEAT_HIGH; temp_setpoint = 200.0f; frequency_hz = 65000.0f;
    }
}

void pid_control(void)
{
    error = temp_setpoint - temp_input;
    integral += error * 0.01f;
    if (integral > integral_max) integral = integral_max;
    if (integral < -integral_max) integral = -integral_max;

    float output = Kp * error + Ki * integral;
    pwm_duty = output;
    if (pwm_duty > 90.0f) pwm_duty = 90.0f;
    if (pwm_duty < 0.0f) pwm_duty = 0.0f;

    frequency_hz += 0.02f * error;
    if (frequency_hz > f_max) frequency_hz = f_max;
    if (frequency_hz < f_min) frequency_hz = f_min;
}

void set_pwm(float duty, float freq)
{
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t compare = (uint32_t)((float)period * (duty / 100.0f));
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare);

    uint32_t prescaler = __HAL_TIM_GET_PRESCALER(&htim1);
    float timer_clk = (float)SystemCoreClock / (float)(prescaler + 1);
    uint32_t new_arr = (uint32_t)((timer_clk / freq)) - 1u;
    if (new_arr < 3u) new_arr = 3u;

    if (new_arr != period) {
        __HAL_TIM_DISABLE(&htim1);
        __HAL_TIM_SET_AUTORELOAD(&htim1, new_arr);
        uint32_t new_compare = (uint32_t)((float)new_arr * (duty / 100.0f));
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, new_compare);
        __HAL_TIM_ENABLE(&htim1);
    }
}

void soft_start(float target_duty, float freq)
{
    float cur = 0.0f;
    for (; cur <= target_duty; cur += softstart_step) {
        set_pwm(cur, freq);
        HAL_Delay(softstart_delay_ms);
    }
    set_pwm(target_duty, freq);
}

void emergency_stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_DISABLE(&htim1);
    while (1) { }
}
