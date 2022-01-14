#ifndef OTA_UPDATER_H_
#define OTA_UPDATER_H_

#define SERVER_IP "192.168.0.4"
#define SERVER_URL "http://" SERVER_IP "/firmware/main/"
#define ROM_0_URL  SERVER_URL "rom0.bin"
#define ROM_1_URL  SERVER_URL "rom1.bin"
#define SPIFFS_URL SERVER_URL "spiff_rom.bin"
void OtaUpdate();


#endif /* OTA_UPDATER_H_ */