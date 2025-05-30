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

#include "stdlib.h"
#include "mik32_hal_timer32.h"

#define START_BIT_LENGTH 4500
#define ZERO_BIT_LENGTH 500 
#define ONE_BIT_LENGTH 1500
#define TOL 200

#define ADC_THRESHOLD 3500


#define MIK32V2

ADC_HandleTypeDef hadc;
#define ADC_OFFSET      0
#define ADC_CHANNELS    1

TIMER32_HandleTypeDef htimer32_1;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel3;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel1;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel2;

TIMER32_HandleTypeDef htimer32_2;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel2_beep;

static void Timer32_1_Init(void);
static void Timer32_2_Init(void);

void SystemClock_Config();
void GPIO_Init();
static void ADC_Init(void);
static void Scr1_Timer_Init(void);
static void GPIO_Config(void);

uint32_t adc_avg[ADC_CHANNELS] = { 0 };
uint32_t countADC = 0;

void DelayMs(uint32_t time_ms);

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
    UART_Init(UART_0, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M | UART_CONTROL1_M_8BIT_M, 0, 0);
    PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_UART_0_M;
    Scr1_Timer_Init();
    GPIO_Init();
    ADC_Init();

    Timer32_1_Init();
    HAL_Timer32_Channel_Enable(&htimer32_channel3);
    HAL_Timer32_Channel_Enable(&htimer32_channel1);
    HAL_Timer32_Channel_Enable(&htimer32_channel2);
    HAL_Timer32_Value_Clear(&htimer32_1);
    HAL_Timer32_Start(&htimer32_1);

    Timer32_2_Init();
    HAL_Timer32_Channel_Enable(&htimer32_channel2_beep);
    HAL_Timer32_Value_Clear(&htimer32_2);
    HAL_Timer32_Start(&htimer32_2);

    /* Разрешить прерывания по уровню для линии EPIC GPIO_IRQ */
    HAL_EPIC_MaskLevelSet(HAL_EPIC_GPIO_IRQ_MASK);
    /* Разрешить глобальные прерывания */
    HAL_IRQ_EnableInterrupts();

    uint32_t adc_corrected_value, adc_raw_value;

    // массивы для speaker

    static uint16_t elka [16]= {520, 880, 880, 780, 880, 700, 520, 520,  520, 880, 880, 780, 880, 700, 520, 520};
    static uint16_t timeelka [16]= {400,400,400,400,400,400,400,400,400,400,400,400,400,600,20,400};


    static uint16_t StarWars [39] = {392, 392, 392, 311, 466, 392, 311, 466, 392,
   				     587, 587, 587, 622, 466, 369, 311, 466, 392,
    				     784, 392, 392, 784, 739, 698, 659, 622, 659,
    				     415, 554, 523, 493, 466, 440, 466, 311, 369,
				     311, 466, 392};
    static uint16_t timeStarWars [39] = {350, 350, 350, 250, 100, 350, 250, 100, 700,
    					 350, 350, 350, 250, 100, 350, 250, 100, 700,
    					 350, 250, 100, 350, 250, 100, 100, 100, 450,
    					 150, 350, 250, 100, 100, 100, 450, 150, 350, 
					 250, 100, 750};

    static uint16_t MARIO[75] = {150, 300, 150, 150, 300, 600, 600, 450, 150, 300, 
	    			 300, 150, 150, 300, 210, 210, 150, 300, 150, 150, 
				 300, 150, 150, 450, 450, 150, 300, 300, 150, 150, 
				 300, 210, 210, 150, 300, 150, 150, 300, 150, 150, 
				 450, 150, 150, 150, 300, 150, 150, 150, 150, 150, 
				 150, 150, 0,   150, 150, 150, 300, 150, 300, 150, 
				 600, 150, 150, 150, 300, 150, 150, 150, 150, 150, 
				 150, 150, 300, 450, 600};

    HAL_Timer32_Channel_Init(&htimer32_channel1);
    HAL_Timer32_Channel_Init(&htimer32_channel3);
    HAL_Timer32_Channel_Init(&htimer32_channel2);

    while (1)
    {
        uint32_t timenow;
        timenow = SCR1_TIMER->MTIME;
        HAL_GPIO_TogglePin(GPIO_1, GPIO_PIN_15);

//для проверки спикера
        //                         for (int i = 0; i < 16; i++)
        //                         {
	//				 htimer32_2.Top = 32000000/elka[i];
        //                                 htimer32_channel2_beep.OCR = htimer32_2.Top >> 1;
    	//				 HAL_Timer32_Init(&htimer32_2);
        //                                 HAL_Timer32_Channel_Init(&htimer32_channel2_beep);
        //                                 DelayMs(timeelka[i]);
        //                               //xprintf("htimer32_2.Top = %d, htimer32_channel2.OCR = %d\r\n",
        //                               //htimer32_2.Top, htimer32_channel2_beep.OCR);
        //                         }

        if(ready)
         {
		if (cmd == 0xA7580707)  //кнопка return
		 {
			 if (htimer32_channel3.OCR > 0)
			 {
		    	     htimer32_channel3.OCR = 0; 
			     HAL_Timer32_Channel_Init(&htimer32_channel3);
			     htimer32_channel1.OCR += 640000;
			     HAL_Timer32_Channel_Init(&htimer32_channel1);
			     DelayMs(50);
			     htimer32_channel1.OCR = 0;
			     HAL_Timer32_Channel_Init(&htimer32_channel1);
			 }

			 if (htimer32_channel1.OCR > 0)
                         {
			     htimer32_channel1.OCR = 0;
                             HAL_Timer32_Channel_Init(&htimer32_channel1);
                             htimer32_channel3.OCR += 640000;
                             HAL_Timer32_Channel_Init(&htimer32_channel3);
                             DelayMs(50);
                             htimer32_channel3.OCR = 0;
                             HAL_Timer32_Channel_Init(&htimer32_channel3);
                         }
                         htimer32_channel3.OCR = 0;               //назад
                         htimer32_channel1.OCR = 0;               //вперед
	 	         HAL_Timer32_Channel_Init(&htimer32_channel3);
			 HAL_Timer32_Channel_Init(&htimer32_channel1);
//		         xprintf("Button: Return\r\nCommand: STOP\r\n");
			 cmd == 0;
		 }
	        
		if (cmd == 0x9F600707)   //кнопка вперед
                 {
			 if (htimer32_channel1.OCR < 640000)
			 {
                         	if (htimer32_channel3.OCR > 0)
                         	{
                               		htimer32_channel3.OCR =  htimer32_channel3.OCR - 128000;
                                	htimer32_channel1.OCR = 0;
                         	}
                         	else
                         	{
                                	htimer32_channel1.OCR =  htimer32_channel1.OCR + 128000;
                                	htimer32_channel3.OCR = 0;
                         	}
			  	HAL_Timer32_Channel_Init(&htimer32_channel3);
			  	HAL_Timer32_Channel_Init(&htimer32_channel1);
                         	cmd == 0;
			 }
//			 xprintf("Button: UP\r\nCommand: MoveForward\r\n");
                 }

		if (cmd == 0x9E610707)   //кнопка назад
                 {
			 if (htimer32_channel3.OCR < 640000)
			 {
			 	if (htimer32_channel1.OCR > 0)
                         	{	
                                	htimer32_channel1.OCR =  htimer32_channel1.OCR - 128000;
                                	htimer32_channel3.OCR = 0;
                         	}
                         	else
                         	{
                                	htimer32_channel3.OCR =  htimer32_channel3.OCR + 128000;
                                	htimer32_channel1.OCR = 0;
                         	}
			  	HAL_Timer32_Channel_Init(&htimer32_channel3);
			  	HAL_Timer32_Channel_Init(&htimer32_channel1);
                        	cmd == 0;
			 }
//			 xprintf("Button: BACK\r\nCommand: MoveBack\r\n");
                 }

		if (cmd == 0x9D620707)  //кнопка вправо
                 {
                         if (htimer32_channel2.OCR > 38400)
                         {
                                htimer32_channel2.OCR =  htimer32_channel2.OCR - 2400;
                                HAL_Timer32_Channel_Init(&htimer32_channel2);
				cmd == 0;
                         }
//			 xprintf("Button: LEFT\r\nCommand: MoveLeft\r\n");
                 }
                
                if (cmd == 0x9A650707)  //кнопка влево
                 {
                         if (htimer32_channel2.OCR < 57600)
                         {              
                                htimer32_channel2.OCR =  htimer32_channel2.OCR + 2400;
				HAL_Timer32_Channel_Init(&htimer32_channel2);
                                cmd == 0;
                         }
//			 xprintf("Button: RIGHT\r\nCommand: MoveRight\r\n");
                 }

		if (cmd == 0x97680707)  //кнопка Enter
                 {
			 htimer32_channel2.OCR = 48000;
			 HAL_Timer32_Channel_Init(&htimer32_channel2);
                         cmd == 0;
//		 	 xprintf("Button: ENTER\r\nCommand: CentralPosition\r\n");
                 }

 //xprintf("CMD: 0x%08X\r\n", cmd);   //для вывода любого принимаемого слова

             ready = 0;
         }
	
            HAL_ADC_SINGLE_AND_SET_CH(hadc.Instance, 0);
            adc_raw_value = HAL_ADC_WaitAndGetValue(&hadc);
            adc_corrected_value = adc_raw_value - ADC_OFFSET;
	    adc_avg[0] = (adc_avg[0]*1 + adc_corrected_value*1023) >> 10;

            if (countADC % 16000 == 0) 
	    {
		    	xprintf("ADC[%d]: %4d/%4d/%4d (%d,%03d V)\r\n", 
			0, adc_raw_value, adc_corrected_value, adc_avg[0],
                        ((adc_avg[0] * 1200) / 4095) / 1000,
                        ((adc_avg[0] * 1200) / 4095) % 1000);
	       		if (adc_avg[0] > ADC_THRESHOLD) 
			{
		 		HAL_GPIO_WritePin(GPIO_0, GPIO_PIN_10, 0);
		 		htimer32_channel1.OCR = 0;
                 		HAL_Timer32_Channel_Init(&htimer32_channel1);
                 		htimer32_channel3.OCR += 640000;
                 		HAL_Timer32_Channel_Init(&htimer32_channel3);
                 		DelayMs(2000);
                 		htimer32_channel3.OCR = 0;
                 		HAL_Timer32_Channel_Init(&htimer32_channel3);
//		 		xprintf("YOU SHALL NOT PASS!\r\n");
                                
				//speaker
            //                     for (int i = 0; i < 16; i++)
            //                     {
            //                             htimer32_2.Top = 32000000/elka[i];
            //                             htimer32_channel2_beep.OCR = htimer32_2.Top >> 1;
            //                             HAL_Timer32_Init(&htimer32_2);
            //                             HAL_Timer32_Channel_Init(&htimer32_channel2_beep);
            //                             DelayMs(timeelka[i]);
                                       //xprintf("htimer32_2.Top = %d, htimer32_channel2.OCR = %d\r\n",
                                       //htimer32_2.Top, htimer32_channel2_beep.OCR);
            //                     }
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

void DelayMs(uint32_t time_ms)
{
    volatile uint64_t t0 = (SCR1_TIMER->MTIME);
    while ((SCR1_TIMER->MTIME - t0)/32000 < time_ms);
}

static void Timer32_1_Init(void)
{
    htimer32_1.Instance = TIMER32_1;
    htimer32_1.Top = 640000;
    htimer32_1.State = TIMER32_STATE_DISABLE;
    htimer32_1.Clock.Source = TIMER32_SOURCE_PRESCALER;
    htimer32_1.Clock.Prescaler = 0;
    htimer32_1.InterruptMask = 0;
    htimer32_1.CountMode = TIMER32_COUNTMODE_FORWARD;
    HAL_Timer32_Init(&htimer32_1);

    htimer32_channel3.TimerInstance = htimer32_1.Instance;
    htimer32_channel3.ChannelIndex = TIMER32_CHANNEL_3;
    htimer32_channel3.PWM_Invert = TIMER32_CHANNEL_NON_INVERTED_PWM;
    htimer32_channel3.Mode = TIMER32_CHANNEL_MODE_PWM;
    htimer32_channel3.CaptureEdge = TIMER32_CHANNEL_CAPTUREEDGE_RISING;
    htimer32_channel3.OCR = 0;
    htimer32_channel3.Noise = TIMER32_CHANNEL_FILTER_OFF;
    HAL_Timer32_Channel_Init(&htimer32_channel3);

    htimer32_channel1.TimerInstance = htimer32_1.Instance;
    htimer32_channel1.ChannelIndex = TIMER32_CHANNEL_1;
    htimer32_channel1.PWM_Invert = TIMER32_CHANNEL_NON_INVERTED_PWM;
    htimer32_channel1.Mode = TIMER32_CHANNEL_MODE_PWM;
    htimer32_channel1.CaptureEdge = TIMER32_CHANNEL_CAPTUREEDGE_RISING;
    htimer32_channel1.OCR = 0;
    htimer32_channel1.Noise = TIMER32_CHANNEL_FILTER_OFF;
    HAL_Timer32_Channel_Init(&htimer32_channel1);

    htimer32_channel2.TimerInstance = htimer32_1.Instance;
    htimer32_channel2.ChannelIndex = TIMER32_CHANNEL_2;
    htimer32_channel2.PWM_Invert = TIMER32_CHANNEL_NON_INVERTED_PWM;
    htimer32_channel2.Mode = TIMER32_CHANNEL_MODE_PWM;
    htimer32_channel2.CaptureEdge = TIMER32_CHANNEL_CAPTUREEDGE_RISING;
    htimer32_channel2.OCR = 48000;
    htimer32_channel2.Noise = TIMER32_CHANNEL_FILTER_OFF;
    HAL_Timer32_Channel_Init(&htimer32_channel2);
}

static void Timer32_2_Init(void)
{   
    htimer32_2.Instance = TIMER32_2;
    htimer32_2.Top = 32000000/1000;
    htimer32_2.State = TIMER32_STATE_DISABLE;
    htimer32_2.Clock.Source = TIMER32_SOURCE_PRESCALER;
    htimer32_2.Clock.Prescaler = 0;
    htimer32_2.InterruptMask = 0;
    htimer32_2.CountMode = TIMER32_COUNTMODE_FORWARD;
    HAL_Timer32_Init(&htimer32_2);

    htimer32_channel2_beep.TimerInstance = htimer32_2.Instance;
    htimer32_channel2_beep.ChannelIndex = TIMER32_CHANNEL_2;
    htimer32_channel2_beep.PWM_Invert = TIMER32_CHANNEL_NON_INVERTED_PWM;
    htimer32_channel2_beep.Mode = TIMER32_CHANNEL_MODE_PWM;
    htimer32_channel2_beep.CaptureEdge = TIMER32_CHANNEL_CAPTUREEDGE_RISING;
    htimer32_channel2_beep.OCR = 32000000/1000/2;
    htimer32_channel2_beep.Noise = TIMER32_CHANNEL_FILTER_OFF;
    HAL_Timer32_Channel_Init(&htimer32_channel2_beep);
}


void GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_PCC_GPIO_0_CLK_ENABLE();
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_2_CLK_ENABLE();
    __HAL_PCC_GPIO_IRQ_CLK_ENABLE();
    __HAL_PCC_EPIC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_INPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_UP;
    HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);
    HAL_GPIO_InitInterruptLine(GPIO_MUX_PORT1_15_LINE_7, GPIO_INT_MODE_CHANGE);
    
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
    hadc.Init.Sel = ADC_CHANNEL0;
    hadc.Init.EXTRef = ADC_EXTREF_OFF;
    hadc.Init.EXTClb = ADC_EXTCLB_ADCREF; /* Выбор источника внешнего опорного напряжения: «1» - внешний вывод; «0» - настраиваемый ОИН */
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
