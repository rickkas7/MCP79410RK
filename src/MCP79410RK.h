#ifndef __MCP79410RK_H
#define __MCP79410RK_H

#include "Particle.h"

class MCP79410; // Forward declaration

/**
 * @brief Abstract base class for MCP79410SRAM and MCP79410EEPROM
 *
 * You cannot construct one of these, as it's abstract.
 */
class MCP79410MemoryBase {
public:
	/**
	 * @brief Constructor
	 */
	MCP79410MemoryBase(MCP79410 *parent);

	/**
	 * @brief Destructor
	 */
	virtual ~MCP79410MemoryBase();

	/**
	 * @brief Return the length of the memory in bytes (abstract)
	 */
	virtual size_t length() const = 0;

	/**
	 * @brief Value to erase to (abstract)
	 */
	virtual uint8_t eraseValue() const = 0;

	/**
	 * @brief Erase the memory
	 *
	 * The MCP79410 doesn't have an erase primitive so this just writes the eraseValue()
	 * to each byte but you could imagine with flash, this would be different.
	 */
	virtual bool erase();

	/**
	 * @brief Templated accessor to get data from a specific offset. This API works like the
	 * [EEPROM API](https://docs.particle.io/reference/device-os/firmware/#eeprom).
	 *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware regiser address)
	 * @param t The object to read data into
	 */
    template <typename T> T &get(size_t addr, T &t) {
        readData(addr, (uint8_t *)&t, sizeof(T));
        return t;
    }

	/**
	 * @brief Templated accessor to set data at a specific offset. This API works like the
	 * [EEPROM API](https://docs.particle.io/reference/device-os/firmware/#eeprom).
	 *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware regiser address)
	 * @param t The object to write data from
	 */
    template <typename T> const T &put(size_t addr, const T &t) {
        writeData(addr, (const uint8_t *)&t, sizeof(T));
        return t;
    }

    /**
     * @brief Low-level API to read data from memory
     *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware register address)
     * @param data Pointer to buffer to store data in
     * @param dataLen Number of bytes to read
     */
	virtual bool readData(size_t addr, uint8_t *data, size_t dataLen) = 0;

    /**
     * @brief Low-level API to write data to memory
     *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware register address)
     * @param data Pointer to buffer containing the data to write to memory. Buffer is not modified (is const).
     * @param dataLen Number of bytes to write
     */
	virtual bool writeData(size_t addr, const uint8_t *data, size_t dataLen) = 0;

protected:
	MCP79410 *parent; //!< The MCP79410 object that this object is associated with
};

/**
 * @brief Class for accessing the MCP79410 SRAM (static RAM)
 *
 * The SRAM is fast to access and does not wear out. It's saved as long as there's power or the backup battery,
 * so it's not as non-volatile as the EEPROM, but a good place to store up to 64 bytes of data.
 *
 * You do not instantiate one of these, use the rtc.sram() method to get a reference to this object.
 *
 * While you can use readData() and writeData(), you can also use the get() and put() methods like
 * the [EEPROM API](https://docs.particle.io/reference/device-os/firmware/#eeprom) to read and write
 * primitives (int, bool, etc.) as well as structures. Note that you cannot get and put a String or
 * char *, however, you can only store an array of characters.
 */
class MCP79410SRAM : public MCP79410MemoryBase {
public:
	/**
	 * @brief Class to read and write the 64-byte SRAM (battery backed RAM) block in the MCP79410
	 *
	 * You should never construct one of these; the object is exposed by the MCP79410 class.
	 */
	MCP79410SRAM(MCP79410 *parent);

	/**
	 * @brief Destructor. Not normally used as MCP79410 constructs this object and the MCP79410 is
	 * typically a global object.
	 */
	virtual ~MCP79410SRAM();

	/**
	 * @brief Returns the length (64)
	 */
	virtual size_t length() const { return 64; };

	/**
	 * @brief Erase erases to 0
	 *
	 * Note that on cold power up, the values are random, not zero!
	 */
	virtual uint8_t eraseValue() const { return 0; };


    /**
     * @brief Low-level API to read data from memory
     *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware register address)
     * @param data Pointer to buffer to store data in
     * @param dataLen Number of bytes to read
     */
	virtual bool readData(size_t addr, uint8_t *data, size_t dataLen);

    /**
     * @brief Low-level API to write data to memory
     *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware register address)
     * @param data Pointer to buffer containing the data to write to memory. Buffer is not modified (is const).
     * @param dataLen Number of bytes to write
     */
	virtual bool writeData(size_t addr, const uint8_t *data, size_t dataLen);
};

/**
 * @brief Class for accessing the MCP79410 EEPROM
 *
 * The EEPROM is non-volatile, and values persist even when both the power and backup battery are removed.
 * It is slow to write to and has a finite number of write cycles.
 *
 * You do not instantiate one of these, use the rtc.eeprom() method to get a reference to this object.
 *
 * The default from the factory is 0xff bytes in all locations.
 *
 * While you can use readData() and writeData(), you can also use the get() and put() methods like
 * the [EEPROM API](https://docs.particle.io/reference/device-os/firmware/#eeprom) to read and write
 * primitives (int, bool, etc.) as well as structures. Note that you cannot get and put a String or
 * char *, however, you can only store an array of characters.
 *
 */

class MCP79410EEPROM : public MCP79410MemoryBase {
public:
	/**
	 * @brief Class to read and write the 64-byte SRAM (battery backed RAM) block in the MCP79410
	 *
	 * You should never construct one of these; the object is exposed by the MCP79410 class.
	 */
	MCP79410EEPROM(MCP79410 *parent);

