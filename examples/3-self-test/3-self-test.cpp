// This is the MCP79410 self-test suite. It runs continuously and tests the RTC, wake modes, SRAM, and EEPROM.


// You should not normally define MCP79410_ENABLE_PROTECTED_WRITE. It allows writing to the protected block of EEPROM,
// a separate 8-byte EEPROM block. This is normally used for things like MAC addresses or board identification that you
// set once at the factory and do not update in the field. If used, define this before the first #include "MCP79410RK.h"
#define MCP79410_ENABLE_PROTECTED_WRITE

#include "MCP79410RK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);
MCP79410 rtc;

void runTimeClassTests();

const unsigned long testPeriodMs = 10000;

unsigned long stateTime = 0;

typedef struct {
	int a;
	int b;
	char c[16];
} TestStruct;

char debugBuf[128];

#define assertEqual(v1, v2, type) \
	if (v1 != v2) Log.error("test failed line=%u v1=" type " v2=" type " %s", __LINE__, v1, v2, debugBuf); \
	debugBuf[0] = 0;

enum {
	START_WAIT_STATE,
	TIME_CLASS_STATE,
	ALARM_FROM_NOW_START_STATE,
	ALARM_FROM_NOW_WAIT_STATE,
	ALARM_SEC_START_STATE,
	ALARM_SEC_WAIT_STATE,
	ALARM_SEC_WAIT2_STATE,
	ALARM_SEC_WAIT3_STATE,
	ALARM_SEC_WAIT4_STATE,
	ALARM_MIN_START_STATE,
	ALARM_MIN_WAIT_STATE,
	ALARM_HOUR_START_STATE,
	ALARM_HOUR_WAIT_STATE,
	ALARM_DAYOFWEEK_START_STATE,
	ALARM_DAYOFWEEK_WAIT_STATE,
	ALARM_DAYOFMONTH_START_STATE,
	ALARM_DAYOFMONTH_WAIT_STATE,

	SRAM_INITIAL_CHECK_STATE,
	SRAM_BYTE_READ_STATE,
	SRAM_BYTE_WRITE_READ_STATE,
	SRAM_DATA_WRITE_READ_STATE,

	EEPROM_INITIAL_CHECK_STATE,
	EEPROM_BYTE_READ_STATE,
	EEPROM_BYTE_WRITE_READ_STATE,
	EEPROM_DATA_WRITE_READ_STATE,
	EEPROM_PROTECTED_BLOCK_STATE,
	EEPROM_BLOCK_PROTECTION_STATE,
	EEPROM_CLEANUP_STATE,

	DONE_STATE
};
int state = START_WAIT_STATE;
int curAlarm = 0;

void setup() {
	rtc.setup();
	pinMode(D8, INPUT);
}

