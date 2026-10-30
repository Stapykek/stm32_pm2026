#include <stdint.h>
#include <stm32f10x.h>
#include <stdbool.h>

#define DELAY_MIN   15625U // 64 гц (100000/64)
#define DELAY_MAX   64000000U // 1/64 гц (1000000*64)
#define DELAY_START 1000000U // в начале светодиод мигает с частотой 1Гц

static uint32_t blink_delay = DELAY_START;

bool is_a_pressed = false;
bool is_c_pressed = false;

#define DEBOUNCING_TIME_US 20000U
#define LISTENING_STEP_US 20000U

// старое
// void delay(uint32_t ticks) {
// 	for (int i=0; i<ticks; i++) {
// 		__NOP();
// 	}
// }

void delay_us(uint32_t us) {
	__asm volatile (
		"push {r0}\r\n"
		"mov R0, %0\r\n"
		"_loop:\r\n"
			"cmp R0, #0\r\n"
			"beq _exit\r\n"
			"sub R0, R0, #1\r\n"
			"nop\r\n"
			"b _loop\r\n"
		"_exit:\r\n"
		"pop {r0}\r\n"
		:: "r"(9*us) //for 72Mhz
	);
}

void led_init(void) {
	// включили тактирование порта C ввода/вывода
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	// настроили порт PC13 как Output
	GPIOC->CRH = GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13) | GPIO_CRH_MODE13_1;
}

void buttons_init(void){
	// включаем тактирование порта A ввода/вывода
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	//настроили порт PA0 как Input
	GPIOA->CRL = GPIOA->CRL & ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0) | GPIO_CRL_CNF0_1;
	// PA0 enable pull-up
	GPIOA->ODR |= GPIO_ODR_ODR0;
	//настроили порт PA1 как Input
	GPIOA->CRL = GPIOA->CRL & ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1) | GPIO_CRL_CNF1_1;
	// PA1 enable pull-up
	GPIOA->ODR |= GPIO_ODR_ODR1;
}

// проверка значения в регистре вывода порта PA0
bool is_a_button_pressed(void) {
	return !(GPIOA->IDR & GPIO_IDR_IDR0);
}

// проверка значения в регистре вывода порта PA1
bool is_c_button_pressed(void) {
	return !(GPIOA->IDR & GPIO_IDR_IDR1);
}

void increase_delay(void){
	if (blink_delay < DELAY_MAX){
		blink_delay *= 2;
	}
	if (blink_delay > DELAY_MAX){
		blink_delay = DELAY_MAX;
	}
}

void decrease_delay(void){
	if (blink_delay > DELAY_MIN){
		blink_delay /= 2;
	}
	if (blink_delay < DELAY_MIN){
		blink_delay = DELAY_MIN;
	}
}

void handle_buttons(void){
	bool a_now = is_a_button_pressed();
	// PA0 уменьшает частоту
	if(a_now && !(is_a_pressed)){
		delay_us(DEBOUNCING_TIME_US);
		if(is_a_button_pressed()){
			is_a_pressed = true;
			increase_delay();
		}
	} else if (!a_now && is_a_pressed){
		delay_us(DEBOUNCING_TIME_US);
		if(!is_a_button_pressed())
			is_a_pressed = false;
	}

	bool c_now = is_c_button_pressed();
	// PA1 увеличивает частоту
	if( c_now && !(is_c_pressed)){
		delay_us(DEBOUNCING_TIME_US);
		if(is_c_button_pressed()){
			is_c_pressed = true;
			decrease_delay();
		}
	} else if (!c_now && is_c_pressed){
		delay_us(DEBOUNCING_TIME_US);
		if(!is_c_button_pressed())
			is_c_pressed = false;
	}

}

void delay_with_buttons(uint32_t us){
	for(uint32_t i = 0; i < us; i += LISTENING_STEP_US){

		uint32_t curr_step = LISTENING_STEP_US;

		if (us - i < curr_step)
			curr_step = us - i;

		handle_buttons();
		delay_us(curr_step);
	}
}

int __attribute((noreturn)) main(void) {
	led_init();
	buttons_init();

    while (1) {
	   	GPIOC->ODR |= GPIO_ODR_ODR13;
		delay_with_buttons(blink_delay);
	    GPIOC->ODR &= ~GPIO_ODR_ODR13;
	    delay_with_buttons(blink_delay);
    }
}
