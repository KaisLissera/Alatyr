/*
 * uart_ezh.cpp
 *
 *  Created on: 2023.07.31
 *      Author: Kais Lissera
 */

#include <interface_F103.h>

//Uart_t
/////////////////////////////////////////////////////////////////////

void Uart_t::Init(USART_TypeDef* _Usart, uint32_t CurrentClockHz, uint32_t Bod) {
	Usart = _Usart; // Set class parameter

	// USART must be disabled before configuring
	Usart->CR1 &= ~USART_CR1_M_Msk; // 1 stop bit, 8 data bits
	Usart->BRR = (uint32_t)CurrentClockHz/Bod; // Setup baud rate
	Usart->CR1 |= USART_CR1_TE | USART_CR1_RE ; //USART TX RX enable
}

void Uart_t::UpdateBaudrate(uint32_t CurrentClockHz, uint32_t Bod){
	Disable();
	Usart->BRR = (uint32_t)CurrentClockHz/Bod; // Setup baud rate
	Enable();
}

//Simple UART byte transmit, wait until fully transmitted
void Uart_t::TxByte(uint8_t data) {
	while ((Usart->SR & USART_SR_TXE) == 0) {}
	Usart->DR = data;
}

//Simple UART byte receive, wait until receive
uint8_t Uart_t::RxByte(uint8_t* fl, uint32_t timeout) {
	while ((Usart->SR & USART_SR_RXNE) == 0) {
		timeout--;
		if(timeout == 0) {
			*fl = retvTimeout;
			return 0; // Can't receive data
		}
	}
	// Data received
	uint8_t data = Usart->DR;
	*fl = retvOk;
	return data;
}

void Uart_t::EnableDmaRequest() {
	Usart->CR3 |= USART_CR3_DMAT | USART_CR3_DMAR;
}

//DmaTx_t
/////////////////////////////////////////////////////////////////////

// Initialize memory to peripheral DMA TX channel
void DmaTx_t::Init(DMA_Channel_TypeDef* _Channel, uint32_t PeriphRegAdr,
		IRQn_Type dmaVector, uint8_t DmaIrqPrio , DmaChPrio_t ChPrio) {
	// Setup class parameters
	Channel = _Channel;
	BufferStartPtr = 0;
	BufferEndPtr = 0;

	Channel -> CCR =  DMA_CCR_MINC; // Memory increment
	Channel -> CCR |= ChPrio << DMA_CCR_PL_Pos; // DMA channel priority
	Channel -> CCR |= DMA_CCR_DIR_Msk; // 1 - Read from memory
	// Memory and peripheral sizes
	Channel -> CCR |= (0b00 << DMA_CCR_MSIZE_Pos) | (0b00 << DMA_CCR_PSIZE_Pos); // 8 bit
	nvic::SetupIrq(dmaVector, DmaIrqPrio);
	Channel -> CCR |= DMA_CCR_TCIE;
	Channel -> CPAR = PeriphRegAdr; //Peripheral register
	DmaChannelNumber = ReturnChannelNumberDma(Channel);
}

uint8_t DmaTx_t::Start() {
	if(CheckStatus() != 0)
		return retvBusy; //Nothing changes if DMA already running

	if(BufferStartPtr == BufferEndPtr)
		return retvEmpty; //Nothing changes if Buffer Empty

	uint32_t NumberOfBytesReady;
	if (BufferEndPtr > BufferStartPtr)
		NumberOfBytesReady = GetNumberOfBytesInBuffer();
	else
		NumberOfBytesReady = TX_BUFFER_SIZE - BufferStartPtr;
	Channel -> CNDTR = NumberOfBytesReady;
	Channel -> CMAR = (uint32_t)&Buffer[BufferStartPtr];
	BufferStartPtr = (BufferStartPtr + NumberOfBytesReady) % TX_BUFFER_SIZE;
	Channel -> CCR |= DMA_CCR_EN;

	return retvOk;
}

uint32_t DmaTx_t::CheckStatus() {
	uint32_t temp = Channel -> CCR;
	return temp & DMA_CCR_EN;
}

uint32_t DmaTx_t::GetNumberOfBytesInBuffer() {
	if (BufferEndPtr >= BufferStartPtr)
		return BufferEndPtr - BufferStartPtr;
	else
		return TX_BUFFER_SIZE - BufferStartPtr + BufferEndPtr;
}

uint8_t DmaTx_t::WriteToBuffer(uint8_t data) {
	uint32_t EndPtrTemp = (BufferEndPtr + 1) % TX_BUFFER_SIZE;
	if (EndPtrTemp == BufferStartPtr)
		return retvOutOfMemory;

	Buffer[BufferEndPtr] = data;
	BufferEndPtr = EndPtrTemp;
	return retvOk;
}

