#include "components.h"
#include "radio.h"
#include "sx1278.h"

static component_t component = {
	.name = "SX1278",
	.messagesIn = 1,
	.messagesOut = 1
};

static const char config_html_start[] asm("_binary_radio_sx1278_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/radio_sx1278_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
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
	radioEncryptInit(&aesEncryptContext, sx1278Key);


	if (!aesEncryptContext){
		ESP_LOGE(component.name, "radioEncryptInit failed");
	}

	radioDecryptInit(&aesDecryptContext, sx1278Key);

	if (!aesDecryptContext){
		ESP_LOGE(component.name, "aes_encrypt_init failed");
	}
}

static void radioSX1278SendRadioMessage(message_t * message) {
	int length;

	unsigned char buffer[sizeof(message_t)] = {0};
	memset(buffer, 0, sizeof(message_t));

	length = radioCreateBuffer(buffer, message);

	radioEncrypt(aesEncryptContext, buffer, &length);

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

		int success = radioDecrypt(aesDecryptContext, buffer, &length);

		if (!success) {
			ESP_LOGE(component.name, "Failed to decrypt packet.");
			continue;
		}

		radioCreateMessage(&message, buffer, length);

		componentSendMessage(&component, &message);
	}
}

static void IRAM_ATTR radioSX1278ISR(void * arg){

	static message_t message;

	message.valueType = MESSAGE_INTERRUPT;
	message.intValue = (uint32_t) arg;

	xQueueSendFromISR(component.messageQueue, &message, 0);
}

static void task(void * arg) {

	gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (0x01 << CONFIG_RADIO_SX1278_IRQ_GPIO);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(CONFIG_RADIO_SX1278_IRQ_GPIO, radioSX1278ISR, (void*) CONFIG_RADIO_SX1278_IRQ_GPIO);
    gpio_isr_register(radioSX1278ISR, (void*) CONFIG_RADIO_SX1278_IRQ_GPIO, ESP_INTR_FLAG_LOWMED, NULL);

    ESP_ERROR_CHECK(gpio_wakeup_enable(CONFIG_RADIO_SX1278_IRQ_GPIO, GPIO_INTR_HIGH_LEVEL));
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

void radioSX1278Init(void) {

	component.configPage	= &configPage;
	component.loadNVS		= &loadNVS;
	component.saveNVS		= &saveNVS;
	component.task			= &task;

	componentsAdd(&component);
}