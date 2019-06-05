#ifndef _RADIO_H_
#define _RADIO_H_

int radioCreateBuffer(unsigned char * radioBuffer, message_t * message);
void radioCreateMessage(message_t * message, unsigned char * radioBuffer, int bufferLength);

void radioEncryptInit(void ** aesEncryptContext, char * key);
void radioDecryptInit(void ** aesDecryptContext, char * key);

int radioDecrypt(void * aesDecryptContext, unsigned char * buffer, int * length);
void radioEncrypt(void * aesEncryptContext, unsigned char * buffer, int * length);


void radioInit(void);

#endif