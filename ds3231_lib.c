#include "ds3231_lib_private.h"
#include "ds3231_lib.h"


/**
 * ds3231 registers memory map
 */
static const uint8_t REG_SECONDS = 0x00u;
static const uint8_t REG_HOURS = 0x02u;
static const uint8_t REG_CONTROL = 0x0Eu;
static const uint8_t REG_STATUS  = 0x0Fu;
static const uint8_t REG_ALARM1_SECONDS = 0x07u;
static const uint8_t REG_ALARM2_MINUTES = 0x0Bu;
static const uint8_t REG_TEMP_MSB = 0x11u;
/**
 * bitmasks, and bitshift constants for ds3231 bitfields
 */

static const uint8_t BIT_SHIFT_HOURS_24_12_SELECT_BIT = 0x06u;
static const uint8_t BIT_SHIFT_OSF_FLAG  = 0x07;
static const uint8_t BIT_MASK_OSF_FLAG   = 0b10000000;
static const uint8_t BIT_SHIFT_AXMX = 0x07u;
static const uint8_t BIT_SHIFT_DYDT = 0x06u;
static const uint8_t BIT_SHIFT_AMPM = 0x05u;


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
                ){
    #ifdef CONFIG_USE_I2C_DEVICE
    if(NULL == dev){
        return false;
    }
    #endif
    if(false == i2c_initialized){
        dev->i2c_scl_num = i2c_scl_num;
        dev->i2c_sda_num = i2c_sda_num;
        #ifdef CONFIG_USE_I2C_PORT
            dev->i2c_port = i2c_port;
        #endif
        bool err = __ds3231_i2c_init(dev);
        if(false == err){
            return err;
        }else{
            return err;
        }

    }else if(true == i2c_initialized && false == dev->__i2c_init_f){
        dev->__i2c_init_f = true;
        return true;
    }else{
        return true;
    }

}

/**
 * @brief internal conversion from decimal to bcd up to 99.
 * @param [number][in]: a pointer to the number to be converted.
 * if the number is larger than 99 0 is returned.
 * */
static uint8_t dec_to_bcd(uint8_t* number){
    if(NULL == number){
        return 0;
    }else if(99 < *number){
        return 0;
    }else{
        uint8_t temp_number = *number;
        uint8_t temp_tens = 0;
        uint8_t temp_ones = 0;
        while(temp_number > 0x09){
            temp_number -= 0x0A;
            temp_tens += 1;
        }
        temp_ones = temp_number;
        temp_tens <<= 4u;
        temp_ones |= temp_tens;
        return temp_ones;
    }
}

/**convert from bcd in ds3231 time/alarm regs into decimal. up to 99.
 * @param [number] a pointer to a number in bcd format smaller than 99.
 * @param [width_of_tens] how many bits 1..4 for the tens in the bcd format
 * @returns a decimal format (uint8_t) of the number.
 * @return in case the number is NULL returns 0. 
 * */
static uint8_t bcd_to_dec(uint8_t* number,uint8_t width_of_tens){
    if(NULL == number){
        return 0;
    }else if(*number < 0x0A){
        return *number;
    }else if (99 < *number){
        return 0;
    }else{
        uint8_t temp_number = *number;
        //clear high nibble to get ones
        const uint8_t temp_ones = temp_number &~ (0xf0u);
        uint8_t temp_tens = temp_number >> 4u;
        uint8_t mask = 0x00;
        switch(width_of_tens){
            default: 
                mask = 0x00;
                break;
            case(0x01):
                mask = 0xfe;
                break;
            case(0x02):
                mask = 0xfc;
                break;
            case(0x03):
                mask = 0xf8;
                break;
            case(0x04):
                mask = 0xf0;
                break;
        }
        temp_tens &= ~(mask);
        return ((temp_tens * 0x0Au) + temp_ones);
    }
}

/**
 * time set/get functions
 */

