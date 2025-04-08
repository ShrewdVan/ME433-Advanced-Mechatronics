// The included library
#include <stdio.h>
#include "pico/stdlib.h"   
#include "hardware/gpio.h"

// The monitored pin is phisical pin29 (GPIO 22)
#define GPIO_WATCH_PIN 22

#ifndef DEBOUNCE_DELAY_MS
#define DEBOUNCE_DELAY_MS 30
#endif

// The variable "count" used to record the time button has been pressed
static int count = 0;
// The indicator for the LED 
static int LED_FLAG = 1; 

// Function used to initilize the led
int pico_led_init(void);
// Function used to set the status of led
void pico_set_led(bool led_on);
// Function used to toggle the led
void pico_toggle_led();

// Function called when interrupt occurs with debounced optimazation
void gpio_callback(uint gpio, uint32_t events) {
    sleep_ms(DEBOUNCE_DELAY_MS);
    if (gpio_get(22) == 0){
        count ++;
        pico_toggle_led();
        printf("Button has been pressed for %d times\n", count);
    } else{
        ;
    }
}

// Main function stars here
int main() {
    // Initialization
    stdio_init_all();
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    printf("Hello Toggle and Count IRQ\n");
    gpio_init(GPIO_WATCH_PIN);
    
    // Enable the interrupt
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Set default status to be "bright"
    pico_set_led(true);
    // Wait forever
    while (1);
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

// Toggle the led by checking the indicator of the led status
void pico_toggle_led(){
    if (LED_FLAG == 1){
        pico_set_led(false);
        LED_FLAG = 0;
    } else{
        pico_set_led(true);
        LED_FLAG = 1;
    }
}

