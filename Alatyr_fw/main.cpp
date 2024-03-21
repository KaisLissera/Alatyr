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

// Uart shell for debug
Uart_t CmdUart; // UART1
DmaChannel_t DmaCh4 = {.Channel = DMA1_Channel4, .Number = 4, .Irq = DMA1_Channel4_IRQn};
uint8_t CmdTxBuffer[128];
DmaTx_t CmdTxDma(&DmaCh4, CmdTxBuffer, 128);
//DmaChannel_t DmaCh5 = {.Channel = DMA1_Channel5, .Number = 5, .Irq = DMA1_Channel5_IRQn};
//uint8_t DmaRxBuffer[128];
//DmaRx_t DmaRxUart1(&DmaCh5, DmaRxBuffer, 128);
Cli_t CmdCli(&CmdTxDma, &CmdUart);

// Bluetooth
Uart_t BleUart; // UART2
DmaChannel_t DmaCh7 = {.Channel = DMA1_Channel7, .Number = 7, .Irq = DMA1_Channel7_IRQn};
uint8_t BleTxBuffer[128];
DmaTx_t BleTxDma(&DmaCh7, BleTxBuffer, 128);
DmaChannel_t DmaCh6 = {.Channel = DMA1_Channel6, .Number = 6, .Irq = DMA1_Channel6_IRQn};
uint8_t BleRxBuffer[256];
DmaRx_t BleRxDma(&DmaCh6, BleRxBuffer, 256);
Cli_t BleCli(&BleTxDma, &BleRxDma);

// Neopixel
#define NEOPIXEL_LENGTH 6
DmaChannel_t DmaCh5 = {.Channel = DMA1_Channel5, .Number = 5, .Irq = DMA1_Channel5_IRQn};
uint8_t NeopixelBuffer[24*NEOPIXEL_LENGTH];
Neopixel_t LedStrip(TIM1, 1, &DmaCh5, NeopixelBuffer, NEOPIXEL_LENGTH);

Button_t Button1(PA0, PullUp);
Button_t Button2(PC13, PullUp);

extern "C" {
	void DMA1_Channel4_IRQHandler(){
		CmdTxDma.IrqHandler();
	}
	void DMA1_Channel7_IRQHandler(){
		BleTxDma.IrqHandler();
	}
	void DMA1_Channel5_IRQHandler(){
		LedStrip.IrqHandler();
	}
}

int main(){
	flash::SetFlashPrefetchAndLatency(1);
	rcc::EnableHSI8();
	rcc::SetupPLL(pllSrcHsiDiv2, 8);
	rcc::EnablePLL();
	rcc::SwitchSysClk(sysClkPll);

	//Enable peripheral clock
	rcc::EnableClkAHB(RCC_AHBENR_DMA1EN);

	rcc::EnableClkAPB1(RCC_APB1ENR_USART2EN);

	rcc::EnableClkAPB2(RCC_APB2ENR_TIM1EN);
	rcc::EnableClkAPB2(RCC_APB2ENR_AFIOEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_IOPAEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_IOPBEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_IOPCEN);
	rcc::EnableClkAPB2(RCC_APB2ENR_USART1EN);

	uint32_t currentSystemClock = rcc::GetCurrentSystemClock();
	uint32_t currentAhbClock = rcc::GetCurrentAHBClock(currentSystemClock);
	uint32_t currentApb1Clock = rcc::GetCurrentAPB1Clock(currentAhbClock);
	uint32_t currentApb2Clock = rcc::GetCurrentAPB2Clock(currentAhbClock);

	//USART1
	gpio::Afio1Remap(AFIO_MAPR_USART1_REMAP);
	gpio::SetupPin(PB6, AfOutput10MHzPushPull); // USART1 TX afer remap
	gpio::SetupPin(PB7, InputWithPull, PullUp); // USART1 RX after remap
	CmdUart.Init(USART1, currentApb2Clock, 115200);
	CmdUart.EnableDmaRequest(USART_CR3_DMAT);
	CmdUart.Enable();

	//USART1 DMA for command line
	CmdTxDma.Init((uint32_t)&USART1->DR);
//	DmaRxUart1.Init((uint32_t)&USART1->DR);
//	DmaRxUart1.Start();

	//USART2 for BLE
	gpio::SetupPin(PA2, AfOutput10MHzPushPull); // USART2 TX
	gpio::SetupPin(PA3, InputWithPull, PullUp); // USART2 RX
	BleUart.Init(USART2, currentApb1Clock, 9600);
	BleUart.EnableDmaRequest(USART_CR3_DMAT|USART_CR3_DMAR);
	BleUart.Enable();

	//DMA for BLE
	BleTxDma.Init((uint32_t)&USART2->DR);
	BleRxDma.Init((uint32_t)&USART2->DR);
	BleRxDma.Start();

	gpio::SetupPin(PA4, Output10MHzPushPull);
	gpio::SetupPin(PA5, Output10MHzPushPull);
	gpio::SetupPin(PA6, InputFloating);
	gpio::ActivatePin(PA4); // BLE PWRC
	gpio::ActivatePin(PA5); // BLE RST

	//Neopixel
	gpio::SetupPin(PB1, Output10MHzPushPull);
	gpio::ActivatePin(PB1); // 5V DC-DC enable
	gpio::SetupPin(PA8, AfOutput10MHzPushPull); // TIM1 channel 1
	LedStrip.Init(rcc::GetCurrentTimersClock(currentApb2Clock), 0);
	LedStrip.Clear();

	// Buttons
	Button1.Init();
	Button2.Init();

	CmdCli.Printf("%d\n\r",currentSystemClock);
	uint32_t counter = 0;

	BleCli.Printf("AT+VER\r\n");
//	DelayMs(100);
	BleCli.Printf("AT+MAC\r\n");
//	DelayMs(100);
	BleCli.Printf("AT+NAMEGenshinVision\r\n");
//	DelayMs(100);
	BleCli.Printf("AT+NAME\r\n");
//	DelayMs(100);
	BleCli.Printf("AT+HOSTEN0\r\n");
//	DelayMs(100);
	BleCli.Printf("AT+BATT69\r\n");
//	DelayMs(100);

	while(1){
		for(uint8_t i = 0; i < NEOPIXEL_LENGTH; i++)
			LedStrip.WriteLedColor(i, MakeHexGrbColor((counter + i) & 255, 15));
		LedStrip.Update();
		counter++;

		ButtonState_t button1 = Button1.CheckState();
		ButtonState_t button2 = Button2.CheckState();

		switch(button1){
		case Pressed:
			CmdCli.Printf("Button 1 pressed\n\r");
			break;
		case HoldDown:
			CmdCli.Printf("Button 1 hold down\n\r");
			break;
		case Idle:
			break;
		case Released:
			CmdCli.Printf("Button 1 released\n\r");
			break;
		}

		switch(button2){
		case Pressed:
			CmdCli.Printf("Button 2 pressed\n\r");
			break;
		case HoldDown:
			CmdCli.Printf("Button 2 hold down\n\r");
			break;
		case Idle:
			break;
		case Released:
			CmdCli.Printf("Button 2 released\n\r");
			break;
		}
//		DelayMs(50);

		if(BleRxDma.GetNumberOfBytesReady() > 0){
			BleCli.Printf("HELLO\r\n");
			CmdCli.Printf("response = ");
		}
		while(BleRxDma.GetNumberOfBytesReady() > 0)
			CmdTxDma.WriteChar(BleRxDma.ReadChar());
	}
	return 0;
}
