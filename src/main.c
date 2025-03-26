#include "mik32_hal.h"
#include <mik32_memory_map.h>
#include "mik32_hal_pcc.h"
#include "mik32_hal_gpio.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_scr1_timer.h"
#include "uart_lib.h"
#include "xprintf.h"
#include "scr1_timer.h"

#define START_BIT_LENGTH 4500
#define ZERO_BIT_LENGTH 500 
#define ONE_BIT_LENGTH 1500
#define TOL 200

void SystemClock_Config();
void GPIO_Init();

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

    GPIO_Init();

    /* Разрешить прерывания по уровню для линии EPIC GPIO_IRQ */
    HAL_EPIC_MaskLevelSet(HAL_EPIC_GPIO_IRQ_MASK);
    /* Разрешить глобальные прерывания */
    HAL_IRQ_EnableInterrupts();

    while (1)
    {
	uint32_t timenow;
        timenow = SCR1_TIMER->MTIME;
        HAL_GPIO_TogglePin(GPIO_1, GPIO_PIN_15);
		for (volatile int i = 0; i < 100000; i++);
	if(ready)
		{
		xprintf("CMD: 0x%08X\r\n", cmd);
		ready = 0;
		}
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
}

void GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_IRQ_CLK_ENABLE();
    __HAL_PCC_EPIC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_INPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);
    HAL_GPIO_InitInterruptLine(GPIO_MUX_PORT1_15_LINE_7, GPIO_INT_MODE_CHANGE);
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
