#include "ds3231_lib_private.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "driver/gpio.h"


static const uint8_t  ds3231_i2c_device_address = 0b1101000u; //taken from https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf
static const uint32_t ds3231_i2c_device_speed  =  400000u;
static const uint8_t  ds3231_i2c_max_reg_address = 0x12u;
static const int32_t  ds3231_i2c_timeout_single = 30; /** minimum is 28(number of bits) / 400_000 = 0.07ms */
static const int32_t  ds3231_i2c_timeout_multi = ds3231_i2c_timeout_single * ds3231_i2c_max_reg_address; /** minimum is 28(number of bits) / 400_000 = 0.07ms */


bool __ds3231_i2c_init(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }else if(true == dev->__i2c_init_f){
        return false;
    }else{
        const i2c_master_bus_config_t bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port =  (i2c_port_num_t)dev->i2c_port,
            .scl_io_num = (gpio_num_t)dev->i2c_scl_num,
            .sda_io_num = (gpio_num_t)dev->i2c_sda_num,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true
        };
        const i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = ds3231_i2c_device_address,
            .scl_speed_hz = ds3231_i2c_device_speed
        };


        i2c_master_bus_handle_t* i2c_bus_handle = malloc(sizeof(i2c_master_bus_handle_t));
        if (NULL == i2c_bus_handle){
            return false;
        }
        dev->i2c_bus = (i2c_master_bus_handle_t*)i2c_bus_handle;

        esp_err_t err = i2c_new_master_bus(&bus_config,(i2c_master_bus_handle_t*)dev->i2c_bus);
        if(ESP_OK != err){
            return false;
        }
        i2c_master_dev_handle_t * i2c_dev_handle = malloc(sizeof(i2c_master_dev_handle_t));
        if(NULL == i2c_dev_handle){
            return false;
        }
        dev->i2c_dev = (i2c_master_dev_handle_t*)i2c_dev_handle;
        err = i2c_master_bus_add_device(
            *((i2c_master_bus_handle_t*)dev->i2c_bus),
            &device_config,
            (i2c_master_dev_handle_t*)dev->i2c_dev
        );
        if(ESP_OK != err){
            return false;
        }

        dev->__i2c_init_f = true;
        return true;     
    }


}
bool __ds3231_i2c_deinit(ds3231_dev_t* dev){
    if(NULL == dev){
        return false;
    }
    else if(false == dev->__i2c_init_f){
        return false;
    }
    else if(NULL == dev->i2c_bus || NULL == dev->i2c_dev){
        return false;
    }else{
        
        esp_err_t err = i2c_master_bus_rm_device(*((i2c_master_dev_handle_t*)dev->i2c_dev));
        if(ESP_OK != err){
            return false;
        }
        err = i2c_del_master_bus(*((i2c_master_bus_handle_t*)dev->i2c_bus));
        if(ESP_OK != err){
            return false;
        }else{
            dev->__i2c_init_f = false;
            free(dev->i2c_bus);
            free(dev->i2c_dev);
            dev->i2c_bus = NULL;
            dev->i2c_dev = NULL;
            return true;
        }
    }

}

/**
 * write a single register in ds3231
 */
bool __ds3231_i2c_write_single(ds3231_dev_t* dev, uint8_t reg_address,uint8_t data){
    if(NULL == dev || ds3231_i2c_max_reg_address < reg_address){
        return false;
    }
    else if(false == dev->__i2c_init_f){
        return false;
    }else{
         i2c_master_transmit_multi_buffer_info_t buffer_info[2] = {
            {.write_buffer = &reg_address,.buffer_size = 1},
            {.write_buffer = &data, .buffer_size = 1}
        };
        esp_err_t err = i2c_master_multi_buffer_transmit(
            *((i2c_master_dev_handle_t*)dev->i2c_dev),
            buffer_info,
            2,
            ds3231_i2c_timeout_single
        );
        if(ESP_OK != err){
            return false;
        }else{
            return true;
        }
    }

}

bool __ds3231_i2c_write_multi(ds3231_dev_t* dev, uint8_t* data, uint8_t reg_address_start, uint8_t byte_length){
    if(NULL == dev
                || ds3231_i2c_max_reg_address < (reg_address_start + byte_length - 1)
                || NULL == data){
        return false;
    }else if(false == dev->__i2c_init_f){
        return false;
    }else{
        i2c_master_transmit_multi_buffer_info_t buffer_info[2] = {
            {.write_buffer = &reg_address_start,.buffer_size = 1},
            {.write_buffer = data, .buffer_size = (size_t)byte_length}
        };
        esp_err_t err = i2c_master_multi_buffer_transmit(
            *((i2c_master_dev_handle_t*)dev->i2c_dev),
            buffer_info,
            2,
            ds3231_i2c_timeout_single
        );
        if(ESP_OK != err){
            return false;
        }else{
            return true;
        }
    }
}

bool __ds3231_i2c_read_single(ds3231_dev_t* dev, uint8_t reg_address, uint8_t* data_out){
    if(NULL == dev || ds3231_i2c_max_reg_address < reg_address){
        return false;
    }
    else if(false == dev->__i2c_init_f){
        return false;
    }else{
        esp_err_t err = i2c_master_transmit_receive(
            (*((i2c_master_dev_handle_t*)dev->i2c_dev)),
            &reg_address,
            1,
            data_out,
            1,
            ds3231_i2c_timeout_single
        );
        if(ESP_OK != err){
            return false;
        }else{
            return true;
        }
    }
}
bool __ds3231_i2c_read_multi(ds3231_dev_t* dev, uint8_t reg_address_start, uint8_t* data_out, uint8_t byte_length){
    if(NULL == dev || ds3231_i2c_max_reg_address < (reg_address_start + byte_length - 1)){
        return false;
    }
    else if(false == dev->__i2c_init_f){
        return false;
    }else{
        esp_err_t err = i2c_master_transmit_receive(
            (*((i2c_master_dev_handle_t*)dev->i2c_dev)),
            &reg_address_start,
            1,
            data_out,
            (size_t)byte_length,
            ds3231_i2c_timeout_multi
        );
        if(ESP_OK != err){
            return false;
        }else{
            return true;
        }
    }
}

