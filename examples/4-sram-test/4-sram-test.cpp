
// This is the SRAM test program. It's designed to unit test the SRAM, though
// it does show how to use all of the features in a somewhat obscure way.
//
// Flash this to a device with the MCP79410 attached and monitor the USB serial.
// It will run through tests and then prompt you to unplug the USB power and plug it back in
// to test that the SRAM can maintain data with no USB power (only battery).

#include "MCP79410RK.h"

SYSTEM_THREAD(ENABLED);

MCP79410 rtc;
SerialLogHandler logHandler;

const uint32_t RESUME_MAGIC1 = 0x4e67de7f;

typedef struct {
	int a;
	int b;
	char c[16];
} TestStruct;

enum {
	START_WAIT_STATE = 0,
	INITIAL_CHECK_STATE,
	BYTE_READ_STATE,
	BYTE_WRITE_READ_STATE,
	DATA_WRITE_READ_STATE,
	UNPOWER_STATE,
	RESUME_SUCCESS_STATE,
	WAIT_STATE,
	DONE_STATE
};
int state = START_WAIT_STATE;
unsigned long stateTime = 0;
bool hasPowerFail;
MCP79410Time powerUpTime;
MCP79410Time powerDownTime;

char debugBuf[128];

#define assertEqual(v1, v2, type) \
	if (v1 != v2) Log.error("test failed line=%u v1=" type " v2=" type " %s", __LINE__, v1, v2, debugBuf); \
	debugBuf[0] = 0;


void setup() {
	Serial.begin();

	rtc.setup();

	// The power failure information isn't all that useful (it's missing the year and second, oddly), but
	// if you do want to use it, you should get the data in setup(). The reason is that setting the RTC
	// clears the power failure information, and we often reset the RTC from cloud time after connecting
	// to the cloud.
	hasPowerFail = rtc.getPowerFail();
	if (hasPowerFail) {
		rtc.getPowerDownTime(powerDownTime);
		rtc.getPowerUpTime(powerUpTime);
	}

	// Wait for a USB serial connection for up to 10 seconds
	waitFor(Serial.isConnected, 10000);


}

void loop() {
	rtc.loop();

	uint8_t buf[64];
	bool bResult;
	uint32_t u32 = 0;

	switch(state) {
	case START_WAIT_STATE:
		if (millis() - stateTime >= 15000) {
			state = INITIAL_CHECK_STATE;
		}
		break;


	case INITIAL_CHECK_STATE:
		rtc.sram().get(0, u32);
		if (u32 == RESUME_MAGIC1) {
			state = RESUME_SUCCESS_STATE;
			break;
		}

		// Clear the power failure register
		rtc.clearPowerFail();

		bResult = rtc.sram().erase();
		assertEqual(bResult, true, "%d");

		bResult = rtc.sram().readData(0, buf, 64);
		assertEqual(bResult, true, "%d");
		for(size_t ii = 0; ii < 64; ii++) {
			snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
			assertEqual(buf[ii], 0, "%02x");
		}
		Log.trace("INITIAL_CHECK test completed");

		state = BYTE_READ_STATE;
		break;

	case BYTE_READ_STATE:
		{
			for(size_t ii = 0; ii < 64; ii++) {
				bResult = rtc.sram().readData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(buf[0], 0, "%02x");
			}
		}
		Log.trace("BYTE_READ test completed");
		state = BYTE_WRITE_READ_STATE;
		break;

	case BYTE_WRITE_READ_STATE:
		{
			uint8_t b;

			for(size_t ii = 0; ii < 64; ii++) {
				b = (uint8_t) rand();

				buf[0] = b;
				bResult = rtc.sram().writeData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				bResult = rtc.sram().readData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(buf[0], b, "%02x");
			}

		}
		Log.trace("BYTE_WRITE_READ test completed");
		state = DATA_WRITE_READ_STATE;
		break;

	case DATA_WRITE_READ_STATE:
		{
			uint8_t a1 = (uint8_t) rand(), b1;
			int8_t a2 = (int8_t) rand(), b2;
			int16_t a3 = (int16_t) rand(), b3;
			int32_t a4 = (int32_t) rand(), b4;
			const char *a5 = "testing!";
			char b5[10];
			TestStruct a6, b6;


			size_t ii = 0;
			rtc.sram().put(ii, a1);
			ii += sizeof(a1);

			rtc.sram().put(ii, a2);
			ii += sizeof(a2);

			rtc.sram().put(ii, a3);
			ii += sizeof(a3);

			rtc.sram().put(ii, a4);
			ii += sizeof(a4);

			rtc.sram().writeData(ii, (const uint8_t *)a5, sizeof(b5));
			ii += sizeof(b5);

			a6.a = rand();
			a6.b = rand();
			strcpy(a6.c, "hello world");
			rtc.sram().put(ii, a6);
			ii += sizeof(a6);

			ii = 0;
			rtc.sram().get(ii, b1);
			ii += sizeof(b1);
			assertEqual(a1, b1, "%02x");

			rtc.sram().get(ii, b2);
			ii += sizeof(b2);
			assertEqual(a2, b2, "%d");

			rtc.sram().get(ii, b3);
			ii += sizeof(b3);
			assertEqual(a3, b3, "%d");

			rtc.sram().get(ii, b4);
			ii += sizeof(b4);
			assertEqual(a4, b4, "%d");

			rtc.sram().readData(ii, (uint8_t *)b5, sizeof(b5));
			ii += sizeof(b5);
			if (strcmp(a5, b5) != 0) {
				Log.error("string mismatch a5=%s b5=%s line=%d", a5, b5, __LINE__);
			}

			rtc.sram().get(ii, b6);
			ii += sizeof(b6);
			assertEqual(a6.a, b6.a, "%d");
			assertEqual(a6.b, b6.b, "%d");
			if (strcmp(a6.c, b6.c) != 0) {
				Log.error("string mismatch a6.c=%s b6.c=%s line=%d", a6.c, b6.c, __LINE__);
			}

		}
		Log.trace("DATA_WRITE_READ test completed");
		state = UNPOWER_STATE;
		break;


	case UNPOWER_STATE:
		u32 = RESUME_MAGIC1;
		rtc.sram().put(0, u32);

		Log.info("remove USB power for several seconds, then power back up");
		state = WAIT_STATE;
		break;


	case RESUME_SUCCESS_STATE:
		Log.info("successfully saved data with power removed!");
		bResult = rtc.sram().erase();
		assertEqual(bResult, true, "%d");


		assertEqual(hasPowerFail, true, "%d");
		Log.info("powerDown=%s", powerDownTime.toStringRaw().c_str());
		Log.info("powerUp=%s", powerUpTime.toStringRaw().c_str());

		state = DONE_STATE;
		break;

	case WAIT_STATE:
		break;

	case DONE_STATE:
		//
		Log.info("all tests completed!");
		state = WAIT_STATE;
		stateTime = millis();
		break;
	}
}

