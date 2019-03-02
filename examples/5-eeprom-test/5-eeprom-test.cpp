
// This is the EEPROM test program. It's designed to unit test the EEPROM, though
// it does show how to use all of the features in a somewhat obscure way.
//
// Flash this to a device with the MCP79410 attached and monitor the USB serial.
// It will run through tests and then prompt you to unplug the USB power and plug it back in.
// It will run more tests and prompt you to unplug the USB power and battery.


// You should not normally define MCP79410_ENABLE_PROTECTED_WRITE. It allows writing to the protected block of EEPROM,
// a separate 8-byte EEPROM block. This is normally used for things like MAC addresses or board identification that you
// set once at the factory and do not update in the field. If used, define this before the first #include "MCP79410RK.h"
#define MCP79410_ENABLE_PROTECTED_WRITE

#include "MCP79410RK.h"

SYSTEM_THREAD(ENABLED);

MCP79410 rtc;

// Set to LOG_LEVEL_TRACE for more detailed status and debugging/
SerialLogHandler logHandler(LOG_LEVEL_INFO);

const uint32_t RESUME_MAGIC1 = 0xe63cb98a;
const uint32_t RESUME_MAGIC2 = 0xfd4e1502;

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
	PROTECTED_BLOCK_STATE,
	BLOCK_PROTECTION_STATE,
	CLEANUP_STATE,
	POWER_CHECK_STATE,
	BATTERY_CHECK_STATE,
	DONE_STATE,
	WAIT_STATE
};
int state = START_WAIT_STATE;
unsigned long stateTime = 0;

char debugBuf[128];

#define assertEqual(v1, v2, type) \
	if (v1 != v2) Log.error("test failed line=%u v1=" type " v2=" type " %s", __LINE__, v1, v2, debugBuf); \
	debugBuf[0] = 0;


void setup() {
	Serial.begin();

	rtc.setup();

	// Wait for a USB serial connection for up to 10 seconds
	waitFor(Serial.isConnected, 10000);
}