bool ds3231_get_time(ds3231_dev_t* dev,
                      ds3231_time_data_t* time_data){
    if(NULL == dev || NULL == time_data){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t buffer[7] = {0};
        uint8_t width_of_tens_hours = 0;
        bool res = __ds3231_i2c_read_multi(dev,REG_SECONDS,buffer,7);
        if(!res){
            return false;
        }else{
            time_data->seconds = bcd_to_dec(&buffer[0],3);
            time_data->minutes = bcd_to_dec(&buffer[1],3);
            width_of_tens_hours =(buffer[2] >> BIT_SHIFT_HOURS_24_12_SELECT_BIT) &0x01u ? 1 : 2;
            time_data->hours = bcd_to_dec(&buffer[2],width_of_tens_hours);
            time_data->day_of_week = buffer[3];
            time_data->day_of_month = bcd_to_dec(&buffer[4],2);
            time_data->month = bcd_to_dec(&buffer[5],1);
            time_data->year = bcd_to_dec(&buffer[6],4);
            return true;
        }
    }
                    
}

bool ds3231_set_time(ds3231_dev_t* dev, bool use_24_format,
                      ds3231_time_data_t* time_data){                  
    if(NULL == dev || NULL == time_data){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t buffer[7] = {0};  
        buffer[0] = dec_to_bcd(&time_data->seconds);
        buffer[1] = dec_to_bcd(&time_data->minutes);
        buffer[2] = dec_to_bcd(&time_data->hours);
        //set to 12 hours format
        if(!use_24_format){
            buffer[2] |= (1u << BIT_SHIFT_HOURS_24_12_SELECT_BIT);
            buffer[2] |= ((uint8_t)time_data->pm << 0x06u); //set PM flag if needed
        }
        buffer[3] = time_data->day_of_week; //no need to convert to bcd. MAX is 7
        buffer[4] = dec_to_bcd(&time_data->day_of_month);
        buffer[5] = dec_to_bcd(&time_data->month);
        buffer[6] = dec_to_bcd(&time_data->year);
        bool res = ds3231_clear_oscillator_stop_flag(dev);
        if(!res){
            return false;
        }else{
            res = __ds3231_i2c_write_multi(dev,buffer,REG_SECONDS,7);
            return res;
        }
    }
}

/**
 * configuration functions
 */

bool ds3231_get_oscillator_stop_flag(ds3231_dev_t* dev, bool* is_stopped){
    if(NULL == dev || NULL == is_stopped){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t reg_val = 0;
        bool res_read = __ds3231_i2c_read_single(dev,REG_STATUS,&reg_val);
        if(!res_read){
            return res_read;
        }else{
            *is_stopped = (reg_val >> BIT_SHIFT_OSF_FLAG) & 0x01;
            return true;
        }
    }
}


bool ds3231_clear_oscillator_stop_flag(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res_op = __ds3231_i2c_read_single(dev,REG_STATUS,&current_reg);
        if(!res_op){
            return false;
        }else{
            current_reg |= BIT_MASK_OSF_FLAG;
            res_op = __ds3231_i2c_write_single(dev,REG_STATUS,current_reg);
            return res_op;
        }
    }
}


bool ds3231_deinit(ds3231_dev_t* dev){
    return __ds3231_i2c_deinit(dev);
}


bool ds3231_disable_oscillator(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_CONTROL,&current_reg);
        if(!res){
            return res;
        }else{
            current_reg |= (0x01u << 0x07u);
            res = __ds3231_i2c_write_single(dev,REG_CONTROL,current_reg);
            return res;
        }
    }
}
bool ds3231_enable_oscillator(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_CONTROL,&current_reg);
        if(!res){
            return res;
        }else{
            current_reg &= ~(0x01u << 0x07u);
            res = __ds3231_i2c_write_single(dev,REG_CONTROL,current_reg);
            return res;
        }
    }
}

bool ds3231_enable_32khz_output(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_STATUS,&current_reg);
        if(!res){
            return res;
        }else{
            current_reg |= (0x01u << 0x03u); //set EN32KHZ to 1
            res = __ds3231_i2c_write_single(dev,REG_STATUS,current_reg);
            return res;
        }
    }
}

bool ds3231_disable_32khz_output(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_STATUS,&current_reg);
        if(!res){
            return res;
        }else{
            current_reg &= ~(0x01u << 0x03u); //set EN32KHZ bit to 0
            res = __ds3231_i2c_write_single(dev,REG_STATUS,current_reg);
            return res;
        }
    }
}

