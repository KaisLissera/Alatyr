/*
 * uart_ezh.h
 *
 *  Created on: 2023.07.31
 *      Author: Kais Lissera
 */

#ifndef INC_INTERFACE_H_
#define INC_INTERFACE_H_

#include <stdarg.h>
#include <stdint.h>
#include <cstring>
//
#include <lib_F103.h>
//
#include <gpio_F103.h>
#include <rcc_F103.h>

/////////////////////////////////////////////////////////////////////

typedef enum {
	dmaLowChPrio 		= 0b00,
	dmaMediumChPrio 	= 0b01,
	dmaHighChPrio 		= 0b10,
	dmaVeryHighChPrio 	= 0b11
} DmaChPrio_t;

constexpr uint32_t ReturnChannelNumberDma(DMA_Channel_TypeDef* ch){
	if(ch == DMA1_Channel1)
		return 1;
	else if(ch == DMA1_Channel2)
		return 2;
	else if(ch == DMA1_Channel3)
		return 3;
	else if(ch == DMA1_Channel4)
		return 4;
	else if(ch == DMA1_Channel5)
		return 5;
	else if(ch == DMA1_Channel6)
		return 6;
	else if(ch == DMA1_Channel7)
		return 7;
	else
		ASSERT_SIMPLE(0); //Bad DMA channel name
} //ReturnChNum_DMA end

//UartBase_t - implementation of base UART functions
/////////////////////////////////////////////////////////////////////

class Uart_t {
protected:
	USART_TypeDef* Usart;
public:
	void Init(USART_TypeDef* _Usart, uint32_t CurrentClockHz, uint32_t Bod);
	void UpdateBaudrate( uint32_t CurrentClockHz, uint32_t Bod);
	inline void Enable() { Usart->CR1 |= USART_CR1_UE; }
	inline void Disable() { Usart->CR1 &= ~USART_CR1_UE; }
	void EnableDmaRequest();
	void TxByte(uint8_t data);
	uint8_t RxByte(uint8_t* fl = NULL, uint32_t timeout = 0xFFFF);
}; //Uart_t end

//UartDma_t - enables capability to transmit and receive data through DMA
/////////////////////////////////////////////////////////////////////
/*
* Need IRQ Handler wrapper for correct operation
* Wrapper example
*
UartDma_t UartName(UART_PARAMS, DMA_UART_TX_CHANNEL, DMA_UART_RX_CHANNEL);
extern "C" {
void DMAx_Channelx_IRQHandler(){ -
	UartName.DmaIrqHandler();
}
void DMAx_Channelx_IRQHandler(){ -
	UartName.DmaIrqHandler();
} }
*/

//DMA buffers sizes
#define TX_BUFFER_SIZE 		(256UL)
#define RX_BUFFER_SIZE 		(256UL)

// Memory to peripheral DMA TX channel
class DmaTx_t{
protected:
	uint8_t Buffer[TX_BUFFER_SIZE];
	uint32_t BufferStartPtr;
	uint32_t BufferEndPtr;
	DMA_Channel_TypeDef* Channel;
	uint8_t DmaChannelNumber;
public:
	void Init(DMA_Channel_TypeDef* _Channel, uint32_t PeriphRegAdr, IRQn_Type dmaVector,
			uint8_t DmaIrqPrio = 0, DmaChPrio_t ChPrio = dmaLowChPrio);
	uint8_t Start();
	uint32_t GetNumberOfBytesInBuffer();
	uint32_t CheckStatus(); //0 - disable
	uint8_t WriteToBuffer(uint8_t data);
	uint8_t IrqHandler();
}; //DmaTx_t end

// Peripheral to memory DMA RX channel
class DmaRx_t {
protected:
	uint8_t Buffer[RX_BUFFER_SIZE];
	uint32_t BufferStartPtr;
	uint32_t GetBufferEndPtr();
	DMA_Channel_TypeDef* Channel;
	int8_t DmaChannelNumber;
public:
	void Init(DMA_Channel_TypeDef* _Channel, uint32_t PeriphRegAdr, IRQn_Type dmaVector,
			uint8_t DmaIrqPrio = 0, DmaChPrio_t ChPrio = dmaLowChPrio);
	void Start();
	inline void Stop();
	uint32_t GetNumberOfBytesInBuffer();
	uint32_t CheckStatus(); //0 - disable
	uint8_t ReadFromBuffer();
//	uint8_t IrqHandler();
}; //DmaRx_t end

//Cli_t - Simple command line interface
/////////////////////////////////////////////////////////////////////

#define COMMAND_BUFFER_SIZE (128UL)
#define ARG_BUFFER_SIZE (10UL)

class Cli_t {
protected:
	DmaTx_t* TxChannel;
	DmaRx_t* RxChannel;
	void PutBinary(uint32_t binary);
	void PutString(const char* text);
	void PutInt(int32_t number);
	void PutUnsignedInt(uint32_t number);
	void PutUnsignedHex(uint32_t number);
public:
	Cli_t(DmaTx_t* _TxChannel, DmaRx_t* _RxChannel) {
		TxChannel = _TxChannel;
		RxChannel = _RxChannel;
	}
	char CommandBuffer[COMMAND_BUFFER_SIZE];
	char ArgBuffer[ARG_BUFFER_SIZE];
	uint8_t EchoEnabled = 1;
	//Methods
	void Clear() { CommandBuffer[0] = '\0'; ArgBuffer[0] = '\0';}
	void Echo();
	void Printf(const char* text, ...);
	char* Read();
	char* ReadLine();
	void ReadCommand(); //Read command with argument if exist, put in buffer
};

#endif /* INC_INTERFACE_H_ */
