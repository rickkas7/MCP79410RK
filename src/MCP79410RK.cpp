#include "MCP79410RK.h"

static Logger log("app.rtc");


MCP79410MemoryBase::MCP79410MemoryBase(MCP79410 *parent) : parent(parent) {

}

MCP79410MemoryBase::~MCP79410MemoryBase() {

}

bool MCP79410MemoryBase::erase() {
	bool bResult = false;
	uint8_t buf[16];
	memset(buf, eraseValue(), sizeof(buf));

	size_t len = length();
	size_t offset = 0;

	while(offset < len) {
		size_t count = sizeof(buf);
		if (count > (len - offset)) {
			count = len - offset;
		}
		bResult = writeData(offset, buf, count);
		if (!bResult) {
			break;
		}

		offset += count;
	}

	return bResult;
}

//
//
//

MCP79410SRAM::MCP79410SRAM(MCP79410 *parent) : MCP79410MemoryBase(parent) {

}

MCP79410SRAM::~MCP79410SRAM() {

}

bool MCP79410SRAM::readData(size_t addr, uint8_t *data, size_t dataLen) {

	if ((addr + dataLen) > length()) {
		// Attempt to read past end is an error and nothing will be read
		return false;
	}

	int stat = parent->deviceRead(MCP79410::REG_I2C_ADDR, MCP79410::REG_SRAM + addr, data, dataLen);

	return (stat == 0);
}

bool MCP79410SRAM::writeData(size_t addr, const uint8_t *data, size_t dataLen) {

	if ((addr + dataLen) > length()) {
		// Attempt to write past end is an error and nothing will be writen
		return false;
	}

	int stat = parent->deviceWrite(MCP79410::REG_I2C_ADDR, MCP79410::REG_SRAM + addr, data, dataLen);

	return (stat == 0);
}


//
//
//
MCP79410EEPROM::MCP79410EEPROM(MCP79410 *parent) : MCP79410MemoryBase(parent) {

}

MCP79410EEPROM::~MCP79410EEPROM() {

}

uint8_t MCP79410EEPROM::getBlockProtection() const {
	uint8_t buf[1];

	int stat = parent->deviceRead(MCP79410::EEPROM_I2C_ADDR, MCP79410::EEPROM_STATUS, buf, 1);
	if (stat == 0) {
		return (buf[0] >> 2) & 0x3;
	}
	else {
		return 0;
	}
}

bool MCP79410EEPROM::setBlockProtection(uint8_t value) {

	uint8_t buf[1];

	buf[0] = (value & 3) << 2;

	int stat = parent->deviceWrite(MCP79410::EEPROM_I2C_ADDR, MCP79410::EEPROM_STATUS, buf, 1);
	if (stat == 0) {
		parent->waitForEEPROM();
		return true;
	}
	else {
		return false;
	}

}


bool MCP79410EEPROM::protectedBlockRead(uint8_t *buf) {
	int stat = parent->deviceRead(MCP79410::EEPROM_I2C_ADDR, MCP79410::EEPROM_PROTECTED, buf, MCP79410::EEPROM_PROTECTED_BLOCK_SIZE);

	return (stat == 0);
}


bool MCP79410EEPROM::readData(size_t addr, uint8_t *data, size_t dataLen) {

	if ((addr + dataLen) > length()) {
		// Attempt to read past end is an error and nothing will be read
		return false;
	}

	int stat = parent->deviceRead(MCP79410::EEPROM_I2C_ADDR, addr, data, dataLen);

	return (stat == 0);
}

bool MCP79410EEPROM::writeData(size_t addr, const uint8_t *data, size_t dataLen) {

	if ((addr + dataLen) > length()) {
		// Attempt to write past end is an error and nothing will be writen
		return false;
	}

	int stat = parent->deviceWriteEEPROM(addr, data, dataLen);

	return (stat == 0);
}



MCP79410Time::MCP79410Time() {
	clear();
}

MCP79410Time::~MCP79410Time() {

}


MCP79410Time::MCP79410Time(const MCP79410Time &other) {
	*this = other;
}

MCP79410Time &MCP79410Time::operator=(const MCP79410Time &other) {
	rawYear = other.rawYear;
	rawMonth = other.rawMonth;
	rawDayOfMonth = other.rawDayOfMonth;
	rawDayOfWeek = other.rawDayOfWeek;
	rawHour = other.rawHour;
	rawMinute = other.rawMinute;
	rawSecond = other.rawSecond;
	alarmMode = other.alarmMode;

	return *this;
}