void loop() {
	bool bResult;
	MCP79410Time t;
	int sec;
	uint8_t buf[64];
	uint32_t u32 = 0;

	switch(state) {
	case START_WAIT_STATE:
		if (millis() - stateTime >= testPeriodMs) {
			state = TIME_CLASS_STATE;
		}
		break;

	case TIME_CLASS_STATE:
		runTimeClassTests();
		state = SRAM_INITIAL_CHECK_STATE;
		break;

	case ALARM_FROM_NOW_START_STATE:
		rtc.clearAlarm(curAlarm);
		assertEqual((int)digitalRead(D8), 0, "%d");
		bResult = rtc.setAlarm(3, true, curAlarm);
		stateTime = millis();
		state = ALARM_FROM_NOW_WAIT_STATE;
		break;

	case ALARM_FROM_NOW_WAIT_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			Log.trace("ALARM_FROM_NOW test completed!");

			state = ALARM_MIN_START_STATE;
		}
		else
		if (millis() - stateTime >= 5000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;

	case ALARM_SEC_START_STATE:
		rtc.clearAlarm(curAlarm);
		assertEqual((int)digitalRead(D8), 0, "%d");

		rtc.getRTCTime(t);
		sec = (t.getSecond() + 3) % 60;

		t.clear();
		t.setAlarmSecond(sec);
		bResult = rtc.setAlarm(t, true, curAlarm);
		stateTime = millis();
		state = ALARM_SEC_WAIT_STATE;
		break;

	case ALARM_SEC_WAIT_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			state = ALARM_SEC_WAIT2_STATE;
			stateTime = millis();
		}
		else
		if (millis() - stateTime >= 5000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;

	case ALARM_SEC_WAIT2_STATE:
		// This test makes sure that the timer is only firing when transitioning into that second, not
		// repeatedly
		if (digitalRead(D8) == 1) {
			Log.error("refire test failed %d", __LINE__);
			state = DONE_STATE;
		}
		if (millis() - stateTime >= 2000) {
			Log.trace("ALARM_SEC test part1 completed! Will now wait 1 minute for timer to fire again...");
			state = ALARM_SEC_WAIT3_STATE;
		}
		break;

	case ALARM_SEC_WAIT3_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			Log.trace("ALARM_SEC test part2 completed! Will now wait 1 minute to make sure timer was cleared...");

			rtc.clearAlarm(curAlarm);

			state = ALARM_SEC_WAIT4_STATE;
			stateTime = millis();
		}
		else
		if (millis() - stateTime >= 63000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;

	case ALARM_SEC_WAIT4_STATE:
		if (digitalRead(D8) == 1) {
			Log.error("clear timer test failed %d", __LINE__);
			state = DONE_STATE;
		}
		if (millis() - stateTime >= 64000) {
			Log.trace("ALARM_SEC test completed!");
			state = DONE_STATE;
		}
		break;


	case ALARM_MIN_START_STATE:

		rtc.clearAlarm(curAlarm);
		assertEqual((int)digitalRead(D8), 0, "%d");

		// Set the RTC to
		rtc.getRTCTime(t);
		t.setSecond(55);
		t.setMinute(29);
		rtc.setRTCTime(t.toUnixTime());

		t.clear();
		t.setAlarmMinute(30);
		bResult = rtc.setAlarm(t, true, curAlarm);
		stateTime = millis();
		state = ALARM_MIN_WAIT_STATE;
		break;

	case ALARM_MIN_WAIT_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			Log.trace("ALARM_MIN test completed!");
			state = ALARM_HOUR_START_STATE;
			stateTime = millis();
		}
		else
		if (millis() - stateTime >= 8000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;

	case ALARM_HOUR_START_STATE:

		rtc.clearAlarm(curAlarm);
		assertEqual((int)digitalRead(D8), 0, "%d");

		// Set the RTC to
		rtc.getRTCTime(t);
		t.setSecond(55);
		t.setMinute(59);
		t.setHour(3);
		rtc.setRTCTime(t.toUnixTime());

		t.clear();
		t.setAlarmHour(4);
		bResult = rtc.setAlarm(t, true, curAlarm);
		stateTime = millis();
		state = ALARM_HOUR_WAIT_STATE;
		break;

	case ALARM_HOUR_WAIT_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			Log.trace("ALARM_HOUR test completed!");
			state = ALARM_DAYOFWEEK_START_STATE;
			stateTime = millis();
		}
		else
		if (millis() - stateTime >= 8000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;

	case ALARM_DAYOFWEEK_START_STATE:

		rtc.clearAlarm(curAlarm);
		assertEqual((int)digitalRead(D8), 0, "%d");

		// Set the RTC to
		rtc.getRTCTime(t);
		t.setSecond(55);
		t.setMinute(59);
		t.setHour(23);
		t.setDayOfWeek(3);
		t.setDayOfMonth(27);
		t.setMonth(2);
		t.setYear(2019);
		rtc.setRTCTime(t.toUnixTime());

		t.clear();
		t.setAlarmDayOfWeek(4);
		bResult = rtc.setAlarm(t, true, curAlarm);
		stateTime = millis();
		state = ALARM_DAYOFWEEK_WAIT_STATE;
		break;

	case ALARM_DAYOFWEEK_WAIT_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			Log.trace("ALARM_DAYOFWEEK test completed!");

			state = ALARM_DAYOFMONTH_START_STATE;
			stateTime = millis();
		}
		else
		if (millis() - stateTime >= 8000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;

	case ALARM_DAYOFMONTH_START_STATE:

		rtc.clearAlarm(curAlarm);
		assertEqual((int)digitalRead(D8), 0, "%d");

		// Set the RTC to
		rtc.getRTCTime(t);
		t.setSecond(55);
		t.setMinute(59);
		t.setHour(23);
		t.setDayOfWeek(3);
		t.setDayOfMonth(27);
		t.setMonth(2);
		t.setYear(2019);
		rtc.setRTCTime(t.toUnixTime());

		t.clear();
		t.setAlarmDayOfMonth(28);
		bResult = rtc.setAlarm(t, true, curAlarm);
		stateTime = millis();
		state = ALARM_DAYOFMONTH_WAIT_STATE;
		break;

	case ALARM_DAYOFMONTH_WAIT_STATE:
		if (digitalRead(D8) == 1) {
			assertEqual(rtc.getInterrupt(curAlarm), true, "%d");
			rtc.clearInterrupt(curAlarm);
			assertEqual(rtc.getInterrupt(curAlarm), false, "%d");
			assertEqual((int)digitalRead(D8), 0, "%d");

			Log.trace("ALARM_DAYOFMONTH test completed!");

			// Restore the correct time
			rtc.setRTCFromCloud();

			state = ALARM_SEC_START_STATE;
			stateTime = millis();
		}
		else
		if (millis() - stateTime >= 8000) {
			Log.error("alarm did not fire %d", __LINE__);
			state = DONE_STATE;
		}
		break;
	case SRAM_INITIAL_CHECK_STATE:
		bResult = rtc.sram().erase();
		assertEqual(bResult, true, "%d");

		bResult = rtc.sram().readData(0, buf, 64);
		assertEqual(bResult, true, "%d");
		for(size_t ii = 0; ii < 64; ii++) {
			snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
			assertEqual(buf[ii], 0, "%02x");
		}
		Log.trace("SRAM_INITIAL_CHECK test completed");

		state = SRAM_BYTE_READ_STATE;
		break;

	case SRAM_BYTE_READ_STATE:
		{
			for(size_t ii = 0; ii < 64; ii++) {
				bResult = rtc.sram().readData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(buf[0], 0, "%02x");
			}
		}
		Log.trace("SRAM_BYTE_READ test completed");
		state = SRAM_BYTE_WRITE_READ_STATE;
		break;

	case SRAM_BYTE_WRITE_READ_STATE:
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
		Log.trace("SRAM_BYTE_WRITE_READ test completed");
		state = SRAM_DATA_WRITE_READ_STATE;
		break;

	case SRAM_DATA_WRITE_READ_STATE:
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
		Log.trace("SRAM_DATA_WRITE_READ test completed");
		state = EEPROM_INITIAL_CHECK_STATE;
		break;


	case EEPROM_INITIAL_CHECK_STATE:
		{
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
				Log.error("EEPROM was not erased");
				rtc.eeprom().erase();
				stateTime = millis();
				state = DONE_STATE;
				break;
			}

			Log.trace("EEPROM initial check completed");
			state = EEPROM_BYTE_READ_STATE;
		}
		break;

	case EEPROM_BYTE_READ_STATE:
		{
			for(size_t ii = 0; ii < sizeof(buf); ii++) {
				bResult = rtc.eeprom().readData(ii, buf, 1);
				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(bResult, true, "%d");

				snprintf(debugBuf, sizeof(debugBuf), "ii=%u", ii);
				assertEqual(buf[0], 0xff, "%02x");
			}
		}
		Log.trace("EEPROM byte read test completed");
		state = EEPROM_BYTE_WRITE_READ_STATE;
		break;

	case EEPROM_BYTE_WRITE_READ_STATE:
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
		Log.trace("EEPROM byte write read test completed");
		state = EEPROM_DATA_WRITE_READ_STATE;
		break;

	case EEPROM_DATA_WRITE_READ_STATE:
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
		Log.trace("EEPROM_DATA_WRITE_READ test completed");

		state = EEPROM_PROTECTED_BLOCK_STATE;
		break;

	case EEPROM_PROTECTED_BLOCK_STATE:
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

		Log.trace("EEPROM_PROTECTED_BLOCK test completed");

		state = EEPROM_BLOCK_PROTECTION_STATE;
		break;

	case EEPROM_BLOCK_PROTECTION_STATE:
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

		Log.trace("EEPROM_BLOCK_PROTECTION test completed");

		state = EEPROM_CLEANUP_STATE;
		break;

	case EEPROM_CLEANUP_STATE:
		Log.trace("cleaning up, erasing eeprom");
		rtc.eeprom().erase();

		state = ALARM_FROM_NOW_START_STATE;
		break;

	case DONE_STATE:
		Log.info("all tests completed!");
		stateTime = millis();
		state = START_WAIT_STATE;
		break;

	}

	rtc.loop();
}

