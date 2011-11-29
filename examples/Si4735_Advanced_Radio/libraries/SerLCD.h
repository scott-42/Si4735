//=====================================================================
//                           LCD FUNCTIONS
//=====================================================================

//SerLCD Helper Functions
void selectLineOne(){  //puts the cursor at line 0 char 0.
   Serial.print(0xFE, BYTE);   //command flag
   Serial.print(128, BYTE);    //position  
}

void selectLineTwo(){  //puts the cursor at line 0 char 0.
   Serial.print(0xFE, BYTE);   //command flag
   Serial.print(192, BYTE);    //position   
}

void goTo(int position) { //position = line 1: 0-15, line 2: 16-31, 31+ defaults back to 0
    if (position<16){ 
      Serial.print(0xFE, BYTE);   //command flag
      Serial.print((position+128), BYTE);    //position
    }
    else if (position<32){
      Serial.print(0xFE, BYTE);   //command flag
      Serial.print((position+48+128), BYTE);    //position 
    }
    else { 
      goTo(0); 
    }   
}

void clearLCD(){
   Serial.print(0xFE, BYTE);   //command flag
   Serial.print(0x01, BYTE);   //clear command.   
}

void backlightOn(){  //turns on the backlight
    Serial.print(0x7C, BYTE);   //command flag for backlight stuff
    Serial.print(157, BYTE);    //light level.   
}

void backlightOff(){  //turns off the backlight
    Serial.print(0x7C, BYTE);   //command flag for backlight stuff
    Serial.print(128, BYTE);     //light level for off.   
}

void serCommand(){   //a general function to call the command flag for issuing all other commands   
  Serial.print(0xFE, BYTE);
}

void clearLine1(){
  goTo(0);
  Serial.print("                ");
  goTo(0);
}

void clearLine2(){
  goTo(16);
  Serial.print("                ");
  goTo(16);
}
