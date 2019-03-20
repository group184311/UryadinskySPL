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

volatile uint16_t commutation = (uint16_t)1000; //тактов коммутации
volatile uint16_t pause = (uint16_t)1000; //тактов паузы
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
		TIM3->ARR = (uint16_t)1000;

		//Разрешение прерывания таймера по переполнению
		TIM3->DIER |= TIM_DIER_UIE;

		//Вкл прерывания таймера 3
		NVIC_EnableIRQ(TIM3_IRQn);
		TIM3->CR1 |= TIM_CR1_CEN;

	for(;;){
		if ((GPIOB->IDR & GPIO_IDR_IDR5) && what_btn_prs != 1) {
		TIM3->CNT = 0x0;
		//максимальное значение периода
		TIM3->ARR = 0xFFFF;
		//отключается разрешение прерывания по переполнению
		TIM3->DIER &= ~TIM_DIER_UIE;

		commutation = (uint16_t)1000;
		what_btn_prs = 1;
		}
		else if ((!(GPIOC->IDR & GPIO_IDR_IDR15)) && what_btn_prs == 0) {
			what_btn_prs = 2;
			commutation = (uint16_t)2000;
		}
		else if ((!(GPIOB->IDR & GPIO_IDR_IDR0)) && what_btn_prs == 0) {
			what_btn_prs = 3;
			commutation = (uint16_t)3000;
		}
		else if ((GPIOA->IDR & GPIO_IDR_IDR11) && what_btn_prs == 0) {
			what_btn_prs = 4;
			commutation = (uint16_t)4000;
		}
		else if ((!(GPIOB->IDR & GPIO_IDR_IDR5)) && what_btn_prs == 1) {
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
		else if ((GPIOC->IDR & GPIO_IDR_IDR15) && what_btn_prs == 2) {
			what_btn_prs = 0;
			commutation = (uint16_t)1000;
		}
		else if ((GPIOB->IDR & GPIO_IDR_IDR0) && what_btn_prs == 3) {
			what_btn_prs = 0;
			commutation = (uint16_t)1000;
		}
		else if ((!(GPIOA->IDR & GPIO_IDR_IDR11)) && what_btn_prs == 4) {
			what_btn_prs = 0;
			commutation = (uint16_t)1000;
		}
	}
}

void TIM3_IRQHandler(void){
	//Сброс флага переполнения таймера
	TIM3->SR &= ~TIM_SR_UIF;
	//Введение понятий комутация/пауза
	if (GPIOC->IDR & GPIO_IDR_IDR13){
		GPIOC->BSRR = GPIO_BSRR_BR13;
		TIM3->ARR = commutation;
	}
	else {
		GPIOC->BSRR = GPIO_BSRR_BS13;
		TIM3->ARR = pause;
	}
}
