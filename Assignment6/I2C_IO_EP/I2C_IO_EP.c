#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// SCI PINS defined
#define SCL_PIN 5
#define SDA_PIN 4
#define I2C_PORT i2c0
// I/O Expander address defined
#define EP_DEVICE_ADDRESS 0b0100111
#define EP_IODIR_ADDRESS 0x00
#define EP_IOCON_ADDRESS 0x05
#define EP_GPIO_ADDRESS 0x09
#define EP_OLAT_ADDRESS 0x0A

// 8 bits int used to accept the data sent from the I/O Expander
static uint8_t buf;
// uint8_t array with 2 elements for I2C writing, with the [0] referring to register address
//                                                         [1] referring to data needs to write in
uint8_t I2C_array[2];

// Function used to initialize the pico default led gpio pin
int pico_led_init(void);
// Function used to control the led 
void pico_set_led(bool led_on);

// General function used to write the data into specific address from specific device
void WritePin(uint8_t advice, uint8_t register, uint8_t data);
// General function used to read the data from the specific address from specific device
uint8_t ReadPin(uint8_t advice, uint8_t register);

// Function used to initialize the I/O Expander, incluing (1) Set the mode to be Bite-Reading Mode
//                                                        (2) Set the GPIO 7 to be an output
//                                                        (3) Set the GPIO 7 low as the default status
void EP_init();


// Main function starts here
int main()
{
    // ============================== Initialization block ================================
    stdio_init_all();
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);

    EP_init();

    // uint8_t variable used to record the last voltage level, becasue we wire a pull-up resistor externally,
    // therefore it's idle high
    uint8_t previous_voltage = 0b1;

    // Reading and Writing loop
    while (true) {
        // Read from the I/O Expander
        buf = ReadPin(EP_DEVICE_ADDRESS, EP_GPIO_ADDRESS);
        // Get the last bit, representing the logic voltage level of GPIO 0, by bit shifting
        uint8_t current_voltage = buf & 0b1;
        // Compared with the previous voltage level
        if (current_voltage != previous_voltage){
            // debounce optimization
            sleep_ms(10);
            uint8_t confirm = ReadPin(EP_DEVICE_ADDRESS, EP_GPIO_ADDRESS) & 0b1;
            if (confirm == current_voltage){
                // If it does changed after the debounce procedure, that's a real change instead of oscillation
                // Then bit shifting to the GPIO 7.
                WritePin(EP_DEVICE_ADDRESS, EP_OLAT_ADDRESS, (previous_voltage << 7));
                // Update the privous voltage vaule
                previous_voltage = current_voltage;
            }
        }

        printf("The bool vaule of the GPIO 0 is now %d\r\n",current_voltage);
        // Heartbeating led for debugging
        pico_set_led(!gpio_get(PICO_DEFAULT_LED_PIN));
        sleep_ms(100);
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

void EP_init(){

    // Change the mode to Bite-Read Mode
    WritePin(EP_DEVICE_ADDRESS, EP_IOCON_ADDRESS, 0b00100000);

    // Change the GPIO 7 to be an output pin
    WritePin(EP_DEVICE_ADDRESS, EP_IODIR_ADDRESS, 0b01111111);

    // Set the default volatge at the GPIO 7 to be low
    WritePin(EP_DEVICE_ADDRESS, EP_OLAT_ADDRESS, 0b00000000);
}

void WritePin(uint8_t device_addr, uint8_t reg_addr, uint8_t data){
    uint8_t I2C_buf[2];
    I2C_buf[0] = reg_addr;
    I2C_buf[1] = data;

    int result = i2c_write_blocking(I2C_PORT, device_addr, I2C_buf, 2, false);
    // Make sure the ACK bit works as expected
    if (result < 0){
        printf("Error occurs druing Writing Communication!\r\n ");
    }
}

uint8_t ReadPin(uint8_t device_addr, uint8_t reg_addr){
    uint8_t I2C_buf;
    // variable used to accpet the exit status code of the i2c blocking function
    int result;

    result = i2c_write_blocking(I2C_PORT, device_addr, &reg_addr, 1, true);
    // Make sure the ACK bit works as expected
    if (result < 0){
        printf("Error occurs druing Writing Communication!\r\n ");
    }

    result = i2c_read_blocking(I2C_PORT, device_addr, &I2C_buf, 1, false);
    // Make sure the ACK bit works as expected
    if (result < 0){
        printf("Error occurs druing Reading Communication!\r\n ");
    }
    // Return the value sent from the device
    return I2C_buf;
}
