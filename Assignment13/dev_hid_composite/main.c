#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdio.h"
#include <math.h>
#include "hardware/gpio.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */

// =========================== Define Struct ========================
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

// =========================== Global Variable ========================
static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

volatile static double accel_x = 0.0, accel_y = 0.0, accel_z = 0.0, gyro_x = 0.0, gyro_y = 0.0, gyro_z = 0.0, temp = 0.0;
uint8_t buf[14];
int Imu_Ready = 0;
int8_t volatile delta_x, delta_y;
volatile static uint8_t Click_Byte = 0;
volatile bool static left_button_pushed = 0, right_button_pushed = 0;


// =========================== Function Blocking ========================

// Led blinking to indicate the stutus of the usb 
void led_blinking_task(void);
// Usb hid sending function
void hid_task(void);
// Imu initialization function
void imu_init();
// Pin intialization
void mouse_pin_init_all();
// Interrupt callback Function
void gpio_callback_click(uint, uint32_t);
// Imu communication checking Function
int i2c_check();


// Main Function starts here
int main(void)
{
    stdio_init_all();
    board_init();

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    mouse_pin_init_all();

    imu_init();
    sleep_ms(200);

    // Interrupt Function enabled
    gpio_set_irq_enabled_with_callback(LEFT_CLICK, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback_click);
    gpio_set_irq_enabled_with_callback(RIGHT_CLICK, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback_click);
    
    uint8_t reg;
    while (1){
        reg = ACCEL_XOUT_H;
        int confirm = i2c_write_blocking(I2C_PORT, IMU_ADD, &reg, 1, true);
        if (confirm = 1){
            gpio_put(G_LED,1);  
        } 
        confirm = i2c_read_blocking(I2C_PORT, IMU_ADD, buf, 14, false);

        accel_x = (double)((int16_t)(buf[0] << 8 | buf[1])) * 0.000061;
        accel_y = (double)((int16_t)(buf[2] << 8 | buf[3])) * 0.000061;
        accel_z = (double)((int16_t)(buf[4] << 8 | buf[5])) * 0.000061;
        temp = (double)((int16_t)(buf[6] << 8 | buf[7])) / 340.00 + 36.53 ;
        gyro_x = (double)((int16_t)(buf[8] << 8 | buf[9])) * 0.007630;
        gyro_y = (double)((int16_t)(buf[10] << 8 | buf[11])) * 0.007630;
        gyro_z = (double)((int16_t)(buf[12] << 8 | buf[13])) * 0.007630;


        tud_task(); // tinyusb device task
        led_blinking_task();

        hid_task();
        }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      if ( btn )
      {
        uint8_t keycode[6] = { 0 };
        keycode[0] = HID_KEY_A;

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_keyboard_key = true;
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE:
    {
    // Click checking blocking. Use 2 buttons to simulate the left and right click    
        // Check whether the left button is pushed
        if (left_button_pushed == 1){
            // Debounce
            sleep_ms(5);
            if (gpio_get(LEFT_CLICK) == 0){
                // The bit 0 corresponds to the left click
                Click_Byte |= 1;
            }
        } else if (left_button_pushed == 0){
            sleep_ms(5);
            if (gpio_get(LEFT_CLICK)){
                Click_Byte &= ~1;
            }
        }

        // Check whether the right button is pushed
        if (right_button_pushed == 1){
            // Debounce
            sleep_ms(5);
            if (gpio_get(RIGHT_CLICK) == 0){
                // The bit 1 corresponds to the left click
                Click_Byte |= 2;
            }
        } else if (right_button_pushed == 0){
            sleep_ms(5);
            if (gpio_get(RIGHT_CLICK)){
                Click_Byte &= ~2;
            }
        }

        // Map the board tilt to the mouse moving
        delta_x = (int8_t)(-accel_x / 1.5 * 30);
        delta_y = (int8_t)(accel_y / 1.2 * 30); 
    

        // 2 button for right + left click, board tilt for mouse moving, no scroll, no pan
        tud_hid_mouse_report(REPORT_ID_MOUSE, Click_Byte, delta_x, delta_y, 0, 0);
    }
    break;

    case REPORT_ID_CONSUMER_CONTROL:
    {
      // use to avoid send multiple consecutive zero report
      static bool has_consumer_key = false;

      if ( btn )
      {
        // volume down
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        has_consumer_key = true;
      }else
      {
        // send empty key report (release key) if previously has key pressed
        uint16_t empty_key = 0;
        if (has_consumer_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;

    case REPORT_ID_GAMEPAD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_gamepad_key = false;

      hid_gamepad_report_t report =
      {
        .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
        .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = GAMEPAD_HAT_UP;
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
    }
    break;

    default: break;
  }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_MOUSE, btn);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1u;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        blink_interval_ms = 0;
        board_led_write(true);
      }else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

void mouse_pin_init_all(){
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);

    gpio_init(LEFT_CLICK);
    gpio_init(RIGHT_CLICK);

    gpio_set_dir(LEFT_CLICK, 0);
    gpio_set_dir(RIGHT_CLICK, 0);

    gpio_pull_up(LEFT_CLICK);
    gpio_pull_up(RIGHT_CLICK);

}


void gpio_callback_int(){
    Imu_Ready = 1;
}

void gpio_callback_click(uint gpio, uint32_t events){
    if (gpio == LEFT_CLICK){
        if (events == GPIO_IRQ_EDGE_FALL){
            left_button_pushed = 1;
        } else if (events == GPIO_IRQ_EDGE_RISE){
            left_button_pushed = 0;
        }
    } else if (gpio == RIGHT_CLICK){
        if (events == GPIO_IRQ_EDGE_FALL){
            right_button_pushed = 1;
        } else if (events == GPIO_IRQ_EDGE_RISE){
            right_button_pushed = 0;
        }
    }

}


void imu_init(){

    uint8_t command[2]; 
    // Resetting the entire device
    command[0] = PWR_MGMT_1;
    command[1] = 0b10000000;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    sleep_ms(100);
    // Turn on the chip
    command[0] = PWR_MGMT_1;
    command[1] = 0;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // Set the Gyro ranged +- 2000
    command[0] = GYRO_CONFIG;
    command[1] = 0b00011000;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);
    // Set the Acce ranged +- 2g
    command[0] = ACCEL_CONFIG;
    command[1] = 0;
    i2c_write_blocking(I2C_PORT, IMU_ADD, command, 2, false);

}

int i2c_check(){
    uint8_t buf[2];
    buf[0] = WHO_AM_I;
    int confirm = i2c_write_blocking(I2C_PORT, IMU_ADD, &buf[0], 1, true);
    if (confirm != 1){
        return 2;
    }
    confirm = i2c_read_blocking(I2C_PORT, IMU_ADD, &buf[1], 1, false);
    if (confirm != 1){
        return 3;
    }
    if (buf[1] == 0x68){
        return 1;
    } else{
        return 0;
    }
}