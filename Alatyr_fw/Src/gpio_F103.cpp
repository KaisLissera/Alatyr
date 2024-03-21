/*
 * gpio.cpp
 *
 *  Created on: Aug 12, 2023
 *      Author: KONSTANTIN
 */

#include <gpio_F103.h>

//ezhgpio
/////////////////////////////////////////////////////////////////////

void gpio::SetupPin(GPIO_TypeDef* Gpio, uint8_t Pin, PinMode_t pinMode, PinPupd_t pinPupd){
	if(Pin < 8) {
		Gpio -> CRL &= ~(0b1111UL << (4*Pin));
		Gpio -> CRL |= pinMode << (4*Pin);
	}
	else{
		Gpio -> CRH &= ~(0b1111UL << (4*(Pin - 8)));
		Gpio -> CRH |= pinMode << (4*(Pin - 8));
	}
	Gpio->ODR |= pinPupd << Pin;
}

uint8_t gpio::GetPinInput(GPIO_TypeDef* gpio, uint32_t pin) {
	if((gpio->IDR & (0b1 << pin)) == 0)
		return 0;
	else
		return 1;
}

//Simple buttons
/////////////////////////////////////////////////////////////////////

ButtonState_t Button_t::CheckState() {
	uint8_t CurrentState = gpio::GetPinInput(Gpio, Pin);
	if(CurrentState == IdleState) {
		if(CurrentState != PreviousState) {
			PreviousState = CurrentState;
			return Released;
		}
		else
			return Idle;
	} else {
		if(CurrentState != PreviousState) {
			PreviousState = CurrentState;
			return Pressed;
		}
		else
			return HoldDown;
	}
}
