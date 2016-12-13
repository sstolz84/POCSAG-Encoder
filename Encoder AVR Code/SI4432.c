/*
 * SI4432.c
 *
 * Created: 24-9-2014 15:12:27
 *  Author: Jelmer Bruijn
 */ 

#include <avr/io.h>
#include <math.h>
#include <util/delay.h>

#include "SI4432.h"

volatile uint8_t spiret;

void SelectSI(){
	SPIPORT&=~(1<<SPI_SS);
}

void DeselectSI(){
	SPIPORT|=(1<<SPI_SS);
}

void WriteSI(uint8_t registerno, uint8_t val){
	SelectSI();
	SPDR=(0x80|registerno);
	while(!(SPSR & (1<<SPIF)));
	SPDR=val;
	while(!(SPSR & (1<<SPIF)));
	DeselectSI();
}

uint8_t ReadSI(uint8_t registerno){
	SelectSI();
	SPDR=registerno;
	while(!(SPSR & (1<<SPIF)));
	SPDR=0x00;
	while(!(SPSR & (1<<SPIF)));
	DeselectSI();
	return SPDR;
}

void SetupFreq(){
	float centre = xmitfreq;
	// Copy&Paste from RadioHead Library (RH_RF22.cpp)
	// Copyright (C) 2011 Mike McCauley
    uint8_t fbsel = 0x40;
    //if (centre < 240.0 || centre > 960.0) // 930.0 for early silicon
	//return false;
    if (centre >= 480.0){
		centre /= 2;
		fbsel |= 0x20;
    }
    centre /= 10.0;
    float integerPart = floor(centre);
    float fractionalPart = centre - integerPart;

    uint8_t fb = (uint8_t)integerPart - 24; // Range 0 to 23
    fbsel |= fb;
    uint16_t fc = fractionalPart * 64000;
    WriteSI(0x75, fbsel);
    WriteSI(0x76, fc >> 8);
    WriteSI(0x77, fc & 0xff);
    
	uint16_t offsettwo = 0;
	offsettwo+=offset;
	WriteSI(0x74,(uint8_t)((offsettwo>>8)&0xFF)); // Frequency Offset 2
	WriteSI(0x73,(uint8_t)((offsettwo)&0xFF));    // Frequency Offset 1
}

void SetupPower(){
	WriteSI(0x6D,(uint8_t)(xmitpower&0x07));      // TX Power
}

void SetupModule(){
	WriteSI(0x6E,0x09); // TX Data Rate 1
	WriteSI(0x6F,0xD5); // TX Data Rate 0
	WriteSI(0x70,0x24); // Modulation Mode Control 1
	WriteSI(0x71,0x0A); // Modulation Mode Control 2
	WriteSI(0x72,0x07); // Frequency Deviation
	WriteSI(0x0B,0x10); // GPIO0 Configuration
	WriteSI(0x0C,0x0F); // GPIO1 Configuration (set gpio1 to data clock out)
	WriteSI(0x0D,0x14); // GPIO2 Configuration (set gpio2 to data out)
}

void SIXmit(){
	WriteSI(0x07,0x0B);
}

void SIRX(){
	WriteSI(0x07,0x07);
}

void SIXmitStop(){
	WriteSI(0x07,0x01);
}
void SetupSPI(void){
	SPIPORT_DDR|=(1<<SPI_SS)|(1<<SPI_MOSI)|(1<<SPI_CLK);
	SPCR|=(1<<SPE)|(1<<MSTR);
	//DDRA|=(1<<PORTA7);
	GPIODDR|=(1<<GPIO_DATA_CLK)|(1<<GPIO_DATA_IN);
	DeselectSI();
	SetupModule();
}

void WaitForSignal(){
	uint32_t temp;
	uint16_t count;
	SIRX();
	while(1){
		for(count=0;count<1000;count++){
			temp+=ReadSI(0x26);
		}
		spiret = temp/1000;
		temp=0;
		if(spiret>34){
			spiret=0;
		}
	}
}