	/**
	 * @brief Destructor. Not normally used as MCP79410 constructs this object and the MCP79410 is
	 * typically a global object.
	 */
	virtual ~MCP79410EEPROM();

	/**
	 * @brief Get the EEPROM block protection register
	 *
	 * | Constant | Value | Description |
	 * | -------- | ----- | ----------- |
	 * | MCP79410::EEPROM_PROTECT_NONE | 0x0; | Able to write all bytes |
	 * | MCP79410::EEPROM_PROTECT_UPPER_QUARTER | 0x1 | 0x60 - 0x7f protected from writing |
	 * | MCP79410::EEPROM_PROTECT_UPPER_HALF | 0x2 | 0x40 - 0x7f protected from writing |
	 * | MCP79410::EEPROM_PROTECT_ALL | 0x3 | All bytes protected from writing |
   	 *
	 * The default value from the factory is 0 (EEPROM_PROTECT_NONE).
	 *
	 * Setting it to other values will cause writes to the protected sections to not actually write until
	 * you change the protection register to allow writes again.
	 *
	 * Note: block protection is separate from the 8-byte protected block, which is confusing.
	 */
	uint8_t getBlockProtection() const;

	/**
	 * @brief Sets the block protection.
	 *
	 * @param value The value, such as MCP79410::EEPROM_PROTECT_NONE. See getBlockProtection() for more details.
	 */
	bool setBlockProtection(uint8_t value);


	/**
	 * @brief Read from the 8-byte protected block in EEPROM
	 *
	 * @param buf A pointer to an 8-byte buffer to hold the data.
	 *
	 * The constant MCP79410::EEPROM_PROTECTED_BLOCK_SIZE can be used instead of 8.
	 *
	 * This is different from the block protection (getBlockProtection() and setBlockProtection(). This protected
	 * block can only be written to using a special technique. See MCP79410::eepromProtectedBlockWrite.
	 *
	 * This block is normally used for things like MAC addresses, board identification codes, etc. that are
	 * programmed once at the factory and never reset in the field.
	 *
	 * The default from the factory is 0xff bytes for all 8 bytes.
	 */
	bool protectedBlockRead(uint8_t *buf);

	/**
	 * @brief Returns the length (128)
	 */
	virtual size_t length() const { return 128; };

	/**
	 * @brief Erased value is 0xff.
	 */
	virtual uint8_t eraseValue() const { return 0xff; };


    /**
     * @brief Low-level API to read data from memory
     *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware register address)
     * @param data Pointer to buffer to store data in
     * @param dataLen Number of bytes to read
     */
	virtual bool readData(size_t addr, uint8_t *data, size_t dataLen);

    /**
     * @brief Low-level API to write data to memory
     *
	 * @param addr Address in the memory block (0 = beginning of block; do not use hardware register address)
     * @param data Pointer to buffer containing the data to write to memory. Buffer is not modified (is const).
     * @param dataLen Number of bytes to write
     */
	virtual bool writeData(size_t addr, const uint8_t *data, size_t dataLen);
};



/**
 * @brief Container to hold a time value similar to how it's stored on the MCP79410,
 * and also convert to other useful formats
 */
class MCP79410Time {
public:
	/**
	 * @brief Constructor, clears object with clear()
	 *
	 * This object allocates no heap memory. It should only have 8 bytes of storge for an object instance.
	 */
	MCP79410Time();

	/**
	 * @brief Destructor
	 */
	virtual ~MCP79410Time();

	/**
	 * @brief Construct a time object from another time object
	 */
	MCP79410Time(const MCP79410Time &other);

	/**
	 * @brief Copy another time object into this object
	 */
	MCP79410Time &operator=(const MCP79410Time &other);

	/**
	 * @brief Clear all of the fields
	 *
	 * Most fields are set to 0 unless invalid. month and dayOfMonth are set to 1.
	 */
	void clear();

	/**
	 * @brief Fill in the fields of this object from a Unix time value (seconds past January 1, 1970) at GMT.
	 *
	 * You can set this from the value returned by [Time.now()](https://docs.particle.io/reference/device-os/firmware/#now-), for example.
	 */
	void fromUnixTime(time_t unixTime);

	/**
	 * @brief Convert this object to a Unix time value (seconds past January 1, 1970) at GMT
	 *
	 * The rawDayOfWeek field is ignored.
	 */
	time_t toUnixTime() const;

	/**
	 * @brief Gets the year: 2000 <= year < 2099
	 */
	int getYear() const;

	/**
	 * @brief Sets the year
	 *
	 * @param value 2000 <= value <= 2099
	 *
	 * Actually, the value is stored modulo 100, so it really doesn't matter what century you pass.
	 *
	 * Setting the day, month, or year manually does not reset dayOfWeek; it will be inaccurate when setting the date this way!
	 */
	void setYear(int value);

	/**
	 * @brief Gets the month 1 <= month <= 12, 1 = January, 2 = February, ..., 12 = December
	 */
	int getMonth() const;

	/**
	 * @brief Sets the month
	 *
	 * @param value 1 <= value <= 12, 1 = January, 2 = February, ..., 12 = December
	 *
	 * Setting the day, month, or year manually does not reset dayOfWeek; it will be inaccurate when setting the date this way!
	 */
	void setMonth(int value);

	/**
	 * @brief Gets the day of the month, 1 <= dayOfMonth <= 31 (smaller for some months, of course)
	 */
	int getDayOfMonth() const;

	/**
	 * @brief Sets the day of the month
	 *
	 * @param value 1 <= value <= 31 (smaller for some months, of course)
	 *
	 * Setting the day, month, or year manually does not reset dayOfWeek; it will be inaccurate when setting the date this way!
	 */
	void setDayOfMonth(int value);