void runTimeClassTests() {

	// Test BCD functions
	{
		assertEqual(MCP79410Time::bcdToInt(0x00), 0, "%d");
		assertEqual(MCP79410Time::bcdToInt(0x01), 1, "%d");
		assertEqual(MCP79410Time::bcdToInt(0x09), 9, "%d");
		assertEqual(MCP79410Time::bcdToInt(0x10), 10, "%d");
		assertEqual(MCP79410Time::bcdToInt(0x20), 20, "%d");
		assertEqual(MCP79410Time::bcdToInt(0x99), 99, "%d");

		assertEqual(MCP79410Time::intToBcd(0), 0x00, "%d");
		assertEqual(MCP79410Time::intToBcd(1), 0x01, "%d");
		assertEqual(MCP79410Time::intToBcd(9), 0x09, "%d");
		assertEqual(MCP79410Time::intToBcd(10), 0x10, "%d");
		assertEqual(MCP79410Time::intToBcd(20), 0x20, "%d");
		assertEqual(MCP79410Time::intToBcd(99), 0x99, "%d");

		for(int ii = 0; ii < 100; ii++)  {
			assertEqual(MCP79410Time::bcdToInt(MCP79410Time::intToBcd(ii)), ii, "%d");
		}
	}

	// Test raw value to normal int values
	{
		MCP79410Time t;
		t.rawYear = 0;
		assertEqual(2000, t.getYear(), "%d");
		t.rawYear = 0x19;
		assertEqual(2019, t.getYear(), "%d");
		t.rawYear = 0x99;
		assertEqual(2099, t.getYear(), "%d");

		t.rawMonth = 1;
		assertEqual(1, t.getMonth(), "%d");
		t.rawMonth = 2;
		assertEqual(2, t.getMonth(), "%d");
		t.rawMonth = 9;
		assertEqual(9, t.getMonth(), "%d");
		t.rawMonth = 0x10;
		assertEqual(10, t.getMonth(), "%d");
		t.rawMonth = 0x11;
		assertEqual(11, t.getMonth(), "%d");
		t.rawMonth = 0x12;
		assertEqual(12, t.getMonth(), "%d");
		t.rawMonth = 0x22;
		assertEqual(2, t.getMonth(), "%d"); // Leap year bit set
		t.rawMonth = 0x32;
		assertEqual(12, t.getMonth(), "%d");

		t.rawDayOfMonth = 1;
		assertEqual(1, t.getDayOfMonth(), "%d");
		t.rawDayOfMonth = 2;
		assertEqual(2, t.getDayOfMonth(), "%d");
		t.rawDayOfMonth = 9;
		assertEqual(9, t.getDayOfMonth(), "%d");
		t.rawDayOfMonth = 0x10;
		assertEqual(10, t.getDayOfMonth(), "%d");
		t.rawDayOfMonth = 0x20;
		assertEqual(20, t.getDayOfMonth(), "%d");
		t.rawDayOfMonth = 0x30;
		assertEqual(30, t.getDayOfMonth(), "%d");
		t.rawDayOfMonth = 0x81;
		assertEqual(1, t.getDayOfMonth(), "%d"); // unused bits should be cleared

		// Hour - 24 hour mode
		t.rawHour = 0;
		assertEqual(0, t.getHour(), "%d");
		t.rawHour = 1;
		assertEqual(1, t.getHour(), "%d");
		t.rawHour = 9;
		assertEqual(9, t.getHour(), "%d");
		t.rawHour = 0x10;
		assertEqual(10, t.getHour(), "%d");
		t.rawHour = 0x11;
		assertEqual(11, t.getHour(), "%d");
		t.rawHour = 0x12;
		assertEqual(12, t.getHour(), "%d");
		t.rawHour = 0x13;
		assertEqual(13, t.getHour(), "%d");
		t.rawHour = 0x20;
		assertEqual(20, t.getHour(), "%d");
		t.rawHour = 0x23;
		assertEqual(23, t.getHour(), "%d");

		// Hour - 12 hour mode
		const uint8_t MODE_12HR = 0x40;
		const uint8_t MODE_PM = 0x20;

		t.rawHour = MODE_12HR | 0x12; // 12AM = 0
		assertEqual(0, t.getHour(), "%d");

		t.rawHour = MODE_12HR | 1;
		assertEqual(1, t.getHour(), "%d");

		t.rawHour = MODE_12HR | 9;
		assertEqual(9, t.getHour(), "%d");

		t.rawHour = MODE_12HR | 0x10;
		assertEqual(10, t.getHour(), "%d");

		t.rawHour = MODE_12HR | 0x11;
		assertEqual(11, t.getHour(), "%d");

		t.rawHour = MODE_12HR | MODE_PM | 0x12;
		assertEqual(12, t.getHour(), "%d");

		t.rawHour = MODE_12HR | MODE_PM | 0x01;
		assertEqual(13, t.getHour(), "%d");

		t.rawHour = MODE_12HR | MODE_PM | 0x02;
		assertEqual(14, t.getHour(), "%d");

		t.rawHour = MODE_12HR | MODE_PM | 0x09;
		assertEqual(21, t.getHour(), "%d");

		t.rawHour = MODE_12HR | MODE_PM | 0x10;
		assertEqual(22, t.getHour(), "%d");

		t.rawHour = MODE_12HR | MODE_PM | 0x11;
		assertEqual(23, t.getHour(), "%d");

		// Minute
		t.rawMinute = 0x00;
		assertEqual(0, t.getMinute(), "%d");

		t.rawMinute = 0x09;
		assertEqual(9, t.getMinute(), "%d");

		t.rawMinute = 0x10;
		assertEqual(10, t.getMinute(), "%d");

		t.rawMinute = 0x59;
		assertEqual(59, t.getMinute(), "%d");

		t.rawMinute = 0x59 | 0x80;
		assertEqual(59, t.getMinute(), "%d"); // ignore high bit

		// Second
		t.rawSecond = 0x00;
		assertEqual(0, t.getSecond(), "%d");

		t.rawSecond = 0x09;
		assertEqual(9, t.getSecond(), "%d");

		t.rawSecond = 0x10;
		assertEqual(10, t.getSecond(), "%d");

		t.rawSecond = 0x59;
		assertEqual(59, t.getSecond(), "%d");

		t.rawSecond = 0x59 | 0x80;
		assertEqual(59, t.getSecond(), "%d"); // ignore high bit
	}

	// Unix Time Tests
	{
		// https://www.unixtimestamp.com/
		// 1551099686 seconds since Jan 01 1970. (UTC)
		// 2019-02-25T13:01:26+00:00 in ISO 8601 (Monday)

		MCP79410Time t;
		t.fromUnixTime(1551099686);
		assertEqual(2019, t.getYear(), "%d");
		assertEqual(2, t.getMonth(), "%d");
		assertEqual(25, t.getDayOfMonth(), "%d");
		assertEqual(1, t.getDayOfWeek(), "%d"); // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
		assertEqual(13, t.getHour(), "%d");
		assertEqual(1, t.getMinute(), "%d");
		assertEqual(26, t.getSecond(), "%d");

//		assertEqual(t.toUnixTime(), 1551099686LU, "%lu");


		// 1609459199
		// 2020-12-31T23:59:59+00:00 in ISO 8601 (Thursday)
		t.fromUnixTime(1609459199);
		assertEqual(2020, t.getYear(), "%d");
		assertEqual(12, t.getMonth(), "%d");
		assertEqual(31, t.getDayOfMonth(), "%d");
		assertEqual(4, t.getDayOfWeek(), "%d"); // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
		assertEqual(23, t.getHour(), "%d");
		assertEqual(59, t.getMinute(), "%d");
		assertEqual(59, t.getSecond(), "%d");

		assertEqual(t.toUnixTime(), 1609459199LU, "%lu");

		// 1551484800
		// Sat, 02 Mar 2019 00:00:00 +0000 in RFC 822, 1036, 1123, 2822
		t.fromUnixTime(1551484800);
		assertEqual(2019, t.getYear(), "%d");
		assertEqual(3, t.getMonth(), "%d");
		assertEqual(2, t.getDayOfMonth(), "%d");
		assertEqual(6, t.getDayOfWeek(), "%d"); // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
		assertEqual(0, t.getHour(), "%d");
		assertEqual(0, t.getMinute(), "%d");
		assertEqual(0, t.getSecond(), "%d");

		assertEqual(t.toUnixTime(), 1551484800LU, "%lu");

		// 1551615640
		// Sun, 03 Mar 2019 12:20:40 +0000 in RFC 822, 1036, 1123, 2822
		t.fromUnixTime(1551615640);
		assertEqual(2019, t.getYear(), "%d");
		assertEqual(3, t.getMonth(), "%d");
		assertEqual(3, t.getDayOfMonth(), "%d");
		assertEqual(0, t.getDayOfWeek(), "%d"); // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
		assertEqual(12, t.getHour(), "%d");
		assertEqual(20, t.getMinute(), "%d");
		assertEqual(40, t.getSecond(), "%d");

		assertEqual(t.toUnixTime(), 1551615640LU, "%lu");

	}

	{
		// Alarm tests
		MCP79410Time t;
		t.setAlarmSecond(30);
		assertEqual(30, t.getSecond(), "%d");
		assertEqual(0, t.alarmMode, "%d");

		t.setAlarmMinute(40);
		assertEqual(40, t.getMinute(), "%d");
		assertEqual(1, t.alarmMode, "%d");

		t.setAlarmHour(0);
		assertEqual(0, t.getHour(), "%d");
		assertEqual(2, t.alarmMode, "%d");

		t.setAlarmHour(23);
		assertEqual(23, t.getHour(), "%d");
		assertEqual(2, t.alarmMode, "%d");

		t.setAlarmDayOfWeek(0);
		assertEqual(0, t.getDayOfWeek(), "%d");
		assertEqual(1, t.rawDayOfWeek, "%d");
		assertEqual(3, t.alarmMode, "%d");

		t.setAlarmDayOfWeek(1);
		assertEqual(1, t.getDayOfWeek(), "%d");
		assertEqual(2, t.rawDayOfWeek, "%d");
		assertEqual(3, t.alarmMode, "%d");

		t.setAlarmDayOfWeek(6);
		assertEqual(6, t.getDayOfWeek(), "%d");
		assertEqual(7, t.rawDayOfWeek, "%d");
		assertEqual(3, t.alarmMode, "%d");

		t.setAlarmDayOfMonth(25);
		assertEqual(25, t.getDayOfMonth(), "%d");
		assertEqual(4, t.alarmMode, "%d");

		// 1551099686 seconds since Jan 01 1970. (UTC)
		// 2019-02-25T13:01:26+00:00 in ISO 8601 (Monday)

		t.setAlarmTime(1551099686LU);
		assertEqual(2, t.getMonth(), "%d");
		assertEqual(25, t.getDayOfMonth(), "%d");
		assertEqual(1, t.getDayOfWeek(), "%d"); // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
		assertEqual(13, t.getHour(), "%d");
		assertEqual(1, t.getMinute(), "%d");
		assertEqual(26, t.getSecond(), "%d");
		assertEqual(7, t.alarmMode, "%d");


	}


	Log.info("runTimeClassTests completed!");
}


