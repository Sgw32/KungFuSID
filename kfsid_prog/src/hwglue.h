#ifndef HWGLUE_H_
#define HWGLUE_H_

#include <stddef.h>
#include <stdint.h>

uint8_t efShowROM(void* addr);
uint8_t efHideROM(void* addr);
uint8_t __fastcall__ efPeekCartROM(void* addr);
void* __fastcall__ efCopyCartROM(void* dest, const void* src, size_t count);

uint8_t __fastcall__ hwInit(uint8_t* pManufacturerId, uint8_t* pDeviceId);
uint8_t hwReInit(void);
uint8_t __fastcall__ hwGetBank(void);
void __fastcall__ hwSetBank(uint8_t nBank);
uint8_t __fastcall__ hwSectorErase(uint8_t* pBase);
uint8_t __fastcall__ hwWriteBlock(uint8_t* pDst);
uint8_t __fastcall__ hwGetSlot(void);
void __fastcall__ hwSetSlot(uint8_t nSlot);

extern uint8_t _BLOCK_BUFFER_START__;
#define BLOCK_BUFFER (&_BLOCK_BUFFER_START__)

#endif
