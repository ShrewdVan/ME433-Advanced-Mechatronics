#include <stdio.h>
#include "pico/stdlib.h"
#include "camera.h"
#include "motor.h"


void path_update();


int main()
{
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Hello, camera!\n");

    init_camera_pins();
    pin_init_all();
 
    char m[10];
    scanf("%s",m);
    
    while (true) {
        // uncomment these and printImage() when testing with python 
        //char m[10];
        //scanf("%s",m);

        setSaveImage(1);
        while(getSaveImage()==1){}
        convertImage();
        path_update(); // calculate the position of the center of the ine


        //setPixel(IMAGESIZEY/2,com,0,255,0); // draw the center so you can see it in python
        //printImage();
        //printf("%d\r\n",com); // comment this when testing with python
    }
}


void path_update(){
    int com = findLine(IMAGESIZEY/2);
    int error = com - 40;
    int new_duty = 0;

    if (error > 10){
        new_duty = 30-4*error;
        if (new_duty < 20){
            new_duty = 20;
        }
        wheel_update(1,1,new_duty);
        wheel_update(0,1,40);
    } 
    else if (error < 10){
        new_duty = 30+4*error;
        if (new_duty < 20){
            new_duty = 20;
        }
        wheel_update(1,1,40);
        wheel_update(0,1,new_duty);
    } else {
        wheel_update(1,1,30);
        wheel_update(0,1,30);
    }

    printf("com:%d, error: %d, duty:%d\r\n",com, error,new_duty);
}
