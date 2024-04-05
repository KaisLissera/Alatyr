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
#include <rcc_F103.h>

/////////////////////////////////////////////////////////////////////

typedef enum {
	dmaLowChPrio 		= 0b00,
	dmaMediumChPrio 	= 0b01,
	dmaHighChPrio 		= 0b10,
	dmaVeryHighChPrio 	= 0b11
} DmaChPrio_t;

typedef struct{
	DMA_Channel_TypeDef* Channel;
	uint8_t Number;
	IRQn_Type Irq;
} DmaChannel_t;

// Writer and reader interfaces
/////////////////////////////////////////////////////////////////////
class iWriter_t{
public:
	virtual uint8_t WriteChar(uint8_t data)=0;
	virtual uint8_t StartTransmission()=0;
};

class iReader_t{
public:
	virtual uint8_t ReadChar(retv_t* retv)=0;
	virtual uint32_t GetNumberOfBytesReady()=0;
};

//UartBase_t - implementation of base UART functions
/////////////////////////////////////////////////////////////////////

class Uart_t:public iWriter_t, public iReader_t {
protected:
	USART_TypeDef* Usart;
public:
	void Init(USART_TypeDef* _Usart, uint32_t CurrentClockHz, uint32_t Bod);
	void UpdateBaudrate( uint32_t CurrentClockHz, uint32_t Bod);
	inline void Enable() { Usart->CR1 |= USART_CR1_UE; }
	inline void Disable() { Usart->CR1 &= ~USART_CR1_UE; }
	inline void EnableDmaRequest(uint32_t request) {Usart->CR3 |= request;}

	// Writer interface
	uint8_t WriteChar(uint8_t data);
	uint8_t StartTransmission(){ return retvOk;} // Empty function for compatibility

	// Reader interface
	uint8_t ReadChar(retv_t* retv = NULL);
	uint32_t GetNumberOfBytesReady(); // Empty function for compatibility
}; //Uart_t end

//UartDma_t - enables capability to transmit and receive data through DMA
/////////////////////////////////////////////////////////////////////
/*
* Need IRQ Handler wrapper for correct operation
* Wrapper example
*
UartDma_t UartName(UART_PARAMS, DMA_UART_TX_CHANNEL, DMA_UART_RX_CHANNEL);
extern "C" {
void DMAx_Channelx_IRQHandler(){
	UartName.DmaIrqHandler();
}
void DMAx_Channelx_IRQHandler(){
	UartName.DmaIrqHandler();
} }
*/

// Memory to peripheral DMA TX channel
class DmaTx_t:public iWriter_t {
protected:
	uint8_t* Buffer;
	uint16_t BufferSize;
	uint16_t BufferStartPtr;
	uint16_t BufferEndPtr;
	DmaChannel_t* Dma;
public:
	DmaTx_t(DmaChannel_t* dma, uint8_t* buffer, uint16_t bufferSize){
		Dma = dma;
		Buffer = buffer;
		BufferSize = bufferSize;
	}
	void Init(uint32_t PeriphRegAdr, uint8_t DmaIrqPrio = 0, DmaChPrio_t ChPrio = dmaLowChPrio);
	uint32_t GetNumberOfBytesInBuffer();
	uint32_t CheckStatus(); //0 - disable
	uint8_t IrqHandler();

	// Writer interface
	uint8_t WriteChar(uint8_t data);
	uint8_t StartTransmission();
}; //DmaTx_t end

// Peripheral to memory DMA RX channel
class DmaRx_t:public iReader_t {
protected:
	uint8_t* Buffer;
	uint16_t BufferSize;
	uint16_t BufferStartPtr;
	uint16_t GetBufferEndPtr();
	DmaChannel_t* Dma;
public:
	DmaRx_t(DmaChannel_t* dma, uint8_t* buffer, uint16_t bufferSize){
		Dma = dma;
		Buffer = buffer;
		BufferSize = bufferSize;
	}
	void Init(uint32_t PeriphRegAdr, DmaChPrio_t ChPrio = dmaLowChPrio);
	void Start();
	inline void Stop();
	uint32_t CheckStatus(); //0 - disable

	// Reader interface
	uint8_t ReadChar(retv_t* retv = NULL);
	uint32_t GetNumberOfBytesReady();
}; //DmaRx_t end

//Cli_t - Simple command line interface
/////////////////////////////////////////////////////////////////////

int32_t strToInt(char* text);

#define COMMAND_BUFFER_SIZE (128UL)

class Cli_t {
protected:
	iWriter_t* TxChannel;
	iReader_t* RxChannel;
	void PutBinary(uint32_t binary);
	void PutString(const char* text);
	void PutInt(int32_t number);
	void PutUnsignedInt(uint32_t number);
	void PutUnsignedHex(uint32_t number);
public:
	Cli_t(iWriter_t* _TxChannel, iReader_t* _RxChannel) {
		TxChannel = _TxChannel;
		RxChannel = _RxChannel;
	}
	char CommandBuffer[COMMAND_BUFFER_SIZE];
	//Methods
	void Clear() {CommandBuffer[0] = '\0';}
	void Printf(const char* text, ...);
	char* Read();
	char* ReadLine();
};

#endif /* INC_INTERFACE_H_ */
