#ifndef _DEVICE_H_
#define _DEVICE_H_

void deviceInit(void);
char * deviceGetUniqueName(void);
void deviceLog(const char * template, ...);

#endif