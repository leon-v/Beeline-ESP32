#include <crypto/crypto.h>
#include <math.h>

#include "radio_sx1278.h"

#include "components.h"

static void radioShift(unsigned char * buffer, int amount, int * length){

	if (amount > 0){
		for (int i = * length + amount; i > 0; i--){
			buffer[i] = buffer[i - amount];
		}
	}
	else if (amount < 0) {

		for (int i = 0; i < * length; i++){
			buffer[i] = buffer[i + -amount];
		}
	}

	* length = * length + amount;
}

#define ENC_TEST_CHAR 0xAB
#define AES_BLOCK_SIZE 16

void radioEncryptInit(void ** aesEncryptContext, char * key) {
	* aesEncryptContext = aes_encrypt_init( (unsigned char *) key, strlen(key));
}

void radioEncrypt(void * aesEncryptContext, unsigned char * buffer, int * length) {

	if (!aesEncryptContext) {
		return;
	}

	radioShift(buffer, 1, length);
	buffer[0] = ENC_TEST_CHAR;

	int blocks = ceil( (float) * length / AES_BLOCK_SIZE);

	for (int i = 0; i < blocks; i++) {

		int offset = i * AES_BLOCK_SIZE;
		aes_encrypt(aesEncryptContext, buffer + offset, buffer + offset);
	}

	*length = (blocks * AES_BLOCK_SIZE);
}

void radioDecryptInit(void ** aesDecryptContext, char * key) {
	* aesDecryptContext = aes_decrypt_init( (unsigned char *) key, strlen(key));
}

int radioDecrypt(void * aesDecryptContext, unsigned char * buffer, int * length) {

	if (!aesDecryptContext) {
		return 0;
	}

	int blocks = ceil( (float) * length / AES_BLOCK_SIZE);

	for (int i = 0; i < blocks; i++) {

		int offset = i * AES_BLOCK_SIZE;
		aes_decrypt(aesDecryptContext, buffer + offset, buffer + offset);
	}

	if (buffer[0] != ENC_TEST_CHAR){
		return 0;
	}

	radioShift(buffer, -1, length);

	return 1;
}

int radioCreateBuffer(unsigned char * radioBuffer, message_t * message) {

	int length = 0;
	unsigned char * bytes = radioBuffer;

	length = strlen(message->deviceName) + 1;
	memcpy(bytes, message->deviceName, length);
	bytes+= length;

	length = strlen(message->sensorName) + 1;
	memcpy(bytes, message->sensorName, length);
	bytes+= length;

	length = sizeof(message->valueType);
	memcpy(bytes, &message->valueType, length);
	bytes+= length;

	switch (message->valueType){

		case MESSAGE_INT:
			length = sizeof(message->intValue);
			memcpy(bytes, &message->intValue, length);
			bytes+= length;
		break;
		case MESSAGE_FLOAT:
			length = sizeof(message->floatValue);
			memcpy(bytes, &message->floatValue, length);
			bytes+= length;
		break;
		case MESSAGE_DOUBLE:
			length = sizeof(message->doubleValue);
			memcpy(bytes, &message->doubleValue, length);
			bytes+= length;
		break;
		case MESSAGE_STRING:
			length = strlen(message->stringValue) + 1;
			memcpy(bytes, message->stringValue, length);
			bytes+= length;
		break;
	}

	return bytes - radioBuffer;
}

void radioCreateMessage(message_t * message, unsigned char * radioBuffer, int bufferLength) {

	unsigned char * bytes = radioBuffer;
	int length;

	length = strlen((char *) bytes);
	memcpy(message->deviceName, bytes, length);
	message->deviceName[length] = '\0';
	bytes+= length + 1;

	length = strlen((char *) bytes);
	memcpy(message->sensorName, bytes, length);
	message->sensorName[length] = '\0';
	bytes+= length + 1;

	length = sizeof(message->valueType);
	memcpy(&message->valueType, bytes, length);
	bytes+= length;

	switch (message->valueType){

		case MESSAGE_INT:
			length = sizeof(message->intValue);
			memcpy(&message->intValue, bytes, length);
			bytes+= length;
		break;

		case MESSAGE_FLOAT:
			length = sizeof(message->intValue);
			memcpy(&message->floatValue, bytes, length);
			bytes+= length;
		break;

		case MESSAGE_DOUBLE:
			length = sizeof(message->doubleValue);
			memcpy(&message->doubleValue, bytes, length);
			bytes+= length;
		break;

		case MESSAGE_STRING:
			length = strlen((char *) bytes) + 1;
			memcpy(message->stringValue, bytes, length);
			bytes+= length + 1;
		break;
	}

}

void radioInit(void) {

	#if RADIO_SX1278_DRIVER_ENABLE == 'y'
		radioSX1278Init();
	#endif

}