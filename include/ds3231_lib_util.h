#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * utilites for display representation of time
 */


/**
 * @brief convert up to two digits decimal number to tm1637 7 segment display representation. appends zeros if tens is 0
 * @param [decimal_number][in] decimal number to be converted. must be less than 100
 * @param [tm1637_number][out] pointer to an array where the result will be stored
 * @param [byte_size_tm1637][in] the byte size of tm1637_number (must be at least two)
 * 
 * @returns true on success false on fail
 */
bool ds3231_decimal_to_tm1637(uint8_t decimal_number,uint8_t* tm1637_number, uint8_t byte_size_tm1637,bool dot);
