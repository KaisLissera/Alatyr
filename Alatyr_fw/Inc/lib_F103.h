/*
 * ezhLib.h
 *
 *  Created on: 2023.07.31
 *      Author: Kais_Lissera
 */

#ifndef INC_LIB_H_
#define INC_LIB_H_

#include <board.h>
#include <stdint.h>
#include <stm32f103xb.h>

//GPIO pins definitions
/////////////////////////////////////////////////////////////////////
#define PA0		GPIOA, 0
#define PA1		GPIOA, 1
#define PA2		GPIOA, 2
#define PA3		GPIOA, 3
#define PA4		GPIOA, 4
#define PA5		GPIOA, 5
#define PA6		GPIOA, 6
#define PA7		GPIOA, 7
#define PA8		GPIOA, 8
#define PA9		GPIOA, 9
#define PA10	GPIOA, 10
#define PA11	GPIOA, 11
#define PA12	GPIOA, 12
#define PA13	GPIOA, 13
#define PA14	GPIOA, 14
#define PA15	GPIOA, 15

#define PB0		GPIOB, 0
#define PB1		GPIOB, 1
#define PB2		GPIOB, 2
#define PB3		GPIOB, 3
#define PB4		GPIOB, 4
#define PB5		GPIOB, 5
#define PB6		GPIOB, 6
#define PB7		GPIOB, 7
#define PB8		GPIOB, 8
#define PB9		GPIOB, 9
#define PB10	GPIOB, 10
#define PB11	GPIOB, 11
#define PB12	GPIOB, 12
#define PB13	GPIOB, 13
#define PB14	GPIOB, 14
#define PB15	GPIOB, 15

#define PC0		GPIOC, 0
#define PC1		GPIOC, 1
#define PC2		GPIOC, 2
#define PC3		GPIOC, 3
#define PC4		GPIOC, 4
#define PC5		GPIOC, 5
#define PC6		GPIOC, 6
#define PC7		GPIOC, 7
#define PC8		GPIOC, 8
#define PC9		GPIOC, 9
#define PC10	GPIOC, 10
#define PC11	GPIOC, 11
#define PC12	GPIOC, 12
#define PC13	GPIOC, 13
#define PC14	GPIOC, 14
#define PC15	GPIOC, 15

//Return values
/////////////////////////////////////////////////////////////////////

typedef enum {
	retvOk,
	retvFail,
	retvTimeout,
	retvBusy,
	retvInProgress,
	retvCmdError,
	retvCmdUnknown,
	retvBadValue,
	retvNew,
	retvSame,
	retvLast,
	retvEmpty,
	retvOverflow,
	retvNotANumber,
	retvWriteProtect,
	retvWriteError,
	retvEndOfFile,
	retvNotFound,
	retvBadState,
	retvDisconnected,
	retvCollision,
	retvCRCError,
	retvNACK,
	retvNoAnswer,
	retvOutOfMemory,
	retvNotAuthorised,
	retvNoChanges,
	retvTooHighSystemClock
} retv_t;

typedef enum {
	Enable 		= 0,
	Disable 	= 1
} Ability_t;

//Simple assert
/////////////////////////////////////////////////////////////////////

#define ASSERT_SIMPLE(x) if((x) == 0) { while(1); }

//Simple circular buffer template - not used for now
/////////////////////////////////////////////////////////////////////
//Example
//static CircBuffer_t<uint8_t,1024> test;

template <typename T, uint32_t Size>
class CircBuffer_t {
private:
	T Buffer_[Size];
	uint32_t start_;
	uint32_t end_;
public:
	CircBuffer_t() {
		start_ = 0;
		end_ = 0;
	}
	T Read() {
		T temp = Buffer_[start_];
		start_ = (start_ + 1) % Size;
		return temp;
	}
	uint32_t Length() {
		if (end_ > start_)
			return end_ - start_;
		else
			return Size - start_ + end_;
	}
	void Write(T data) {
		end_ = (end_ + 1) % Size;
		Buffer_[end_] = data;
	}
	void Clear() {
		start_ = 0;
		end_ = 0;
	}
};

//Commands templates
/////////////////////////////////////////////////////////////////////

/*Callback without arguments
 * Example:
 * void HelpCallback();
 * Command_t Help((char*)"help", HelpCallback);
*/
struct Command_t {
	const char* Name;
	void (*Callback)();

	Command_t(const char* _Name, void (*_Callback)()) {
		Name = _Name;
		Callback = _Callback;
	}
};

/*Callback with argument
 * Example:
 * void EchoCallback(uint32_t);
 * CommandWithArgs_t Echo((char*)"echo", EchoCallback);
 */
struct CommandWithArgs_t {
	const char* Name;
	void (*Callback)(uint32_t);

	CommandWithArgs_t(const char* _Name, void (*_Callback)(uint32_t)) {
		Name = _Name;
		Callback = _Callback;
	}
};

#endif /* INC_LIB_H_ */
