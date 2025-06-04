#include "motor.h"

void pin_init_all(){

    gpio_set_function(PWM_RIGHT, GPIO_FUNC_PWM);
    gpio_set_function(PWM_LEFT, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(PWM_RIGHT);
    float div = 150.0;
    unsigned int wrap = 99;
    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, wrap);

    // Set the velocity to be 0 as default
    pwm_set_gpio_level(PWM_RIGHT, 0);
    pwm_set_gpio_level(PWM_LEFT, 0);

    pwm_set_enabled(slice_num, true);

    gpio_init(DIR_RIGHT);
    gpio_init(DIR_LEFT);
    gpio_set_dir(DIR_RIGHT, GPIO_OUT);
    gpio_set_dir(DIR_LEFT, GPIO_OUT);

    gpio_put(DIR_RIGHT, 1);
    gpio_put(DIR_LEFT, 1);

}

// Update the duty cycle and the phrase direction
void wheel_update(bool wheel, bool dir, int pwm){

    // 1 for right wheel and 0 for left wheel
    // 1 for moving forward and 0 for moving backwards
    // pwm should be the percentage of the speed, which is between 0 and 100
    if (wheel == 1){
        duty_cycle_right = (int)((float)pwm*0.8 + 19);

        gpio_put(DIR_RIGHT,dir);
        pwm_set_gpio_level(PWM_RIGHT, duty_cycle_right);
    } else if (wheel == 0){
        duty_cycle_left = (int)((float)pwm*0.8 + 19);

        gpio_put(DIR_LEFT,dir);
        pwm_set_gpio_level(PWM_LEFT, duty_cycle_left);
    }

    //printf("left:%d, right:%d\r\n",duty_cycle_left,duty_cycle_right);

}