	/**
	 * @brief Get the day of week: 0 = Sunday, 1 = Monday, 2 = Tuesday, ..., 6 = Saturday
	 */
	int getDayOfWeek() const;

	/**
	 * @brief Sets the dayOfWeek value
	 *
	 * @param value 0 = Sunday, 1 = Monday, 2 = Tuesday, ..., 6 = Saturday
	 *
	 * Note: The rawDayOfWeek value is 1 to 7, the value returned by this function is 0 - 6!
	 */
	void setDayOfWeek(int value);

	/**
	 * @brief Gets the hour in 24-hour format (0 <= hour <= 23)
	 *
	 * Note that the RTC may have a 12-hour + AM/PM format configured, but this method always returns 24-hour.
	 */
	int getHour() const;

	/**
	 * @brief Sets the hour in 24-hour format (0 <= hour <= 23)
	 *
	 * @param value Hour in 24-hour format (0 <= value <= 23)
	 */
	void setHour(int value);

	/**
	 * @brief Gets the minute (0 <= minute < 60)
	 */
	int getMinute() const;

	/**
	 * @brief Sets the minute
	 *
	 * @param value The minute: 0 <= value < 60
	 */
	void setMinute(int value);

	/**
	 * @brief Gets the second value (0 <= second < 60)
	 */
	int getSecond() const;

	/**
	 * @brief Sets the second
	 *
	 * @param value The second: 0 <= value < 60
	 */
	void setSecond(int value);

	/**
	 * @brief Set the time values for an alarm when second equals alarm
	 *
	 * @param second The second value to trigger the alarm at
	 *
	 * This is typically used to alarm every minute when the second equals the specified value. Note
	 * that this will only alarm on transition into an equal condition; it has no concept of a missed
	 * second alarm.
	 *
	 * You typically use it like this:
	 *
	 * ```
	 * MCP79410Time t;
	 * t.setAlarmSecond(secondsValue);
	 * bool bResult = rtc.setAlarm(t);
	 * ```
	 *
	 * In the more common case of "seconds from now" you don't need to use the MCP79410Time object
	 * and can just call rtc.setAlarm(secondsFromNow).
	 */
	void setAlarmSecond(int second);

	/**
	 * @brief Set the time values for an alarm when minute equals alarm
	 *
	 * @param minute The minute value to trigger the alarm at
	 *
	 * This is typically used to alarm every hour when the minute equals the specified value. Note
	 * that this will only alarm on transition into an equal condition; it has no concept of a missed
	 * minute alarm. Also, if you clear the interrupt you won't get interrupted again in the same minute
	 * as it only triggers on transition into an equal condition.
	 *
	 * You typically use it like this to wake up every hour on he half hour:
	 *
	 * ```
	 * MCP79410Time t;
	 * t.setAlarmMinute(30);
	 * bool bResult = rtc.setAlarm(t);
	 * ```
	 *
	 */
	void setAlarmMinute(int minute);

	/**
	 * @brief Set the time values for an alarm when hour equals alarm
	 *
	 * @param hour The hour value to trigger the alarm at
	 *
	 * This is typically used to alarm every hour when the hour equals the specified value. Note
	 * that this will only alarm on transition into an equal condition; it has no concept of a missed
	 * hour alarm. Also, if you clear the interrupt you won't get interrupted again in the same hour
	 * as it only triggers on transition into an equal condition.
	 *
	 * You typically use it like this to wake up once a day at midnight
	 *
	 * ```
	 * MCP79410Time t;
	 * t.setAlarmHour(0);
	 * bool bResult = rtc.setAlarm(t);
	 * ```
	 *
	 * Note: the hour is at UTC, not taking into account daylight saving time, so keep that in mind.
	 */
	void setAlarmHour(int hour);

	/**
	 * @brief Set the time values for an alarm when day of week equals alarm
	 *
	 * @param dayOfWeek The day of week value to trigger the alarm at (0 - 6). Sunday = 0, Monday = 1,
	 * ..., Saturday = 6.
	 *
	 * This is typically used to alarm every week when the day of week equals the specified value. Note
	 * that this will only alarm on transition into an equal condition; it has no concept of a missed
	 * day of week alarm. Also, if you clear the interrupt you won't get interrupted again in the same d
	 * as it only triggers on transition into an equal condition.
	 *
	 * You typically use it like this to wake up once a week, Sunday at midnight UTC:
	 *
	 * ```
	 * MCP79410Time t;
	 * t.setAlarmDayOfWeek(0);
	 * bool bResult = rtc.setAlarm(t);
	 * ```
	 *
	 * Note: the time is at UTC, not taking into account daylight saving time, so it will actually trigger
	 * before midnight on the previous day in the United States (and other negative from UTC timezones)!
	 */
	void setAlarmDayOfWeek(int dayOfWeek);

	/**
	 * @brief Set the time values for an alarm when day of month equals alarm
	 *
	 * @param dayOfMonth The day of month value to trigger the alarm at 1 = first of the month
	 *
	 * This is typically used to alarm every month when the day of month equals the specified value. Note
	 * that this will only alarm on transition into an equal condition; it has no concept of a missed
	 * day of month alarm. Also, if you clear the interrupt you won't get interrupted again in the same d
	 * as it only triggers on transition into an equal condition.
	 *
	 * You typically use it like this to wake up once a month, on the 15th of the month at midnight UTC:
	 *
	 * ```
	 * MCP79410Time t;
	 * t.setAlarmDayOfMonth(15);
	 * bool bResult = rtc.setAlarm(t);
	 * ```
	 *
	 * Note: the time is at UTC, not taking into account daylight saving time, so it will actually trigger
	 * before midnight on the previous day in the United States (and other negative from UTC timezones)!
	 */
	void setAlarmDayOfMonth(int dayOfMonth);

