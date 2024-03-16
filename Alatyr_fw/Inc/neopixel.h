/*
 * neopixel.h
 *
 *  Created on: Mar 15, 2024
 *      Author: KONSTANTIN
 */

#ifndef NEOPIXEL_H_
#define NEOPIXEL_H_

#include <stdint.h>
#include <lib_F103.h>
#include <rcc_F103.h>
#include <tim_F103.h>

#define NPX_TIM_FREQUECY 	8000000  	// 1 tick = 125ns
#define NPX_ARR 			9  			// timer reload after 125ns*(9+1) = 1250ns
#define NPX_LOW 			2  			// 125ns*(2+1) = 375ns
#define NPX_HIGH 			5 			// 125ns*(5+1) = 750ns

class Neopixel_t{
protected:
	TIM_TypeDef* Timer;
	uint8_t TimerChannelNumber;
	DmaChannel_t* Dma;
	uint8_t* Buffer;
	uint16_t StripLength;
public:
	Neopixel_t(TIM_TypeDef* timer, uint8_t timNumber,
			DmaChannel_t* channel, uint8_t* buffer, uint16_t stripLength){
		Dma = channel;
		Buffer = buffer;
		StripLength = stripLength;
		Timer = timer;
		TimerChannelNumber = timNumber;
	}
	void Init(uint32_t currentTimerClock, uint8_t dmaIrqPrio) {
		// Setup TIM parameters
		Timer->PSC = (uint32_t)(currentTimerClock/8000000) - 1; // 8 MHz counter clock
		Timer->CR1 |= TIM_CR1_DIR; // Up counter
		Timer->ARR = NPX_ARR; // Timer update frequency 800 kHz
		Timer->EGR = TIM_EGR_UG; // Update event to reload prescaler value
		// Enable channel output and set compare type
		Timer->CCMR1 |= (outputComparePWM1 << TIM_CCMR1_OC1M_Pos); //TEMP!!
		Timer->CCER |= TIM_CCER_CC1E; //TEMP!!
		Timer->CCER |= 0b1 << (TimerChannelNumber-1)*4; // Output compare enable output
		if (Timer == TIM1)
			TIM1->BDTR |= TIM_BDTR_MOE; // Main output enable
		TIM1->DIER |= TIM_DIER_UDE; // Timer generates interrupt on update event

		// Setup DMA parameters
		Dma->Channel->CCR =  DMA_CCR_MINC; // Memory increment
		Dma->Channel->CCR |= dmaLowChPrio << DMA_CCR_PL_Pos; // DMA low priority
		Dma->Channel->CCR |= DMA_CCR_DIR_Msk; // 1 - Read from memory
		// Memory and peripheral sizes memory 8 bit, peripheral 16 bit
		Dma->Channel->CCR |= (0b00 << DMA_CCR_MSIZE_Pos) | (0b01 << DMA_CCR_PSIZE_Pos);
		nvic::SetupIrq(Dma->Irq, dmaIrqPrio);
		Dma->Channel->CCR |= DMA_CCR_TCIE; // Interrupt after DMA transmission
	}
	void Update(){
		if((Dma->Channel->CCR & DMA_CCR_EN) != 0)
			return; //Nothing changes if DMA already running
		Dma->Channel->CNDTR = StripLength*24;
		Dma->Channel->CMAR = (uint32_t)&Buffer[0];
		Dma->Channel->CPAR = (uint32_t)&TIM1->CCR1; //TEMP!!
		// Enable DMA and Timer
		Dma->Channel->CCR |= DMA_CCR_EN;
		Timer->CR1 |= TIM_CR1_CEN;
	}
	void WriteLedColor(uint8_t ledNumber, uint32_t rgbVal){
		uint32_t mask = 0x800000;
		for(uint32_t i = 0; i < 24; i++){
			if(mask & rgbVal)
				Buffer[ledNumber*24 + i] = NPX_HIGH;
			else
				Buffer[ledNumber*24 + i] = NPX_LOW;
			mask = mask >> 1;
		}
	}
	uint8_t IrqHandler() {
		if(DMA1->ISR & DMA_ISR_TCIF1 << 4*(Dma->Number - 1)){
			DMA1->IFCR = DMA_IFCR_CTCIF1 << 4*(Dma->Number - 1);
			Dma->Channel->CCR &= ~DMA_CCR_EN;
			Timer->CR1 &= ~TIM_CR1_CEN;
			Timer->CNT = 9;
			return retvOk;
		} else
			return retvFail;
	}
};

#endif /* NEOPIXEL_H_ */