uint8_t DmaTx_t::IrqHandler() {
	if(DMA1->ISR & DMA_ISR_TCIF1 << 4*(DmaChannelNumber - 1)){
		DMA1->IFCR = DMA_IFCR_CTCIF1 << 4*(DmaChannelNumber - 1);
		Channel -> CCR &= ~DMA_CCR_EN;
		Start();
		return retvOk;
	} else
		return retvFail;
}

//DmaRx_t
/////////////////////////////////////////////////////////////////////

// Initialize peripheral to memory DMA RX channel
void DmaRx_t::Init(DMA_Channel_TypeDef* _Channel, uint32_t PeriphRegAdr,
		IRQn_Type dmaVector, uint8_t DmaIrqPrio, DmaChPrio_t ChPrio){
	// Setup class parameters
	Channel = _Channel;
	BufferStartPtr = 0;

	// Initialize DMA RX
	Channel -> CCR =  DMA_CCR_MINC | DMA_CCR_CIRC; // Memory increment, circular mode
	Channel -> CCR |= ChPrio << DMA_CCR_PL_Pos; // DMA channel priority
	// Memory and peripheral sizes)
	Channel -> CCR |= (0b00 << DMA_CCR_MSIZE_Pos) | (0b00 << DMA_CCR_PSIZE_Pos); // 8 bit
	Channel -> CPAR = PeriphRegAdr; //Peripheral register
	DmaChannelNumber = ReturnChannelNumberDma(Channel);
}

uint32_t DmaRx_t::GetBufferEndPtr() {
	return RX_BUFFER_SIZE - (Channel -> CNDTR);
}

void DmaRx_t::Start() {
	Channel -> CNDTR = RX_BUFFER_SIZE;
	Channel -> CMAR = (uint32_t)&Buffer[0];
	Channel -> CCR |= DMA_CCR_EN;
}

inline void DmaRx_t::Stop() {
	Channel -> CCR &= ~DMA_CCR_EN;
}

uint32_t DmaRx_t::GetNumberOfBytesInBuffer() {
	uint32_t BufferEndPtr = GetBufferEndPtr();
	if (BufferEndPtr >= BufferStartPtr)
		return BufferEndPtr - BufferStartPtr;
	else
		return RX_BUFFER_SIZE - BufferStartPtr + BufferEndPtr;
}

uint32_t DmaRx_t::CheckStatus() {
	uint32_t temp = Channel -> CCR;
	return temp & DMA_CCR_EN;
}

uint8_t DmaRx_t::ReadFromBuffer() {
	uint8_t temp = Buffer[BufferStartPtr];
	BufferStartPtr = (BufferStartPtr + 1) % RX_BUFFER_SIZE;
	return temp;
}

//Cli_t
/////////////////////////////////////////////////////////////////////

void Cli_t::PutString(const char* text) {
	uint8_t length = strlen(text);
	for(uint32_t i = 0; i < length; i++) {
		TxChannel->WriteToBuffer((uint8_t)text[i]);
	}
}

void Cli_t::PutBinary(uint32_t number) {
	TxChannel->WriteToBuffer((uint8_t)'0');
	TxChannel->WriteToBuffer((uint8_t)'b');
	if(number == 0){
		TxChannel->WriteToBuffer((uint8_t)'0');
		return;
	}

	uint32_t mask = 1UL << 31;
	while((number & mask) == 0) {
		mask = mask >> 1;
	}
	while(mask > 0) {
		if((number & mask))
			TxChannel->WriteToBuffer((uint8_t)'1');
		else
			TxChannel->WriteToBuffer((uint8_t)'0');
		mask = mask >> 1;
	}
}

void Cli_t::PutUnsignedInt(uint32_t number) {
	if(number == 0){
		TxChannel->WriteToBuffer((uint8_t)'0');
		return;
	}

	char IntBuf[10] = "";
	uint32_t i = 0;
	while(number > 0){
		uint32_t temp = number % 10;
		number = (uint32_t)number/10;
		IntBuf[i] = '0' + temp;
		i++;
	}
	for(uint32_t k = 0; k < i; k++) {
		TxChannel->WriteToBuffer((uint8_t)IntBuf[i - k - 1]);
	}
}

void Cli_t::PutUnsignedHex(uint32_t number) {
	TxChannel->WriteToBuffer((uint8_t)'0');
	TxChannel->WriteToBuffer((uint8_t)'x');
	if(number == 0){
		TxChannel->WriteToBuffer((uint8_t)'0');
		return;
	}

	char IntBuf[10] = "";
	uint32_t i = 0;
	while(number > 0){
		uint32_t temp = number & 0xF;
		number = number >> 4;
		if(temp < 10)
			IntBuf[i] = '0' + temp;
		else
			IntBuf[i] = 'A' + temp - 10;
		i++;
	}
	for(uint32_t k = 0; k < i; k++) {
		TxChannel->WriteToBuffer((uint8_t)IntBuf[i - k - 1]);
	}
}

