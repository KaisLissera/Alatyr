/*
 * gpio.h
 *
 *  Created on: Aug 12, 2023
 *      Author: Kais Lissera
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

#include <stdint.h>
//
#include <lib_F103.h>
//
#include <rcc_F103.h>

//GPIO setup
/////////////////////////////////////////////////////////////////////

typedef enum{
	InputAnalog 			= 0b0000,
	InputFloating 			= 0b0100,
	InputWithPull 			= 0b1000,

	Output10MHzPushPull 	= 0b0001,
	Output10MHzOpenDrain 	= 0b0101,
	AfOutput10MHzPushPull 	= 0b1001,
	AfOutput10MHzOpenDrain 	= 0b1101,

	Output2MHzPushPull 		= 0b0010,
	Output2MHzOpenDrain 	= 0b0110,
	AfOutput2MHzPushPull 	= 0b1010,
	AfOutput2MHzOpenDrain 	= 0b1110,

	Output50MHzPushPull 	= 0b0011,
	Output50MHzOpenDrain 	= 0b0111,
	AfOutput50MHzPushPull 	= 0b1011,
	AfOutput50MHzOpenDrain 	= 0b1111,
} PinMode_t;

typedef enum{
	PullUp 		= 1,
	PullDown 	= 0
} PinPupd_t;

namespace gpio{
	void SetupPin(GPIO_TypeDef* gpio, uint8_t pin,
			PinMode_t pinMode, PinPupd_t pinPupd = PullDown);
	inline void ActivatePin(GPIO_TypeDef* gpio, uint32_t pin){
		gpio->BSRR = 0b1UL << pin;
	}
	inline void DeactivatePin(GPIO_TypeDef* gpio, uint32_t pin){
		gpio->BSRR = 0b1UL << (pin + 16);}
	inline void TogglePin(GPIO_TypeDef* gpio, uint32_t pin){
		gpio->ODR ^= 0b1UL << pin;
	}
	uint8_t GetPinInput(GPIO_TypeDef* gpio, uint32_t pin);
	inline void Afio1Remap(uint32_t afio1Remap){
		AFIO->MAPR |= afio1Remap;
	}
	inline void Afio2Remap(uint32_t afio2Remap){
		AFIO->MAPR2 |= afio2Remap;
	}
}//GPIO end

//Simple buttons
/////////////////////////////////////////////////////////////////////

typedef enum{
	Idle,
	HoldDown,
	Pressed,
	Released
} ButtonState_t;

class Button_t{
private:
	GPIO_TypeDef* Gpio;
	uint8_t Pin;
	uint8_t IdleState;
	uint8_t PreviousState;
public:
	Button_t(GPIO_TypeDef* _Gpio, uint8_t _Pin, PinPupd_t _PupdType){
		Gpio =_Gpio;
		Pin = _Pin;
		//
		switch(_PupdType) {
		case(PullUp):
			IdleState = 1;
			PreviousState = 1;
			break;
		case(PullDown):
			IdleState = 0;
			PreviousState = 0;
			break;
		default:
			ASSERT_SIMPLE(0);
		}
	}

	void Init(){
		gpio::SetupPin(Gpio,Pin,InputWithPull,PullUp);
	}
	ButtonState_t CheckState();
};

#endif /* INC_GPIO_H_ */
