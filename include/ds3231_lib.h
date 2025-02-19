#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "ds3231_lib_config.h"

#ifdef CONFIG_USE_UTIL
#include "ds3231_lib_util.h"
#endif

/**
 * TODO: 1. seperate config file
 *       2. remove esp_log
 *       3. seperate repo for lib & port
 */

typedef struct{
  /** must be in the range of 0-59 */
  uint8_t seconds;
  /** must be in the range of 0-59 */
  uint8_t minutes;
  /** must be in the range of 0-23 or 1-12 */
  uint8_t hours;
  /** must be in the range of 1-31 */
  uint8_t day_of_month;
  /** must be in the range of 1-12 */
  uint8_t month;
  /** must be in the range of 0-99 */
  uint8_t year;
  /** must be in the range of 1-7 */
  uint8_t day_of_week;
  /** flag for when using 12 hours mode*/
  bool is_12_hours_format;
  /**flag for PM/AM in 12 hours format */
  bool pm;
}ds3231_time_data_t;


typedef struct{
    #ifdef CONFIG_USE_I2C_BUS
    void * i2c_bus;
    #endif
    #ifdef CONFIG_USE_I2C_DEVICE
    void * i2c_dev;
    #endif
    #ifdef CONFIG_USE_I2C_PORT
    int32_t i2c_port;
    #endif
    uint32_t i2c_sda_num;
    uint32_t i2c_scl_num;
    bool __i2c_init_f;
}ds3231_dev_t;


typedef enum{
  DS3231_SQW_1HZ,
  DS3231_SQW_1024HZ,
  DS3231_SQW_4096HZ,
  DS3231_SQW_8192HZ
}ds3231_sqw_frequecy;


typedef enum{
  DS3231_ALARM1_DAY_OF_MONTH_HOURS_MINUTES_SECONDS = 0x00,
  DS3231_ALARM1_ONCE_PER_SECOND = 0x04,
  DS3231_ALARM1_HOURS_MINUTES_SECONDS = 0x08,
  DS3231_ALARM1_MINUTES_SECONDS = 0x0C,
  DS3231_ALARM1_SECONDS         = 0x0E,
  DS3231_ALARM1_DAY_OF_WEEK_HOURS_MINUTES_SECONDS = 0x10,
}ds3231_alarm1_options;


typedef enum{
  DS3231_ALARM2_DAY_OF_MONTH_HOURS_MINUTES = 0x00,
  DS3231_ALARM2_HOURS_MINUTES = 0x04,
  DS3231_ALARM2_MINUTES = 0x06,
  DS3231_ALARM2_ONCE_PER_MINUTE = 0x07,
  DS3231_ALARM2_DAY_OF_WEEK_HOURS_MINUTES = 0x08,
}ds3231_alarm2_options;

/**
 * public API not dependent on microcontroller type
 */


/**
 * initialize i2c driver,set input pins if not null. and set initial values in ds3231 ctrl register
 * 
 * @param i2c_initialized indicates that i2c was initialized before and already valid in dev struct
 */
bool ds3231_init(
                    #ifdef CONFIG_USE_I2C_DEVICE
                    ds3231_dev_t* dev,
                    #endif
                    uint32_t      i2c_sda_num,
                    uint32_t      i2c_scl_num
                    #ifdef CONFIG_USE_I2C_PORT
                    ,int32_t i2c_port
                    #endif
                    ,bool         i2c_initialized
                );

/**
 * set the time in ds3231
 */
bool ds3231_set_time(ds3231_dev_t* dev, bool use_24_format,
                      ds3231_time_data_t* time_data);

/**
 * get the time from ds3231
 */
bool ds3231_get_time(ds3231_dev_t* dev,
                      ds3231_time_data_t* time_data);


/**
 * @brief get the hours mode 12/24 
 * @param [dev][in] a pointer to ds3231_dev_t
 * @param [bool][out] a pointer which the result will be written to. 
 * @returns true on success false on fail
 */
bool ds3231_is_12_hours_mode(ds3231_dev_t* dev,bool* is_12);


