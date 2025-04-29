#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// =========================== Define Blocking ========================

#define IS_RGBW false
#define NUM_PIXELS 4

#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
#define WS2812_PIN 10
#endif

// Check the pin is compatible with the platform
#if WS2812_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

#define PWMPIN 16
#define BRIGHTNESS 0.15
#define SAT 1
#define LED_OFFSET 90


// =========================== Define Struct ========================

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} wsColor;


// =========================== Global Variable ========================

int loop_counter = 0;

uint16_t wrap = 60000;

wsColor Led1, Led2, Led3, Led4;
uint32_t Led1_32, Led2_32, Led3_32, Led4_32;
float hue_1, hue_2, hue_3, hue_4;

// todo get free sm
PIO pio;
uint sm;
uint offset;

// =========================== Function Blocking ========================

// Function using PIO to send the signal with specific format to the RGB Led 
static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);
// Function used to assemble the 3 uint8_t to a 24 bit array, and accept that using a uint32_t
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
// Function used to convert HSB, which is a easier way to represent the color, to the RGB format
wsColor HSBtoRGB(float, float, float);
// Function used to update the hue angle of the 4 leds respectively
void Hue_Update();
// Function used to update the uint32_t for the 4 leds and then send them to ws2812, with a 1 ms at the end to ensure the reset
void wsColor_Update();
// Function used to initialize the PWM periphrial to 50 Hz 
void PWM_init();
// Function used to update the new duty cycle to control the position of the servo motor
uint16_t PWM_Update();



// Main function starts here
int main() {
    //set_sys_clock_48();
    stdio_init_all();
    while (!stdio_usb_connected()){
        ;
    }
    sleep_ms(5);

    PWM_init();

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Infinite loop with a reset when the counter hits 360, indicating a new color wheel
    while (loop_counter < 361) {
        // Reset when it hits 360
        if (loop_counter == 360){
            loop_counter = 0;
        }

        wsColor_Update();
        uint16_t new_PWM = PWM_Update();
        pwm_set_gpio_level(PWMPIN, new_PWM);

        loop_counter ++;
        // 13 ms + 1 ms (inside the wsColor_Update) = 14 ms per iteration
        // 14 ms * 360 = 5040 ms, slightly beyond the 5 sec
        sleep_ms(13);
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}


// =========================== Function Detailed Blocking ========================

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return   
            ((uint32_t) (g) << 16) | 
            ((uint32_t) (r) << 8) |
            (uint32_t) (b) ;
}

wsColor HSBtoRGB(float hue, float sat, float brightness) {
    float red = 0.0;
    float green = 0.0;
    float blue = 0.0;

    if (sat == 0.0) {
        red = brightness;
        green = brightness;
        blue = brightness;
    } else {
        if (hue == 360.0) {
            hue = 0;
        }

        int slice = hue / 60.0;
        float hue_frac = (hue / 60.0) - slice;

        float aa = brightness * (1.0 - sat);
        float bb = brightness * (1.0 - sat * hue_frac);
        float cc = brightness * (1.0 - sat * (1.0 - hue_frac));

        switch (slice) {
            case 0:
                red = brightness;
                green = cc;
                blue = aa;
                break;
            case 1:
                red = bb;
                green = brightness;
                blue = aa;
                break;
            case 2:
                red = aa;
                green = brightness;
                blue = cc;
                break;
            case 3:
                red = aa;
                green = bb;
                blue = brightness;
                break;
            case 4:
                red = cc;
                green = aa;
                blue = brightness;
                break;
            case 5:
                red = brightness;
                green = aa;
                blue = bb;
                break;
            default:
                red = 0.0;
                green = 0.0;
                blue = 0.0;
                break;
        }
    }

    unsigned char ired = red * 255.0;
    unsigned char igreen = green * 255.0;
    unsigned char iblue = blue * 255.0;

    wsColor c;
    c.r = ired;
    c.g = igreen;
    c.b = iblue;
    return c;
}

void Hue_Update(){
    // The led 1 exactly equals to the counter with no offset
    hue_1 = loop_counter;
    // The led 2 has 1 offset, and use "%" to get the remaining part of the angle once it gets out of the bound
    hue_2 = (loop_counter + LED_OFFSET) % 360;
    // The led 3 has 2 offset, and use "%" to get the remaining part of the angle once it gets out of the bound
    hue_3 = (loop_counter + 2 * LED_OFFSET) % 360;
    // The led 4 has 3 offset, and use "%" to get the remaining part of the angle once it gets out of the bound
    hue_4 = (loop_counter + 3 * LED_OFFSET) % 360; 
}

void wsColor_Update(){
    // Call the Hue_Update to get the latest value of the hue
    Hue_Update();
    // Calculate the RGB format of the led 1 and accept the result using a wsColor struct
    Led1 = HSBtoRGB(hue_1, SAT, BRIGHTNESS);
    // Assemble the 3 uint8_t to a 24 bit array and accpet it using a uint32_t
    Led1_32 = urgb_u32(Led1.r, Led1.g, Led1.b);
    // Same as above
    Led2 = HSBtoRGB(hue_2, SAT, BRIGHTNESS);
    Led2_32 = urgb_u32(Led2.r, Led2.g, Led2.b);
    Led3 = HSBtoRGB(hue_3, SAT, BRIGHTNESS);
    Led3_32 = urgb_u32(Led3.r, Led3.g, Led3.b);
    Led4 = HSBtoRGB(hue_4, SAT, BRIGHTNESS);
    Led4_32 = urgb_u32(Led4.r, Led4.g, Led4.b);
    // Send all four info to the RGB led series sequentially
    put_pixel(pio, sm, Led1_32);
    put_pixel(pio, sm, Led2_32);
    put_pixel(pio, sm, Led3_32);
    put_pixel(pio, sm, Led4_32);
    // Set a 1-ms delay to ensure the reset 
    sleep_ms(1);
}

void PWM_init(){
    // System clock has a frequency of 150 MHz, to create a 50 Hz PWM, set divider = 50 and wrap = 60000
    // 150000000 / 50 / 60000 = 50 (Hz)
    gpio_set_function(PWMPIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWMPIN);
    float div = 50;
    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true);
    // Set the angle to be 0 as the starting position
    pwm_set_gpio_level(PWMPIN, 0);
}

uint16_t PWM_Update(){
    float fraction;
    if ((loop_counter < 180) || (loop_counter == 180)){
        // Forward half from 0 to 180
        fraction = ((float)loop_counter/180.0f) * 0.1 + 0.025;
    } else {
        // Backward half from 180 to 0
        fraction = -((float)(loop_counter % 180)/180.0f) * 0.1 + 0.125;
    }
    return (uint16_t)(fraction * wrap);
}
