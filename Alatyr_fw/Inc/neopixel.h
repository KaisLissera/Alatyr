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
#include <interface_F103.h>
#include <tim_F103.h>

#define NPX_TIM_FREQUECY 	8000000  	// 1 tick = 125ns
#define NPX_ARR 			9  			// timer reload after 125ns*(9+1) = 1250ns
#define NPX_LOW 			2  			// 125ns*(2+1) = 375ns
#define NPX_HIGH 			5 			// 125ns*(5+1) = 750ns

// Simple colors
/////////////////////////////////////////////////////////////////////
//Base colors indexes in ColorTable
#define BASE_RED 			0
#define BASE_ORANGE 		1
#define BASE_YELLOW 		2
#define BASE_GREEN 			3
#define BASE_CYAN 			5
#define BASE_BLUE 			6
#define BASE_PUPLE 			7
#define BASE_WHITE 			9

typedef struct{
    uint8_t G;
    uint8_t R;
    uint8_t B;
} Color_t;

//[0, 8] - Table for GBR colors
const Color_t ColorTable[10] = {
		{0,2,0}, {1,2,0}, {1,1,0}, {2,0,0}, {2,0,1}, {1,0,1}, {0,0,2}, {0,1,1}, {0,2,0}, {1,1,1}
};
//brightness [3,127] - values lower then 3 have low color resolution
inline uint32_t ColorToGbr(Color_t color, uint8_t brightness);
// colorIndex[0,255], brightness [3,127] - values lower then 3 have low color resolution
uint32_t MakeHexGrbColor(uint8_t colorIndex, uint8_t brightness);
uint32_t RgbToGrb(uint32_t rgbVal);

//Neopixel_t - driver for neopixel WS2812
/////////////////////////////////////////////////////////////////////
/*
* Need IRQ Handler wrapper and external buffer for correct operation
* Wrapper example
*
#define NEOPIXEL_LENGTH 3
// DMA on neopixel TIM update request
DmaChannel_t DmaCh5 = {.Channel = DMA1_Channel5, .Number = 5, .Irq = DMA1_Channel5_IRQn};
// External buffer
uint8_t NeopixelBuffer[24*NEOPIXEL_LENGTH];
// Neopixel class declaration
Neopixel_t LedStrip(TIM1, 1, &DmaCh5, NeopixelBuffer, NEOPIXEL_LENGTH);
// External interrupt handler wrapper compatible with CMSIS
extern "C" {
	void DMA1_Channel5_IRQHandler(){
		LedStrip.IrqHandler();
	}
}
*/

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
	void Init(uint32_t currentTimerClock, uint8_t dmaIrqPrio);
	void Update();
	void Clear(); // Clear buffer (every bit = NPX_LOW) without update
	void WriteLedColor(uint8_t ledNumber, uint32_t gbrColor);
	inline uint8_t IrqHandler(){
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
