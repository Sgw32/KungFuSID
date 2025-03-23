#ifndef IRQ_H
#define IRQ_H

#include "common.h"

void irq_handler(void);
void initPorts(void);
void reset_SID(void);
//uint32_t instead of uint8_t allows to compile without UXTB instruction, its not needed
void setreg(uint32_t addr,uint32_t data);
void InitHardware(void);


#endif
