#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"
#include "hardware/adc.h"

// =========================== Define Blocking ========================
#define ADC_WATCH_PIN 0

#define SCL_PIN 5
#define SDA_PIN 4
#define I2C_PORT i2c0

#define DEVICE_ADD 0b0111100
#define COMMAND_BYTE 0x00

// =========================== Global Variable ========================

uint8_t ssd1306_buf[513];
unsigned int t_start;
unsigned int t_end;
unsigned int t_diff;
static float FPS;
uint16_t adc_count;
float v_count;
int col_num;


// =========================== Function Blocking ========================

// Function used to initialize the pico default led gpio pin
int pico_led_init(void);
// Function used to control the led 
void pico_set_led(bool led_on);
// Function used to send command to ssd1306
void ssd1306_Send_Command(uint8_t);
// Function using ssd1306_Send_Command to initialize the ssd1306
void ssd1306_Setup();
// Function used to clear all the pixels of the oled
void ssd1306_Clear();
// Function used to display a pure string, ex. "Hello World"
void Load_Const_Str(char *str, int, int);
// Function used to display the analog voltage in ADC count
void Load_adc_Str(int, int);
// Function used to display the analog voltage in real voltage 
void Load_voltage_Str(int, int);
// Function used to display the FPS of the oled screen
// ******** Because the refreshing loop happens in 1 Hz, plus the other instructions, FPS should be a little smaller than "1" **********
void Load_FPS_Str(int, int);
// Function used to push the ssd1306 buf into ssd1306 register
void ssd1306_Update();



// Main Function starts here
int main()
{
    // stdio and pico default led initialization
    stdio_init_all();
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    // ADC periphral initialization
    adc_init(); 
    adc_gpio_init(26); 
    adc_select_input(0); 

    // I2C periphral initialization
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);

    // ssd1306 initialization
    ssd1306_Setup();
    ssd1306_Clear();

    // Refreshing loop
    while (true) {
        
        // Measure the start time
        t_start = to_us_since_boot(get_absolute_time());  
        // Read the ADC count
        adc_count = adc_read();
        // Clear the previous content
        ssd1306_Clear();


        // Display the analog voltage in ADC count in the first colume, first row
        Load_adc_Str(1, 1);
        // Display the analog voltage in ADC count in the first colume, second row
        Load_voltage_Str(1, 2);
        // Display the freshing FPS in the middle position of the bottom row
        Load_FPS_Str(30, 4);
        // Push to the ssd1306 register
        ssd1306_Update();

        // Heartbeating led
        pico_set_led(!gpio_get(PICO_DEFAULT_LED_PIN));

        // 1 Hz corresponds to 1000ms per iteration
        sleep_ms(1000);

        // Measure the end time of the iteration
        t_end = to_us_since_boot(get_absolute_time());  
        // Compute the time passing by per iteration
        t_diff = t_end - t_start;
        // Compute the FPS, which is a static variable, and will be display since second iteration
        FPS = 1000000.0f / (float)t_diff;

    }
}






// =========================== Function Detailed Blocking ========================

int pico_led_init(){
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}   


void pico_set_led(bool led_on){
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}


void ssd1306_Send_Command(uint8_t command){
    uint8_t command_buf[2];
    command_buf[0] = COMMAND_BYTE;
    command_buf[1] = command;
    int result = i2c_write_blocking(I2C_PORT, DEVICE_ADD, command_buf, 2, false);
    if (result < sizeof(command_buf)){
        printf("Error occurs during writing prcedure! \r\n");
    }
}

// zero every pixel value
void ssd1306_Clear() {
    memset(ssd1306_buf, 0, 513); // make every bit a 0, memset in string.h
    ssd1306_buf[0] = 0x40; // first byte is part of command
    i2c_write_blocking(I2C_PORT, DEVICE_ADD, ssd1306_buf, 513, false);
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


void Load_Const_Str(char *str, int x, int y){
    // Set colume number equal to starting colume
    col_num = x;
    // Read every single char
    for (int i = 0; i < strlen(str); i++){
        // Boundary checking and row shifting
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Ranged exceeded");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        // Every row means 128 byte and every colume means 1 bytes
        int position = 128*(y-1) + col_num;
        // Every ASCII char needs 5 columes to display
        col_num +=5;
        // Match the current char to its 5 bytes displaying array and then copy to the ssd1306_buffer
        memcpy(&ssd1306_buf[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

void Load_adc_Str(int x, int y){
    char str[25];
    // Use sprintf to convert the string including adc count to pure string
    sprintf(str, "Analog voltage(ADC) %d", adc_count);
    // Set colume number equal to starting colume
    col_num = x;
    // Read every single char
    for (int i = 0; i < strlen(str); i++){
        // Boundary checking and row shifting
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Ranged exceeded");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        // Every row means 128 byte and every colume means 1 bytes 
        int position = 128*(y-1) + col_num;
        // Every ASCII char needs 5 columes to display
        col_num +=5;
        // Match the current char to its 5 bytes displaying array and then copy to the ssd1306_buffer
        memcpy(&ssd1306_buf[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

void Load_voltage_Str(int x, int y){
    char str[25];
    v_count = (float)adc_count/4096 * 3.3;
    // Use sprintf to convert the string including real voltage count to pure string    
    sprintf(str, "Analog voltage(V): %.2f", v_count);
    // Set colume number equal to starting colume
    col_num = x;
    // Read every single char
    for (int i = 0; i < strlen(str); i++){
        // Boundary checking and row shifting
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Ranged exceeded");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        // Every row means 128 byte and every colume means 1 bytes 
        int position = 128*(y-1) + col_num;
        // Every ASCII char needs 5 columes to display
        col_num += 5;
        // Match the current char to its 5 bytes displaying array and then copy to the ssd1306_buffer
        memcpy(&ssd1306_buf[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

void Load_FPS_Str(int x, int y){
    char str[25];
    // Use sprintf to convert the string including FPS to pure string 
    sprintf(str, "FPS: %.5f", FPS);
    // Set colume number equal to starting colume
    col_num = x;
    // Read every single char
    for (int i = 0; i < strlen(str); i++){
        // Boundary checking and row shifting
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Ranged exceeded");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        // Every row means 128 byte and every colume means 1 bytes 
        int position = 128*(y-1) + col_num;
        // Every ASCII char needs 5 columes to display
        col_num += 5;
        // Match the current char to its 5 bytes displaying array and then copy to the ssd1306_buffer
        memcpy(&ssd1306_buf[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

void ssd1306_Update(){
    ssd1306_Send_Command(SSD1306_PAGEADDR);
    ssd1306_Send_Command(0);
    ssd1306_Send_Command(0xFF);
    ssd1306_Send_Command(SSD1306_COLUMNADDR);
    ssd1306_Send_Command(0);
    ssd1306_Send_Command(128 - 1); // Width
    int result = i2c_write_blocking(I2C_PORT, DEVICE_ADD, ssd1306_buf, 513, false);

    if (result < 513){
        printf("Error occurs during writing prcedure! \r\n");
    }
}