bool ds3231_enable_square_wave_output(ds3231_dev_t* dev, ds3231_sqw_frequecy frequency,bool enable_on_battery_backup){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_CONTROL,&current_reg);
        if(!res){
            return res;
        }else{
            current_reg &= ~(1u << 0x02u); //set INTCN to 0
            current_reg |= (frequency << 3);
            if(enable_on_battery_backup){
                current_reg |= (0x01u << 0x06u);
            }
            res = __ds3231_i2c_write_single(dev,REG_CONTROL,current_reg);
            return res;
        }
    }
}

bool ds3231_disable_square_wave_output(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_CONTROL,&current_reg);
        if(!res){
            return res;
        }else{
            current_reg |= (1u << 0x02u); //set INTCN bit to one
            res = __ds3231_i2c_write_single(dev,REG_CONTROL,current_reg);
            return res;
        }
    }
}


/**
 * ALARMS control
 */

bool ds3231_clear_alarm_flag(ds3231_dev_t* dev, bool alarm2){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t current_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_STATUS,&current_reg);
        if(!res){
            return false;
        }else{
            current_reg &= ~(0x01 << ((uint8_t)alarm2));
            res = __ds3231_i2c_write_single(dev,REG_STATUS,current_reg);
            return res;
        }
    }
}

bool ds3231_set_alarm(ds3231_dev_t* dev, ds3231_time_data_t* time_data, 
                      ds3231_alarm1_options* alarm1_options,
                      ds3231_alarm2_options* alarm2_options){
    if(NULL == dev || (NULL == alarm1_options && NULL == alarm2_options)){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
       uint8_t buffer[4] = {0};
       uint8_t ctrl_reg_buffer = 0;
       bool res = false;
       bool is_12 = false;
       res = __ds3231_i2c_read_single(dev,REG_HOURS,&ctrl_reg_buffer);
       if(!res){
        return res;
       }else{
        is_12 = (ctrl_reg_buffer >> 0x06u) & 0x01u ? true : false;
        ctrl_reg_buffer = 0;
       }       
       if(NULL != alarm1_options){
            if(DS3231_ALARM1_ONCE_PER_SECOND != *alarm1_options && NULL == time_data){
                return false;
            }
            switch(*alarm1_options){
                default:
                    return false;
                case(DS3231_ALARM1_DAY_OF_MONTH_HOURS_MINUTES_SECONDS):
                    buffer[0] = dec_to_bcd(&time_data->seconds);
                    buffer[1] = dec_to_bcd(&time_data->minutes);
                    buffer[2] = dec_to_bcd(&time_data->hours);
                    buffer[2] |= ((uint8_t)is_12 << 0x06u);
                    buffer[2] |= (((uint8_t)time_data->pm)  << 0x05u);
                    buffer[3] = dec_to_bcd(&time_data->day_of_month);
                    buffer[3] &= ~(0x01u << BIT_SHIFT_DYDT); //clear DYDT to put in day of month mode. and set alarm mode
                    break;
                case(DS3231_ALARM1_ONCE_PER_SECOND):
                    buffer[0] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[1] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[2] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[3] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM1_HOURS_MINUTES_SECONDS):
                    buffer[0] = dec_to_bcd(&time_data->seconds);
                    buffer[1] = dec_to_bcd(&time_data->minutes);
                    buffer[2] = dec_to_bcd(&time_data->hours);
                    buffer[2] |= ((uint8_t)is_12 << 0x06u);
                    buffer[2] |= (((uint8_t)time_data->pm)  << 0x05u);
                    buffer[3] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM1_MINUTES_SECONDS):
                    buffer[0] = dec_to_bcd(&time_data->seconds);
                    buffer[1] = dec_to_bcd(&time_data->minutes);
                    buffer[2] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[3] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM1_SECONDS):
                    buffer[0] = dec_to_bcd(&time_data->seconds);
                    buffer[1] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[2] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[3] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM1_DAY_OF_WEEK_HOURS_MINUTES_SECONDS):
                    buffer[0] = dec_to_bcd(&time_data->seconds);
                    buffer[1] = dec_to_bcd(&time_data->minutes);
                    buffer[2] = dec_to_bcd(&time_data->hours);
                    buffer[2] |= (((uint8_t)time_data->pm)  << 0x05u);
                    buffer[2] |= ((uint8_t)is_12 << 0x06u);
                    buffer[3] = dec_to_bcd(&time_data->day_of_week);
                    buffer[3] |= (0x01u << BIT_SHIFT_DYDT); //set DYDT bit to set in day of week mode. and set alarm mode
                    break;
            }
            res = __ds3231_i2c_read_single(dev,REG_CONTROL,&ctrl_reg_buffer);
            if(!res){
                return res;
            }else{
                ctrl_reg_buffer |= 0x01u; //set A1IE
                ctrl_reg_buffer &= ~(0x01u << 1); //reset A2IE
                ctrl_reg_buffer |= (0x01u << 0x02u); //set intc
                res = __ds3231_i2c_write_multi(dev,buffer,REG_ALARM1_SECONDS,4);
                if(!res){
                    return res;
                }else{
                    res = __ds3231_i2c_write_single(dev,REG_CONTROL,ctrl_reg_buffer);
                    if(!res){
                        return res;
                    }else{
                        return ds3231_clear_alarm_flag(dev,false);
                    }
                }
            }
       }else if(NULL != alarm2_options){
            if(DS3231_ALARM2_ONCE_PER_MINUTE != *alarm2_options && NULL == time_data){
                return false;
            }
            switch(*alarm2_options){
                default:
                    return false;
                case(DS3231_ALARM2_DAY_OF_MONTH_HOURS_MINUTES):
                    buffer[0] = dec_to_bcd(&time_data->minutes);
                    buffer[1] = dec_to_bcd(&time_data->hours);
                    buffer[1] |= ((uint8_t)is_12 << 0x06u);
                    buffer[1] |= (((uint8_t)time_data->pm)  << 0x05u);
                    buffer[2] = dec_to_bcd(&time_data->day_of_month);
                    buffer[2] &= ~(0x01u << BIT_SHIFT_DYDT); //ensure DYDT is low to select day_of_month mode
                    break;
                case(DS3231_ALARM2_HOURS_MINUTES):
                    buffer[0] = dec_to_bcd(&time_data->minutes);
                    buffer[1] = dec_to_bcd(&time_data->hours);
                    buffer[1] |= ((uint8_t)is_12 << 0x06u);
                    buffer[1] |= (((uint8_t)time_data->pm)  << 0x05u);
                    buffer[2] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM2_MINUTES):
                    buffer[0] = dec_to_bcd(&time_data->minutes);
                    buffer[1] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[2] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM2_ONCE_PER_MINUTE):
                    buffer[0] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[1] = (0x01u << BIT_SHIFT_AXMX);
                    buffer[2] = (0x01u << BIT_SHIFT_AXMX);
                    break;
                case(DS3231_ALARM2_DAY_OF_WEEK_HOURS_MINUTES):
                    buffer[0] = dec_to_bcd(&time_data->minutes);
                    buffer[1] = dec_to_bcd(&time_data->hours);
                    buffer[1] |= (((uint8_t)time_data->pm)  << 0x05u);
                    buffer[1] |= ((uint8_t)is_12 << 0x06u);
                    buffer[2] = dec_to_bcd(&time_data->day_of_week);
                    buffer[2] |= (0x01u << BIT_SHIFT_DYDT); //set DYDT to enable day of week mode 
                    break;
            }
            res = __ds3231_i2c_read_single(dev,REG_CONTROL,&ctrl_reg_buffer);
            if(!res){
                return res;
            }else{
                ctrl_reg_buffer |= (0x01u << 0x01u); //set A2IE
                ctrl_reg_buffer &= ~(0x01u); // reset A1IE
                ctrl_reg_buffer |= (0x01u << 0x02u); //set intcn
                res = __ds3231_i2c_write_multi(dev,buffer,REG_ALARM2_MINUTES,3);
                if(!res){
                    return res;
                }else{
                    res =  __ds3231_i2c_write_single(dev,REG_CONTROL,ctrl_reg_buffer);
                    if(!res){
                        return res;
                    }else{
                        return ds3231_clear_alarm_flag(dev,true);
                    }
                }
            }
            
       }else{
            return false;
        }
    }
}

