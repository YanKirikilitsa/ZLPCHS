#include <mik32_memory_map.h>
#include <pad_config.h>
#include <gpio.h>
#include <power_manager.h>
#include <wakeup.h>

#include "uart_lib.h"
#include "xprintf.h"
#include "scr1_timer.h"

/*
* Данный пример предназначен для отлаочной платы NUKE MIK32 V0.3
*
* Данный пример демонстрирует работу с GPIO и PAD_CONFIG.
* В примере настраивается вывод, который подключенный к светодиоду, в режим GPIO. 
* Доступна функция ledButton, которая считывает состояние кнопки и зажигает светодиод.
*
*/

#define	MIK32V2		// Используется в HAL библиотеках для выбора способов инициализации аппаратуры


#define PIN_LED1 9	 // Светодиод управляется выводом PORT_0_9
#define PIN_LED2 10	 // Светодиод управляется выводом PORT_0_10
#define PIN_BUTTON 15	 // Кнопка на PORT_1_15


void InitClock(void)
{
	// включение тактирования GPIO
	PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_UART_0_M | PM_CLOCK_APB_P_GPIO_0_M | PM_CLOCK_APB_P_GPIO_1_M;

	// включение тактирования блока для смены режима выводов
	PM->CLK_APB_M_SET |= PM_CLOCK_APB_M_PAD_CONFIG_M | PM_CLOCK_APB_M_WU_M | PM_CLOCK_APB_M_PM_M;
}


void ledBlink(void)
{
	GPIO_0->OUTPUT ^= 1 << PIN_LED1; // Установка сигнала вывода 3 порта 0 в противоположный уровень
}

void ledButton(void)
{
	// Проверка состояния состояния входа 15 порта 1
	if (GPIO_1->STATE & (1 << PIN_BUTTON)) {
		GPIO_0->OUTPUT |= (1 << PIN_LED2); // Установка сигнала вывода 10 порта 0 в высокий уровень
	} else {
		GPIO_0->OUTPUT &= ~(1 << PIN_LED2); // Установка сигнала вывода 10 порта 0 в низкий уровень
	}
}

int main(void)
{
	InitClock(); // Включение тактирования GPIO

	// Установка параметров UART0 - отладочный порт
	UART_Init(UART_0, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M |
						   UART_CONTROL1_M_8BIT_M, 0, 0);

	// Установка вывода 9 порта 0 в режим вывод GPIO (LED1)
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_LED1)); // Сборос режимов для вывода 9
	GPIO_0->DIRECTION_OUT = (1 << PIN_LED1); // Установка направления вывода 3 порта 0 на выход

	// Установка вывода 10 порта 0 в режим вывод GPIO (LED2)
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_LED2)); // Сборос режимов для вывода 9
	GPIO_0->DIRECTION_OUT = (1 << PIN_LED2); // Установка направления вывода 3 порта 0 на выход

	// Установка вывода 15 порта 1 в режим вход GPIO (BUTTON)
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_BUTTON)); // Сборос режимов для вывода 15 
	GPIO_1->DIRECTION_IN = (1 << PIN_BUTTON); // Установка направления вывода 15 порта 1 на вход

	int counter = 0;

	while (1) {
		ledBlink(); /* Инвертируем светодиод LED1 */

		ledButton(); /* Зажигаем светодиод LED2 при нажатой кнопке */

		xprintf("Hello, world! Counter = %d\r\n", counter++);
	
		for (volatile int i = 0; i < 100000; i++);
	}
}

