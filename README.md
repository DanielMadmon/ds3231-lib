
# ds3231 driver library

a lightweight driver for the DS3231 rtc module.

### supported mcu
* currently only esp32 is supported.
* the driver can be ported easily. see porting section below


## API Reference

#### init driver

```c
  ds3231_dev_t dev;
  bool result = ds3231_init(&dev,sda_pin,scl_pin,i2c_port,i2c_init_flag);
```

| Parameter | Type     | Description                |
| :-------- | :------- | :------------------------- |
| `dev` | `ds3231_dev_t*` | handle for the new device
| `sda,scl.port` | uint32_t | i2c pins and port |
| `i2c_initialized` | bool | indicate if i2c is already initialized by the user

#### set time

```c
  bool res = ds3231_set_time(ds3231_dev_t* dev, bool use_24_format,
                      ds3231_time_data_t* time_data);
```
#### get time

```c
  bool res = ds3231_dev_t* dev,
                      ds3231_time_data_t* time_data;
```

#### set alarm

```c
bool res =  ds3231_set_alarm(ds3231_dev_t* dev, ds3231_time_data_t* time_data,
                      ds3231_alarm1_options* alarm1_options,
                      ds3231_alarm2_options* alarm2_options);

```

| Parameter | Type     | Description                |
| :-------- | :------- | :-------------------------
| `dev`     | `ds3231_dev_t` | `a pointer to ds3231_dev_t`
|`time_data`|`ds3231_time_data_t*`|`a pointer to ds3231_time_data_t. NULL if 0NCE_PER_SECOND/MINUTE is used`
|`alarm1_options`|`ds3231_alarm1_options*`|`a pointer to ds3231_alarm1_options. NULL if alarm2 is used.`
|`alarm2_options`|`ds3231_alarm2_options*`|`a pointer to ds3231_alarm2_options. NULL if alarm1 is used.`

### get alarm

```c
bool ds3231_get_alarm(ds3231_dev_t* dev, ds3231_time_data_t* time_data,
                      ds3231_alarm1_options* alarm1_options,
                      ds3231_alarm2_options* alarm2_options);
```


### enable alarm
```c
bool ds3231_enable_alarm(ds3231_dev_t* dev, bool alarm2);
```

### clear alarm flag
```c
bool ds3231_clear_alarm_flag(ds3231_dev_t* dev, bool alarm2);
```

### enable square wave output
```c
bool ds3231_enable_square_wave_output(ds3231_dev_t* dev, ds3231_sqw_frequecy frequency,bool enable_on_battery_backup);
```

### enable 32khz output

```c
bool ds3231_enable_32khz_output(ds3231_dev_t* dev);
```

### porting
* porting to another mcu only requires to implement 6 functions that are declared in [ds3231_lib_private.h](include/ds3231_lib_private.h)
