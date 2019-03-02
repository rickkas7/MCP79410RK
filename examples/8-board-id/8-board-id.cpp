// This example shows how you'd read an 8-byte structure with information about your board (that has the RTC on it)
// from protected block EEPROM.
//
// You'd run code like the 7-board-id-set example to set the values during manufacture.
//
// You'd add code like the following to your own firmware to read the structure and presumably do something with it other
// than just print it to debug serial.

#include "MCP79410RK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;
MCP79410 rtc;

typedef union {
	struct {
		uint16_t boardType;
		uint16_t boardVersion;
		uint32_t featureFlags;
	} data;
	uint8_t bytes[MCP79410::EEPROM_PROTECTED_BLOCK_SIZE]; // 8 bytes
} BoardId;


void setup() {
	rtc.setup();

	// Wait for a USB serial connection for up to 10 seconds. This is just so you can see the Log.info
	// statement, you'd probably leave this out of real code
	waitFor(Serial.isConnected, 10000);

	// Read the BoardId structure from the protected block EEPROM
	BoardId boardId;
	rtc.eeprom().protectedBlockRead(boardId.bytes);

	Log.info("boardType=%04x boardVersion=%04x featureFlags=%08x", boardId.data.boardType, boardId.data.boardVersion, boardId.data.featureFlags);
}

void loop() {
	rtc.loop();
}
