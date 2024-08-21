#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp-idf-tm1637.h"
#include <rom/ets_sys.h>
#include "esp_timer.h"
#include "esp_log.h"

const uint8_t digitToSegment[] = {
 // XGFEDCBA
  0b00111111,    // 0
  0b00000110,    // 1
  0b01011011,    // 2
  0b01001111,    // 3
  0b01100110,    // 4
  0b01101101,    // 5
  0b01111101,    // 6
  0b00000111,    // 7
  0b01111111,    // 8
  0b01101111,    // 9
  0b01110111,    // A
  0b01111100,    // b
  0b00111001,    // C
  0b01011110,    // d
  0b01111001,    // E
  0b01110001     // F
  };

static const char* TAG = "esp-idf-tm1637";


void bitDelay();
uint8_t setBrightness(uint8_t brightness, bool on);
void start(tm1637data_t setup);
void stop(tm1637data_t setup);
uint8_t writeByte(tm1637data_t setup, uint8_t datax);

void bitDelay() {

    delay_us(DEFAULT_BIT_DELAY);
}


void delay_us(uint64_t us) {
    uint64_t microseconds = esp_timer_get_time();

    if (0 != us)
    {
        while (((uint64_t) esp_timer_get_time() - microseconds) <=
               us)
        {
            // Wait
        }
    }   
}


void tmWriteRawData(tm1637data_t setup, char data[]) {
    start(setup);
    writeByte(setup, 0x40); //40H address is automatically incremented by 1 mode,44H fixed address mode
    stop(setup);
    start(setup);
    writeByte(setup, 0xc0); // Set the first address
    for(uint8_t i=0;i<NUM_DISPLAY_ELEMENTS;i++) { 
        writeByte(setup, data[i]);
    }
    stop(setup);
    start(setup);
    writeByte(setup, setBrightness(setup.brightness, setup.on)); // Open display, maximum brightness
    stop(setup);
    delay_us(DEFAULT_BIT_DELAY * 5);
}


void tmInit(tm1637data_t setup) {
    ESP_LOGD(TAG, "Setting Pin %i as CLK\tSetting Pin %i as DIO", setup.clock, setup.data);
    gpio_config_t setupet = {
        .pin_bit_mask = (1ULL<<setup.clock | 1ULL<<setup.data),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&setupet);
    gpio_set_level(setup.clock, HIGH);
    gpio_set_level(setup.data, HIGH);

    delay_us(1000);
    // Turn it all on as a test
    char data[] = {0xff, 0xff, 0xff, 0xff};
    tmWriteRawData(setup, data);

}


uint8_t setBrightness(uint8_t brightness, bool on) {
	uint8_t m_brightness =  (brightness & 0x07) | (on? 0x88 : 0x00);
    return m_brightness;
}


void start(tm1637data_t setup) {
    gpio_set_level(setup.data, 1);
    bitDelay();
    gpio_set_level(setup.data, 0);
    bitDelay();
    // The below is spec, based on datasheet, but
    // will only work with it removed. ¯\_(ツ)_/¯ 
    // gpio_set_level(setup.data, 1);
    // bitDelay();
}


void stop(tm1637data_t setup) {
	gpio_set_level(setup.data, 0);
	bitDelay();
	gpio_set_level(setup.clock, 1);
	bitDelay();
    gpio_set_level(setup.data, 1);
	bitDelay();
}


uint8_t writeByte(tm1637data_t setup, uint8_t datax) {
    uint8_t data = datax;

    // 8 Data Bits
    for(uint8_t i = 0; i < 8; i++) {
        // CLK low
        gpio_set_level(setup.clock, 0);
        bitDelay();

        // Set data bit
        if (data & 0x01)
            gpio_set_level(setup.data, 1);
        else
            gpio_set_level(setup.data, 0);

        bitDelay();

        // CLK high
        gpio_set_level(setup.clock, 1);
        bitDelay();
        data = data >> 1;
    }

    // Wait for acknowledge
    // CLK to zero
    gpio_set_level(setup.clock, 0);
    // The below is spec, based on datasheet, but
    // will only work with it removed. ¯\_(ツ)_/¯ 
    // gpio_set_level(setup.data, 1);
    bitDelay();

    gpio_set_direction(setup.clock, GPIO_MODE_INPUT);
    bitDelay();
    uint8_t ack = gpio_get_level(setup.data);
    if (ack == 0) {
        gpio_set_direction(setup.clock, GPIO_MODE_OUTPUT);
    }
    
    gpio_set_level(setup.data, 0);

    bitDelay();
    gpio_set_level(setup.clock, 0);
    bitDelay();

    return ack;    
}


void tmEncodeDigits(tm1637data_t setup, int data, bool blankEmpty, bool colon) {

    char d1, d2, d3, d4;

    if (data != 0) {
        d4 = encodeDigit(data % 10) | (colon ? 0x80 : 0x00);
    } else {
        if (blankEmpty) {
            d4 = 0x00;
        } else {
            d4 = encodeDigit(0) | (colon ? 0x80 : 0x00);
        }
    }
    if (data > 9) {
        d3 = encodeDigit((data % 100) / 10) | (colon ? 0x80 : 0x00);
    } else {
        if (blankEmpty) {
            d3 = 0x00;
        } else {
            d3 = encodeDigit(0) | (colon ? 0x80 : 0x00);
        }
    }
    if (data > 99) {
        d2 = encodeDigit((data % 1000) / 100) | (colon ? 0x80 : 0x00);
    } else {
        if (blankEmpty) {
            d2 = 0x00;
        } else {
            d2 = encodeDigit(0) | (colon ? 0x80 : 0x00);
        }
    }
    if (data > 999) {
        d1 = encodeDigit((data % 10000) / 1000) | (colon ? 0x80 : 0x00);
    } else {
        if (blankEmpty) {
            d1 = 0x00;
        } else {
            d1 = encodeDigit(0) | (colon ? 0x80 : 0x00);
        }
    }
    tmWriteRawData(setup, (char[]){d1, d2, d3, d4}); 
}


void tmWriteTimeLeadZeroColon(tm1637data_t setup, int time) {
    tmEncodeDigits(setup, time, false, true);
}


void tmWriteTimeLeadZero(tm1637data_t setup, int time) {
    tmEncodeDigits(setup, time, false, false);
}


void tmWriteTimeLeadBlankColon(tm1637data_t setup, int time) {
    tmEncodeDigits(setup, time, true, true);
}


void tmWriteTimeLeadBlank(tm1637data_t setup, int time) {
    tmEncodeDigits(setup, time, true, false);
}


char encodeDigit(char digit)
{
	return digitToSegment[digit & 0x0f];
}