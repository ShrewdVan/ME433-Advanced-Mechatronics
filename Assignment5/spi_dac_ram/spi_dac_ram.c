#include <stdio.h>
#include <math.h> 
#include "pico/stdlib.h"
#include "hardware/spi.h"

// SPI PIN section
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS_DAC 17
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_CS_RAM 20

// Data used to store the data for the Dac, the lenth is 2.
uint8_t data[2];

// Global variable used to consist the array with 7 elements, which is 
//    the format that RAM accpets. 

// 8 bits command 
uint8_t command;
// 16 bits address
static uint16_t address_16;
// two 8 bits int used to store the two parts of the address_16
uint8_t address_8[2];
// four 8 bits int used to represent the float when writing to RAM
uint8_t rx_buffer[4];
// four 8 bits int used to accpet the float when reading from RAM
uint8_t tx_buffer[4];

// union structure used to bit shift the float 
union FloatInt {
    float f;
    uint32_t i;
};

// 32 bits int used to disassemble into four 8 bits
union FloatInt number_in;
// 32 bits int used to accept the assembly of four 8 bits
union FloatInt number_out;
// 16 bits int used to send data to dac
uint16_t number_dac;
// The left most 8 bits of 16 bits number_dac_1
uint8_t number_dac_1;
// The right most 8 bits of 16 bits number_dac_1
uint8_t number_dac_2;


// Function used to manipulate the CSn pin
static inline void cs_select(int PIN_CS);
static inline void cs_deselect(int PIN_CS);

// Function used to Write to Dac
void WriteDac();
// Function used to send command, address, and four bytes data to RAM
void WriteRAM();
// Function used to send command, address, and recieve four bytes data from RAM
void ReadRAM();
// Function including updating calculation and format transformation for channel A
void Update_Data_Dac();
// Function used to send mode setting command to the RAM
void spi_ram_init();
// Function used to initilize the led
int pico_led_init(void);
// Function used to set the status of led
void pico_set_led(bool led_on);

// Main procedure starts here
int main(){
    
    // stdio and pico led initialization
    stdio_init_all();
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS_DAC,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_CS_RAM,   GPIO_FUNC_SIO);    
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Set two Csn pin to be high as default status
    gpio_set_dir(PIN_CS_DAC, GPIO_OUT);
    gpio_set_dir(PIN_CS_RAM, GPIO_OUT);
    gpio_put(PIN_CS_RAM, 1);
    gpio_put(PIN_CS_DAC, 1);

    // Send the command to set sequential mode
    spi_ram_init();

    // Check the port
    while (!stdio_usb_connected()){
        ;
    }
    sleep_ms(50);
    printf("Port connected!\r\n");
    
    // ******************** Writing loop *************************
    for (int i = 0; i < 1000; i++){

        address_8[0] = address_16 >> 8;
        address_8[1] = (address_16) & 0xFF;

        number_in.f = 1.65 + 1.65*sin(2*M_PI*i/1000); 
        tx_buffer[0] = number_in.i >> 24;
        tx_buffer[1] = (number_in.i >> 16) & 0xFF;
        tx_buffer[2] = (number_in.i >> 8) & 0xFF;
        tx_buffer[3] = number_in.i & 0xFF;

        WriteRAM();

        address_16 += 4;
    }

    // Turn on the led as a flag indicating "Finish writing procedure"
    pico_set_led(1);

    // Reset the address for the reading loop 
    address_16 = 0x00;
    // Counter for the reading loop
    int i = 0;

    // ****************** Reading loop (then write to Dac) **************** 
    while (true){
        if (i == 1000){
            i = 0;
            address_16 = 0x00;
        }
        address_8[0] = address_16 >> 8;
        address_8[1] = (address_16) & 0xFF;

        rx_buffer[0] = 0x00;
        rx_buffer[1] = 0x00;
        rx_buffer[2] = 0x00;
        rx_buffer[3] = 0x00;

        ReadRAM();

        number_out.i = ((uint32_t)rx_buffer[0] << 24) |
                    ((uint32_t)rx_buffer[1] << 16) |
                    ((uint32_t)rx_buffer[2] << 8)  |
                    ((uint32_t)rx_buffer[3]);
        
        Update_Data_Dac();
        WriteDac();

        address_16 += 4;
        i++;

        sleep_ms(1);
    }

}





// Function used to manipulate the CSn pin
static inline void cs_select(int PIN_CS) {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(int PIN_CS) {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);  // Active high
    asm volatile("nop \n nop \n nop");
}
void WriteDac(){
    cs_select(PIN_CS_DAC);
    spi_write_blocking(SPI_PORT, data, 2); 
    cs_deselect(PIN_CS_DAC);
}

void WriteRAM(){
    command = 0x02;
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, &command, 1);
    spi_write_blocking(SPI_PORT, address_8, 2);
    spi_write_blocking(SPI_PORT, tx_buffer, 4); 
    cs_deselect(PIN_CS_RAM);
}

void ReadRAM(){
    command = 0x03;
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, &command, 1);
    spi_write_blocking(SPI_PORT, address_8, 2);
    spi_read_blocking(SPI_PORT, 0x00, rx_buffer, 4); 
    cs_deselect(PIN_CS_RAM);
}

// Function used to calculate the sin wave and related data updating manipulation
void Update_Data_Dac(){
    number_dac = 1023 * (number_out.f / 3.3);
    printf("%d\r\n",number_dac);
    // Take the bits 10 - 7 from the result which is in a format of uint16_t, and set it to be the least 4 bits of the data[0]
    number_dac_1 = (number_dac >> 6) & 0x0F;
    // Take the bits 6 - 0 from the result which is in a format of uint16_t, and set it to be the most 6 bits of the data[1]
    number_dac_2 = (number_dac & 0x3F) << 2;
    data[0] = number_dac_1;
    // Edit the 4 most significant bits to meet the format of channel A
    data[0] |= (0b0111<<4);
    data[1] = number_dac_2;
}

void spi_ram_init(){

    uint8_t init_ram[2];
    init_ram[0] = 0b00000001;
    init_ram[1] = 0b01000000;
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, init_ram, 2); 
    cs_deselect(PIN_CS_RAM);

}

int pico_led_init(void) {
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}

// Turn the led on or off
void pico_set_led(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}