#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f1xx_hal.h"

#define MODE_BTN_LOW       GPIO_PIN_0
#define MODE_BTN_MED       GPIO_PIN_1
#define MODE_BTN_HIGH      GPIO_PIN_2
#define MODE_BTN_PORT      GPIOC
#define ESTOP_PIN          GPIO_PIN_3
#define ESTOP_PORT         GPIOC

#define TEMP_ADC_CHANNEL   ADC_CHANNEL_0
#define CURRENT_ADC_CHANNEL ADC_CHANNEL_1

#define PWM_TIMER          &htim1
#define PWM_CHANNEL        TIM_CHANNEL_1

/* Function prototypes */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_TIM1_Init(void);

#endif /* __MAIN_H */
