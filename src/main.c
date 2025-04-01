#define ADC_CONFIG_SAH_TIME_S          8
#define ADC_CONFIG_SAH_TIME_M          (0x3F << ADC_CONFIG_SAH_TIME_S)

#include "mik32_hal_adc.h"
#include "mik32_hal.h"
#include <mik32_memory_map.h>
#include "mik32_hal_pcc.h"
#include "mik32_hal_gpio.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_scr1_timer.h"
#include "uart_lib.h"
#include "xprintf.h"
#include "scr1_timer.h"
#include "gpio.h"

#define START_BIT_LENGTH 4500
#define ZERO_BIT_LENGTH 500 
#define ONE_BIT_LENGTH 1500
#define TOL 200

#define ADC_THRESHOLD 2500


#define MIK32V2

ADC_HandleTypeDef hadc;
#define ADC_OFFSET      0
#define ADC_CHANNELS    1


void SystemClock_Config();
void GPIO_Init();
static void ADC_Init(void);
static void Scr1_Timer_Init(void);
static void GPIO_Config(void);

uint32_t adc_avg[ADC_CHANNELS] = { 0 };
uint32_t countADC = 0;


volatile uint32_t nachalo;
volatile uint32_t cmd;
volatile uint32_t count;
volatile uint32_t ready;
volatile uint32_t start;

int main()
{
    __HAL_SCR1_TIMER_ENABLE(); //Разрешаем работу системного таймера 
    SystemClock_Config();
	// Установка параметров UART0 - отладочный порт
	UART_Init(UART_0, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M |
						   UART_CONTROL1_M_8BIT_M, 0, 0);
	PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_UART_0_M;
    Scr1_Timer_Init();
    GPIO_Init();
    ADC_Init();

    /* Разрешить прерывания по уровню для линии EPIC GPIO_IRQ */
    HAL_EPIC_MaskLevelSet(HAL_EPIC_GPIO_IRQ_MASK);
    /* Разрешить глобальные прерывания */
    HAL_IRQ_EnableInterrupts();

    int16_t adc_corrected_value, adc_raw_value;

    while (1)
    {
        uint32_t timenow;
        timenow = SCR1_TIMER->MTIME;
        HAL_GPIO_TogglePin(GPIO_1, GPIO_PIN_15);

        for (volatile int i = 0; i < 100; i++);

        if(ready)
                {
		if (cmd == 0xA7580707)
		 {
			 xprintf("Button: Return\r\nCommand: STOP\r\n");
		 }
	        
		if (cmd == 0x9F600707)
                 {
                         xprintf("Button: UP\r\nCommand: MoveForward\r\n");
                 }

		if (cmd == 0x9E610707)
                 {
                         xprintf("Button: DOWN\r\nCommand: MoveBack\r\n");
                 }

		if (cmd == 0x9A650707)
                 {
                         xprintf("Button: LEFT\r\nCommand: MoveLeft\r\n");
                 }
                
		if (cmd == 0x9D620707)
                 {
                         xprintf("Button: RIGHT\r\nCommand: MoveRight\r\n");
                 }
//xprintf("CMD: 0x%08X\r\n", cmd);
                ready = 0;
                }

	
            HAL_ADC_SINGLE_AND_SET_CH(hadc.Instance, 0);
            adc_raw_value = HAL_ADC_WaitAndGetValue(&hadc);
            adc_corrected_value = adc_raw_value - ADC_OFFSET;
	    adc_avg[0] = (adc_avg[0] + adc_corrected_value) >> 1;

            if (countADC % 1000 == 0)
            {
                xprintf("ADC[%d]: %4d/%4d/%4d (%d,%03d V)\r\n", 
			0, adc_raw_value, adc_corrected_value, adc_avg[0],
                        ((adc_avg[0] * 1200) / 4095) / 1000,
                        ((adc_avg[0] * 1200) / 4095) % 1000);
	       if (adc_avg[0] > ADC_THRESHOLD)
	       {
		 HAL_GPIO_WritePin(GPIO_0, GPIO_PIN_10, 0);
		 xprintf("YOU SHALL NOT PASS!\r\n");
	       }
	       else 
		 HAL_GPIO_WritePin(GPIO_0, GPIO_PIN_10, 1);       
            }
        countADC++;
    }

}

void SystemClock_Config(void)
{
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;
    HAL_PCC_Config(&PCC_OscInit);
}

void GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_PCC_GPIO_0_CLK_ENABLE();
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_IRQ_CLK_ENABLE();
    __HAL_PCC_EPIC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_INPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);
    HAL_GPIO_InitInterruptLine(GPIO_MUX_PORT1_15_LINE_7, GPIO_INT_MODE_CHANGE);
    
    #ifdef MIK32V2
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    #endif
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);

    /* Настройка выводов АЦП */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_7 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);
}

static void Scr1_Timer_Init(void) 
{
    HAL_SCR1_Timer_Init(HAL_SCR1_TIMER_CLKSRC_INTERNAL, 0);
}

static void ADC_Init(void) 
{
    hadc.Instance = ANALOG_REG;
    hadc.Init.Sel = ADC_CHANNEL4;
    hadc.Init.EXTRef = ADC_EXTREF_OFF;
    HAL_ADC_Init(&hadc);
}


void trap_handler()
{
    if (EPIC_CHECK_GPIO_IRQ())
    {
        if (HAL_GPIO_LineInterruptState(GPIO_LINE_7))
	{
		uint32_t value = HAL_GPIO_ReadPin(GPIO_1, GPIO_PIN_15);
		if (value==1)
		{
			nachalo = SCR1_TIMER->MTIME;
		}
		else  
		{
			uint32_t konec = SCR1_TIMER->MTIME;
			uint32_t delta = (konec - nachalo)/32;

			if (delta > START_BIT_LENGTH - TOL && delta < START_BIT_LENGTH + TOL)
			{
				start = 1;
				ready = 0;
				cmd = 0;
			}
			if (start) 
			{	
				if (delta > ZERO_BIT_LENGTH - TOL && delta < ZERO_BIT_LENGTH + TOL)
				{	
					cmd = (cmd >> 1);
					count++;
				}

				if (delta > ONE_BIT_LENGTH - TOL && delta < ONE_BIT_LENGTH + TOL)
				{
					cmd = 0x80000000 | (cmd >> 1);
					count++;
				}
				if (count == 32)
				{
					count = 0;
					ready = 1;
					start = 0;
				}
			}
		}
        }
        HAL_GPIO_ClearInterrupts();
    }

    /* Сброс прерываний */
    HAL_EPIC_Clear(0xFFFFFFFF);
}

