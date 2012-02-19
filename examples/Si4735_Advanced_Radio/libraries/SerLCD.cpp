#include "SerLCD.h"
SerLCD::SerLCD(){}

void SerLCD::setBaud(int baud){	
	Serial.print(0x7C, BYTE);  
	switch(baud){
		case 2400:
			Serial.print(0x0B, BYTE);  
			break;
		case 4800:
			Serial.print(0x0C, BYTE);  
			break;	
		case 9600:
			Serial.print(0x0D, BYTE); 
			break;
		case 14400:
			Serial.print(0x0E, BYTE);  
			break;
		case 19200:
			Serial.print(0x0F, BYTE);  
			break;
		case 38400:
			Serial.print(0x10, BYTE);  
			break;
		default:
			break;
		}
}

void SerLCD::selectLine(int line){  //puts the cursor at line 1-4, char 0.
   Serial.print(0xFE, BYTE);   //command flag
	switch(line){
		case 1:
		default:
			Serial.print(128, BYTE);    //position
			break;
		case 2:
			Serial.print(192, BYTE);    //position
			break;
		case 3:
			Serial.print(144, BYTE);    //position
			break;
		case 4:
			Serial.print(208, BYTE);    //position
			break;
   }  
}


void SerLCD::goTo(int position) { 
	//Sets cursor to any visable position on the LCD
	//position = line 1: 0-15, line 2: 16-31, 
	//				 line 3: 32-47, line 4: 48-63, 
	//				 64+ defaults back to 0
	Serial.print(0xFE, BYTE);   //command flag	
	if (position>=16 && position<32){
		position=position+48;
	}
	else if (position>=32 && position<48){
		position=position-16;
	}
	else if (position>=48 && position<63){
		position=position+32;      
   }  
	Serial.print(position+128, BYTE);
}

void SerLCD::clear(){
   Serial.print(0xFE, BYTE);   //command flag
   Serial.print(0x01, BYTE);   //clear command.   
}

void SerLCD::backlight(bool state){  //turns on the backlight
	Serial.print(0x7C, BYTE);   //command flag for backlight stuff
	if(state)
		Serial.print(157, BYTE);    //light level.   
	else
		Serial.print(128, BYTE);     //light level for off.   
}

void SerLCD::serCommand(byte cmd){   //a general function to call the command flag for issuing all other commands   
  Serial.print(0xFE, BYTE);
  Serial.print(0xFE, cmd);
}

void SerLCD::clearLine(int line){
	byte pos=0;
	switch(line){
		case 1:
		default:
			break;
		case 2:
			pos=16;
			break;
		case 3:
			pos=32;
			break;
		case 4:
			pos=48;
			break;
	}
  goTo(pos);
  Serial.print("                ");
  goTo(pos);
}
