#include <Component.h>

extern const uint8_t settingsFile[] asm("_binary_Device_json_start");
static component_t component = {
	.settingsFile	= (char *) settingsFile
};

pComponent_t deviceGetComponent(void) {
	return &component;
}