void MCP79410Time::clear() {
	rawYear = 0;
	rawMonth = 1;
	rawDayOfMonth = 1;
	rawDayOfWeek = 1;
	rawHour = 0;
	rawMinute = 0;
	rawSecond = 0;
	alarmMode = 0;
}


void MCP79410Time::fromUnixTime(time_t time) {
	struct tm *tm = gmtime(&time);

	// Technically tm_year is years since 1900 but we can't represent dates not in 2000 - 2099
	setYear(tm->tm_year);

	// 0 <= tm_mon <= 11! We use the more conventional 1-12 for getMonth/setMonth
	setMonth(tm->tm_mon + 1);

	setDayOfMonth(tm->tm_mday);

	setDayOfWeek(tm->tm_wday);


	// days since Sunday – [0, 6]

	setHour(tm->tm_hour); // tm_hour is 0-23
	setMinute(tm->tm_min); // tm_min is 0-59
	setSecond(tm->tm_sec < 60 ? tm->tm_sec : 59); // tm_sec is 0-61, inclusive, because of leap seconds.
}

time_t MCP79410Time::toUnixTime() const {
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));

	tm.tm_year = getYear() - 1900; // tm_year is years since 1900
	tm.tm_mon = getMonth() - 1; // tm_mon is 0-11!
	tm.tm_mday = getDayOfMonth(); // 1-31

	tm.tm_hour = getHour();
	tm.tm_min = getMinute();
	tm.tm_sec = getSecond();

	// time->tm_wday and time->tm_yday are ignored by mktime

	return mktime(&tm);
}

int MCP79410Time::getYear() const {
	// RTC stores time as BCD 0-99. Assume 2000, this won't work in the past 1900 and I don't expect it to still be used in 2100
	return bcdToInt(rawYear) + 2000;
}

void MCP79410Time::setYear(int value) {
	rawYear = intToBcd(value % 100);
}

int MCP79410Time::getMonth() const {
	return bcdToInt(rawMonth & 0x1f);
}

void MCP79410Time::setMonth(int value) {
	rawMonth = intToBcd(value);
}


int MCP79410Time::getDayOfMonth() const {
	return bcdToInt(rawDayOfMonth & 0x3f);
}

void MCP79410Time::setDayOfMonth(int value) {
	rawDayOfMonth &= ~0x3f;
	rawDayOfMonth |= intToBcd(value);
}

int MCP79410Time::getDayOfWeek() const {
	// rawDayOfWeek has the day of week 1 - 7 in the low bits. However, it does not enforce a particular day of week scheme;
	// it just keeps rolling it as time increments.
	return bcdToInt(rawDayOfWeek & 0x7) - 1;
}

void MCP79410Time::setDayOfWeek(int value) {
	rawDayOfWeek &= ~0x7;
	rawDayOfWeek |= intToBcd(value + 1);
}


int MCP79410Time::getHour() const {
	if (rawHour & 0x40) {
		// Bit 6 = 1 (12 hour format)
		int hour12 = bcdToInt(rawHour & 0x1f);

		if (rawHour & 0x20) {
			// Bit 5 = 1 (PM)
			if (hour12 == 12) {
				// 12 PM = 12
				return 12;
			}
			else {
				return hour12 + 12;
			}
		}
		else {
			// Bit 5 = 0 (AM)
			if (hour12 == 12) {
				// 12 AM = 0
				return 0;
			}
			else {
				return hour12;
			}
		}

	}
	else {
		// Bit 6 = 0 (24 hour format)
		return bcdToInt(rawHour & 0x3f);
	}
}

void MCP79410Time::setHour(int value) {
	rawHour = intToBcd(value);
}



int MCP79410Time::getMinute() const {
	return bcdToInt(rawMinute & 0x7f);
}

void MCP79410Time::setMinute(int value) {
	rawMinute = intToBcd(value);
}


int MCP79410Time::getSecond() const {
	// High bit is ST (oscillator enabled) bit.
	return bcdToInt(rawSecond & 0x7f);
}

void MCP79410Time::setSecond(int value) {
	rawSecond &= ~0x7f;
	rawSecond |= intToBcd(value);
}

void MCP79410Time::setAlarmSecond(int second) {
	clear();
	alarmMode = ALARM_SECOND;
	setSecond(second);
}

