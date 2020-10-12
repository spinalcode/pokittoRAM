/* SWSPI, Software SPI library
 * Copyright (c) 2012-2014, David R. Van Wagner, http://techwithdave.blogspot.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <mbed.h>
#include <gpio_api.h>
#include <pinmap.h>
#include "SWSPI.h"


/*
   __ __
1-|  U  |-8
2-|     |-7
3-|     |-6
4-|_____|-5

1 - CS ------------------------ P1_5
2 - SIO1 - Slave Out (MISO?) -- P1_21 MISO
3 - SIO2 - Slave Out 2 -------- P1_22
4 - VSS ----------------------- GND
5 - SIO0 - Slave In (MOSI?) --- P1_20 MOSI
6 - SCK ----------------------- P1_19
7 - SIO3 - Slave Out 3 / Hold - P1_23
8 - VCC ----------------------- 3v3

*/

#define CSPIN 1<<5 // CS pin
#define SILK 1<<6 // clock pin
#define SIO0 1<<20 // Also MOSI
#define SIO1 1<<21 // Also MISO
#define SIO2 1<<22 // 
#define SIO3 1<<23 // 

#define SET_MOSI LPC_GPIO_PORT->SET[1] = SIO0
#define CLR_MOSI LPC_GPIO_PORT->CLR[1] = SIO0

#define GET_MISO (LPC_GPIO_PORT->PIN[1] & SIO1) >> 21

#define SET_CLOCK LPC_GPIO_PORT->SET[1] = SILK
#define CLR_CLOCK LPC_GPIO_PORT->CLR[1] = SILK
#define TOG_CLOCK LPC_GPIO_PORT->NOT[1] |= SILK;


#define SET_SIO0 LPC_GPIO_PORT->SET[1] |= SIO0
#define SET_SIO1 LPC_GPIO_PORT->SET[1] |= SIO1
#define SET_SIO2 LPC_GPIO_PORT->SET[1] |= SIO2
#define SET_SIO3 LPC_GPIO_PORT->SET[1] |= SIO3
#define CLR_SIO0 LPC_GPIO_PORT->CLR[1] |= SIO0
#define CLR_SIO1 LPC_GPIO_PORT->CLR[1] |= SIO1
#define CLR_SIO2 LPC_GPIO_PORT->CLR[1] |= SIO2
#define CLR_SIO3 LPC_GPIO_PORT->CLR[1] |= SIO3
#define GET_SIO0 (LPC_GPIO_PORT->PIN[1] & SIO0) >> 20
#define GET_SIO1 (LPC_GPIO_PORT->PIN[1] & SIO1) >> 21
#define GET_SIO2 (LPC_GPIO_PORT->PIN[1] & SIO2) >> 22
#define GET_SIO3 (LPC_GPIO_PORT->PIN[1] & SIO3) >> 23

SWSPI::SWSPI(PinName mosi_pin, PinName miso_pin, PinName sclk_pin)
{
    mosi = new DigitalOut(mosi_pin);
    miso = new DigitalIn(miso_pin);
    sclk = new DigitalOut(sclk_pin);
    format(8);
    frequency();
    
    // set pins to output
    LPC_GPIO_PORT->DIR[1] |= (1<<20);
    LPC_GPIO_PORT->DIR[1] |= (1<<21);
    LPC_GPIO_PORT->DIR[1] |= (1<<22);
    LPC_GPIO_PORT->DIR[1] |= (1<<23);

}

SWSPI::~SWSPI()
{
    delete mosi;
    delete miso;
    delete sclk;
}

void SWSPI::format(int bits, int mode)
{
    this->bits = bits;
    this->mode = mode;
    polarity = (mode >> 1) & 1;
    phase = mode & 1;
    sclk->write(polarity);
}

void SWSPI::frequency(int hz)
{
    this->freq = hz;
}


uint8_t SWSPI::readQuad()
{
    // set pins to input
    LPC_GPIO_PORT->DIR[1] &= ~(1<<20);
    LPC_GPIO_PORT->DIR[1] &= ~(1<<21);
    LPC_GPIO_PORT->DIR[1] &= ~(1<<22);
    LPC_GPIO_PORT->DIR[1] &= ~(1<<23);

    uint8_t reads[8];
    uint8_t read=0;

    TOG_CLOCK; // Toggle the clock pin

    reads[4] = GET_SIO0;
    reads[5] = GET_SIO1;
    reads[6] = GET_SIO2;
    reads[7] = GET_SIO3;

    wait(1.0/freq/2); // not even fast enough to need this

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

    TOG_CLOCK; // Toggle the clock pin

    reads[0] = GET_SIO0;
    reads[1] = GET_SIO1;
    reads[2] = GET_SIO2;
    reads[3] = GET_SIO3;

    wait(1.0/freq/2); // not even fast enough to need this

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

    read = (reads[7]<<7) | (reads[6]<<6) | (reads[5]<<5) | (reads[4]<<4) | (reads[3]<<3) | (reads[2]<<2) | (reads[1]<<1) | reads[0];
    return read;
}

void SWSPI::writeQuad(int value)
{
    // set pins to output
    LPC_GPIO_PORT->DIR[1] |= (1<<20);
    LPC_GPIO_PORT->DIR[1] |= (1<<21);
    LPC_GPIO_PORT->DIR[1] |= (1<<22);
    LPC_GPIO_PORT->DIR[1] |= (1<<23);

    uint8_t reads[8];
    uint8_t read=0;

    uint8_t bit[8];
    for (int b = 7; b >= 0; --b){
        bit[b] = (value >> b) & 0x01;
    }

    if( bit[4] ){ SET_SIO0; }else{ CLR_SIO0; }
    if( bit[5] ){ SET_SIO1; }else{ CLR_SIO1; }
    if( bit[6] ){ SET_SIO2; }else{ CLR_SIO2; }
    if( bit[7] ){ SET_SIO3; }else{ CLR_SIO3; }

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

    if( bit[0] ){ SET_SIO0; }else{ CLR_SIO0; }
    if( bit[1] ){ SET_SIO1; }else{ CLR_SIO1; }
    if( bit[2] ){ SET_SIO2; }else{ CLR_SIO2; }
    if( bit[3] ){ SET_SIO3; }else{ CLR_SIO3; }

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

    TOG_CLOCK; // Toggle the clock pin

    wait(1.0/freq/2); // not even fast enough to need this

}


int SWSPI::write(int value)
{

    uint8_t read = 0;
    for (int bit = bits-1; bit >= 0; --bit){

        if((value >> bit) & 0x01){ // Set MOSI Value
            SET_MOSI;
        }else{
            CLR_MOSI;
        }
        
        if (phase == 0){
            read |= (GET_MISO << bit); // Read MISO value
        }

        TOG_CLOCK; // Toggle the clock pin

        wait(1.0/freq/2); // not even fast enough to need this

        if (phase == 1){
            read |= (GET_MISO << bit); // Read MISO value
        }

        TOG_CLOCK; // Toggle the clock pin

        wait(1.0/freq/2); // not even fast enough to need this
    }

    return read;
}