	/**
	 * @brief Set the time alarm for a specific time
	 *
	 * @param unixTime The time to set the alarm for.
	 *
	 * Note that only the second, minute, hour, dayOfWeek, dayOfMonth, and month are tested. You can't set
	 * a time alarm for more than a year in advance. Also, the time is at UTC.
	 */
	void setAlarmTime(time_t unixTime);

	/**
	 * @brief Make a reasable representation of this object
	 *
	 * If you pass the string to variable arguments (Log.info, sprintf, etc.) make sure you use
	 * toStringRaw().c_str() to make sure the object is converted to a c-string (null terminated).
	 */
	String toStringRaw() const;

	/**
	 * @brief Utility function to convert a BCD value to an integer
	 *
	 * The MAX79410 stores data in BCD, for example 27 is stored as 0x27 = 39 (decimal). It can only
	 * represent values from 0 - 99 of course.
	 */
	static int bcdToInt(uint8_t value);

	/**
	 * @brief Utility function to convert a integer to a BCD value
	 *
	 * The MAX79410 stores data in BCD, for example 27 is stored as 0x27 = 39 (decimal). It can only
	 * represent values from 0 - 99 of course.
	 */
	static uint8_t intToBcd(int value);

	const uint8_t ALARM_SECOND = 0; //!< ALMxMSK value stored in ALMxWKDAY. This is set automatically when using setAlarmSecond().
	const uint8_t ALARM_MINUTE = 1; //!< ALMxMSK value stored in ALMxWKDAY. This is set automatically when using setAlarmMinute().
	const uint8_t ALARM_HOUR = 2; //!< ALMxMSK value stored in ALMxWKDAY. This is set automatically when using setAlarmHour().
	const uint8_t ALARM_DAY_OF_WEEK = 3; //!< ALMxMSK value stored in ALMxWKDAY. This is set automatically when using setAlarmDayOfWeek().
	const uint8_t ALARM_DAY_OF_MONTH = 4; //!< ALMxMSK value stored in ALMxWKDAY. This is set automatically when using setAlarmDayOfMonth().

	/**
	 * @brief ALMxMSK value stored in ALMxWKDAY. This is set automatically when using setAlarmTime().
	 *
	 * Note that this checks only month, day, day of week, hour, minute, and second. It does not check year.
	 * However since it oddly checks day of week, you can schedule more than a year out, though things
	 * get tricky with leap years. Best to just assume you can only schedule out one year.
	 */
	const uint8_t ALARM_MONTH_DAY_DOW_HMS = 7;

	uint8_t rawYear; //!< MCP79410 raw year value, BCD 0 <= year <= 99. Not used for alarms.

	uint8_t rawMonth; //!< MCP79410 raw month value, BCD 1 <= month <= 12. Also contains leap year bit when reading the time.

	uint8_t rawDayOfMonth; //!< MCP79410 raw day of month value, BCD 1 <= dayOfMonth <= 31.

	/**
	 * @brief MCP79410 raw day of week value, BCD 1 <= dayOfWeek <= 7.
	 *
	 * The RTC does not enforce a day of week convention but this library does. 1 = Sunday, 7 = Saturday.
	 * This is because of how it's set based on the Unix mktime/gmtime functions. However, mktime is zero
	 * based, so be careful of that.
	 */
	uint8_t rawDayOfWeek; // BCD see code for warnings!

	/**
	 * @brief MCP79410 raw hour of day, BCD 0 <= hour <= 23. Or can be 12 hour mode with AM/PM flag.
	 *
	 * When setting from cloud time 24-hour time is always used, but the conversion functions can deal with 12-hour.
	 */
	uint8_t rawHour;

	uint8_t rawMinute; //!< MCP79410 raw minute, BCD 0 <= minute <= 59.

	uint8_t rawSecond; //!< MCP79410 raw second, BCD 0 <= second <= 59. Sometimes contains the oscillator running flag in the high bit.

	/**
	 * @brief Alarm mode ALARM_SECOND, ALARM_MINUTE, ...
	 *
	 * Note that these values are 0-7 and are shifted when storing in the ALxMSK value in the ALMxWKDAY register.
	 */
	uint8_t alarmMode = 0;
};

/**
 * @brief Class for managing the MCP79410 real-time clock chip with SRAM and EEPROM
 *
 * You typically instantiate one of these as a global variable in your program.
 *
 * ```
 * #include "MCP79410RK.h"
 *
 * MCP79410 rtc;
 *
 * void setup() {
 * 	rtc.setup();
 * }
 *
 * void loop() {
 * 	rtc.loop();
 * }
 * ```
 */
class MCP79410 {
public:
	/**
	 * @brief Constructor for MCP79410 objects.
	 *
	 * @param wire The I2C interface to use. Optional, default is Wire. On some devices you can use Wire1.
	 */
	MCP79410(TwoWire &wire = Wire);

	/**
	 * @brief Destructor. Not typically deleted as it's normally instantiated as a global variable.
	 */
	virtual ~MCP79410();


	/**
	 * @brief Sets the time synchronization mode
	 *
	 * | Constant | Value | Description |
	 * | -------- | ----- | ----------- |
	 * | TIME_SYNC_NONE | 0b00 | No automatic time synchronization |
	 * | TIME_SYNC_CLOUD_TO_RTC | 0b01 | RTC is set from cloud time at startup |
	 * | TIME_SYNC_RTC_TO_TIME | 0b10 | Time object is set from RTC at startup (if RTC appears valid) |
	 * | TIME_SYNC_AUTOMATIC | 0b11 | Time is synchronized in both directions (default value) |
	 *
	 * The withXXX() syntax allows you to chain multiple options, fluent-style:
	 *
	 * ```
	 * rtc.withTimeSyncMode(MCP79410::TIME_SYNC_CLOUD_TO_RTC).withBatteryEnable(false).setup();
	 * ```
	 */
	MCP79410 &withTimeSyncMode(uint8_t timeSyncMode);