void MCP79410Time::setAlarmMinute(int minute) {
	clear();
	alarmMode = ALARM_MINUTE;
	setMinute(minute);
}

void MCP79410Time::setAlarmHour(int hour) {
	clear();
	alarmMode = ALARM_HOUR;
	setHour(hour);
}

void MCP79410Time::setAlarmDayOfWeek(int dayOfWeek) {
	clear();
	alarmMode = ALARM_DAY_OF_WEEK;
	setDayOfWeek(dayOfWeek);
}

void MCP79410Time::setAlarmDayOfMonth(int dayOfMonth) {
	clear();
	alarmMode = ALARM_DAY_OF_MONTH;
	setDayOfMonth(dayOfMonth);
}

void MCP79410Time::setAlarmTime(time_t unixTime) {
	fromUnixTime(unixTime);
	alarmMode = ALARM_MONTH_DAY_DOW_HMS;
}

String MCP79410Time::toStringRaw() const {
	char buf[128];

	snprintf(buf, sizeof(buf), "year=%02x month=%02x dayOfMonth=%02x dayOfWeek=%02x hour=%02x minute=%02x second=%02x mode=%d",
			rawYear, rawMonth, rawDayOfMonth, rawDayOfWeek, rawHour, rawMinute, rawSecond, alarmMode);

	return String(buf);
}

// [static]
int MCP79410Time::bcdToInt(uint8_t value) {
	return ((value >> 4) & 0xf) * 10 + (value & 0xf);
}

// [static]
uint8_t MCP79410Time::intToBcd(int value) {
	uint8_t result;

	result = (uint8_t) (((value / 10) % 10) << 4);

	result |= (uint8_t) (value % 10);

	return result;
}

//
//
//

MCP79410::MCP79410(TwoWire &wire) : wire(wire), sramObj(this), eepromObj(this) {

}


MCP79410::~MCP79410() {

}

void MCP79410::setup() {
	wire.begin();

	if (!Time.isValid()) {
		if ((timeSyncMode & TIME_SYNC_RTC_TO_TIME) != 0) {
			time_t rtcTime = getRTCTime();
			if (rtcTime != 0) {
				Time.setTime(rtcTime);
				log.info("set Time from RTC %s", Time.format(rtcTime, TIME_FORMAT_DEFAULT).c_str());
			}
		}
	}

	setupDone = true;
}

void MCP79410::loop() {
	if (!timeSet) {
		// Time has not been synchronized from the cloud
		if (Time.isValid()) {
			// Also check timeSyncedLast, because if we set Time from RTC, then Time will
			// be valid, but not synchronized yet
			unsigned long lastSync = Particle.timeSyncedLast();
			if (lastSync != 0) {
				// Time is valid and synchronized
				if ((timeSyncMode & TIME_SYNC_CLOUD_TO_RTC) != 0) {
					setRTCFromCloud();
				}
				timeSet = true;
			}
		}
	}
}

bool MCP79410::setRTCFromCloud() {
	bool bResult = false;

	if (Time.isValid()) {
		time_t now = Time.now();
		bResult = setRTCTime(now);

		log.info("set RTC from cloud %s", Time.format(now, TIME_FORMAT_DEFAULT).c_str());
	}
	else {
		log.info("cloud time not valid");
	}
	return bResult;
}

bool MCP79410::setRTCTime(time_t unixTime) {
	MCP79410Time time;

	time.fromUnixTime(unixTime);

	// Set the oscillator start bit
	time.rawSecond |= REG_DATE_RTCSEC_ST;

	// Default is to enable the battery
	if (batteryEnable) {
		time.rawDayOfWeek |= REG_RTCWKDAY_VBATEN;
	}
	else {
		time.rawDayOfWeek &= ~REG_RTCWKDAY_VBATEN;
	}

	return deviceWriteRTCTime(REG_DATE_TIME, time) == 0;
}

bool MCP79410::isRTCValid() const {
	return getRTCTime() != 0;
}

time_t MCP79410::getRTCTime() const {
	MCP79410Time time;

	bool bResult = getRTCTime(time);
	if (bResult) {
		return time.toUnixTime();
	}
	else {
		return 0;
	}
}

