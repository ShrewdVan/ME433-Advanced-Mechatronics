#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_SDA 16
#define I2C_SCL 17
#define RIGHT_CLICK 21
#define LEFT_CLICK 14
#define INT_WATCH_PIN 15
#define I2C_PORT i2c0

#define IMU_ADD 0b1101000 
// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
#define INT_ENABLE 0x38
#define INT_STATUS 0x3A
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




volatile static double accel_x = 0.0, accel_y = 0.0, accel_z = 0.0, gyro_x = 0.0, gyro_y = 0.0, gyro_z = 0.0, temp = 0.0;
uint8_t buf[14];
int Imu_Ready = 0;

void imu_init();
void mouse_pin_init_all();
void gpio_callback_int();
int i2c_check();


int main(void)
{
    stdio_init_all();

    // init device stack on configured roothub port
    
    mouse_pin_init_all();

    imu_init();
    sleep_ms(200);

    while (!stdio_usb_connected()){
        sleep_ms(20);
    }

    if (i2c_check() == 1){     
        printf("y");
    } else {
        printf("n");
    }
    //gpio_set_irq_enabled_with_callback(INT_WATCH_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback_int);

    

    uint8_t reg = ACCEL_XOUT_H;
    while (true) {

        reg = ACCEL_XOUT_H;
        int confirm = i2c_write_blocking(I2C_PORT, IMU_ADD, &reg, 1, true);
        if (confirm != 1){
            printf("Error occurs during the I2C writing\r\n");
        }
        confirm = i2c_read_blocking(I2C_PORT, IMU_ADD, buf, 14, false);
        if (confirm != 14){
            printf("Error occurs during the I2C reading\r\n");
        }

        accel_x = (double)((int16_t)(buf[0] << 8 | buf[1])) * 0.000061;
        accel_y = (double)((int16_t)(buf[2] << 8 | buf[3])) * 0.000061;
        accel_z = (double)((int16_t)(buf[4] << 8 | buf[5])) * 0.000061;
        temp = (double)((int16_t)(buf[6] << 8 | buf[7])) / 340.00 + 36.53 ;
        gyro_x = (double)((int16_t)(buf[8] << 8 | buf[9])) * 0.007630;
        gyro_y = (double)((int16_t)(buf[10] << 8 | buf[11])) * 0.007630;
        gyro_z = (double)((int16_t)(buf[12] << 8 | buf[13])) * 0.007630;

        printf("x:%lf, y:%lf\r\n",accel_x, accel_y);

        Imu_Ready = 0;
        sleep_ms(5);
    }
}


void mouse_pin_init_all(){
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_init(INT_WATCH_PIN);
    gpio_set_dir(INT_WATCH_PIN, 0);

}


void gpio_callback_int(){
    Imu_Ready = 1;
}

void imu_init(){

    uint8_t command[2]; 
    // Resetting the entire device
    command[0] = PWR_MGMT_1;
    command[1] = 0b10000000;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    sleep_ms(100);
    // Turn on the chip
    command[0] = PWR_MGMT_1;
    command[1] = 0;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // Set the Gyro ranged +- 2000
    command[0] = GYRO_CONFIG;
    command[1] = 0b00011000;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // Set the Acce ranged +- 2g
    command[0] = ACCEL_CONFIG;
    command[1] = 0;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // // Set the mode to be LOW-POWER and Gyro X as the Clock source
    // command[0] = PWR_MGMT_1;
    // command[1] = 0b00100001;
    // i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // // Set the waking frequency to be 40Hz and no axis got disabled
    // command[0] = PWR_MGMT_2;
    // command[1] = 0b10000000;
    // i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // // Turn on the Ready-Data_Interrupt
    // command[0] = INT_ENABLE;
    // command[1] = 0b00000001;
    // i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);

}

int i2c_check(){
    uint8_t buf[2];
    buf[0] = WHO_AM_I;
    int confirm = i2c_write_blocking(I2C_PORT, IMU_ADD, &buf[0], 1, true);
    if (confirm != 1){
        return 2;
    }
    confirm = i2c_read_blocking(I2C_PORT, IMU_ADD, &buf[1], 1, false);
    if (confirm != 1){
        return 3;
    }
    if (buf[1] == 0x68){
        return 1;
    } else{
        return 0;
    }
}