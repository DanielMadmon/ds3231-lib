#pragma once

#include "ds3231_lib.h"
#include <stddef.h>


/**
 * private API changes between different microcontrollers
 */


/**
 * initialize i2c device. failes on previous initialization
 */
bool __ds3231_i2c_init(ds3231_dev_t* dev);

/**
 * deinitialize i2c device if already initialized
 */
bool __ds3231_i2c_deinit(ds3231_dev_t* dev);

/**
 * write a single register in ds3231
 */
bool __ds3231_i2c_write_single(ds3231_dev_t* dev, uint8_t reg_address,uint8_t data);

/**
 * write multiple registers in ds3231
 */
bool __ds3231_i2c_write_multi(ds3231_dev_t* dev, uint8_t* data, uint8_t reg_address_start, uint8_t byte_length);

/**
 * read a single register in ds3231
 */
bool __ds3231_i2c_read_single(ds3231_dev_t* dev, uint8_t reg_address, uint8_t* data_out);

/**
 * read multiple registers in ds3231
 */
bool __ds3231_i2c_read_multi(ds3231_dev_t* dev, uint8_t reg_address_start, uint8_t* data_out, uint8_t byte_length);