	/**
	 * @brief Sets the battery enable mode
	 *
	 * @param value true = enable the ues of the backup battery, false = disable
	 *
	 * The default is true so you only normally have to do this if you want to disable. You can call this early, before setup()
	 * or at any time to make an immediate change.
	 *
	 * The withXXX() syntax allows you to chain multiple options, fluent-style:
	 *
	 * ```
	 * rtc.withTimeSyncMode(MCP79410::TIME_SYNC_CLOUD_TO_RTC).withBatteryEnable(false).setup();
	 * ```
	 */
	MCP79410 &withBatteryEnable(bool value) { setBatteryEnable(value); return *this; }


	/**
	 * @brief setup call, call during setup()
	 */
	void setup();

	/**
	 * @brief loop call, call on every time through loop()
	 */
	void loop();

	/**
	 * @brief Set the RTC from the cloud time (if valid)
	 *
	 * You normally don't need to do this, as it's done automatically from loop().
	 */
	bool setRTCFromCloud();

	/**
	 * @brief Set the RTC to a specific unixTime
	 *
	 * @param unixTime The Unix time to set to (seconds from January 1, 1970, at UTC).
	 *
	 * You can pass the value returned from [Time.now()](https://docs.particle.io/reference/device-os/firmware/#now-) into this function.
	 */
	bool setRTCTime(time_t unixTime);

	/**
	 * @brief Returns true if the RTC time is believed to be valid.
	 */
	bool isRTCValid() const;

	/**
	 * @brief Get the RTC time as a Unix time value
	 *
	 * The Unix time, seconds from January 1, 1970, at UTC. This has the same semantics as [Time.now()](https://docs.particle.io/reference/device-os/firmware/#now-).
	 */
	time_t getRTCTime() const;

	/**
	 * @brief Get the RTC time as a MCP79410Time object
	 *
	 * This is handy if you want to get the second, hour, minute, dayOfMonth, etc..
	 */
	bool getRTCTime(MCP79410Time &time) const;

	/**
	 * @brief Get the power down time
	 *
	 * This isn't as useful as you'd think. The time does not include a second or year. Only the first
	 * power failure is stored until reset using clearPowerFail(). Also, setting the RTC time (as we
	 * tend to do upon connecting to the cloud), resets the power fail times.
	 *
	 * If you still want to use the power failure times, read them right after calling rtc.setup().
	 */
	bool getPowerDownTime(MCP79410Time &time) const;

	/**
	 * @brief Get the power up time
	 *
	 * This isn't as useful as you'd think. The time does not include a second or year. Only the first
	 * power failure is stored until reset using clearPowerFail(). Also, setting the RTC time (as we
	 * tend to do upon connecting to the cloud), resets the power fail times.
	 *
	 * If you still want to use the power failure times, read them right after calling rtc.setup().
	 */
	bool getPowerUpTime(MCP79410Time &time) const;


	/**
	 * @brief Get the power failure bit. If there are stored power up and down times, this will be true
	 */
	bool getPowerFail() const;

	/**
	 * @brief Clear the power failure times
	 *
	 * This will clear the times returned for getPowerUpTime() and getPowerDownTime() and allow new times
	 * to be stored. If you don't call this, the old times will be preserved, even if there is another
	 * power failure.
	 */
	void clearPowerFail();


	/**
	 * @brief Returns true of the oscillator is running
	 *
	 * The oscillator is off after a cold boot from no battery, no 3V3 power. It will stay off until you
	 * set the time (manually or from the cloud time).
	 *
	 * As long as the CR1220 battery power is applied, the oscillator will continue to run.
	 *
	 * To determine if the clock time is valid you should use isRTCValid() which checks both the oscillator
	 * and the year.
	 */
	bool getOscillatorRunning() const;

	/**
	 * @brief Get the battery enable flag
	 *
	 * This is false when the RTC is started it up the first time. However, once you sync time from the cloud
	 * the first time, the battery will be enabled unless you explicitly `setBatteryEnable(false)`.
	 */
	bool getBatteryEnable() const;

	/**
	 * @brief Set the battery enable state
	 *
	 * Normally the battery is enabled, but if you're using the RTC without a battery you can set this to
	 * false. The RTC will still work, as long as you have 3V3 power. Since 3V3 is still supplied in SLEEP_MODE_DEEP
	 * this is not unreasonable.
	 */
	bool setBatteryEnable(bool value = true);

	/**
	 * @brief Clear the alarm (remove the setting and turn it off)
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 */
	bool clearAlarm(int alarmNum = 0);

	/**
	 * @brief Set an alarm
	 *
	 * @param time A MCP79410Time object for when the alarm should go off. This can be a specific time or a repeating time
	 * but you must have set it using one of the methods like setAlarmSecond(), setAlarmMinute(), ..., setAlarmTime().
	 *
	 * @param polarity Pass true (the default) for compatibility with D8 to wake from SLEEP_MODE_DEEP.
	 * false = active low, falling to wake. true = active high, rising to wake.
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 *
	 * @return true on success. This call will fail and return false if the RTC has not been set or alarmNum is not valid.
	 */
	bool setAlarm(const MCP79410Time &time, bool polarity = true, int alarmNum = 0);

