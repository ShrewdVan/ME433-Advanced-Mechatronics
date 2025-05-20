# ME433-Advanced-Mechatronics
This repository is set for the Advanced Mechatronics course, ME 433 in Northwestern Univerisity in 2025.

The Assignment 1 is focusing on the basic use of github.

The Assignment 2 is a entry level led project with periphrial interrupt service.

The Assignment 3 is a project combining usb CDC, GPIO, and ADC to perform a voltage sensor CUL.

The Assignment 4 is a project talking to the dac to generate a 2 Hz sin wave and a 1 Hz triangle wave simultaneously with spi as the communication protocol.

The Assignment 5 is a project using SPI as the protocol to communicate 2 devices, a Dac and a RAM. fill 1000 sample points into RAM first, then read from RAM and write to dac to create a sin wave.

The Assignment 6 is a project using I2C to communicate with a I/O Expander, read from one specific input pin and control the logic level of another output pin using SPI to overwrite its logic value in register.

The Assignment 7 is a project using I2C to control a oled screen to display the voltage information measured by an analog-to-digital converter.

The Assignment 8 is a project using PIO to send specific signal to control the ws2812 addressable led series, and PWM periphrial to control a servo motor.

The Assignment 9 is a tiny project related to the dual core cooperation to finish three small tasks: read the adc, turn on and turn off the specific led.

The Assignment 10 is to get fimiliar with the signal processing, low-pass filter for here. Use MAF, IIR and FIR to filter the data with a balence between smooth and noise elimination.

The Assignment 11 is to emulate the pico board to be a mouse. By sending the hid-mouse report to the pc, we can control the cursor by hitting the button, control its speed or acceleration, even change the mode of the cursor's movement.

The Assignment 12 is to get to know how a camera work, including row synch and frame synch with bunch of interrupt and flag control. Take the data from camera, send to pico, then send to PC and finally processed by python to see the image.

The Assignment 13 is to play with the Imu unit. There are 2 parts of the assignment, (1) Display the board movement by drawing the line to the direction it tilts (2) Combine the Imu unit with the Mouse, make a special mouse which can be controlled by tilting the board with 2 buttons acting as the left and right click button.

The content will be implemented as the course processes.
