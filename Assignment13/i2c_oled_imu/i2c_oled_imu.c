#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"

// =========================== Define Blocking ========================
#define INT_WATCH_PIN 13
#define SCL_PIN 21
#define SDA_PIN 20
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

// =========================== Global Variable ========================
double accel_x = 0.0, accel_y = 0.0, accel_z = 0.0, gyro_x = 0.0, gyro_y = 0.0, gyro_z = 0.0, temp = 0.0;
uint8_t buf[14];
uint8_t ssd1306_buf[513];
int imu_ready = 0;

// =========================== Function Blocking ======================
// Function used to send command to ssd1306
void ssd1306_Send_Command(uint8_t);
// Function using ssd1306_Send_Command to initialize the ssd1306
void ssd1306_Setup();
// Function used to clear all the pixels of the oled
void ssd1306_Clear();
// Function used to display a pure string, ex. "Hello World"
void Set_Pixel(bool, int, int);
// Function used to draw a horizontal line between two columes at row 16 
void Load_X_Line(int, int);
// Function used to draw a vertical line between two rows at colume 64
void Load_Y_Line(int, int);
// Send the latest info from imu, send to oled and update the X-axis display buffer
void X_accel_Update();
// Send the latest info from imu, send to oled and update the Y-axis display buffer
void Y_accel_Update();
// Update the oled display with the info within the buffer
void ssd1306_Update();
// Initialize the Imu unit to start measuring
void imu_init();
// Check whether the communication between pico and Imu chip is established. ("1" for yes, and other as error)
int i2c_check();
// The interrupt callback function used to repond the interrupt pulse sent from Imu "Int" Pin
void gpio_callback();



// Main Function starts here
int main()
{
    // stdio and pico default led initialization
    stdio_init_all(); 
    // I2C periphral initialization
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_init(INT_WATCH_PIN);
    gpio_set_dir(INT_WATCH_PIN, 0);

    while (!stdio_usb_connected()){
        ;
    }
    sleep_ms(50);
    printf("Starting...\r\n");

    // Imu and Oled initilalization 
    imu_init();
    ssd1306_Setup();
    ssd1306_Clear();
    sleep_ms(200);

    // Check the chip Communication
    if (i2c_check() == 1){     
        printf("I2C conmunication works as expected\r\n");
    } else {
        printf("Failed to build communication with IMU\r\n");
    }

    gpio_set_irq_enabled_with_callback(INT_WATCH_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    uint8_t reg;

    // Inifite Asking loop, the flag is controlled by the ISR Function
    while (true) {
        if (imu_ready == 1){
            ssd1306_Clear();
            reg = ACCEL_XOUT_H;
            int confirm = i2c_write_blocking(I2C_PORT, IMU_ADD, &reg, 1, true);
            if (confirm != 1){
                printf("Error occurs during the I2C writing\r\n");
            }
            confirm = i2c_read_blocking(I2C_PORT, IMU_ADD, buf, 14, false);
            if (confirm != 14){
                printf("Error occurs during the I2C reading\r\n");
            }

            // Assembly the two 8-bit info from I2C to a completed 16-bit info which we are interested in 
            accel_x = (double)((int16_t)(buf[0] << 8 | buf[1])) * 0.000061;
            accel_y = (double)((int16_t)(buf[2] << 8 | buf[3])) * 0.000061;
            accel_z = (double)((int16_t)(buf[4] << 8 | buf[5])) * 0.000061;
            temp = (double)((int16_t)(buf[6] << 8 | buf[7])) / 340.00 + 36.53 ;
            gyro_x = (double)((int16_t)(buf[8] << 8 | buf[9])) * 0.007630;
            gyro_y = (double)((int16_t)(buf[10] << 8 | buf[11])) * 0.007630;
            gyro_z = (double)((int16_t)(buf[12] << 8 | buf[13])) * 0.007630;

            // Update the data to the ssd1306 buffer
            X_accel_Update();
            Y_accel_Update();
            // Turns the info in buffer into display
            ssd1306_Update();

            // Clear the flag to prepare the next generated pulse
            imu_ready = 0;
        }
        sleep_ms(5);
    }
}




// =========================== Function Detailed Blocking ========================
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
    // Set the mode to be LOW-POWER and Gyro X as the Clock source
    command[0] = PWR_MGMT_1;
    command[1] = 0b00100001;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // Set the waking frequency to be 40Hz and no axis got disabled
    command[0] = PWR_MGMT_2;
    command[1] = 0b10000000;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // Turn on the Ready-Data_Interrupt
    command[0] = INT_ENABLE;
    command[1] = 0b00000001;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);

}

