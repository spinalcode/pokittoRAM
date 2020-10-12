// https://os.mbed.com/users/davervw/code/SWSPI/

#define BYTEMODE 0
#define SEQMODE 64
#define PAGEMODE 128
#define DUALMODE 0x3B
#define QUADMODE 0x38

/*
   __ __
1-|  U  |-8
2-|     |-7
3-|     |-6
4-|_____|-5

1 - CS ------------------------ P1_5
2 - SIO1 - Slave Out (MISO) --- P1_21
3 - SIO2 - Slave Out 2 -------- P1_22
4 - VSS ----------------------- GND
5 - SIO0 - Slave In (MOSI) ---- P1_20
6 - SCK ----------------------- P1_6
7 - SIO3 - Slave Out 3 / Hold - P1_23
8 - VCC ----------------------- 3v3


Quad mode uses pins 5,2,3,7 for both in and output

Command	Function

0x01	Write MODE register
0x02	Write to memory address
0x03	Read from memory address
0x05	Read MODE register
0x38	Enter Quad I/O mode
0x3B	Enter Dual I/O mode
0xFF	Reset Dual and Quad mode (return to SPI?)

*/

//SPI spi(EXT10, EXT9, EXT8); // mosi, miso, sclk
SWSPI spi(P1_20, P1_21, P1_6);
DigitalOut SPI_CS(P1_5);


uint8_t readMode(){
    SPI_CS=0;
    spi.write(0x05);
    uint8_t currentMode = spi.write(0x00);
    SPI_CS=1;
    return currentMode;
}

void setProtocol(uint8_t prot){

    SPI_CS=0;
    spi.write(0x01);
    spi.write(prot);
    SPI_CS=1;
}

void setMode(uint8_t mode){
    switch(mode){
        case BYTEMODE:
        case PAGEMODE:
        case SEQMODE:
            SPI_CS=0;
            spi.write(0x01);
            spi.write(mode);
            SPI_CS=1;
            break;
    }
}

void writeToAddress(uint16_t address, const uint8_t* buffer, uint16_t number){
    SPI_CS=0;
    spi.write(0x02);
    uint8_t temp = address >> 8;
    spi.write(temp);
    temp = address & 255;
    spi.write(temp);
    for(int t=0; t<number; t++){
        spi.write(buffer[t]);
    }
    SPI_CS=1;
}

void readFromAddress(uint16_t address, uint8_t* buffer, uint16_t number){
    SPI_CS=0;
    spi.write(0x03);
    uint8_t temp = address >> 8;
    spi.write(temp);
    temp = address & 255;
    spi.write(temp);
    for(int t=0; t<number; t++){
        buffer[t] = spi.write(0x00); // sending dummy bytes will also read the MISO
    }
    SPI_CS=1;
}

void writeToAddressQuad(uint16_t address, const uint8_t* buffer, uint16_t number){
    SPI_CS=0;
    spi.writeQuad(0x02);
    uint8_t temp = address >> 8;
    spi.writeQuad(temp);
    temp = address & 255;
    spi.writeQuad(temp);
    for(int t=0; t<number; t++){
        spi.writeQuad(buffer[t]);
    }
    SPI_CS=1;
}

void readFromAddressQuad(uint16_t address, uint8_t* buffer, uint16_t number){
    SPI_CS=0;
    spi.writeQuad(0x03);
    uint8_t temp = address >> 8;
    spi.writeQuad(temp);
    temp = address & 255;
    spi.writeQuad(temp);
    spi.readQuad(); // read a dummy byte
    for(int t=0; t<number; t++){
        buffer[t] = spi.readQuad(); // sending dummy bytes will also read the MISO
    }
    SPI_CS=1;
}

