#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "ov7670.h"

// =========================== Define Blocking ========================
#define D0 0
#define D1 1 
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define MCLK 10 
#define PCLK 11
#define HS 12
#define VS 13
#define SCL 21
#define SDA 20
#define RST 26

#define I2C_PORT i2c0

#define ROWMAX 60
#define COLMAX 80

// =========================== Global Variable ========================
uint8_t cameraData[ROWMAX * COLMAX * 2];

static volatile uint8_t Sampling = 0;
static volatile uint8_t ImageRequest= 0;
static volatile uint8_t row_allowed = 0;
static volatile uint8_t rowIndex = 0;
static volatile uint8_t colIndex = 0;
static volatile uint16_t DataIndex = 0;

// =========================== Define Struct ========================
typedef struct cameraImage{
    uint8_t index;
    uint8_t r[ROWMAX*COLMAX];
    uint8_t g[ROWMAX*COLMAX];
    uint8_t b[ROWMAX*COLMAX];
} cameraImage_t;
static volatile struct cameraImage picture;

// =========================== Function Blocking ========================
void PWM_init();
void I2C_init();
void camera_pin_init();
void gpio_callback(uint gpio, uint32_t events);
void init_camera();
void OV7670_write_register(uint8_t reg, uint8_t value);
uint8_t OV7670_read_register(uint8_t reg);
void StartSampling();
uint8_t WhetherSampling();
uint32_t GetRowNumber();
uint32_t GetPixelNumber();
void PrintImage();
void ConvertRGB();