	/**
	 * @brief Set an alarm
	 *
	 * @param secondsFromNow The number of seconds from now the alarm should go off. This is typically what you do to
	 * emulate the behavior of SLEEP_MODE_DEEP.
	 *
	 * @param polarity Pass true (the default) for compatibility with D8 to wake from SLEEP_MODE_DEEP.
	 * false = active low, falling to wake. true = active high, rising to wake.
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 *
	 * @return true on success. This call will fail and return false if the RTC has not been set or alarmNum is not valid.
	 */
	bool setAlarm(int secondsFromNow, bool polarity = true, int alarmNum = 0);

	/**
	 * @brief Returns true if the given alarmNum is in alarm state
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 *
	 * When using both alarms this is how you determine which one has gone off. You must clear the alarm using clearInterrupt()
	 * or it won't fire again!
	 */
	bool getInterrupt(int alarmNum = 0);

	/**
	 * @brief Clears the interrupt for the given alarm
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 *
	 * You must clear the alarm using clearInterrupt() or it won't fire again!
	 */
	void clearInterrupt(int alarmNum = 0);

	/**
	 * @brief Returns true if the given alarm is currently enabled
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 *
	 * To enable an alarm, use setAlarm(). To disable an alarm, use clearAlarm().
	 */
	uint8_t getAlarmEnableBit(int alarmNum) { return (alarmNum == 0) ? REG_CONTROL_ALM0EN : REG_CONTROL_ALM1EN; };

	/**
	 * @brief Function to get the register base address for a given alarm
	 *
	 * @param alarmNum Default is 0 if this parameter is omitted. Otherwise, must be 0 or 1.
	 *
	 * @param offset 0 to get the base, or a constant like MCP79410::REG_ALARM_WKDAY_OFFSET (3).
	 *
	 * @return Returns either MCP79410::REG_ALARM0 or MCP79410::REG_ALARM1 plus the given offset
	 */
	uint8_t getAlarmRegister(int alarmNum, int offset = 0) { return ((alarmNum == 0) ? REG_ALARM0 : REG_ALARM1) + offset; };


	/**
	 * @brief Gets the object to access the SRAM
	 */
	MCP79410SRAM &sram() { return sramObj; };

	/**
	 * @brief Gets the object to access the EEPROM
	 */
	MCP79410EEPROM &eeprom() { return eepromObj; };

	/**
	 * @brief Enters square wave output mode on MFP
	 *
	 * @param freq Frequency, one of the constants below
	 *
	 * | Constant | Value | Description | Trimming |
	 * | -------- | ----- | ----------- | -------- |
	 * | MCP79410::SQUARE_WAVE_1_HZ | 0x0 | Set the square wave output frequency on the MFP to 1 Hz | Yes |
	 * | MCP79410::SQUARE_WAVE_4096_HZ | 0x1 | Set the square wave output frequency on the MFP to 4.096 kHz (4096 Hz) | Yes |
	 * | MCP79410::SQUARE_WAVE_8192_HZ | 0x2 | Set the square wave output frequency on the MFP to 8.192 kHz (8192 Hz)| Yes |
	 * | MCP79410::SQUARE_WAVE_32768_HZ | 0x3 | Set the square wave output frequency on the MFP to 32.767 kHz | No |
	 *
	 * Trimming: Yes if affected by digital trimming or false if not.
	 *
	 * This mode is typically used to test or calibrate the clock. See section 5.6 (p. 30) in the datasheet. See also
	 * examples/6-oscillator-trim.
	 */
	bool setSquareWaveMode(uint8_t freq);

	/**
	 * @brief Leaves square wave output mode on MFP, allowing alarms to be set again
	 */
	bool clearSquareWaveMode();

	/**
	 * @brief Sets the oscillator trim value
	 *
	 * @param trim A signed 8-bit integer: -127 to +127.
	 */
	bool setOscTrim(int8_t trim);

	/**
	 * @brief Read a time value (RTC time, alarm time, or power failure or restore time)
	 *
	 * @param addr The address to read from
	 *
	 * @param time A reference to a MCP79410Time to save the data to
	 *
	 * @param timeMode the format of the time at that address, one of the constant below:
	 *
	 * | Constant | Value | Description |
	 * | --- | --- | --- |
	 * | MCP79410::TIME_MODE_RTC | 0 | Mode for deviceReadTime when reading the RTC
	 * | MCP79410::TIME_MODE_ALARM | 1 | Mode for deviceReadTime when reading the alarm times
	 * | MCP79410::TIME_MODE_POWER | 2 | Mode for deviceReadTime when reading the power failure times
	 *
	 */
	int deviceReadTime(uint8_t addr, MCP79410Time &time, int timeMode) const;

	/**
	 * @brief Write RTC time
	 *
	 * Sets the current time in the real-time clock. time should specify a time value at UTC. Normally
	 * you'd use setRTCFromCloud() or setRTCTime() instead. Those functions eventually call this.
	 */
	int deviceWriteRTCTime(uint8_t addr, const MCP79410Time &time);

	/**
	 * @brief Read a register byte.
	 *
	 * @param addr the register address to read from. For example MCP79410::REG_RTCWKDAY.
	 *
	 * This can only be used for registers, not for EEPROM.
	 */
	uint8_t deviceReadRegisterByte(uint8_t addr) const;

	/**
	 * @brief Write a register byte.
	 *
	 * @param addr the register address to write to. For example MCP79410::REG_RTCWKDAY.
	 *
	 * @param value the value to set
	 *
	 * This can only be used for registers, not for EEPROM. To set certain bits (read then write),
	 * use deviceWriteRegisterFlag() or deviceWriteRegisterByteMask() instead.
	 */
	int deviceWriteRegisterByte(uint8_t addr, uint8_t value);