bool MCP79410::getRTCTime(MCP79410Time &time) const {
	int stat = deviceReadTime(REG_DATE_TIME, time, TIME_MODE_RTC);
	if (stat == 0) {
		if (time.rawYear > 0 && getOscillatorRunning()) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

/**
 *
 */
bool MCP79410::getPowerDownTime(MCP79410Time &time) const {
	return deviceReadTime(REG_POWER_DOWN, time, TIME_MODE_POWER) == 0;
}

bool MCP79410::getPowerUpTime(MCP79410Time &time) const {
	return deviceReadTime(REG_POWER_UP, time, TIME_MODE_POWER) == 0;
}

bool MCP79410::getPowerFail() const {
	return (deviceReadRegisterByte(REG_RTCWKDAY) & REG_RTCWKDAY_PWRFAIL) != 0;
}

bool MCP79410::getOscillatorRunning() const {
	return (deviceReadRegisterByte(REG_RTCWKDAY) & REG_RTCWKDAY_OSCRUN) != 0;
}


void MCP79410::clearPowerFail() {
	deviceWriteRegisterFlag(REG_RTCWKDAY, REG_RTCWKDAY_PWRFAIL, false);
}

bool MCP79410::getBatteryEnable() const {
	return (deviceReadRegisterByte(REG_RTCWKDAY) & REG_RTCWKDAY_VBATEN) != 0;
}

bool MCP79410::setBatteryEnable(bool value /* = true */) {
	batteryEnable = value;
	if (setupDone) {
		return deviceWriteRegisterFlag(REG_RTCWKDAY, REG_RTCWKDAY_VBATEN, value) == 0;
	}
	else {
		return true;
	}
}


bool MCP79410::clearAlarm(int alarmNum) {
	if (alarmNum < 0 || alarmNum > 1) {
		// Invalid alarmNum, must be 0 or 1
		return false;
	}

	return deviceWriteRegisterFlag(REG_CONTROL, getAlarmEnableBit(alarmNum), false) == 0;
}

bool MCP79410::setAlarm(const MCP79410Time &time, bool polarity, int alarmNum) {
	if (alarmNum < 0 || alarmNum > 1) {
		// Invalid alarmNum, must be 0 or 1
		return false;
	}

	if (getOscillatorRunning()) {
		uint8_t buf[6];

		// Clear any existing alarm interrupt, otherwise this one will not fire. Fixed in 0.0.2.
		clearInterrupt(alarmNum);

		// log.trace("setAlarm %s polarity=%d alarmNum=%d", time.toStringRaw().c_str(), polarity, alarmNum);

		buf[0] = time.rawSecond;
		buf[1] = time.rawMinute;
		buf[2] = time.rawHour;
		buf[3] = time.rawDayOfWeek;
		buf[4] = time.rawDayOfMonth;
		buf[5] = time.rawMonth;

		if (polarity) {
			// REG_ALARM_WKDAY_ALMPOL: 1 = alarm triggered, 0 = alarm did not trigger
			buf[3] |= REG_ALARM_WKDAY_ALMPOL;
		}
		else {
			buf[3] &= ~REG_ALARM_WKDAY_ALMPOL;
		}
		buf[3] |= (time.alarmMode & 0x7) << 4;

		uint8_t reg = getAlarmRegister(alarmNum);

		// log.trace("setAlarm %02x%02x%02x%02x%02x%02x starting at reg=%02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], reg);

		int stat = deviceWrite(REG_I2C_ADDR, reg, buf, sizeof(buf));
		if (stat == 0) {
			deviceWriteRegisterFlag(REG_CONTROL, getAlarmEnableBit(alarmNum), true);
		}

		return (stat == 0);
	}
	else {
		return false;
	}
}

bool MCP79410::setAlarm(int secondsFromNow, bool polarity, int alarmNum) {
	// log.trace("setAlarm secondsFromNow=%d polarity=%d alarmNum=%d", secondsFromNow, polarity, alarmNum);
	if (alarmNum < 0 || alarmNum > 1) {
		// Invalid alarmNum, must be 0 or 1
		return false;
	}

	time_t unixTime = getRTCTime();
	if (unixTime != 0) {
		unixTime += secondsFromNow;

		// Set an alarm for month, dayOfMonth, dayOfWeek, hour, minute, second
		MCP79410Time time;
		time.setAlarmTime(unixTime);

		return setAlarm(time, polarity, alarmNum);
	}
	else {
		// RTC is not set or not running, cannot set an alarm
		return false;
	}
}

bool MCP79410::getInterrupt(int alarmNum) {
	uint8_t wkday = deviceReadRegisterByte(getAlarmRegister(alarmNum, REG_ALARM_WKDAY_OFFSET));

	// REG_ALARM_WKDAY_ALMIF: 1 = alarm triggered, 0 = alarm did not trigger

	return (wkday & REG_ALARM_WKDAY_ALMIF) != 0;
}

void MCP79410::clearInterrupt(int alarmNum) {
	deviceWriteRegisterFlag(getAlarmRegister(alarmNum, REG_ALARM_WKDAY_OFFSET), REG_ALARM_WKDAY_ALMIF, false);
}

bool MCP79410::setSquareWaveMode(uint8_t freq) {
	if ((freq & ~SQUARE_WAVE_MASK) != 0) {
		// Invalid freq value
		return false;
	}

	// Enable the oscillator
	deviceWriteRegisterFlag(REG_DATE_RTCSEC, REG_DATE_RTCSEC_ST, true);

	// Clear these bits
	uint8_t clearBits = REG_CONTROL_ALM1EN | REG_CONTROL_ALM0EN | SQUARE_WAVE_MASK;

	return deviceWriteRegisterByteMask(REG_CONTROL, ~clearBits, REG_CONTROL_SQWEN | freq) == 0;
}

bool MCP79410::clearSquareWaveMode() {
	return deviceWriteRegisterFlag(REG_CONTROL, REG_CONTROL_SQWEN, false) == 0;
}

bool MCP79410::setOscTrim(int8_t trim) {
	uint8_t value;

	if (trim < 0) {
		// sign bit in 0x80: 0 = negative, subtract trim value
		value = (uint8_t) -trim;
	}
	else {
		// sign bit in 0x80: 1 = positive, add trim value
		value = (uint8_t) trim | 0x80;
	}
	return deviceWriteRegisterByte(REG_OSCTRIM, value) == 0;
}


int MCP79410::deviceReadTime(uint8_t addr, MCP79410Time &time, int timeMode) const {
	uint8_t buf[8];
	int stat = -1;

	if (timeMode == TIME_MODE_RTC || timeMode == TIME_MODE_ALARM) {
		size_t numBytes = (timeMode == TIME_MODE_RTC) ? 7 : 6;
		stat = deviceRead(REG_I2C_ADDR, addr, buf, numBytes);
		if (stat == 0) {
			time.rawSecond = buf[0];
			time.rawMinute = buf[1];
			time.rawHour = buf[2];
			time.rawDayOfWeek = buf[3];
			time.rawDayOfMonth = buf[4];
			time.rawMonth = buf[5];
			if (timeMode == TIME_MODE_RTC) {
				time.rawYear = buf[6];
			}
			else {
				time.rawYear = MCP79410Time::intToBcd(Time.year());
			}
		}
	}
	else
	if (timeMode == TIME_MODE_POWER) {
		stat = deviceRead(REG_I2C_ADDR, addr, buf, 4);
		if (stat == 0) {
			time.rawSecond = 0;
			time.rawMinute = buf[0];
			time.rawHour = buf[1];
			time.rawDayOfMonth = buf[2];
			time.rawMonth = buf[3];
			time.rawYear = MCP79410Time::intToBcd(Time.year());
		}
	}

	return stat;
}

int MCP79410::deviceWriteRTCTime(uint8_t addr, const MCP79410Time &time) {
	uint8_t buf[7];

	buf[0] = time.rawSecond;
	buf[1] = time.rawMinute;
	buf[2] = time.rawHour;
	buf[3] = time.rawDayOfWeek;
	buf[4] = time.rawDayOfMonth;
	buf[5] = time.rawMonth;
	buf[6] = time.rawYear;

	return deviceWrite(REG_I2C_ADDR, addr, buf, sizeof(buf));
}

uint8_t MCP79410::deviceReadRegisterByte(uint8_t addr) const {
	uint8_t buf[1];

	if (deviceRead(REG_I2C_ADDR, addr, buf, 1) == 0) {
		// log.trace("deviceReadRegisterByte addr=%02x value=%02x", addr, buf[0]);
		return buf[0];
	}
	else {
		// log.trace("deviceReadRegisterByte addr=%02x failed", addr);
		return 0;
	}
}

int MCP79410::deviceWriteRegisterByte(uint8_t addr, uint8_t value) {
	uint8_t buf[1];

	// log.trace("deviceWriteRegisterByte addr=%02x value=%02x", addr, value);

	buf[0] = value;

	return deviceWrite(REG_I2C_ADDR, addr, buf, 1);
}

int MCP79410::deviceWriteRegisterFlag(uint8_t addr, uint8_t value, bool set) {

	// log.trace("deviceWriteRegisterFlag addr=%02x value=%02x set=%d", addr, value, set);

	if (set) {
		return deviceWriteRegisterByteMask(addr, 0xff, value);
	}
	else {
		return deviceWriteRegisterByteMask(addr, ~value, 0);
	}
}


int MCP79410::deviceWriteRegisterByteMask(uint8_t addr, uint8_t andMask, uint8_t orMask) {
	uint8_t value = deviceReadRegisterByte(addr);

	value &= andMask;
	value |= orMask;

	// log.trace("deviceWriteRegisterByteMask addr=%02x value=%02x andMask=%02x orMask=%02x", addr, value, andMask, orMask);

	return deviceWriteRegisterByte(addr, value);
}



int MCP79410::deviceRead(uint8_t i2cAddr, uint8_t addr, uint8_t *buf, size_t bufLen) const {
	// log.trace("deviceRead i2cAddr=%02x addr=%02x bufLen=%u", i2cAddr, addr, bufLen);

	int stat = 0;
	size_t offset = 0;

	while(offset < bufLen) {
		wire.beginTransmission(i2cAddr);
		wire.write(addr + offset);
		stat = wire.endTransmission(false);
		if (stat == 0) {
			// Maximum read is 32 because of the limitation of the Wire implementation
			size_t count = bufLen - offset;
			if (count > 32) {
				count = 32;
			}

			// log.trace("deviceRead addr=%u count=%u", addr + offset, count);

			count = wire.requestFrom(i2cAddr, (uint8_t) count, (uint8_t) true);
			for(size_t ii = 0; ii < count; ii++) {
				buf[ii + offset] = wire.read();
			}
			offset += count;
		}
		else {
			log.info("deviceRead failed stat=%d", stat);
			break;
		}
	}
	return stat;
}

int MCP79410::deviceWrite(uint8_t i2cAddr, uint8_t addr, const uint8_t *buf, size_t bufLen) {
	// log.trace("deviceWrite i2cAddr=%02x addr=%02x bufLen=%u", i2cAddr, addr, bufLen);

	int stat = 0;
	size_t offset = 0;

	while(offset < bufLen) {
		wire.beginTransmission(i2cAddr);
		wire.write(addr + offset);

		// Maximum write is 31, not 32, because of the address byte
		size_t count = bufLen - offset;
		if (count > 31) {
			count = 31;
		}

		// log.trace("deviceWrite addr=%u count=%u", addr + offset, count);

		for(size_t ii = 0; ii < count; ii++) {
			wire.write(buf[ii + offset]);
		}

		stat = wire.endTransmission(true);
		if (stat != 0) {
			log.info("deviceWrite failed stat=%d", stat);
			break;
		}

		offset += count;
	}

	return stat;
}

int MCP79410::deviceWriteEEPROM(uint8_t addr, const uint8_t *buf, size_t bufLen) {
	// log.trace("deviceWriteEEPROM addr=%02x bufLen=%u buf[0]=%02x", addr, bufLen, buf[0]);

	int stat = 0;
	size_t offset = 0;

	while(offset < bufLen) {
		wire.beginTransmission(EEPROM_I2C_ADDR);
		wire.write(addr + offset);

		// Maximum EEPROM write is 7 bytes. However, I get random-ish failures for multi-byte writes
		// for reasons that are not obvious. Doing single byte writes is inefficient, but works
		// fine so use that for now. EEPROM writes should be relatively infrequent.
		size_t count = bufLen - offset;
		if (count > 1) {
			count = 1;
		}

		// if (bufLen != 1) {
		//	log.trace("deviceWriteEEPROM addr=%02x count=%u", addr + offset, count);
		// }

		for(size_t ii = 0; ii < count; ii++) {
			wire.write(buf[ii + offset]);
		}

		stat = wire.endTransmission(true);
		if (stat != 0) {
			log.info("deviceWriteEEPROM failed stat=%d", stat);
			break;
		}

		waitForEEPROM();

		offset += count;
	}

	return stat;
}

void MCP79410::waitForEEPROM() {
	for(size_t tries = 0; tries < 50; tries++) {
		wire.beginTransmission(EEPROM_I2C_ADDR);
		int stat = wire.endTransmission(true);
		if (stat == 0) {
			// log.trace("deviceWriteEEPROM got ack after %u tries", tries);
			break;
		}
	}
}
