
#include "../include/otaUpdater.h"
#include <SmingCore/SmingCore.h>


rBootHttpUpdate* otaUpdater = nullptr;

void  OtaUpdate_CallBack(bool result);

//void  Switch() {
//	uint8 before, after;
//	before = rboot_get_current_rom();
//	if (before == 0)
//		after = 1;
//	else
//		after = 0;
//	//Serial.printf("Swapping from rom %d to rom %d.\r\n", before, after);
//	rboot_set_current_rom(after);
//	//Serial.println("Restarting...\r\n");
//	System.restart();
//}

void OtaUpdate() {

	uint8 slot;
	rboot_config bootconf;

	//Serial.println("Updating...");

// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater)
		delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();

// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0)
		slot = 1;
	else
		slot = 0;

#ifndef RBOOT_TWO_ROMS
// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
#else
// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], ROM_1_URL);
	}
#endif

#ifndef DISABLE_SPIFFS
// use user supplied values (defaults for 4mb flash in makefile)
	if (slot == 0) {
		otaUpdater->addItem(RBOOT_SPIFFS_0, SPIFFS_URL);
	} else {
		otaUpdater->addItem(RBOOT_SPIFFS_1, SPIFFS_URL);
	}
#endif

// request switch and reboot on success
//otaUpdater->switchToRom(slot);
// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

// start update
	otaUpdater->start();
}

void  OtaUpdate_CallBack(bool result) {

	//Serial.println("In callback...");
	if (result == true) {
// success
		uint8 slot;
		slot = rboot_get_current_rom();
		if (slot == 0)
			slot = 1;
		else
			slot = 0;
		rboot_set_current_rom(slot);
		System.restart();
	} else {
// fail

		//Serial.println("CALLBACK OTA FAIL...");
	}
}