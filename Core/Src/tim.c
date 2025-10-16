#include "main.h"
#include "stm32f1xx_hal.h"

TIM_HandleTypeDef htim1;

void MX_TIM1_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakConfig = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 719; // example ARR; code will adjust ARR dynamically for requested frequency
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        /* Initialization Error */
        while(1);
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while(1);
    }

    sBreakConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakConfig.DeadTime = 72;
    sBreakConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakConfig) != HAL_OK) {
        while(1);
    }
}
