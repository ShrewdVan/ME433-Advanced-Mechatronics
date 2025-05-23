#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <math.h>
#include <stdlib.h>

// =========================== Define Blocking ========================
#define PWM_A 16
#define PWM_B 17
#define DIR_B 18
#define DIR_A 19

// =========================== Global Variable ========================
static int duty_cycle_A = 0, duty_cycle_B = 0, new_duty = 0;
static char command, wheel_select; 

// =========================== Function Blocking ========================

void pin_init_all();
void wheel_update();

// Main Function starts here
int main()
{

    stdio_init_all();
    pin_init_all();

    while (!stdio_usb_connected()){
        ;
    }
    sleep_ms(50);
    printf("Motor");

    // Infinite asking loop
    while (true) {

        // Asking which wheel to control
        printf("Select the wheel by sending 'L' or 'R' \r\n");
        scanf("%c", &wheel_select);
        // Asking whether acclerating or declerating
        printf("'+' for speed up and '-' for slow down\r\n");
        scanf("%c", &command);

        wheel_update();
        printf("duty cycle for A : %d, B: %d, duty: %d\r\n", duty_cycle_A, duty_cycle_B, new_duty);
        
        sleep_ms(50);
    }
}



void pin_init_all(){

    gpio_set_function(PWM_A, GPIO_FUNC_PWM);
    gpio_set_function(PWM_B, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(PWM_A);
    float div = 150.0;
    unsigned int wrap = 99;
    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, wrap);

    // Set the velocity to be 0 as default
    pwm_set_gpio_level(PWM_A, 0);
    pwm_set_gpio_level(PWM_B, 0);

    pwm_set_enabled(slice_num, true);

    gpio_init(DIR_A);
    gpio_init(DIR_B);
    gpio_set_dir(DIR_A, GPIO_OUT);
    gpio_set_dir(DIR_B, GPIO_OUT);

    gpio_put(DIR_A, 1);
    gpio_put(DIR_B, 1);

}

// Update the duty cycle and the phrase direction
void wheel_update(){
            if (wheel_select == 'L'){

            switch (command){

                case ('+'):
                    duty_cycle_A ++;
                    new_duty = abs((int)(duty_cycle_A * 99 / 100));
                    if (duty_cycle_A >= 0){
                        gpio_put(DIR_A, 1);
                        pwm_set_gpio_level(PWM_A, new_duty);
                    } else {
                        gpio_put(DIR_A, 0);
                        pwm_set_gpio_level(PWM_A, new_duty);
                    }

                    break;
                
                case ('-'):
                    duty_cycle_A --;
                    new_duty = abs((int)(duty_cycle_A * 99 / 100));
                    if (duty_cycle_A >= 0){
                        gpio_put(DIR_A, 1);
                        pwm_set_gpio_level(PWM_A, new_duty);
                    } else {
                        gpio_put(DIR_A, 0);
                        pwm_set_gpio_level(PWM_A, new_duty);
                    }

                    break;

                default:
                    printf("Wrong Command\r\n");

                    break;
            }

        } else if (wheel_select == 'R'){

            switch (command){

                case ('+'):
                    duty_cycle_B ++;
                    new_duty = abs((int)(duty_cycle_B * 99 / 100));
                    if (duty_cycle_B >= 0){
                        gpio_put(DIR_B, 1);
                        pwm_set_gpio_level(PWM_B, new_duty);
                    } else {
                        gpio_put(DIR_B, 0);
                        pwm_set_gpio_level(PWM_B, new_duty);
                    }

                    break;
                
                case ('-'):
                    duty_cycle_B --;
                    new_duty = abs((int)(duty_cycle_B * 99 / 100));
                    if (duty_cycle_B >= 0){
                        gpio_put(DIR_B, 1);
                        pwm_set_gpio_level(PWM_B, new_duty);
                    } else {
                        gpio_put(DIR_B, 0);
                        pwm_set_gpio_level(PWM_B, new_duty);
                    }

                    break;

                default:
                    printf("Wrong Command\r\n");

                    break;
            }

        } else{

            printf("Wrong command for wheel seletion\r\n");
        }
}