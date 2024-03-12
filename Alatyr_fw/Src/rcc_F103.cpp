/*
 * rcc_F072.cpp
 *
 *  Created on: 2023.07.31
 *      Author: Kais Lissera
 */

#include <lib_F103.h>
//
#include <rcc_F103.h>

//Simple delays
/////////////////////////////////////////////////////////////////////

#if (USE_SYSTICK_DELAY == 1)
extern "C"
void SysTick_Handler(void) {
	if(systickCount > 0) systickCount--;
}

void DelayMs(uint32_t ms) {
	NVIC_EnableIRQ(SysTick_IRQn);
	NVIC_SetPriority(SysTick_IRQn, 0);
	systickCount = ms;
	SysTick->VAL = 0x0u;
	// In stm32f0x7x SysTick frequency equals core frequency HCLK divided by 8
	uint32_t Clk = rcc::GetCurrentSystemClock();
	SysTick->LOAD = (uint32_t)(Clk/1000 + 1); // SYS_CLK/1000 + 1 - precise value
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk; //Clock, interrupt, systick enable
	while(systickCount);
}
#endif

void BlockingDelay(uint32_t ms) {
	uint32_t Clk = rcc::GetCurrentSystemClock();
	uint32_t temp = ms*Clk >> 4; // Check why 20!!!
	for(volatile uint32_t i = 0; i < temp; i++) {};
}

//rcc
/////////////////////////////////////////////////////////////////////

void rcc::ResetAll(){
	RCC->APB1RSTR = ~0x0;
	RCC->APB2RSTR = ~0x0;

	RCC->APB1RSTR = 0;
	RCC->APB2RSTR = 0;
}

void rcc::DisableAllClocks(){
	RCC->AHBENR = 0x14;
	RCC->APB1ENR = 0x0;
	RCC->APB2ENR = 0x0;
}

uint8_t rcc::EnableLSI(uint32_t Timeout) {
	RCC->CSR |= RCC_CSR_LSION;
	while(!(RCC->CSR & RCC_CSR_LSIRDY)) {
		Timeout--;
		if (Timeout == 0)
			return retvFail; //Unable to start LSI
	}
	return retvOk;
}

uint8_t rcc::EnableHSI8(uint32_t Timeout) {
	RCC->CR |= RCC_CR_HSION;
	while(!(RCC->CR & RCC_CR_HSIRDY)) {
		Timeout--;
		if (Timeout == 0)
			return retvFail; //Unable to start HSI
	}
	return retvOk;
}

uint8_t rcc::EnableHSE(uint32_t Timeout) {
	RCC->CR &= ~RCC_CR_HSEBYP; //HSE must not be bypassed
	RCC->CR |= RCC_CR_HSEON; //HSE on
	while(!(RCC->CR & RCC_CR_HSERDY)) {
		Timeout--;
		if (Timeout == 0)
			return retvFail; //Unable to start HSE
	}
	return retvOk;
}

uint8_t rcc::EnablePLL(uint32_t Timeout) {
	RCC->CR |= RCC_CR_PLLON; //Enable PLL, PLL must NOT be used as system clock
	while(!(RCC->CR & RCC_CR_PLLRDY)) { // PLL locked
		Timeout--;
		if (Timeout == 0)
			return retvFail; //PLL not locked
	}
	return retvOk;
}

// sysClkHsi, sysClkHse, sysClkPll, sysClkHsi48
uint8_t rcc::SwitchSysClk(SysClkSource_t SysClkSource, uint32_t Timeout) {
	RCC->CFGR &= ~RCC_CFGR_SW; // Clear
	RCC->CFGR |= (SysClkSource << RCC_CFGR_SW_Pos); // Switch system clock
	// Check system clock switch status
	while((RCC->CFGR & RCC_CFGR_SWS_Msk) != ((uint32_t)SysClkSource << RCC_CFGR_SWS_Pos)) {
		Timeout--;
		if (Timeout == 0)
			return retvFail; //Unable to switch system clock
	}
	return retvOk;
}

// 2 <= pllMul <= 16
// PLL must be disabled
uint8_t rcc::SetupPLL(PllSource_t pllSrc, uint32_t pllMul) {
	// Check arguments
	if (pllMul < 2 and pllMul > 16)
		return retvBadValue;
	// Transform to register values
	// 0b1110, 0b1111 - PLLMUL x16
	pllMul = pllMul - 2; // 2,3,4 => 0b0000, 0b0001
    //
	// No HSE divider for PLL source
	RCC->CFGR &= ~RCC_CFGR_PLLXTPRE;
	RCC->CFGR &= ~RCC_CFGR_PLLMULL_Msk;
	RCC->CFGR |= pllMul << RCC_CFGR_PLLMULL_Pos;
	// Select pll source
	RCC->CFGR &= ~RCC_CFGR_PLLSRC;
	RCC->CFGR |= pllSrc << RCC_CFGR_PLLSRC_Pos;

    return retvOk;
}

