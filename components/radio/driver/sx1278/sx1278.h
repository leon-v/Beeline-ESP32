
#ifndef __LORA_H__
#define __LORA_H__

void sx1278Reset(void);
void sx1278ExplicitHeaderMode(void);
void sx1278ImplicitHeaderMode(int size);
void sx1278Idle(void);
void sx1278Sleep(void);
void sx1278Receive(void);
void sx1278SetTXPower(int level);
void sx1278SetFrequency(long frequency);
void sx1278SetSpreadingFactor(int sf);
void sx1278SetBandwidth(long sbw);
void sx1278SetCoding_rate(int denominator);
void sx1278SetPreambleLength(long length);
void sx1278SetSyncWord(int sw);
void sx1278EnableCRC(void);
void sx1278DisableCRC(void);
int sx1278Init(void);
void sx1278SendPacket(uint8_t *buf, int size);
int sx1278ReceivePacket(uint8_t *buf, int size);
int sx1278Received(void);
int sx1278PacketRSSI(void);
float sx1278PacketSNR(void);
void sx1278Close(void);
int sx1278Unitialized(void);
void sx1278DumpRegisters(void);

#endif