void loop() {
	rtc.loop();

	uint8_t buf[128];
	bool bResult;

	switch(state) {
	case START_WAIT_STATE:
		if (millis() - stateTime >= 15000) {
			state = INITIAL_CHECK_STATE;
		}
		break;

	case INITIAL_CHECK_STATE:
		{
			uint32_t u32;
			rtc.eeprom().get(0, u32);
			if (u32 == RESUME_MAGIC1) {
				uint32_t u32 = RESUME_MAGIC2;
				rtc.eeprom().put(0, u32);

				Log.info("remove USB and battery power for a several seconds to test non-volatility");
				state = WAIT_STATE;
				break;
			}
			else
			if (u32 == RESUME_MAGIC2) {
				Log.info("no battery, no usb test complete");
				state = CLEANUP_STATE;
				break;

			}

			uint8_t protection = rtc.eeprom().getBlockProtection();
			assertEqual(protection, 0, "%02x");

			bResult = rtc.eeprom().readData(0, buf, sizeof(buf));
			assertEqual(bResult, true, "%d");

			bool isErased = true;
			for(size_t ii = 0; ii < sizeof(buf); ii++) {
				if (buf[ii] != 0xff) {
					snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
					assertEqual(buf[ii], 0xff, "%02x");
					isErased = false;
				}
			}
			if (!isErased) {
				Log.info("EEPROM was not erased");
				rtc.eeprom().erase();
				stateTime = millis();
				state = START_WAIT_STATE;
				//break;
			}

			Log.info("initial check completed");
			state = BYTE_READ_STATE;
		}
		break;

	case BYTE_READ_STATE:
		{
			for(size_t ii = 0; ii < sizeof(buf); ii++) {
				bResult = rtc.eeprom().readData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(buf[0], 0xff, "%02x");
			}
		}
		Log.info("byte read test completed");
		state = BYTE_WRITE_READ_STATE;
		break;

	case BYTE_WRITE_READ_STATE:
		{
			uint8_t b;

			for(size_t ii = 0; ii < sizeof(buf); ii++) {
				b = (uint8_t) rand();

				buf[0] = b;
				bResult = rtc.eeprom().writeData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				bResult = rtc.eeprom().readData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(buf[0], b, "%02x");
			}

		}
		Log.info("byte write read test completed");
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
			rtc.eeprom().put(ii, a1);
			ii += sizeof(a1);

			rtc.eeprom().put(ii, a2);
			ii += sizeof(a2);

			rtc.eeprom().put(ii, a3);
			ii += sizeof(a3);

			rtc.eeprom().put(ii, a4);
			ii += sizeof(a4);

			rtc.eeprom().writeData(ii, (const uint8_t *)a5, sizeof(b5));
			ii += sizeof(b5);

			a6.a = rand();
			a6.b = rand();
			strcpy(a6.c, "hello world");
			rtc.eeprom().put(ii, a6);
			ii += sizeof(a6);

			ii = 0;
			rtc.eeprom().get(ii, b1);
			ii += sizeof(b1);
			assertEqual(a1, b1, "%02x");

			rtc.eeprom().get(ii, b2);
			ii += sizeof(b2);
			assertEqual(a2, b2, "%d");

			rtc.eeprom().get(ii, b3);
			ii += sizeof(b3);
			assertEqual(a3, b3, "%d");

			rtc.eeprom().get(ii, b4);
			ii += sizeof(b4);
			assertEqual(a4, b4, "%d");

			rtc.eeprom().readData(ii, (uint8_t *)b5, sizeof(b5));
			ii += sizeof(b5);
			if (strcmp(a5, b5) != 0) {
				Log.error("string mismatch a5=%s b5=%s line=%d", a5, b5, __LINE__);
			}

			rtc.eeprom().get(ii, b6);
			ii += sizeof(b6);
			assertEqual(a6.a, b6.a, "%d");
			assertEqual(a6.b, b6.b, "%d");
			if (strcmp(a6.c, b6.c) != 0) {
				Log.error("string mismatch a6.c=%s b6.c=%s line=%d", a6.c, b6.c, __LINE__);
			}

		}
		Log.trace("DATA_WRITE_READ test completed");

		state = PROTECTED_BLOCK_STATE;
		break;

	case PROTECTED_BLOCK_STATE:
		{
			uint8_t aBlock[MCP79410::EEPROM_PROTECTED_BLOCK_SIZE];
			uint8_t bBlock[MCP79410::EEPROM_PROTECTED_BLOCK_SIZE];

			rtc.eeprom().protectedBlockRead(aBlock);
			for(size_t ii = 0; ii < MCP79410::EEPROM_PROTECTED_BLOCK_SIZE; ii++) {
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(aBlock[ii], 0xff, "%02x");
			}

			for(size_t ii = 0; ii < MCP79410::EEPROM_PROTECTED_BLOCK_SIZE; ii++) {
				bBlock[ii] = (uint8_t) rand();
			}
			rtc.eepromProtectedBlockWrite(bBlock);

			rtc.eeprom().protectedBlockRead(aBlock);
			for(size_t ii = 0; ii < MCP79410::EEPROM_PROTECTED_BLOCK_SIZE; ii++) {
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(aBlock[ii], bBlock[ii], "%02x");
			}

			for(size_t ii = 0; ii < MCP79410::EEPROM_PROTECTED_BLOCK_SIZE; ii++) {
				bBlock[ii] = 0xff;
			}
			rtc.eepromProtectedBlockWrite(bBlock);

			rtc.eeprom().protectedBlockRead(aBlock);
			for(size_t ii = 0; ii < MCP79410::EEPROM_PROTECTED_BLOCK_SIZE; ii++) {
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(aBlock[ii], 0xff, "%02x");
			}


		}

		Log.trace("PROTECTED_BLOCK test completed");

		state = BLOCK_PROTECTION_STATE;
		break;

	case BLOCK_PROTECTION_STATE:
		{
			uint8_t protection = rtc.eeprom().getBlockProtection();
			assertEqual(protection, 0, "%02x");

			int a = rand(), b;

			rtc.eeprom().put(0, a);

			rtc.eeprom().get(0, b);
			assertEqual(b, a, "%d");

			b = -1;
			rtc.eeprom().put(0, b);

			b = 0;
			rtc.eeprom().get(0, b);
			assertEqual(b, -1, "%d");

			rtc.eeprom().setBlockProtection(MCP79410::EEPROM_PROTECT_ALL);

			rtc.eeprom().put(0, a);

			rtc.eeprom().get(0, b);
			assertEqual(b, -1, "%d");

			rtc.eeprom().setBlockProtection(MCP79410::EEPROM_PROTECT_NONE);

			protection = rtc.eeprom().getBlockProtection();
			assertEqual(protection, 0, "%02x");

		}

		Log.trace("BLOCK_PROTECTION test completed");

		{
			uint32_t u32 = RESUME_MAGIC1;
			rtc.eeprom().put(0, u32);

			Log.info("remove USB power for a few seconds to test non-volatility");
		}

		state = WAIT_STATE;
		break;

	case CLEANUP_STATE:
		Log.trace("cleaning up, erasing eeprom");
		rtc.eeprom().erase();

		state = DONE_STATE;
		break;


	case DONE_STATE:
		//
		Log.info("all tests completed!");
		state = WAIT_STATE;
		stateTime = millis();
		break;

	case WAIT_STATE:
		break;
	}
}