// APB1
void rcc::SetBusDividers(AhbDiv_t AhbDiv, ApbDiv_t Apb1Div, ApbDiv_t Apb2Div) {
    uint32_t Temp = RCC->CFGR;
    Temp &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk);  // Clear bits
    Temp |= AhbDiv  << RCC_CFGR_HPRE_Pos;
    Temp |= Apb1Div << RCC_CFGR_PPRE1_Pos;
    Temp |= Apb2Div << RCC_CFGR_PPRE2_Pos;
    RCC->CFGR = Temp;
}

uint32_t rcc::GetCurrentSystemClock() {
    uint32_t Temp = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;  // System clock switch status
    switch(Temp) {
        case sysClkHsi: return HSI8_FREQ_HZ;
#ifdef HSE_FREQ_HZ
        case sysClkHse: return HSE_FREQ_HZ;
#endif
        case sysClkPll: {
            uint32_t PllSource = (RCC->CFGR & RCC_CFGR_PLLSRC_Msk) >> RCC_CFGR_PLLSRC_Pos;
            uint32_t pllMul = (RCC->CFGR & RCC_CFGR_PLLMULL_Msk) >> RCC_CFGR_PLLMULL_Pos;
            pllMul = pllMul + 2; // Conversion to multiplier
            switch(PllSource) {
            	case pllSrcHsiDiv2:
            		return HSI8_FREQ_HZ*pllMul/2;
#ifdef HSE_FREQ_HZ
                case pllSrcHse:
                	return HSE_FREQ_HZ*pllMul;
#endif
            } // Switch on PLL source
        } break; //case sysClkPll:
    } // Switch on system clock status
    return retvFail;
}

uint32_t rcc::GetCurrentAHBClock(uint32_t currentSystemClockHz) {
	uint32_t Temp = ((RCC->CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos);
	switch(Temp) {
		case ahbDiv1: return currentSystemClockHz;
		case ahbDiv2: return currentSystemClockHz >> 1;
		case ahbDiv4: return currentSystemClockHz >> 2;
		case ahbDiv8: return currentSystemClockHz >> 3;
		case ahbDiv16: return currentSystemClockHz >> 4;
		case ahbDiv64: return currentSystemClockHz >> 6;
		case ahbDiv128: return currentSystemClockHz >> 7;
		case ahbDiv256: return currentSystemClockHz >> 8;
		case ahbDiv512: return currentSystemClockHz >> 9;
		default:
			return retvFail;
	}
}

uint32_t rcc::GetCurrentAPB1Clock(uint32_t currentAhbClockHz) {
	uint32_t Temp = ((RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos);
	switch(Temp) {
		case apbDiv1: return currentAhbClockHz;
		case apbDiv2: return currentAhbClockHz >> 1;
		case apbDiv4: return currentAhbClockHz >> 2;
		case apbDiv8: return currentAhbClockHz >> 3;
		case apbDiv16: return currentAhbClockHz >> 4;
		default:
			return retvFail;
	}
}

uint32_t rcc::GetCurrentAPB2Clock(uint32_t currentAhbClockHz) {
	uint32_t Temp = ((RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos);
	switch(Temp) {
		case apbDiv1: return currentAhbClockHz;
		case apbDiv2: return currentAhbClockHz >> 1;
		case apbDiv4: return currentAhbClockHz >> 2;
		case apbDiv8: return currentAhbClockHz >> 3;
		case apbDiv16: return currentAhbClockHz >> 4;
		default:
			return retvFail;
	}
}

uint32_t rcc::GetCurrentTimersClock(uint32_t currentApbClockHz) {
	uint32_t Temp = ((RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos);
	switch(Temp) { // if APB prescaler = 1 -> APB clock x1, else x2
		case apbDiv1: return currentApbClockHz;
		case apbDiv2: return currentApbClockHz;
		case apbDiv4: return currentApbClockHz >> 1;
		case apbDiv8: return currentApbClockHz >> 2;
		case apbDiv16: return currentApbClockHz >> 3;
		default:
			return retvFail;
	}
}

//flash
/////////////////////////////////////////////////////////////////////
/*
 *  0 - SYSCLK < 24 MHz
 *  1 - 24 MHz < SYSCLK < 48 MHz
 *  2 - 48 MHz < SYSCLK < 72 MHz
 */
void flash::SetFlashPrefetchAndLatency(uint8_t Latency) {
    uint32_t Temp = FLASH->ACR;
    Temp &= ~FLASH_ACR_LATENCY_Msk;
    Temp |= FLASH_ACR_PRFTBE; // Enable prefetch
    Temp |= Latency << FLASH_ACR_LATENCY_Pos;
    FLASH->ACR = Temp;
//    while(FLASH->ACR != tmp);
}
