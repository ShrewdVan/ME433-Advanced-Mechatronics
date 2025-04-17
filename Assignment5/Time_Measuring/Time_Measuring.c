#include <stdio.h>
#include "pico/stdlib.h"

#define CYCLE_TIME 6.667

volatile float f1, f2;
volatile float f_add, f_sub, f_multi, f_div;
float cycle_add, cycle_sub, cycle_multi, cycle_div;

/*
 It takes roughly 1 more clock cycle to each math manipulation if you test all four
 in one run than the scenario testing them one by one individually.

 Here's the result if you test each of them once at a time:
            Addtion takes around 7.499625 clock cycles.
            subtraction takes around 7.499625 clcok cycles.
            multiplication takes around 7.649618 clock cycles.
            division takes around 20.248987 clock cycles.
*/

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected()){
        ;
    }
    sleep_ms(10);
    printf("Starts\r\n");

    printf("Enter two floats to use:\r\n");
    scanf("%f %f", &f1, &f2);

    // Time consumption calculation blocking for addition manipulation
    absolute_time_t t_add_1 = get_absolute_time();
    for (int i=0; i < 1000; i++){
        f_add = f1 + f2;
    }
    absolute_time_t t_add_2 = get_absolute_time();

    uint64_t t_add = to_us_since_boot(t_add_2 - t_add_1);
    printf("Time for 1000 add is %llu ms\r\n", t_add);
    cycle_add = (float)t_add / CYCLE_TIME;
    printf("Cycle used for add is %f\r\n",cycle_add);

    sleep_ms(100);

    // Time consumption calculation blocking for subtraction manipulation
    absolute_time_t t_sub_1 = get_absolute_time();
    for (int i=0; i < 1000; i++){
        f_sub = f1 - f2;
    }
    absolute_time_t t_sub_2 = get_absolute_time();

    uint64_t t_sub = to_us_since_boot(t_sub_2 - t_sub_1);
    printf("Time for 1000 sub is %llu ms\r\n", t_sub);
    cycle_sub = (float)t_sub / CYCLE_TIME;
    printf("Cycle used for sub is %f\r\n",cycle_sub);

    sleep_ms(100);

    // Time consumption calculation blocking for multi manipulation
    absolute_time_t t_multi_1 = get_absolute_time();
    for (int i=0; i < 1000; i++){
        f_multi = f1 * f2;
    }
    absolute_time_t t_multi_2 = get_absolute_time();

    uint64_t t_multi = to_us_since_boot(t_multi_2 - t_multi_1);
    printf("Time for 1000 multi is %llu ms\r\n", t_multi);
    cycle_multi = (float)t_multi / CYCLE_TIME;
    printf("Cycle used for multi is %f\r\n",cycle_multi);

    sleep_ms(100);

    // Time consumption calculation blocking for div manipulation
    absolute_time_t t_div_1 = get_absolute_time();
    for (int i=0; i < 1000; i++){
        f_div = f1 / f2;
    }
    absolute_time_t t_div_2 = get_absolute_time();

    uint64_t t_div = to_us_since_boot(t_div_2 - t_div_1);
    printf("Time for 1000 div is %llu ms\r\n", t_div);
    cycle_div = (float)t_div / CYCLE_TIME;
    printf("Cycle used for div is %f\r\n",cycle_div);

}
