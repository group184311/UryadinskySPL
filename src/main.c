/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
Конфигурация кнопки №1: порт GPIOB, вывод PIN5, внутренняя подтяжка к земле;
Конфигурация кнопки №2: порт GPIOC, вывод PIN15, внутренняя подтяжка к питанию;
Время коммутации при зажатой кнопке №2: 1000 мс;
Конфигурация кнопки №3: порт GPIOB, вывод PIN0, внутренняя подтяжка к питанию;
Время коммутации при зажатой кнопке №3: 1500 мс;
Конфигурация кнопки №4: порт GPIOA, вывод PIN11, внутренняя подтяжка к земле;
Время коммутации при зажатой кнопке №4: 2000 мс.
*/


#include "stm32f10x.h"

// макроопределения кнопок
#define BTN1PRS (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5))
#define BTN2PRS (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15))
#define BTN3PRS (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0))
#define BTN4PRS (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11))

// макроопределения режимов работы СД
#define DOLEDON GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13)
#define LEDON GPIO_SetBits(GPIOC, GPIO_Pin_13)
#define LEDOFF GPIO_ResetBits(GPIOC, GPIO_Pin_13)

//макроопределения времени паузы и коммутации
#define TIME_LED_STANDARD 	(uint16_t)1000
#define TIME_COMMUT_BTN2 	(uint16_t)2000
#define TIME_COMMUT_BTN3 	(uint16_t)3000
#define TIME_COMMUT_BTN4 	(uint16_t)4000

uint16_t *pcommutation;
uint16_t *ppause;

int main(void)
{
	volatile uint16_t commutation = TIME_LED_STANDARD; //тактов коммутации
	volatile uint16_t pause = TIME_LED_STANDARD; //тактов паузы
	pcommutation = &commutation;
	ppause = &pause;
	//абстрактный флаг, алгоритм выполняется только при смене его значения (см. в главном цикле)
	uint8_t what_btn_prs = 0;

	//Тактирование порта А, Б, Ц
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef gpio_init; //экземпляр структуры для конфигурирования ГПИО

	// Конфигурирование периферии А
	gpio_init.GPIO_Pin = GPIO_Pin_11;
	gpio_init.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &gpio_init);

	// Конфигурирование периферии Б
	gpio_init.GPIO_Pin = GPIO_Pin_0;
	gpio_init.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &gpio_init);

	gpio_init.GPIO_Pin = GPIO_Pin_5;
	gpio_init.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOB, &gpio_init);

	// Конфигурирование периферии Ц
	gpio_init.GPIO_Pin = GPIO_Pin_13;
	gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio_init);

	gpio_init.GPIO_Pin = GPIO_Pin_15;
	gpio_init.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &gpio_init);

	//Тактирование таймера ТИМ3
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	//Частота работы таймера 1 KГц
	TIM3->PSC = 36000;

	//Период таймера
	TIM3->ARR = TIME_LED_STANDARD;

	//Разрешение прерывания таймера по переполнению
	TIM3->DIER |= TIM_DIER_UIE;

	//Вкл прерывания таймера 3
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->CR1 |= TIM_CR1_CEN;

	for(;;) {
		if (BTN1PRS && what_btn_prs != 1) {
			TIM3->CNT = 0x0;
			//максимальное значение периода
			TIM3->ARR = 0xFFFF;
			//отключается разрешение прерывания по переполнению
			TIM3->DIER &= ~TIM_DIER_UIE;

			commutation = TIME_LED_STANDARD;
			what_btn_prs = 1;
		}
		else if (BTN2PRS && what_btn_prs == 0) {
			what_btn_prs = 2;
			commutation = TIME_COMMUT_BTN2;
		}
		else if (BTN3PRS && what_btn_prs == 0) {
			what_btn_prs = 3;
			commutation = TIME_COMMUT_BTN3;
		}
		else if (BTN4PRS && what_btn_prs == 0) {
			what_btn_prs = 4;
			commutation = TIME_COMMUT_BTN4;
		}
		else if (!BTN1PRS && what_btn_prs == 1) {
			uint16_t now_pause = TIM3->CNT;
			if (now_pause > 300) {
				pause = now_pause;
			}
			//включается разрешение прерывания по переполнению
			TIM3->DIER |= TIM_DIER_UIE;
			what_btn_prs = 0;
			//принудительная генерация события (иначе придется ждать, пока таймер
			//не дотикает до макс значения, а это противоречит заданию)
			TIM3->EGR = 0x0001;
		}
		else if (!BTN2PRS && what_btn_prs == 2) {
			what_btn_prs = 0;
			commutation = TIME_LED_STANDARD;
		}
		else if (!BTN3PRS && what_btn_prs == 3) {
			what_btn_prs = 0;
			commutation = TIME_LED_STANDARD;
		}
		else if (!BTN4PRS && what_btn_prs == 4) {
			what_btn_prs = 0;
			commutation = TIME_LED_STANDARD;
		}
	}
}

void TIM3_IRQHandler(void){
	//Сброс флага переполнения таймера
	TIM3->SR &= ~TIM_SR_UIF;
	//Введение понятий комутация/пауза
	if (DOLEDON){
		LEDON;
		TIM3->ARR = *pcommutation;
	}
	else {
		LEDOFF;
		TIM3->ARR = *ppause;
	}
}
