

#include "MCP79410RK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;
MCP79410 rtc;

void setup() {
	// Make sure you call rtc.setup() from setup!
	rtc.setup();

	rtc.setOscTrim(0);

	rtc.setSquareWaveMode(MCP79410::SQUARE_WAVE_4096_HZ);
}

void loop() {
	// Make sure you call rtc.loop() from loop!
	rtc.loop();
}
