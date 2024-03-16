/*
 * ezhtim.h
 *
 *  Created on: Aug 2, 2023
 *      Author: KONSTANTIN
 */

#ifndef INC_TIM_H_
#define INC_TIM_H_

#include <stdint.h>
//
#include <lib_F103.h>
//

typedef enum{
	DownCounter 	= 0,
	UpCounter 		= 1
} CounterDirection_t;

typedef enum{
	outputCompareFrozen 			= 0b000,
	outputCompareActiveOnMatch 		= 0b001,
	outputCompareInactiveOnMatch 	= 0b010,
	outputCompareToggle 			= 0b011,
	outputCompareForceInactive 		= 0b101,
	outputCompareForceActive 		= 0b110,
	outputComparePWM1 				= 0b110,
	outputComparePWM2 				= 0b111
} OutputCompare_t;

typedef enum{
	triggerOnReset,
	triggerOnEnable,
	triggerOnUpdate,
	triggerOnComparePulse,
	triggerOnCompareOC1REF,
	triggerOnCompareOC2REF,
	triggerOnCompareOC3REF,
	triggerOnCompareOC4REF
} MasterMode_t;

typedef enum{
	slaveModeDisabled,
	slaveModeEncoder1,
	slaveModeEncoder2,
	slaveModeEncoder3,
	slaveModeReset,
	slaveModeGated,
	slaveModeTrigger,
	slaveModeExternalClock,
	slaveModeResetTrigger
} SlaveMode_t;

typedef enum{
	Itr0,
	Itr1,
	Itr2,
	Itr3
} TriggerSelection_t;

class Timer_t {
private:
	TIM_TypeDef*  Timer;
public:
	// Frequency in Hz
	void Init(TIM_TypeDef*  _Timer, uint32_t currentTimerClock,  uint32_t Frequency, uint32_t ReloadValue, CounterDirection_t Dir) {
		Timer = _Timer;
		//Frequency
		Timer->PSC = (uint32_t)(currentTimerClock/Frequency) - 1;
		if (Timer == TIM1)
			TIM1->BDTR |= TIM_BDTR_MOE; // Main output enable
		if(Dir)
			Timer->CR1 |= TIM_CR1_DIR; // 1 - Up counter
		else
			Timer->CR1 &= ~TIM_CR1_DIR; // 0 - Down counter
		Timer->ARR = ReloadValue;
		Timer->EGR = TIM_EGR_UG; // Update event to reload prescaler value
	}

	void ConfigureChannel(uint8_t ChannelNumber, OutputCompare_t CompareType) {
		//Channel Compare Type
		switch (ChannelNumber) {
		case 1:
			Timer->CCMR1 |= (CompareType << TIM_CCMR1_OC1M_Pos);
			Timer->CCER |= TIM_CCER_CC1E;
			break;
		case 2:
			Timer->CCMR1 |= (CompareType << TIM_CCMR1_OC2M_Pos);
			Timer->CCER |= TIM_CCER_CC2E;
			break;
		case 3:
			Timer->CCMR2 |= (CompareType << TIM_CCMR2_OC3M_Pos);
			Timer->CCER |= TIM_CCER_CC3E;
			break;
		case 4:
			Timer->CCMR2 |= (CompareType << TIM_CCMR2_OC4M_Pos);
			Timer->CCER |= TIM_CCER_CC4E;
			break;
		default:
			ASSERT_SIMPLE(0); //Bad timer channel number
		}
	}

	void ChannelEnable(uint8_t channelNumber) {
		if(channelNumber > 4)
			return;
		Timer->CCER |= 0b1 << (channelNumber-1)*4;
	}

	void LoadCompareValue(uint8_t ChannelNumber, uint16_t OutputCompareValue) {
		switch (ChannelNumber) {
		case 1:
			Timer->CCR1 = OutputCompareValue;
			break;
		case 2:
			Timer->CCR2 = OutputCompareValue;
			break;
		case 3:
			Timer->CCR3 = OutputCompareValue;
			break;
		case 4:
			Timer->CCR4 = OutputCompareValue;
			break;
		default:
			ASSERT_SIMPLE(0); //Bad timer channel number
		} //switch end
	}

	void SetMasterMode(MasterMode_t MasterMode){
		Timer->CR2 |= MasterMode << TIM_CR2_MMS_Pos;
	}

	void SetOnePulseMode(uint32_t PulseNumber){
		Timer->RCR = PulseNumber - 1;
		Timer->CR1 |= TIM_CR1_OPM;
		Timer->EGR = TIM_EGR_UG; // Update event to reload counter value
	}

	void SetSlaveMode(SlaveMode_t SlaveMode, TriggerSelection_t TriggerSelection){
		Timer->SMCR |= SlaveMode << TIM_SMCR_SMS_Pos;
		Timer->SMCR |= TriggerSelection << TIM_SMCR_TS_Pos; // Change only if SMS=000
	}

	void StartCount() {
		Timer->CR1 |= TIM_CR1_CEN;
	}

	void StopCount(uint16_t LoadCounterValue = 0) {
		Timer->CR1 &= ~TIM_CR1_CEN;
		Timer->CNT = LoadCounterValue;
	}
}; // Timer_t end

#endif /* INC_TIM_H_ */
