// This is the sleep mode sample program. It assumes that the MFP on the MCP79410 is connected to pin D8.
// You must have a 2.2K pull-up resistor connected to MFP and D8. It really must be a 2.2K resistor.
// Because SLEEP_MODE_DEEP puts a 13K pull-down resistor on D8, and MFP is an open-collector driver,
// if you use, say a 10K pull-up resistor you'll only get 1.9V, not enough to wake up!

#include "MCP79410RK.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

MCP79410 rtc;

void setup() {
	// Make sure you call rtc.setup() from setup!
	rtc.setup();
}

void loop() {
	// Make sure you call rtc.loop() from loop!
	rtc.loop();


	// Wait 20 seconds after boot to try sleep
	if (millis() > 20000) {
		if (rtc.setAlarm(10)) {
			Log.info("About to SLEEP_MODE_DEEP for 10 seconds");
			System.sleep(SLEEP_MODE_DEEP);
		}
		else {
			Log.info("Failed to setAlarm, not sleeping");
			delay(10000);
		}
	}
}
