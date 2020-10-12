
#include <Pokitto.h>

#include "swspi.h"
//#include "qspi.cpp"

#include "globals.h"
#include "font.h"
#include "buttonhandling.h"
#include "ram.h"
#include "screen.h"

// print text
void myPrint(char x, char y, const char* text) {
  uint8_t numChars = strlen(text);
  uint8_t x1 = 0;//2+x+28*y;
  for (uint8_t t = 0; t < numChars; t++) {
    uint8_t character = text[t] - 32;
    Pokitto::Display::drawSprite(x+((x1++)*8), y, font88[character]);
  }
}

void setFPS(int fps){
  myDelay = 1000 / fps;
}

int lastLoad=0;


int main(){
    using PC=Pokitto::Core;
    using PD=Pokitto::Display;
    using PB=Pokitto::Buttons;
    using PS=Pokitto::Sound;

    PC::begin();
    PD::load565Palette(background_pal);
    PD::invisiblecolor = 0;
    PD::persistence = true;
    PD::adjustCharStep = 0;
    PD::adjustLineStep = 0;

    PD::lineFillers[0] = myBGFiller; // map layer

    updateButtons(); // update buttons
    while(_A[HELD]){
        updateButtons(); // update buttons
    }



    // Chip must be deselected
    SPI_CS = 1;
    // Setup the spi for 8 bit data, high steady state clock,
    spi.format(8, 0);

    //int spiFreq = 24000000; //RAM max speed is 20MHZ, but 24 seems to work
    int spiFreq   = 100; //RAM max speed is 20MHZ, but 24 seems to work
    spi.frequency(spiFreq);

    setMode(SEQMODE);
    setProtocol(QUADMODE);

    char tempText[10];
    //sprintf(tempText,"FPS:%d",fpsCount);
    //myPrint(0,0,"sending image");

  // send image data to ram chip currently from flash
    SPI_CS=1;
//    writeToAddressQuad(0, &background[2], 220*176);
    uint8_t tempBuff[5]={4,0,0,0,0};
    writeToAddressQuad(65535, tempBuff, 1);

    uint8_t temp=0;
    readFromAddressQuad(65535,&temp,1);

    SPI_CS=0;


    myPrint(0,8,"done");

    lastMillis = PC::getTime();

    while( PC::isRunning() ){
        
        updateButtons();
        if(!PC::update()) continue;

//        char tempText[10];
        sprintf(tempText,"FPS:%d",fpsCount);
        myPrint(0,12,tempText);
    sprintf(tempText,"Read:%d",temp);
    myPrint(0,0,tempText);

        fpsCounter++;
        frameCount++;

        if(PC::getTime() >= lastMillis+1000){
            lastMillis = PC::getTime();
            fpsCount = fpsCounter;
            fpsCounter = 0;
        }

    }
    
    return 0;
}
