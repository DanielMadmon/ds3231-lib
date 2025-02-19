#include "ds3231_lib_util.h"


static const uint8_t tm1637_symbol_table[10] = {
    //0
    0b00111111,
    //1
    0b00000110,
    //2
    0b01011011,
    //3
    0b01001111,
    //4
    0b01100110,
    //5
    0b01101101,
    //6
    0b01111101,
    //7
    0b00000111,
    //8
    0b01111111,
    //9
    0b01101111,
}; //Taken from https://crates.io/crates/tm1637-embedded-hal


bool ds3231_decimal_to_tm1637(uint8_t decimal_number,uint8_t* tm1637_number, uint8_t byte_size_tm1637){
    if(decimal_number > 99 || NULL == tm1637_number || 0x02u < byte_size_tm1637){
        return false;
    }else{
        uint8_t tens = 0;
        uint8_t temp_num = decimal_number;
        while(temp_num > 0x09u){
            temp_num -= 0x0Au;
            tens += 1;
        }
        tm1637_number[0] = tm1637_symbol_table[tens];
        tm1637_number[1] = tm1637_symbol_table[temp_num];
        return true;
    }
}