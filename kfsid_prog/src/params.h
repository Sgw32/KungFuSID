#ifndef PARAMS_H_
#define PARAMS_H_

#include <stdint.h>

extern char g_kfsidDeviceState[16];
extern char g_kfsidInstalledVersion[17];

uint8_t refreshDeviceInfo(void);
void checkDeviceInfo(void);
void autoTestDevice(void);
void editParameters(void);

#endif