void Cli_t::PutInt(int32_t number) {
	if(number >= 0)
		PutUnsignedInt(number);
	else{
		TxChannel->WriteToBuffer('-');
		PutUnsignedInt(-number);
	}
}

void Cli_t::Printf(const char* text, ...) {
	// String format processing
	va_list args;
	va_start(args, text); // Start string processing
	uint8_t length = strlen(text);
	for(uint32_t i = 0; i < length; i++) {
		if(text[i] == '%'){ // if argument found
			i++;
			switch(text[i]) {
				case 'd': // integer
					PutInt(va_arg(args,int));
					break;
				case 'u': // unsigned integer
					PutUnsignedInt(va_arg(args,uint32_t));
					break;
				case 's': // string
					PutString(va_arg(args,char*));
					break;
				case 'c': // char
					TxChannel->WriteToBuffer(va_arg(args,int));
					break;
				case 'b': // print uint32 as binary
					PutBinary(va_arg(args,uint32_t));
					break;
				case 'x': // print uint32 as hex
					PutUnsignedHex(va_arg(args,uint32_t));
					break;
				default:
					TxChannel->WriteToBuffer((uint8_t)'%');
					TxChannel->WriteToBuffer((uint8_t)text[i]);
			}
		} else
			TxChannel->WriteToBuffer((uint8_t)text[i]);
	}
	va_end(args); // End format processing
	//
	TxChannel->Start();
}

char* Cli_t::Read() {
	uint32_t CmdBufferPtr = 0;
	char temp = (char)RxChannel->ReadFromBuffer();
	//Clear buffer beginning from useless symbols
	while((temp == '\n') || (temp == ' ')) {
		temp = (char)RxChannel->ReadFromBuffer();
	}
	//Get command from buffer
	while((temp != '\r') && (temp != '\n') && (temp != ' ')) {
		CommandBuffer[CmdBufferPtr] = temp;
		CmdBufferPtr++;
		temp = (char)RxChannel->ReadFromBuffer();
	}
	//Add terminators to strings
	CommandBuffer[CmdBufferPtr] = '\0';
	ArgBuffer[0] = '\0';
	//
	if(EchoEnabled)
		Echo();
	return CommandBuffer;
}

char* Cli_t::ReadLine() {
	uint32_t CmdBufferPtr = 0;
	char temp = (char)RxChannel->ReadFromBuffer();
	//Clear buffer beginning from useless symbols
	while((temp == '\n') || (temp == ' ')) {
		temp = (char)RxChannel->ReadFromBuffer();
	}
	//Get command from buffer
	while((temp != '\r') && (temp != '\n')) {
		CommandBuffer[CmdBufferPtr] = temp;
		CmdBufferPtr++;
		temp = (char)RxChannel->ReadFromBuffer();
	}
	//Add terminators to strings
	CommandBuffer[CmdBufferPtr] = '\0';
	ArgBuffer[0] = '\0';
	//
	if(EchoEnabled)
		Echo();
	return CommandBuffer;
}

void Cli_t::ReadCommand() {
	uint32_t CmdBufferPtr = 0;
	char temp = (char)RxChannel->ReadFromBuffer();
	//Clear buffer beginning from useless symbols
	while((temp == '\n') || (temp == ' ')) {
		temp = (char)RxChannel->ReadFromBuffer();
	}
	//Get command from buffer
	while((temp != '\r') && (temp != '\n') && (temp != ' ') && (RxChannel->GetNumberOfBytesInBuffer() != 0)) {
		CommandBuffer[CmdBufferPtr] = temp;
		CmdBufferPtr++;
		temp = (char)RxChannel->ReadFromBuffer();
	}
	//Get argument from buffer if exist
	uint32_t ArgumentBufferPtr = 0;
	if (temp == ' ') {
		temp = (char)RxChannel->ReadFromBuffer();
		while((temp != '\r') && (temp != '\n') && (RxChannel->GetNumberOfBytesInBuffer() != 0)) {
			ArgBuffer[ArgumentBufferPtr] = temp;
			ArgumentBufferPtr++;
			temp = (char)RxChannel->ReadFromBuffer();
		}
	}
	//Add terminators to strings
	ArgBuffer[ArgumentBufferPtr] = '\0';
	CommandBuffer[CmdBufferPtr] = '\0';
	//
	if(EchoEnabled)
		Echo();
}

void Cli_t::Echo() {
	if(ArgBuffer[0] != '\0')
		Printf("[ECHO] %s %s\n\r",CommandBuffer, ArgBuffer);
	else
		Printf("[ECHO] %s\n\r", CommandBuffer);
}