	/**
	 * @brief Set or clear a register flag bit
	 *
	 * @param addr the register address to read/write to. For example MCP79410::REG_RTCWKDAY.
	 *
	 * @param value The value. Typically this is a single bit set, for example MCP79410::REG_RTCWKDAY_VBATEN = 0x08 to
	 * set or clear the MCP79410::REG_RTCWKDAY_VBATEN in the MCP79410::REG_RTCWKDAY register.
	 *
	 * @param set True to set the flag, false to clear the flag.
	 *
	 * This is a convenience method built over deviceWriteRegisterByteMask() to make it easier to set or clear flag bits.
	 */
	int deviceWriteRegisterFlag(uint8_t addr, uint8_t value, bool set);

	/**
	 * @brief Write to a register using bit masks
	 *
	 * @param addr the register address to read/write to. For example MCP79410::REG_RTCWKDAY.
	 *
	 * @param andMask Logically ANDed to the contents of the register (1st step)
	 *
	 * @param orMask Logically ORed to the content of the register (2nd step)
	 *
	 * To clear a flag you'd pass a bitwise negation of the flag it in the andMask.
	 * For example: ~ MCP79410::REG_RTCWKDAY_VBATEN in andMask. Note the bitwise negation ~. And 0 in orMask.
	 *
	 * To set a flag you'd pass the 0xff in andMask and the flag in orMask (for example, MCP79410::REG_RTCWKDAY_VBATEN).
	 *
	 * However you'd normally use deviceWriteRegisterFlag() which generates the masks for you instead.
	 */
	int deviceWriteRegisterByteMask(uint8_t addr, uint8_t andMask, uint8_t orMask);

	/**
	 * @brief Reads from either registers or an EEPROM register
	 *
	 * @param i2cAddr The I2C address, either MCP79410::REG_I2C_ADDR or MCP79410::EEPROM_I2C_ADDR.
	 *
	 * @param addr The address in the block. For REG_I2C_ADDR a register address or a SRAM address.
	 *
	 * @param buf The buffer to read data into
	 *
	 * @param bufLen The length of data to read. Do not read past the end
	 *
	 */
	int deviceRead(uint8_t i2cAddr, uint8_t addr, uint8_t *buf, size_t bufLen) const;

	/**
	 * @brief Writes to either registers or an EEPROM register
	 *
	 * @param i2cAddr The I2C address, either MCP79410::REG_I2C_ADDR or MCP79410::EEPROM_I2C_ADDR.
	 *
	 * @param addr The address in the block. For REG_I2C_ADDR a register address or a SRAM address.
	 *
	 * @param buf The buffer to write
	 *
	 * @param bufLen The length of data to write. Do not write past the end
	 *
	 * You should not use this for writing EEPROM data. Use deviceWriteEEPROM() instead.
	 */
	int deviceWrite(uint8_t i2cAddr, uint8_t addr, const uint8_t *buf, size_t bufLen);

	/**
	 * @brief Write to EEPROM
	 *
	 * @param addr The address 0 <= addr <= 0x7f
	 *
	 * @param buf The buffer to write
	 *
	 * @param bufLen The length of data to write. Do not write past the end; (addr + bufLen) < 0x80 must be true.
	 *
	 * This is a separate function from deviceWrite because writing bulk EEPROM data requires special handling.
	 * The number of bytes you can write at once is limited, and you need to check for completion before continuing.
	 */
	int deviceWriteEEPROM(uint8_t addr, const uint8_t *buf, size_t bufLen);

	/**
	 * @brief Function to wait for an EEPROM write to complete
	 *
	 * This is used by setBlockProtection() and deviceWriteEEPROM(). It's unlikely that you would ever call it manually.
	 */
	void waitForEEPROM();


#ifdef MCP79410_ENABLE_PROTECTED_WRITE
	/**
	 * @brief Write to the 8-byte protected block in EEPROM
	 *
	 * Note the 8-byte protected block is different than the block protection mode for the main 128 byte EEPROM!
	 *
	 * This is normally disabled, to minimize the chance of writing the protected block.
	 *
	 * This is typically used to store things like MAC addresses, though I think it would be a great place
	 * to put board identification information.
	 *
	 * The constant MCP79410::EEPROM_PROTECTED_BLOCK_SIZE is the value 8 so you don't have to hardcode it.
	 *
	 * You must `#define MCP79410_ENABLE_PROTECTED_WRITE` before including MCP79410RK.h in order to enable use
	 * of this function.
	 */
	bool eepromProtectedBlockWrite(const uint8_t *buf) {
		deviceWriteRegisterByte(REG_EE_UNLOCK, 0x55);
		deviceWriteRegisterByte(REG_EE_UNLOCK, 0xAA);

		// Use deviceWrite because the whole write needs to be done in one transaction
		int stat = deviceWrite(EEPROM_I2C_ADDR, EEPROM_PROTECTED, buf, EEPROM_PROTECTED_BLOCK_SIZE);
		if (stat == 0) {
			waitForEEPROM();
		}

		return (stat == 0);
	}
#endif

	static const uint8_t TIME_SYNC_NONE = 0b00; //!< No automatic time synchronization
	static const uint8_t TIME_SYNC_CLOUD_TO_RTC = 0b01; //!< RTC is set from cloud time at startup
	static const uint8_t TIME_SYNC_RTC_TO_TIME = 0b10; //!< Time object is set from RTC at startup (if RTC appears valid)
	static const uint8_t TIME_SYNC_AUTOMATIC = 0b11; //!< Time is synchronized in both directions (the default value)


