/*
 * neopixel.cpp
 *
 *  Created on: Mar 17, 2024
 *      Author: KONSTANTIN
 */

#include <neopixel.h>
inline uint32_t ColorToGbr(Color_t color, uint8_t brightness){
	return (color.G*brightness << 16) | (color.R*brightness << 8) | color.B*brightness;
}

// colorIndex[0,255], brightness [3,127] - values lower then 3 have low color resolution
uint32_t MakeHexGrbColor(uint8_t colorIndex, uint8_t brightness){
	uint8_t baseColorIndex = colorIndex / 32;
	Color_t baseColor = ColorTable[baseColorIndex];
	baseColor.R *= brightness;
	baseColor.G *= brightness;
	baseColor.B *= brightness;
	Color_t nextColor = ColorTable[baseColorIndex + 1];
	nextColor.R *= brightness;
	nextColor.G *= brightness;
	nextColor.B *= brightness;
	uint8_t baseColorOffset = colorIndex % 32;
	int32_t dG = baseColorOffset*(nextColor.G - baseColor.G);
	int32_t dR = baseColorOffset*(nextColor.R - baseColor.R);
	int32_t dB = baseColorOffset*(nextColor.B - baseColor.B);
	dG = dG/32; dR = dR/32; dB = dB/32;
	return 	((baseColor.G + dG) << 16) | ((baseColor.R + dR) << 8) | (baseColor.B + dB);
}

uint32_t RgbToGrb(uint32_t rgbVal){
	uint32_t grbVal = 0;
	grbVal |= (rgbVal & 0xFF0000) >> 8;
	grbVal |= (rgbVal & 0x00FF00) << 8;
	grbVal |= rgbVal & 0x0000FF;
	return grbVal;
}

void Neopixel_t::Init(uint32_t currentTimerClock, uint8_t dmaIrqPrio){
	// Setup TIM parameters
	Timer->PSC = (uint32_t)(currentTimerClock/NPX_TIM_FREQUECY) - 1; // 8 MHz counter clock
	Timer->CR1 |= TIM_CR1_DIR; // Up counter
	Timer->ARR = NPX_ARR; // Timer update frequency 800 kHz
	Timer->EGR = TIM_EGR_UG; // Update event to reload prescaler value
	// Enable channel output and set compare type
	Timer->CCMR1 |= (outputComparePWM1 << TIM_CCMR1_OC1M_Pos); //TEMP!!
	Timer->CCER |= TIM_CCER_CC1E; //TEMP!!
	Timer->CCER |= 0b1 << (TimerChannelNumber-1)*4; // Output compare enable output
	if (Timer == TIM1)
		TIM1->BDTR |= TIM_BDTR_MOE; // Main output enable
	TIM1->DIER |= TIM_DIER_UDE; // Timer generates DMA request on update event

	// Setup DMA parameters
	Dma->Channel->CCR =  DMA_CCR_MINC; // Memory increment
	Dma->Channel->CCR |= dmaLowChPrio << DMA_CCR_PL_Pos; // DMA low priority
	Dma->Channel->CCR |= DMA_CCR_DIR_Msk; // 1 - Read from memory
	// Memory and peripheral sizes memory 8 bit, peripheral 16 bit
	Dma->Channel->CCR |= (0b00 << DMA_CCR_MSIZE_Pos) | (0b01 << DMA_CCR_PSIZE_Pos);
	nvic::SetupIrq(Dma->Irq, dmaIrqPrio);
	Dma->Channel->CCR |= DMA_CCR_TCIE; // Interrupt after DMA transmission
}

void Neopixel_t::Update(){
	if((Dma->Channel->CCR & DMA_CCR_EN) != 0)
		return; //Nothing changes if DMA already running
	Dma->Channel->CNDTR = StripLength*24;
	Dma->Channel->CMAR = (uint32_t)&Buffer[0];
	Dma->Channel->CPAR = (uint32_t)&TIM1->CCR1; //TEMP!!
	// Enable DMA and Timer
	Dma->Channel->CCR |= DMA_CCR_EN;
	Timer->CR1 |= TIM_CR1_CEN;
}

void Neopixel_t::Clear(){
	for(uint32_t i = 0; i < 24*StripLength; i++)
		Buffer[i] = NPX_LOW;
}

void Neopixel_t::WriteLedColor(uint8_t ledNumber, uint32_t gbrColor){
	// Writing to buffer
	uint32_t mask = 0x800000;
	for(uint32_t i = 0; i < 24; i++){
		if(mask & gbrColor)
			Buffer[ledNumber*24 + i] = NPX_HIGH;
		else
			Buffer[ledNumber*24 + i] = NPX_LOW;
		mask = mask >> 1;
	}
}