bool ds3231_disable_alarm(ds3231_dev_t*dev, bool alarm2){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t ctrl_reg_current = 0;
        bool res = false;
        res = __ds3231_i2c_read_single(dev,REG_CONTROL,&ctrl_reg_current);
        if(!res){
            return res;
        }else{
            ctrl_reg_current &= ~(0x01 << ((uint8_t)alarm2));
            return __ds3231_i2c_write_single(dev,REG_CONTROL,ctrl_reg_current);
        }
    }
}

bool ds3231_enable_alarm(ds3231_dev_t* dev, bool alarm2){
    if(NULL == dev){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t ctrl_reg_current = 0;
        bool res = false;
        res = __ds3231_i2c_read_single(dev,REG_CONTROL,&ctrl_reg_current);
        if(!res){
            return res;
        }else{
            ctrl_reg_current |= (0x01 << ((uint8_t)alarm2));
            return __ds3231_i2c_write_single(dev,REG_CONTROL,ctrl_reg_current);
        }
    }
}

static void set_hours(bool is_12_hours,ds3231_time_data_t* time_data,uint8_t* hours_reg){
    if(is_12_hours){
        time_data->hours = bcd_to_dec(hours_reg,1);
        time_data->is_12_hours_format = is_12_hours;
        time_data->pm = *hours_reg >> BIT_SHIFT_AMPM &0x01u;
    }else{
        time_data->hours = bcd_to_dec(hours_reg,2);
        time_data->is_12_hours_format = is_12_hours;
    }
}

