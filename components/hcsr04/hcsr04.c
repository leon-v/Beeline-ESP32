#include <driver/timer.h>

#include "components.h"
#include "device.h"

static component_t component = {
	.name = "HCSR04",
	.messagesOut = 1,
};

#define POWER_ENABLE_PIN 23

#define TIMER_DIVIDER		2  //  Hardware timer clock divider
#define TIMER_SCALE			(TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_SCALEUS		TIMER_SCALE * 1000000

static const char config_html_start[] asm("_binary_hcsr04_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/hcsr04_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static uint32_t timerCount;
static uint32_t samples;
static uint32_t required;
static uint32_t delay;


static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSu32(nvsHandle, "timerCount"	, timerCount);
	componentsSetNVSu32(nvsHandle, "samples"	, samples);
	componentsSetNVSu32(nvsHandle, "required"	, required);
	componentsSetNVSu32(nvsHandle, "delay"		, delay);
}

static void loadNVS(nvs_handle nvsHandle){
	timerCount =	componentsGetNVSu32(nvsHandle, "timerCount"	, 0);
	samples =		componentsGetNVSu32(nvsHandle, "samples"	, 50);
	required =		componentsGetNVSu32(nvsHandle, "required"	, 20);
	delay =			componentsGetNVSu32(nvsHandle, "delay"		, 20);
}

static unsigned int echoTimeoutCount = 0;
void IRAM_ATTR hcsr04TimerISR(void * arg){

	echoTimeoutCount++;

	timer_pause(TIMER_GROUP_0, TIMER_0);

	TIMERG0.int_clr_timers.t0 = 1;

	uint64_t timerTicks = 0;

	xQueueSendFromISR(component.queue, &timerTicks, NULL);
}

static unsigned int echoChangeCount = 0;
void IRAM_ATTR hcsr04EchoISR(void * arg){

	echoChangeCount++;

	// Pause timer first so nothing else can delay
	timer_pause(TIMER_GROUP_0, TIMER_0);

	uint32_t pin = (uint32_t) arg;

	// If echo pin went up, the pulse has finished transmitting
	if (gpio_get_level(pin)){

		timer_start(TIMER_GROUP_0, TIMER_0);
	}

	// If echo pin went down, the pulse echo was heard
	else{

		uint64_t timerTicks;
		timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &timerTicks);

		xQueueSendFromISR(component.queue, &timerTicks, NULL);
	}
}

#define POWER_UP_DELAY 200

void hcsr04Enable(void){

	gpio_set_level(POWER_ENABLE_PIN, 1);

	vTaskDelay(POWER_UP_DELAY / portTICK_RATE_MS);

	// Setup GPIO for echo pin
	gpio_config_t io_conf;
	io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (0x01 << CONFIG_HCSR04_ECHO_PIN);
    gpio_config(&io_conf);
}

void hcsr04Disable(void){

	// Setup GPIO for echo pin
	gpio_config_t io_conf;
	io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (0x01 << CONFIG_HCSR04_ECHO_PIN);
    gpio_config(&io_conf);

    vTaskDelay(POWER_UP_DELAY / portTICK_RATE_MS);

	gpio_set_level(POWER_ENABLE_PIN, 0);
}


void hcsr04Trigger(void){

	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

	timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);

	gpio_set_level(CONFIG_HCSR04_TRIG_PIN, 1);

	ets_delay_us(10);

	gpio_set_level(CONFIG_HCSR04_TRIG_PIN, 0);
}

static uint64_t queueItem = 0;

static void task(void * arg) {

	// Create default IO config
	gpio_config_t io_conf;
	io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;


    // Attach the ISR to the GPIO interrupt
    gpio_isr_handler_add(CONFIG_HCSR04_ECHO_PIN, hcsr04EchoISR, (void*) CONFIG_HCSR04_ECHO_PIN);

    // Setup the GPIO the trigger pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (0x01 << CONFIG_HCSR04_TRIG_PIN);
    gpio_config(&io_conf);


    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (0x01 << POWER_ENABLE_PIN);
    gpio_config(&io_conf);

    // Setup the timer that will count the time between pulse TX and RX
    timer_config_t config;
    config.auto_reload = TIMER_AUTORELOAD_DIS;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = 0;

    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    // Enable alarm to trigger timeout of the pulse and attach the ISR.
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 0.01 * TIMER_SCALE);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, hcsr04TimerISR, NULL, ESP_INTR_FLAG_IRAM, NULL);

    timer_enable_intr(TIMER_GROUP_0, TIMER_0);

    componentSetReady(&component);

	int count = 0;

	while (true) {

		if (componentReadyWait("Wake Timer") != ESP_OK) {
			continue;
		}

		if (componentQueueRecieve(&component, "Wake Timer", &queueItem) != ESP_OK) {
			continue;
		}

		ESP_LOGW(component.name, "Echo change count: %u, Echo timeout count %u.", echoChangeCount, echoTimeoutCount);

		// Skip if 0 / disabled
		if (!timerCount) {
			continue;
		}

		if (++count < timerCount) {
			continue;
		}

		hcsr04Enable();

		count = 0;

		// ESP_LOGW(component.name, "Got queue item from wake timer");

		int loop = 0;;
		int actualSamples = 0;

		static message_t message;
		strcpy(message.deviceName, deviceGetUniqueName());
		strcpy(message.sensorName, component.name);
		message.valueType = MESSAGE_FLOAT;
		message.floatValue = 0.00;

		uint64_t timerTicks;

		while (loop < samples) {

			// Don't delay on the first iteration since there is no ring
			if (loop){

				// Power down the device if the ring delay is longer than the power up / down delay
				if (delay > (POWER_UP_DELAY * 2)) {

					hcsr04Disable();

					vTaskDelay( (delay - (POWER_UP_DELAY * 2)) / portTICK_RATE_MS);

					hcsr04Enable();
				}
				else{
					vTaskDelay(delay / portTICK_RATE_MS);
				}
			}

			loop++;

			hcsr04Trigger();

			if (!xQueueReceive(component.queue, &timerTicks, 20 / portTICK_RATE_MS)) {
				// ESP_LOGE(component.name, " No response from module");
				continue;
			}

			if (timerTicks == 0){
				// ESP_LOGE(component.name, "Timeout while waiting for echo.");
				continue;
			}

			actualSamples++;
			message.floatValue+= timerTicks;

			if (actualSamples >= required){
				break;
			}
		}

		hcsr04Disable();

		if (actualSamples < required){
			ESP_LOGE(component.name, "Not enough samples.");
			continue;
		}

		message.floatValue = message.floatValue / actualSamples;

		message.floatValue = message.floatValue / TIMER_SCALEUS;

		message.floatValue = message.floatValue / 0.58;

		componentSendMessage(&component, &message);
	}

	vTaskDelete(NULL);

	return;
}


void hcsr04Init(void) {

	component.configPage		= &configPage;
	component.task				= &task;
	component.loadNVS			= &loadNVS;
	component.saveNVS			= &saveNVS;
	component.queueItemLength	= sizeof(queueItem);
	component.queueLength		= 1;

	componentsAdd(&component);
}