/**
 * @brief get the temperature from ds3231
 * @param [dev][in] a pointer to ds3231_dev_t
 * @param [number][out] a pointer to int8_t. the integar value will be written to it.
 * @param [fraction][out] a pointer to uint8_t. the fractional value will be written to it.
 * fraction: 
 *  0 = number + 0.
 *  1 = number + 0.25.
 *  2 = number + 0.50.
 *  3 = number + 0.75.
 * @returns true on success false on fail
 */
bool ds3231_get_temperature(ds3231_dev_t*dev, int8_t* number,uint8_t* fraction);

/**
 * @brief set alarm according to time_data in ds3231_dev_t. 
 * @param [dev] a pointer to ds3231_dev_t
 * @param [time_data] a pointer to ds3231_time_data_t. NULL if 0NCE_PER_SECOND/MINUTE is used.
 * @param [alarm1_options] a pointer to ds3231_alarm1_options. NULL if alarm2 is used.
 * @param [alarm2_options] a pointer to ds3231_alarm2_options. NULL if alarm1 is used.
 */
bool ds3231_set_alarm(ds3231_dev_t* dev, ds3231_time_data_t* time_data, 
                      ds3231_alarm1_options* alarm1_options,
                      ds3231_alarm2_options* alarm2_options);


/**
 * @brief get alarm1/alarm2 data.does not indicate if alarm is enabled, 
 * only used to show it's trigger settings
 * @param [dev] [in] a pointer to ds3231_dev_t.
 * @param [time_data] [out] a pointer to ds3231_time_data_t.
 * @param [alarm1_options] [out] a pointer to ds3231_alarm1_options, can be NULL if alarm2 is read.
 * @param [alarm2_options] [out] a pointer to ds3231_alarm2_options, can be NULL if alarm1 is read.
 */
bool ds3231_get_alarm(ds3231_dev_t* dev, ds3231_time_data_t* time_data,
                      ds3231_alarm1_options* alarm1_options,
                      ds3231_alarm2_options* alarm2_options);



/**
 * @brief unset alarm 
 * @param[dev][in] a pointer to ds3231_dev_t.
 * @param[alarm2][in] true if unsetting alarm2. false if unsetting alarm1.
 */
bool ds3231_disable_alarm(ds3231_dev_t*dev, bool alarm2);

/**
 * @brief enable alarm with data and mode already written to alarm1/2 registeries
 * @param[dev][in] a pointer to ds3231_dev_t.
 * @param[alarm2][in] true if enabling alarm2. false if enabling alarm1. 
 */
bool ds3231_enable_alarm(ds3231_dev_t* dev, bool alarm2);



/**
 * clear alarm interrupt flag
 */
bool ds3231_clear_alarm_flag(ds3231_dev_t* dev, bool alarm2);



/**
 * enable square wave output and frequency
 */
bool ds3231_enable_square_wave_output(ds3231_dev_t* dev, ds3231_sqw_frequecy frequency,bool enable_on_battery_backup);

/**
 * enable square wave output and frequency
 */
bool ds3231_disable_square_wave_output(ds3231_dev_t* dev);


/**
 * get the oscillator stop flag (default on after initial power on)
 */
bool ds3231_get_oscillator_stop_flag(ds3231_dev_t* dev, bool* is_stopped);

/**
 * clear oscillator stop flag
 */
bool ds3231_clear_oscillator_stop_flag(ds3231_dev_t* dev);
/**
 * enable 32768hz oscillator output
 */
bool ds3231_enable_32khz_output(ds3231_dev_t* dev);

/**
 * disable 32768hz oscillator output
 */
bool ds3231_disable_32khz_output(ds3231_dev_t* dev);


/**
 * enable rtc internal oscillator
 */
bool ds3231_enable_oscillator(ds3231_dev_t* dev);

/**
 * disable rtc internal oscillator
 */
bool ds3231_disable_oscillator(ds3231_dev_t* dev);


/**
 * deinitialize i2c driver
 */
bool ds3231_deinit(ds3231_dev_t* dev);