	static const uint8_t EEPROM_PROTECTED_BLOCK_SIZE = 8; //!< EEPROM protected block size in bytes

	static const uint8_t EEPROM_PROTECT_NONE = 0x0; //!< EEPROM write protection disabled
	static const uint8_t EEPROM_PROTECT_UPPER_QUARTER = 0x1; //!< EEPROM write protection protects addresses 0x60 to 0x7f from writing
	static const uint8_t EEPROM_PROTECT_UPPER_HALF = 0x2; //!< EEPROM write protection protects addresses 0x40 to 0x7f from writing
	static const uint8_t EEPROM_PROTECT_ALL = 0x3; //!< EEPROM write protection fully enabled

	static const uint8_t SQUARE_WAVE_1_HZ = 0x0;//!< Set the square wave output frequency on the MFP to 1 Hz. This is affected by digital trimming.
	static const uint8_t SQUARE_WAVE_4096_HZ = 0x1;//!< Set the square wave output frequency on the MFP to 4.096 kHz (4096 Hz). This is affected by digital trimming.
	static const uint8_t SQUARE_WAVE_8192_HZ = 0x2;//!< Set the square wave output frequency on the MFP to 8.192 kHz (8192 Hz). This is affected by digital trimming.
	static const uint8_t SQUARE_WAVE_32768_HZ = 0x3;//!< Set the square wave output frequency on the MFP to 32.767 kHz. This is the direct crystal output and not affected by trimming.
	static const uint8_t SQUARE_WAVE_MASK = 0x3; //!< All of the bits used for square wave output frequency

	static const int TIME_MODE_RTC = 0; //!< Mode for deviceReadTime when reading the RTC
	static const int TIME_MODE_ALARM = 1; //!< Mode for deviceReadTime when reading the alarm times
	static const int TIME_MODE_POWER = 2; //!< Mode for deviceReadTime when reading the power failure times

protected:

	static const uint8_t REG_I2C_ADDR    = 0b1101111; //!< I2C address (0x6f) for reading and writing the registers and SRAM
	static const uint8_t REG_DATE_TIME  = 0x00; //!< Start of date and time register
	static const uint8_t REG_DATE_RTCSEC  = 0x00; //!< Second value and oscillator start/stop bit
	static const uint8_t   REG_DATE_RTCSEC_ST  = 0x80; //!< Oscillator start/stop bit (1 = run, 0 = stop)

	static const uint8_t REG_RTCWKDAY = 0x03; //!< Day of week and status bits register
	static const uint8_t  REG_RTCWKDAY_OSCRUN = 0x20; //!< Bit for oscillator currently running (1 = running)
	static const uint8_t  REG_RTCWKDAY_PWRFAIL = 0x10; //!< Bit for power failure set (1 = values are set)
	static const uint8_t  REG_RTCWKDAY_VBATEN = 0x08; //!< Bit for battery enabled (1 = battery enabled)

	static const uint8_t REG_CONTROL  = 0x07; //!< Control register
	static const uint8_t  REG_CONTROL_SQWEN = 0x40; //!< Square wave enabled (used for calibration)
	static const uint8_t  REG_CONTROL_ALM1EN = 0x20; //!< Alarm 1 enabled
	static const uint8_t  REG_CONTROL_ALM0EN = 0x10; //!< Alarm 0 enabled

	static const uint8_t REG_OSCTRIM = 0x08; //!< Oscillator trim register

	static const uint8_t REG_EE_UNLOCK  = 0x09; //!< EEPROM protected block unlock register

	static const uint8_t REG_ALARM0  = 0x0a; //!< Start of alarm 0 regiseters
	static const uint8_t REG_ALARM1  = 0x11; //!< Start of alarm 1 regiseters
	static const uint8_t  REG_ALARM_WKDAY_OFFSET = 3; //!< Offset of alarm WKDAY register, relative to REG_ALARM0 or REG_ALARM1. Actual address 0x0d, 0x14.
	static const uint8_t  REG_ALARM_WKDAY_ALMPOL = 0x80; //!< Bit for alarm polarity
	static const uint8_t  REG_ALARM_WKDAY_ALMIF = 0x08; //!< Alarm interrupt flag (1 = alarm triggered).

	static const uint8_t REG_POWER_DOWN  = 0x18; //!< Time of last power down register
	static const uint8_t REG_POWER_UP  = 0x1c; //!< Time of last power up register

	static const uint8_t REG_SRAM  = 0x20; //!< Beginning of SRAM data (64 bytes)

	static const uint8_t EEPROM_I2C_ADDR = 0b1010111; //!< I2C address (0x57) for reading and writing the EEPROM
	static const uint8_t EEPROM_PROTECTED = 0xf0; //!< Beginning of the 8-byte protected area of the EEPROM
	static const uint8_t EEPROM_STATUS = 0xff; //!< Bits to write protect the main EEPROM



	TwoWire &wire; //!< The I2C interface to use. Typically Wire (the default) but could be Wire1 on some devices.
	bool setupDone = false; //!< True after rtc.setup() has been called.
	bool timeSet = false; //!< True after the RTC has been set from cloud time the first time.
	bool batteryEnable = true; //!< True if the battery should be enabled.
	uint8_t timeSyncMode = TIME_SYNC_AUTOMATIC; //!< Time synchronization mode. Default is automatic.

	MCP79410SRAM sramObj; //!< Object to access the SRAM (static non-volatile RAM). Use the public sram() method to access it.
	MCP79410EEPROM eepromObj; //!< Object to access the EEPROM. Use the public eeprom() method to access it.

	friend class MCP79410SRAM;
	friend class MCP79410EEPROM;
};

#endif /* __MCP79410RK_H */