// Main Function Starts here
int main()
{
    stdio_init_all();

    while (!stdio_usb_connected()){
        sleep_ms(20);
    }
    printf("Camera\n");

    camera_pin_init();
    PWM_init();
    I2C_init();
    printf("Start initializing the camera\n");
    init_camera();
    printf("Finish initializing the camera\n");


    gpio_set_irq_enabled_with_callback(PCLK, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(VS, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(HS, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    sleep_ms(1000);

    // Inifinite Sampling loop
    while (true) {
        // Wait here forever until the user asks to sample by sending something 
        char m[10];
        scanf("%s",m);

        StartSampling();
        // Wait until the Sampling process is down
    
        // NOTE: The flag here CANNOT be the flag trigged by the VS, the code would pass the wating before the 
        //       camera chaning the blocking flag, and then the flag changed by intterupt,
        //       indicating the image sampling procedure starts............. which would cause the
        //       senario that new data sent in while the code is processing the data............
        //       In that case,the first image would to be pure black and huge chaos in the further result. 
        //
        //       Kinda Tricky and Vague bug here
        //       You won't want to see that :) 
        //
        while(WhetherSampling() == 1){}

        // Convert the raw data
        ConvertRGB();
        // Send to PC for python for the furthing processing
        PrintImage();
    }
}


// =========================== Function Detailed Blocking ========================

// Pin initialization Function
void camera_pin_init(){
    // D0 to D7 represents the byte info of half pixel
    gpio_init(D0);
    gpio_set_dir(D0, 0);
    gpio_init(D1);
    gpio_set_dir(D1, 0);
    gpio_init(D2);
    gpio_set_dir(D2, 0);
    gpio_init(D3);
    gpio_set_dir(D3, 0);
    gpio_init(D4);
    gpio_set_dir(D4, 0);
    gpio_init(D5);
    gpio_set_dir(D5, 0);
    gpio_init(D6);
    gpio_set_dir(D6, 0);
    gpio_init(D7);
    gpio_set_dir(D7, 0);

    // Reset pin used for the soft reset at the beginning
    gpio_init(RST);
    gpio_set_dir(RST, 1);
    gpio_put(RST, 1);

    //************************ 
    //   3 Key Pulse Signal
    //************************

    // PCLK is the Byte pulse, flash twice for 1 pixel, each one represents a byte info
    gpio_init(PCLK);
    gpio_set_dir(PCLK, 0);
    // VS is the Frame Synch signal, it will temporailly be dragged low when a new image is starting and then back to high
    gpio_init(VS);
    gpio_set_dir(VS, 0);
    // VS is the Row Synch signal, it will be dragged high during a row and back to low at the end of the row
    gpio_init(HS);
    gpio_set_dir(HS, 0);

}


void PWM_init(){
    // Pico has a default clock with frequency of 150 MHz
    // Set a Pwm to be 150/2/(3+1) = 18 MHz
    gpio_set_function(MCLK, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(MCLK);
    float div = 2;
    pwm_set_clkdiv(slice_num, div);
    uint16_t wrap = 3;
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true);
    pwm_set_gpio_level(MCLK, wrap / 2);
    sleep_ms(1000);

}

void I2C_init(){
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    // If you have already connected with another device which has a build-in default pull-up resistor, delete the following two lines
    gpio_pull_up(I2C_SCL);
    gpio_pull_up(I2C_SDA);
}



void gpio_callback(uint gpio, uint32_t events){
    // If the trigged pin is VS, that's a frame Synch
    if (gpio == VS){
        // Make sure the flag is "1", which changed after the user send the 1
        if(ImageRequest == 1){
            // Initialize all the position and status
            rowIndex = 0;
            colIndex = 0;
            DataIndex = 0;
            Sampling = 1;
        }
    }

    // If the trigged pin is HS, that's a row Synch
    else if (gpio == HS){
        // If it's under the case the user asks for the sampling
        if (ImageRequest == 1){
            // If it's still within the resolution range we want
            if (Sampling == 1){
                // Reopen the flag if everything goes fine
                row_allowed = 1;
                rowIndex ++;

                // If the Index has go beyond the range, shut down the sampling procedure
                if (rowIndex == ROWMAX){
                    Sampling = 0;
                    ImageRequest = 0;
                    row_allowed = 0;
                    rowIndex = 0;
                }
            }
        }

    } 

    // If the trigged pin is VS, that's a byte Synch
    else if (gpio == PCLK) {
        // If it's under the case the user asks for the sampling
        if (ImageRequest == 1){
            // If it's still within the resolution range we want
            if (Sampling == 1){
                // If both the colume is also within the range
                if (row_allowed == 1 && colIndex < COLMAX*2){
                    // Get all the voltage level on the pin
                    uint32_t d = gpio_get_all();
                    // Get the first byte, because we wire the D0-D7 exactly the Gpio0- Gpio7
                    cameraData[DataIndex] = d & 0xFF;
                    // Move on and offset the Index
                    DataIndex ++;
                    colIndex ++;
                
                    // If the PCLK trigged twice the MAXCOL value, that means it already hits the ending boundary of the row 
                    if (colIndex == COLMAX * 2){
                        // Shut down the flag to close the sampling procedure for the rest of this row
                        row_allowed = 0;
                        colIndex = 0;
                    }

                    // If the PCLK trigged twice the resolution value, that's the end of this image.
                    if (DataIndex == ROWMAX * COLMAX * 2){
                        // Sampling down, shut down all the flags
                        Sampling = 0;
                        ImageRequest = 0;
                    }
                }
            }
        }
    }
}


// init the camera with RST and I2C commands
void init_camera(){
    // hardware reset the camera
    gpio_put(RST, 0);
    sleep_ms(1);
    gpio_put(RST, 1);
    sleep_ms(1000);

    OV7670_write_register(0x12, 0x80); // software reset
    sleep_ms(1000);

    // perform all the I2C writes for init
    // 25MHz * PLL / divisor = 24MHz for 30fps
    OV7670_write_register(OV7670_REG_CLKRC, 1); // div 1
    OV7670_write_register(OV7670_REG_DBLV, 0); // no pll

    // set colorspace to RGB565
    int i = 0;
    for(i=0; i<3; i++){
        OV7670_write_register(OV7670_rgb[i][0],OV7670_rgb[i][1]);
    }

    // init regular registers
    for(i=0; i<92; i++){
        OV7670_write_register(OV7670_init[i][0],OV7670_init[i][1]);
    }

    // init image size
    
    // Window settings were tediously determined empirically.
    // I hope there's a formula for this, if a do-over is needed.
    //{vstart,hstart,edge_offset,pclk_delay}
            //{9, 162, 2, 2},  // SIZE_DIV1  640x480 VGA
            //{10, 174, 4, 2}, // SIZE_DIV2  320x240 QVGA
            //{11, 186, 2, 2}, // SIZE_DIV4  160x120 QQVGA
            //{12, 210, 0, 2}, // SIZE_DIV8  80x60   ...
            //{15, 252, 3, 2}, // SIZE_DIV16 40x30

    uint8_t value;
    uint8_t size = OV7670_SIZE_DIV8; // 80x60
    uint16_t vstart = 12;
    uint16_t hstart = 210;
    uint16_t edge_offset = 0;
    uint16_t pclk_delay = 2;

    // Enable downsampling if sub-VGA, and zoom if 1:16 scale
    value = (size > OV7670_SIZE_DIV1) ? OV7670_COM3_DCWEN : 0;
    if (size == OV7670_SIZE_DIV16)
    value |= OV7670_COM3_SCALEEN;
    OV7670_write_register(OV7670_REG_COM3, value);

    // Enable PCLK division if sub-VGA 2,4,8,16 = 0x19,1A,1B,1C
    value = (size > OV7670_SIZE_DIV1) ? (0x18 + size) : 0;
    OV7670_write_register(OV7670_REG_COM14, value);

    // Horiz/vert downsample ratio, 1:8 max (H,V are always equal for now)
    value = (size <= OV7670_SIZE_DIV8) ? size : OV7670_SIZE_DIV8;
    OV7670_write_register(OV7670_REG_SCALING_DCWCTR, value * 0x11);

    // Pixel clock divider if sub-VGA
    value = (size > OV7670_SIZE_DIV1) ? (0xF0 + size) : 0x08;
    OV7670_write_register(OV7670_REG_SCALING_PCLK_DIV, value);

    // Apply 0.5 digital zoom at 1:16 size (others are downsample only)
    value = (size == OV7670_SIZE_DIV16) ? 0x40 : 0x20; // 0.5, 1.0
    // Read current SCALING_XSC and SCALING_YSC register values because
    // test pattern settings are also stored in those registers and we
    // don't want to corrupt anything there.

    uint8_t xsc = OV7670_read_register(OV7670_REG_SCALING_XSC);
    uint8_t ysc = OV7670_read_register(OV7670_REG_SCALING_YSC);

    xsc = (xsc & 0x80) | value; // Modify only scaling bits (not test pattern)
    ysc = (ysc & 0x80) | value;
    // Write modified result back to SCALING_XSC and SCALING_YSC
    OV7670_write_register(OV7670_REG_SCALING_XSC, xsc);
    OV7670_write_register(OV7670_REG_SCALING_YSC, ysc);

    // Window size is scattered across multiple registers.
    // Horiz/vert stops can be automatically calc'd from starts.
    uint16_t vstop = vstart + 480;
    uint16_t hstop = (hstart + 640) % 784;
    OV7670_write_register(OV7670_REG_HSTART, hstart >> 3);
    OV7670_write_register(OV7670_REG_HSTOP, hstop >> 3);
    OV7670_write_register(OV7670_REG_HREF,(edge_offset << 6) | ((hstop & 0b111) << 3) | (hstart & 0b111));
    OV7670_write_register(OV7670_REG_VSTART, vstart >> 2);
    OV7670_write_register(OV7670_REG_VSTOP, vstop >> 2);
    OV7670_write_register(OV7670_REG_VREF, ((vstop & 0b11) << 2) | (vstart & 0b11));
    OV7670_write_register(OV7670_REG_SCALING_PCLK_DELAY, pclk_delay);

    sleep_ms(300); // allow camera to settle with new settings 

    //OV7670_test_pattern(OV7670_TEST_PATTERN_COLOR_BAR);

    sleep_ms(300);

    uint8_t p = OV7670_read_register(OV7670_REG_PID);
    printf("pid = %d\n",p);

    uint8_t v = OV7670_read_register(OV7670_REG_VER);
    printf("ver = %d\n",v);
}


// I2C write to the camera
void OV7670_write_register(uint8_t reg, uint8_t value){
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, OV7670_ADDR, buf, 2, false);
    sleep_ms(1); // after each
}

// I2C read from the camera
uint8_t OV7670_read_register(uint8_t reg){
    uint8_t buf;
    i2c_write_blocking(I2C_PORT, OV7670_ADDR, &reg, 1, false);  // true to keep master control of bus
    i2c_read_blocking(I2C_PORT, OV7670_ADDR, &buf, 1, false);  // false - finished with bus
    return buf;
}

void StartSampling(){
    ImageRequest = 1;
}


uint8_t WhetherSampling(){
    return ImageRequest;
}

uint32_t GetRowNumber(){
    return rowIndex;
}

uint32_t GetPixelNumber(){
    return DataIndex;
}

void ConvertRGB(){
    int i = 0;
    for(i=0;i<COLMAX*ROWMAX;i++){
        uint8_t byte1 = cameraData[2*i];
        uint8_t byte2 = cameraData[2*i+1];
        
        picture.r[i] = byte1>>3;
        picture.g[i] = ((byte1 & 0b111)<<3) | (byte2>>5);
        picture.b[i] = byte2 & 0b11111;
        
    }
}


void PrintImage(){
    for (uint16_t i = 0; i < COLMAX * ROWMAX; i++){
        printf("%d %d %d %d\r\n",i, picture.r[i],picture.g[i],picture.b[i]);
        //sleep_ms(50);
    }
}


