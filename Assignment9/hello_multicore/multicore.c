/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"

// =========================== Define Blocking ========================
#define FLAG_VALUE 123

// =========================== Global Variable ========================
uint16_t adc_count;
int command;

// =========================== Main Function for Core 1 ========================
void core1_entry() {

    uint32_t g;
    multicore_fifo_push_blocking(FLAG_VALUE);
    g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE){
        printf("Hmm, that's not right on core 1!❌\n");
    }else{
        printf("core 1 is ready ✅!\n");
    }

    // Infinite working loop
    while (true){
        // Read the command requested
        g = multicore_fifo_pop_blocking();
        switch (g){
            // command 0 -> Read the voltage at ADC0
            case 0:

                adc_count = adc_read();
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;

            // command 1 -> Turn on the led on the GPIO 15
            case 1:

                gpio_put(15, 1);
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;
            
            // command 2 -> Turn off the led on the GPIO 15
            case 2:

                gpio_put(15,0);
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;

            // Do nothing otherwise
            default:
                break;
        }

        tight_loop_contents();

    }
}





int main() {

    // stdio and initialization
    stdio_init_all();
    while (!stdio_usb_connected()){
        ;
    }
    sleep_ms(20);
    printf("Hello, multicore!\n");

    // ADC periphral initialization
    adc_init(); 
    adc_gpio_init(26); 
    adc_select_input(0); 

    // GPIO initialization
    gpio_init(15);
    gpio_set_dir(15, 1);
    gpio_put(15,0);

    uint32_t g;

    // Load the core 1 main function
    multicore_launch_core1(core1_entry);
    // Waiting for setting up
    sleep_ms(50);
    g = multicore_fifo_pop_blocking();
    multicore_fifo_push_blocking(FLAG_VALUE);

    if (g != FLAG_VALUE)
        printf("Hmm, that's not right on core 0!❌\n");
    else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("core 0 is ready ✅!\n");
    }

    // Infinite asking and send loop
    while (true){
        // Ask the user for the command requsted
        printf("Send the Command:");
        scanf("%d", &command);
        printf("\r\n");

        switch (command){
            case 0:

                // Send command to core1
                multicore_fifo_push_blocking(command);
                // Check whether it works fine
                g = multicore_fifo_pop_blocking();
                if (g != FLAG_VALUE){
                printf("Error occurs on core 0 accepeting ADC count!❌\n");
                }else {
                // Print the result back to the terminal
                printf("The current voltage measured by ADC0 is %f V.\n", (float)adc_count/4096.0*3.3);
                }
                break;
            
            case 1:

                // Send command to core1
                multicore_fifo_push_blocking(command);
                // Check whether it works fine
                g = multicore_fifo_pop_blocking();
                if (g != FLAG_VALUE){
                printf("Error occurs on core 0 turning on the LED!❌\n");
                }else {
                // Print the result back to the terminal
                printf("LED on GPIO 15 successfully turns on\n");
                }
                break;

            case 2:

                // Send command to core1
                multicore_fifo_push_blocking(command);
                // Check whether it works fine
                g = multicore_fifo_pop_blocking();
                if (g != FLAG_VALUE){
                printf("Error occurs on core 0 turning off the LED!\n");
                }else {
                // Print the result back to the terminal
                printf("LED on GPIO 15 successfully turns off\n");
                }
                break;
            
            default:

                // Send the error info to the terminal
                printf("No such command exists❌\n");
                break;
                
        }
    }

}
