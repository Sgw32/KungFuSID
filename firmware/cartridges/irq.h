#ifndef IRQ_H
#define IRQ_H

void irq_handler(void);
void initPorts(void);
void SID_emulator(void);
void reset_SID(void);
void setreg(uint8_t addr,uint8_t data);
void InitHardware(void);


#endif
