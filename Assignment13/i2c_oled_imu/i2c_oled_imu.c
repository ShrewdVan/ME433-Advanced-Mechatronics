#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"

// =========================== Define Blocking ========================
#define ADC_WATCH_PIN 0

#define SCL_PIN 5
#define SDA_PIN 4
#define I2C_PORT i2c0

#define OLED_ADD 0b0111100
#define IMU_ADD 0b1101000
#define COMMAND_BYTE 0x00

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
// sensor data registers:
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75


void pico_set_led(bool led_on);
int pico_led_init();
void imu_init();
int i2c_check();

int main()
{
    // stdio and pico default led initialization
    stdio_init_all();
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    // I2C periphral initialization
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);

    if (i2c_check() == 0){
        while (true){
            pico_set_led(gpio_get(PICO_DEFAULT_LED_PIN));
        }
    } else if (i2c_check() == 1) {
        printf("I2C conmunication works as expected");
    }

    imu_init();

    while (true) {
        uint8_t buf[2];
        uint8_t result;
        uint8_t reg = ACCEL_XOUT_H;
        i2c_write_blocking(i2c0, IMU_ADD, &reg, 1, true);
        i2c_read_blocking(i2c0, IMU_ADD, buf, 2, false);
        int16_t accel_x = (int16_t)((int8_t)buf[0] << 8 | buf[1]);

        printf("The acceleration on the x direction is %f\r\n", (float)accel_x * 0.000061);
        sleep_ms(10);
    }
}


int pico_led_init(){
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}   

void pico_set_led(bool led_on){
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}

void imu_init(){
    uint8_t command[2];
    command[0] = PWR_MGMT_1;
    command[1] = 0;
    i2c_write_blocking(i2c0, IMU_ADD, command, 2, false);
    command[0] = ACCEL_CONFIG;
    command[1] = 0;
    i2c_write_blocking(i2c0, IMU_ADD, command, 2, false);
    command[0] = GYRO_CONFIG;
    command[1] = 0b00011000;
    i2c_write_blocking(i2c0, IMU_ADD, command, 2, false);
}

int i2c_check(){
    uint8_t buf[2];
    buf[0] = WHO_AM_I;
    int confirm = i2c_write_blocking(i2c0, IMU_ADD, &buf[0], 1, true);
    if (confirm != 1){
        printf("Error occurs during i2c writing");
    }
    confirm = buf[1] = i2c_read_blocking(i2c0, IMU_ADD, &buf[1], 1, false);
    if (confirm != 1){
        printf("Error occurs during i2c reading");
    }
    if (buf[1] == 0x68){
        return 1;
    } else{
        return 0;
    }
}

