/*
 * main.cpp
 *
 *  Created on: Mar 12, 2024
 *      Author: KONSTANTIN
 */
#include <stdint.h>
#include <interface_F103.h>
#include <rcc_F103.h>
#include <gpio_F103.h>
#include <tim_F103.h>
#include <neopixel.h>

#include <stm32f1xx.h>

Uart_t CmdUart;
DmaTx_t DmaTxUart1;
DmaRx_t DmaRxUart1;
Cli_t CmdCli(&DmaTxUart1, &DmaRxUart1);
//Cli_t CmdCli(&CmdUart, &CmdUart); // Command line without DMA

DmaChannel_t DmaCh5 = {.Channel = DMA1_Channel5, .Number = 5, .Irq = DMA1_Channel5_IRQn};
//Timer_t NeopixelTim1;
uint8_t NeopixelBuffer[24*4];
Neopixel_t LedStrip(TIM1, 1, &DmaCh5, NeopixelBuffer, 3);

extern "C" {
	void DMA1_Channel4_IRQHandler(){
		DmaTxUart1.IrqHandler();
	}
	void DMA1_Channel5_IRQHandler(){
		LedStrip.IrqHandler();
	}
}

int main(){
	flash::SetFlashPrefetchAndLatency(0);
	rcc::EnableHSI8();
	rcc::SetupPLL(pllSrcHsiDiv2, 8);
	rcc::EnablePLL();
	rcc::SwitchSysClk(sysClkPll);

	//Enable peripheral clock
	rcc::EnableClkAHB(RCC_AHBENR_DMA1EN);

	rcc::EnableClkAPB2(RCC_APB2ENR_TIM1EN);
	rcc::EnableClkAPB2(RCC_APB2ENR_AFIOEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_IOPAEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_IOPBEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_USART1EN);

	uint32_t currentSystemClock = rcc::GetCurrentSystemClock();
	uint32_t currentAhbClock = rcc::GetCurrentAHBClock(currentSystemClock);
	uint32_t currentApb2Clock = rcc::GetCurrentAPB2Clock(currentAhbClock);

	//USART1
	gpio::Afio1Remap(AFIO_MAPR_USART1_REMAP);
	gpio::SetupPin(PB6, AfOutput10MHzPushPull); // USART1 TX afer remap
	gpio::SetupPin(PB7, InputWithPull, PullUp); // USART1 RX after remap
	CmdUart.Init(USART1, currentApb2Clock, 115200);
	CmdUart.EnableDmaRequest(USART_CR3_DMAR | USART_CR3_DMAT);
	CmdUart.Enable();

	//USART1 DMA for command line
	DmaTxUart1.Init(DMA1_Channel4, (uint32_t)&USART1->DR, DMA1_Channel4_IRQn);
	DmaRxUart1.Init(DMA1_Channel5, (uint32_t)&USART1->DR, DMA1_Channel5_IRQn);
	DmaRxUart1.Start();

	//Neopixel
	gpio::SetupPin(PB1, Output10MHzPushPull);
	gpio::ActivatePin(PB1); // 5V DC-DC enable
	gpio::SetupPin(PA8, AfOutput10MHzPushPull); // TIM1 channel 1
	LedStrip.Init(rcc::GetCurrentTimersClock(currentApb2Clock), 0);

	while(1){
		CmdCli.Printf("Current clock %d\n\r", currentSystemClock);
		LedStrip.WriteLedColor(0, 0x0F0000);
		LedStrip.WriteLedColor(1, 0x000F00);
		LedStrip.WriteLedColor(2, 0x00000F);
		LedStrip.Update();
		DelayMs(100);

		LedStrip.WriteLedColor(0, 0x000F00);
		LedStrip.WriteLedColor(1, 0x00000F);
		LedStrip.WriteLedColor(2, 0x0F0000);
		LedStrip.Update();
		DelayMs(100);

		LedStrip.WriteLedColor(0, 0x00000F);
		LedStrip.WriteLedColor(1, 0x0F0000);
		LedStrip.WriteLedColor(2, 0x000F00);
		LedStrip.Update();
		DelayMs(100);
	}
	return 0;
}
