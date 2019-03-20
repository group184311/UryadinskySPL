/**
  ******************************************************************************
  * @file    main.c
  * @author  Uryadindky Rus
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
#define BTN1PRS (GPIOB->IDR & GPIO_IDR_IDR5)
#define BTN2PRS (!(GPIOC->IDR & GPIO_IDR_IDR15))
#define BTN3PRS (!(GPIOB->IDR & GPIO_IDR_IDR0))
#define BTN4PRS (GPIOA->IDR & GPIO_IDR_IDR11)

// макроопределения режимов работы СД
#define DOLEDON GPIOC->IDR & GPIO_IDR_IDR13
#define LEDON GPIOC->BSRR = GPIO_BSRR_BR13
#define LEDOFF GPIOC->BSRR = GPIO_BSRR_BS13

//макроопределения времени паузы и коммутации
#define TIME_LED_STANDARD 	(uint16_t)1000
#define TIME_COMMUT_BTN2 	(uint16_t)2000
#define TIME_COMMUT_BTN3 	(uint16_t)3000
#define TIME_COMMUT_BTN4 	(uint16_t)4000

volatile uint16_t commutation = TIME_LED_STANDARD; //тактов коммутации
volatile uint16_t pause = TIME_LED_STANDARD; //тактов паузы
uint8_t what_btn_prs = 0;

int main(void)
{
	//Тактирование порта А, Б, Ц
	RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN);

	//Сброс состояния пинов
	GPIOA->CRH &= ~(GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
	GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0 |GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
	GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13 | GPIO_CRH_CNF15 | GPIO_CRH_MODE15);

	// Конфигурирование периферии А
	GPIOA->CRH |= GPIO_CRH_CNF11_1; //Кнопка 4
	GPIOA->ODR &= ~GPIO_ODR_ODR11;

	// Конфигурирование периферии Б
	GPIOB->CRL |= GPIO_CRL_CNF0_1; //Кнопка 3
	GPIOB->ODR |= GPIO_ODR_ODR0;

	GPIOB->CRL |= GPIO_CRL_CNF5_1;//Кнопка 1
	GPIOB->ODR &= ~GPIO_ODR_ODR5;

	// Конфигурирование периферии Ц
	GPIOC->CRH |= GPIO_CRH_MODE13_1;//Светодиод

	GPIOC->CRH |= GPIO_CRH_CNF15_1; //Кнопка 2
	GPIOC->ODR |= GPIO_ODR_ODR15;

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
		TIM3->ARR = commutation;
	}
	else {
		LEDOFF;
		TIM3->ARR = pause;
	}
}
