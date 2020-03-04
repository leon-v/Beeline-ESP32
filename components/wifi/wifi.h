#ifndef _WIFI_H_
#define _WIFI_H_

pComponent_t wiFiGetComponent(void);

void wiFiClientSetConfig(pComponent_t pComponent, wifi_config_t * wifi_config);

void wiFiAccessPointSetConfig(pComponent_t pComponent, wifi_config_t * wiFiConfig);

void wiFiScanSetConfig(pComponent_t pComponent, wifi_scan_config_t * scanConfig);
esp_err_t wiFiScanSSIDOptionsInit(pComponent_t pComponent);

void wiFiScanBuildOptions(pComponent_t pComponent);

#endif