bool ds3231_get_alarm(ds3231_dev_t* dev, ds3231_time_data_t* time_data,
                      ds3231_alarm1_options* alarm1_options,
                      ds3231_alarm2_options* alarm2_options){
    uint8_t alarm_regs[4] = {0};
    uint8_t alarm_bit_flag = 0;
    uint8_t temp_alarm = 0;
    bool is_12_hours = false;
    bool res = false;
    if(NULL == dev || NULL == time_data){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else if(NULL != alarm1_options){
        res = __ds3231_i2c_read_multi(dev,REG_ALARM1_SECONDS,alarm_regs,4);
        if(!res){
            return false;
        }else{
            alarm_bit_flag = alarm_regs[0] >> BIT_SHIFT_AXMX &0x01;
            temp_alarm = alarm_regs[1] >> BIT_SHIFT_AXMX &0x01;
            temp_alarm <<= 0x01u;
            alarm_bit_flag |= temp_alarm;
            temp_alarm = alarm_regs[2] >> BIT_SHIFT_AXMX &0x01u;
            temp_alarm <<= 0x02u;
            alarm_bit_flag |= temp_alarm;
            temp_alarm = alarm_regs[3] >> BIT_SHIFT_AXMX &0x01u;
            temp_alarm <<= 0x03u;
            alarm_bit_flag |= temp_alarm;
            temp_alarm = alarm_regs[3] >> BIT_SHIFT_DYDT &0x01u;
            temp_alarm <<= 0x04u;
            alarm_bit_flag |= temp_alarm;
            is_12_hours = alarm_regs[2] >> BIT_SHIFT_HOURS_24_12_SELECT_BIT &0x01u;

            switch(alarm_bit_flag){
                default:
                    return false;
                case(DS3231_ALARM1_DAY_OF_MONTH_HOURS_MINUTES_SECONDS):
                    *alarm1_options = alarm_bit_flag;
                    time_data->seconds = bcd_to_dec(&alarm_regs[0],3);
                    time_data->minutes = bcd_to_dec(&alarm_regs[1],3);
                    set_hours(is_12_hours,time_data,&alarm_regs[2]);
                    time_data->day_of_month = bcd_to_dec(&alarm_regs[3],2);
                    *alarm1_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM1_ONCE_PER_SECOND):
                    *alarm1_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM1_HOURS_MINUTES_SECONDS):
                    time_data->seconds = bcd_to_dec(&alarm_regs[0],3);
                    time_data->minutes = bcd_to_dec(&alarm_regs[1],3);
                    set_hours(is_12_hours,time_data,&alarm_regs[2]);
                    *alarm1_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM1_MINUTES_SECONDS):
                    time_data->seconds = bcd_to_dec(&alarm_regs[0],3);
                    time_data->minutes = bcd_to_dec(&alarm_regs[1],3);
                    *alarm1_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM1_SECONDS):
                    time_data->seconds = bcd_to_dec(&alarm_regs[0],3);
                    *alarm1_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM1_DAY_OF_WEEK_HOURS_MINUTES_SECONDS):
                    time_data->seconds = bcd_to_dec(&alarm_regs[0],3);
                    time_data->minutes = bcd_to_dec(&alarm_regs[1],3);
                    set_hours(is_12_hours,time_data,&alarm_regs[2]);
                    alarm_regs[3] &= ~(0xf0);
                    time_data->day_of_week = alarm_regs[3];
                    *alarm1_options = alarm_bit_flag;
                    return true;
            }
        }
    }else if (NULL != alarm2_options){
        res = __ds3231_i2c_read_multi(dev,REG_ALARM2_MINUTES,alarm_regs,3);
        if(!res){
            return false;
        }else{
            alarm_bit_flag = alarm_regs[0] >> BIT_SHIFT_AXMX &0x01u;
            temp_alarm = alarm_regs[1] >> BIT_SHIFT_AXMX &0x01u;
            temp_alarm <<= 0x01u;
            alarm_bit_flag |= temp_alarm;
            temp_alarm = alarm_regs[2] >> BIT_SHIFT_AXMX &0x01u;
            temp_alarm <<= 0x02u;
            alarm_bit_flag |= temp_alarm;
            temp_alarm = alarm_regs[2] >> BIT_SHIFT_DYDT &0x01u;
            temp_alarm <<= 0x03u;
            alarm_bit_flag |= temp_alarm;
            is_12_hours = alarm_regs[1] >> BIT_SHIFT_HOURS_24_12_SELECT_BIT &0x01u;
            switch(alarm_bit_flag){
                default:
                    return false;
                case(DS3231_ALARM2_DAY_OF_MONTH_HOURS_MINUTES):
                    time_data->minutes = bcd_to_dec(&alarm_regs[0],3);
                    set_hours(is_12_hours,time_data,&alarm_regs[1]);
                    time_data->day_of_month = bcd_to_dec(&alarm_regs[2],2);
                    *alarm2_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM2_HOURS_MINUTES):
                    time_data->minutes = bcd_to_dec(&alarm_regs[0],3);
                    set_hours(is_12_hours,time_data,&alarm_regs[1]);
                    *alarm2_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM2_MINUTES):
                    time_data->minutes = bcd_to_dec(&alarm_regs[0],3);
                    *alarm2_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM2_ONCE_PER_MINUTE):
                    *alarm2_options = alarm_bit_flag;
                    return true;
                case(DS3231_ALARM2_DAY_OF_WEEK_HOURS_MINUTES):
                    time_data->minutes = bcd_to_dec(&alarm_regs[0],3);
                    set_hours(is_12_hours,time_data,&alarm_regs[1]);
                    time_data->day_of_week = bcd_to_dec(&alarm_regs[2],2);
                    *alarm2_options = alarm_bit_flag;
                    return true;
            }
        }
    }else{
        return false;
    }
}

bool ds3231_is_12_hours_mode(ds3231_dev_t* dev,bool* is_12){
    if(NULL == dev || NULL == is_12){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t hours_reg = 0;
        bool res = __ds3231_i2c_read_single(dev,REG_HOURS,&hours_reg);
        if(!res){
            return res;
        }else{
            *is_12 = (hours_reg >> BIT_SHIFT_HOURS_24_12_SELECT_BIT) &0x01;
            return true;
        }
    }
}

bool ds3231_get_temperature(ds3231_dev_t*dev, int8_t* number,uint8_t* fraction){
    if(NULL == dev || NULL == number || NULL == fraction){
        return false;
    }else if(!dev->__i2c_init_f){
        return false;
    }else{
        uint8_t reg_buffer[2] = {0};
        bool res = __ds3231_i2c_read_multi(dev,REG_TEMP_MSB,reg_buffer,2);
        if(!res){
            return res;
        }else{
            *number = (int8_t)reg_buffer[0];
            *fraction = reg_buffer[1] >> 0x06u;
            return true;
        }
    }
}