int i2c_check(){
    uint8_t buf[2];
    buf[0] = WHO_AM_I;
    // The WHO_AM_I should be 0x68 if all the stuff goes fine
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


void gpio_callback(){
    // When reveived the pulse from the chip, turns the flag to "1"
    imu_ready = 1;
}


void ssd1306_Send_Command(uint8_t command){
    uint8_t command_buf[2];
    command_buf[0] = COMMAND_BYTE;
    command_buf[1] = command;
    int result = i2c_write_blocking(I2C_PORT, OLED_ADD, command_buf, 2, false);
    if (result < sizeof(command_buf)){
        printf("Error occurs during Oled writing prcedure! \r\n");
    }
}

void ssd1306_Clear() {
    // make every bit a 0, memset in string.h
    memset(ssd1306_buf, 0, 513);
    // first byte is part of command 
    ssd1306_buf[0] = 0x40; 
    int confirm = i2c_write_blocking(I2C_PORT, OLED_ADD, ssd1306_buf, 513, false);
    if (confirm != 513){
        printf("Error occurs during Oled Clearance\r\n");
    }
}

void ssd1306_Setup(){
    sleep_ms(20);
    ssd1306_Send_Command(SSD1306_DISPLAYOFF);
    ssd1306_Send_Command(SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_Send_Command(0x80);
    ssd1306_Send_Command(SSD1306_SETMULTIPLEX);
    ssd1306_Send_Command(0x1F); 
    ssd1306_Send_Command(SSD1306_SETDISPLAYOFFSET);
    ssd1306_Send_Command(0x0);
    ssd1306_Send_Command(SSD1306_SETSTARTLINE);
    ssd1306_Send_Command(SSD1306_CHARGEPUMP);
    ssd1306_Send_Command(0x14);
    ssd1306_Send_Command(SSD1306_MEMORYMODE);
    ssd1306_Send_Command(0x00);
    ssd1306_Send_Command(SSD1306_SEGREMAP | 0x1);
    ssd1306_Send_Command(SSD1306_COMSCANDEC);
    ssd1306_Send_Command(SSD1306_SETCOMPINS);
    ssd1306_Send_Command(0x02);
    ssd1306_Send_Command(SSD1306_SETCONTRAST);
    ssd1306_Send_Command(0x8F);
    ssd1306_Send_Command(SSD1306_SETPRECHARGE);
    ssd1306_Send_Command(0xF1);
    ssd1306_Send_Command(SSD1306_SETVCOMDETECT);
    ssd1306_Send_Command(0x40);
    ssd1306_Send_Command(SSD1306_DISPLAYON);
}


void Set_Pixel(bool status, int x, int y){
    // The x,y augment should be in human-liked form
    int row_num = y - 1;
    int col_num = x - 1;
    int page = row_num / 8;
    int bit_pos = row_num % 8;

    // Every row means 128 bytes behind, and use bit shift to precisely find the bit
    if (page >= 0 && page <= 4 && col_num >= 0 && col_num <= 127) {
        int Byte_Index = page * 128 + col_num + 1;
        
        if (status) {
            ssd1306_buf[Byte_Index] |= (1 << bit_pos);    
        } else {
            ssd1306_buf[Byte_Index] &= ~(1 << bit_pos);     
        }
    }
}

void ssd1306_Update(){
    ssd1306_Send_Command(SSD1306_PAGEADDR);
    ssd1306_Send_Command(0);
    ssd1306_Send_Command(0xFF);
    ssd1306_Send_Command(SSD1306_COLUMNADDR);
    ssd1306_Send_Command(0);
    ssd1306_Send_Command(128 - 1); // Width
    int result = i2c_write_blocking(I2C_PORT, OLED_ADD, ssd1306_buf, 513, false);

    if (result < 513){
        printf("Error occurs during writing prcedure! \r\n");
    }
}

void Load_X_Line(int x1, int x2){
    int left, right;
    if (x1 > x2){
        left = x2, right = x1;
    } else {
        left = x1, right = x2;
    }
    for (int i = left; i <= right; i++){
        Set_Pixel(true,i,16);
    }
}

void Load_Y_Line(int y1, int y2){
    int top, bottom;
    if (y1 > y2){
        bottom = y1, top = y2;
    } else {
        bottom = y2, top = y1;}
    for (int i = top; i <= bottom; i++){
        Set_Pixel(true,64,i);
    }
}

void X_accel_Update(){
    int left, right;
    int gap = (int)(fabs(accel_x) / 1.5 * 63.0);
    if (accel_x > 0){
        right = 64;
        left = right - gap;
        Load_X_Line(left,right);
    } else {
        left = 64;
        right = left + gap;
        Load_X_Line(left,right);
    }
}

void Y_accel_Update(){
    int top, bottom;
    int gap = (int)(fabs(accel_y) / 1.3 * 15.0);
    if (accel_y > 0){
        top = 16;
        bottom = top + gap;
        Load_Y_Line(top,bottom);
    } else {
        bottom = 16;
        top = bottom - gap;
        Load_Y_Line(top,bottom);
    }
}