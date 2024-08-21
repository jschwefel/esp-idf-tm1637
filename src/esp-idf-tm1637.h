#pragma once

#include "esp-idf-tm1637.h"
#include "driver/gpio.h"


#define DEFAULT_BIT_DELAY       150
#define NUM_DISPLAY_ELEMENTS    4

#define LOW     0
#define HIGH    1

typedef struct tm1637data_t {
    gpio_num_t      clock;
    gpio_num_t      data;
    int8_t          brightness;
    // bool            colon;
    bool            on;
} tm1637data_t;




void delay_us(uint64_t us);
void tmWriteRawData(tm1637data_t setup, char data[]);
char encodeDigit(char digit);
void tmInit(tm1637data_t setup);
void tmEncodeDigits(tm1637data_t setup, int data, bool blankEmpty, bool colon);
void tmWriteTimeLeadZeroColon(tm1637data_t setup, int time);
void tmWriteTimeLeadZero(tm1637data_t setup, int time);
void tmWriteTimeLeadBlankColon(tm1637data_t setup, int time);
void tmWriteTimeLeadBlank(tm1637data_t setup, int time);
