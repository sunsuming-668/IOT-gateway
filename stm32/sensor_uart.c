/**
 * STM32G431 传感器采集 + UART发送
 * -----------------------------------------------
 * 硬件：蓝桥杯CT117E-M4开发板
 * 功能：ADC采集温度/光照 → UART2发送JSON给ESP8266
 * HAL库 + STM32CubeMX生成外设初始化
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

/* CubeMX生成的外设句柄 */
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

/* ADC通道定义（根据实际板子修改） */
#define ADC_CH_TEMP     ADC_CHANNEL_1   /* PA0 - 温度传感器 */
#define ADC_CH_LIGHT    ADC_CHANNEL_2   /* PA1 - 光敏电阻 */

#define VREF            3.3f
#define ADC_MAX         4095.0f
#define SEND_INTERVAL   2000            /* 发送间隔(ms) */

/* ADC读取（单次转换） */
static uint16_t ADC_Read(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank    = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

/* ADC值 → 温度(NTC近似线性换算，实际需查表) */
static float Calc_Temperature(uint16_t adc_val)
{
    float voltage = (float)adc_val / ADC_MAX * VREF;
    /* 简化公式：T = (V - 0.76) / 0.0025 + 25 (STM32内部温度传感器) */
    /* 或者板载NTC：T = adc_val * 0.1f (根据实际标定) */
    return voltage * 100.0f;  /* 占位，需根据实际传感器标定 */
}

/* 构建JSON并通过UART发送 */
static void Send_SensorData(float temp, uint16_t light_adc)
{
    char json[128];
    int len = snprintf(json, sizeof(json),
        "{\"device\":\"stm32_g431\",\"temp\":%.1f,\"humidity\":0,\"pressure\":0,\"light\":%d}\n",
        temp, light_adc);

    HAL_UART_Transmit(&huart2, (uint8_t *)json, len, 1000);
}

/* 主循环 */
void Sensor_Task(void)
{
    uint16_t adc_temp, adc_light;
    float temperature;
    char msg[64];

    sprintf(msg, "Sensor task started\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 500);

    while (1)
    {
        /* 1. 读取ADC */
        adc_temp  = ADC_Read(ADC_CH_TEMP);
        adc_light = ADC_Read(ADC_CH_LIGHT);

        /* 2. 换算温度 */
        temperature = Calc_Temperature(adc_temp);

        /* 3. 发送JSON */
        Send_SensorData(temperature, adc_light);

        /* 4. 串口调试打印 */
        sprintf(msg, "ADC: temp=%d light=%d\r\n", adc_temp, adc_light);
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 500);

        HAL_Delay(SEND_INTERVAL);
    }
}
