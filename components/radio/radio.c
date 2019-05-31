#include <crypto/crypto.h>
#include <math.h>

#include "components.h"
#include "sx1278.h"

static component_t component = {
	.name = "Radio",
	.messagesIn = 1,
	.messagesOut = 1
};

static uint32_t	sx1278Frequency;
static uint32_t	sx1278TXSync;
static uint32_t	sx1278RXSync;
static char *	sx1278Key;

static void * aesEncryptContext;
static void * aesDecryptContext;

static void saveNVS(nvs_handle nvsHandle){

	componentsSetNVSu32(nvsHandle, "sx1278Frequency", sx1278Frequency);
	componentsSetNVSu32(nvsHandle, "sx1278TXSync", sx1278TXSync);
	componentsSetNVSu32(nvsHandle, "sx1278RXSync", sx1278RXSync);
	componentsSetNVSString(nvsHandle, sx1278Key, "sx1278Key");
}

static void loadNVS(nvs_handle nvsHandle){

	sx1278Frequency	= componentsGetNVSu32(nvsHandle, "sx1278Frequency", 915000000);
	sx1278TXSync	= componentsGetNVSu32(nvsHandle, "sx1278TXSync", 190);
	sx1278RXSync	= componentsGetNVSu32(nvsHandle, "sx1278RXSync", 191);
	sx1278Key		= componentsGetNVSString(nvsHandle, sx1278Key, "sx1278Key", "-Encryption Key-");

	ESP_LOGI(component.name, "Setting key %s", sx1278Key);
	aesEncryptContext = aes_encrypt_init( (unsigned char *) sx1278Key, strlen(sx1278Key));

	if (!aesEncryptContext){
		ESP_LOGE(component.name, "aes_encrypt_init failed");
	}

	aesDecryptContext = aes_decrypt_init( (unsigned char *) sx1278Key, strlen(sx1278Key));

	if (!aesDecryptContext){
		ESP_LOGE(component.name, "aes_encrypt_init failed");
	}
}

static const char config_html_start[] asm("_binary_radio_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/radio_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static void IRAM_ATTR radioSX1278ISR(void * arg){

	static message_t message;

	message.valueType = MESSAGE_INTERRUPT;
	message.intValue = (uint32_t) arg;

	xQueueSendFromISR(component.messageQueue, &message, 0);
}

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
#define BLOCK_LENGTH 16

static void radioEncrypt(unsigned char * buffer, int * length){

	if (!aesEncryptContext) {
		return;
	}

	radioShift(buffer, 1, length);
	buffer[0] = ENC_TEST_CHAR;

	int blocks = ceil( (float) * length / BLOCK_LENGTH);

	for (int i = 0; i < blocks; i++) {

		int offset = i * BLOCK_LENGTH;
		aes_encrypt(aesEncryptContext, buffer + offset, buffer + offset);
	}

	*length = (blocks * BLOCK_LENGTH);
}

static int radioDecrypt(unsigned char * buffer, int * length){

	if (!aesDecryptContext) {
		return 0;
	}

	int blocks = ceil( (float) * length / BLOCK_LENGTH);

	for (int i = 0; i < blocks; i++) {

		int offset = i * BLOCK_LENGTH;
		aes_decrypt(aesDecryptContext, buffer + offset, buffer + offset);
	}

	if (buffer[0] != ENC_TEST_CHAR){
		return 0;
	}

	radioShift(buffer, -1, length);

	return 1;
}

static int radioCreateBuffer(unsigned char * radioBuffer, message_t * message) {

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

static void radioCreateMessage(message_t * message, unsigned char * radioBuffer, int bufferLength) {

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

static void radioSX1278SendRadioMessage(message_t * message){
	int length;

	unsigned char buffer[sizeof(message_t)] = {0};
	memset(buffer, 0, sizeof(message_t));

	length = radioCreateBuffer(buffer, message);

	radioEncrypt(buffer, &length);

	sx1278SetSyncWord( (unsigned char) sx1278TXSync);

	sx1278SendPacket(buffer, length);
}

static void radioSX1278Rx(void) {

	while(sx1278Received()) {

		int length;
		unsigned char buffer[sizeof(message_t)];
		message_t message;

		length = sizeof(message_t);
		length = sx1278ReceivePacket(buffer, length);

		int success = radioDecrypt(buffer, &length);

		if (!success) {
			ESP_LOGE(component.name, "Failed to decrypt packet.");
			continue;
		}

		radioCreateMessage(&message, buffer, length);

		componentSendMessage(&component, &message);
	}
}

static void task(void * arg) {

	gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (0x01 << CONFIG_IRQ_GPIO);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(CONFIG_IRQ_GPIO, radioSX1278ISR, (void*) CONFIG_IRQ_GPIO);
    gpio_isr_register(radioSX1278ISR, (void*) CONFIG_IRQ_GPIO, ESP_INTR_FLAG_LOWMED, NULL);

    ESP_ERROR_CHECK(gpio_wakeup_enable(CONFIG_IRQ_GPIO, GPIO_INTR_HIGH_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());


	sx1278Init();
	sx1278SetFrequency(sx1278Frequency);
	sx1278EnableCRC();

	componentSetReady(&component);

	while (true) {

		sx1278SetSyncWord( (unsigned char) sx1278RXSync);
		sx1278Receive();    // put into receive mode

		static message_t message;
		if (componentMessageRecieve(&component, &message) != ESP_OK) {
			continue;
		}

		if (message.valueType != MESSAGE_INTERRUPT){
			radioSX1278SendRadioMessage(&message);
			continue;
		}

		else if (message.valueType == MESSAGE_INTERRUPT){
			radioSX1278Rx();
		}
	}
}

void radioInit(void) {

	component.configPage	= &configPage;
	component.loadNVS		= &loadNVS;
	component.saveNVS		= &saveNVS;
	component.task			= &task;

	componentsAdd(&component);
}