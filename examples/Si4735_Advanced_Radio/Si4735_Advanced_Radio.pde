/*
 * Si4735 Advanced Networked Attached Radio Sketch
 * Written by Jon Carrier
 * Based on Si4735_Example sketch code from Ryan Owens for Sparkfun Electronics
 *
 * HARDWARE SETUP:
 * This sketch assumes you are using the Si4735 Shield from SparkFun Electronics.  
 * The shields should be plugged into an Arduino Main Board (Uno, Duemillinove or similar).
 * The radio shield requires logic level conversion in order to communicate with 5V main boards
 *
 * ARDUINO PIN USAGE AND PURPOSE:
 * 0 -  Serial RX (used for remote control through USB)
 * 1 -  Serial TX (used to write to the LCD display)
 * 2 -  ROTARY Encoder A (Also initially acts as the INT_PIN, GPO2)
 * 3 -  ROTARY Encoder B
 * 4 -  
 * 5 -  ROTARY Push Button (Used to switch the local control mode for the rotary encoder)
 * 6 - 
 * 7 -  
 * 8 -  RADIO Power
 * 9 -  RADIO Reset
 * 10 - RADIO Slave Select
 * 11 - SPI MOSI
 * 12 - SPI MISO
 * 13 - SPI CLK
 *
 * USING THE SKETCHES SERIAL INTERFACE:
 * 8 - Increase the volume
 * 2 - Decrease the volume
 * 4 - Step down to the next frequency
 * 6 - Step up to the next frequency
 * - - Seek down to the next channel
 * + - Seek up to the next channel
 * m - Mute the radio
 * u - Unmute the radio
 * c - Show the callsign of the station
 * t - Show the time as reported by the station
 * s - scan the frequency band and report the SNR for each
 *
 * NOTES:
 * This sketch uses the Si4735 in FM mode. Other modes are AM, SW and LW. Check out the datasheet for more information on these
 * modes. All of the functions in the library will work regardless of which mode is being used; however the user must indicate
 * which mode is to be used in the begin() function. See the library documentation for more information.
 */

//===================DEFINE LIBRARIES==================
#include <SPI.h>
#include <Si4735.h>
#include <SerLCD.h>
#include <Rotary.h>
#include <Helper.h>
//===================Create the Object Instances==================
Si4735 radio;
Station tuned;
Rotary rot;
//===================DEFINE RADIO Related Parameters=================
#define EncA 3 //Encoder A
#define EncB 2 //Encoder B
#define PB 5 //Pushbutton

//This counter variable is used to refresh the LCD screen only once.
//This will occur when the user changes the state of the rotary encoder via the pushbutton
//or by rotating the rotary encoder. This is done so that the LCD is not constantly being
//written to. Constant writes to the LCD cause it to DIM, thus this will help increase the
//illumination/contrast of the display.
int refresh_cnt=0;

bool refresh_trigger=false;
bool refresh=true;
bool update=true;

//Define the state of the rotary encoder
//State 0: Rotary Encoder changes the volume
//State 1: Rotary Encoder changes the frequency (step mode)
//State 2: Rotary Encoder changes the frequency (seek mode)
int state=0;

//Define the user configurable settings
volatile byte volume=63; //Start at 100% Volume
//volatile byte volume=54; //Start at 85% Volume
volatile word frequency=10030; //Start at 100.3MHz

//RBDS INFO
bool ps_rdy;

//char ps[9]; //Program Station
char ps_prev[9]; //previous ps
//char radioText[65]; //Radio Text
//char pty[17]; //Program Type
char pty_prev[17]="                ";
byte mode=FM; //mode 0 is FM, mode 1 is AM

long lastUpdate; //Scrolling Refresh Parameter
byte radioText_pos; //Scrolling Position
//=========================END OF PARAMETERS==============================

//#####################################################################
//                              SETUP
//#####################################################################
void setup()
{
	//Create a serial connection
	Serial.begin(9600);
        //Create the Rotary Encoder connection
	rot.begin(EncA,EncB,PB,(*ROTATION));

	//Setup the LCD display and print the initial settings
	backlightOn();
	delay(500);
	goTo(0);
	Serial.print("-=ArduinoRadio=-");
	showFREQ();

        //Configure the radio
	radio.begin(mode);
        radio.setLocale(NA); //Use the North American PTY Lookup Table
        //radio.setLocale(EU); //Use the European PTY Lookup Table
	radio.tuneFrequency(frequency);
	volume=radio.setVolume(volume);

	lastUpdate = millis();
	radioText_pos = 0;
}

//#####################################################################
//                            LOOPING
//#####################################################################
void loop()
{       	     
        //Process the command from the serial connection
	remoteControl();

        //Update and store the RDS information
	ps_rdy=radio.readRDS(); 
	radio.getRDS(&tuned);     

        //If instructed to refresh the display, increment the counter
        if(refresh_trigger){ refresh_cnt++; }
	else refresh_cnt=0;

        //===================DISPLAY BEHAVIOR===================
        //-------------------LINE ONE BEHAVIOR------------------
        if(refresh_cnt>=1 && refresh_cnt<50 ){
        if(refresh_cnt==5 ){
        //For a period of time shortly after changing a parameter
        //Display the Program Type
        showPTY(true);
        }
        //Display the Signal quality information   
                //if(refresh_cnt%5==0){                          
                        //showRSQ();
                //}
        } 
        else { showPS(); }
        //-------------------LINE TWO BEHAVIOR------------------       
	if(refresh_cnt==5){                 
		if(state==2){    
			frequency=radio.getFrequency(refresh);
			refresh=!refresh;
                        showSEEK();                              
		}
	}        
	else if(refresh_cnt>=50 && refresh_cnt<150){      
		showPTY(false);
	}
	else if(refresh_cnt==150){refresh_trigger=false;}    

	showRadioText();
        //================END OF DISPLAY BEHAVIOR===================

        //================MODE SWITCHING BEHAVIOR===================
        //Check to see if the Pushbutton is pressed
	//If the PB is pressed change the state.	
	if(debounce(PB,25)){
                if(refresh_cnt>0 && refresh_cnt<=50){ //Works like a wake up function
		        state=(state+1)%3;//Cycle through state 0, 1, and 2
                }
		frequency=radio.getFrequency(refresh); //Helps with getting the correct frequency to be displayed
		refresh=true; //Give visual feedback to the user 
	}

	while(digitalRead(PB)){}//Deliberate halt in execution
	//This will prevent the code switching modes multiple times
	//During a slow push and release event
	//This does not protect against pushbutton bounce!
        //============END OF MODE SWITCHING BEHAVIOR================
}

//#####################################################################
//                      ROTARY CALLBACK FUNCTION
//#####################################################################

void ROTATION(void){  
	rot.read();
	update=true;//Indicate that the LCD needs to be updated   
	switch(state){
	case 0: //Increase and Decrease Tuned Frequency. FM_Range=[6400:10:10800], AM_Range=[149:1:23000]
		if(rot.isFwdRot()){
                        if(mode==FM){
			        if(frequency+10>10800){ frequency=6400; }
			        else{ frequency+=10; }
                        }
                        else{
                                if(frequency+1>23000){ frequency=149; }
			        else{ frequency+=1; }
                        }
		}
		if(rot.isRevRot()){
                        if(mode==FM){
			        if(frequency-10<6400){ frequency=10800; }
			        else{ frequency-=10; }
                        }
                        else{
                                if(frequency-1<149){ frequency=23000; }
			        else{ frequency-=1; }
                        }
		}    
		break;
	case 1: //Increase and Decrease Volume
		if(rot.isFwdRot())volume++;
		if(rot.isRevRot())volume--;      
		break;
        case 2: //Seek up and down
		if(rot.isFwdRot()) radio.seekUp();
		if(rot.isRevRot()) radio.seekDown();
                break;
        }
}

//#####################################################################
//                      LCD PRINTING FUNCTIONS
//#####################################################################
void showPTY(bool lineONE){
  if(!strcmp(tuned.programType,pty_prev,16)){
    if(lineONE){
          goTo(0);
    }
    else{
          goTo(16);
    }   
    Serial.print(tuned.programType);
    strcpy(pty_prev,tuned.programType);
  }  
}
//----------------------------------------------------------------------
void showCALLSIGN(){
  goTo(0);
  Serial.print("-=[   ");
  Serial.print(tuned.callSign);
  Serial.print("   ]=-");
}
//----------------------------------------------------------------------
void showTIME(){
  Today date;
  radio.getTime(&date); 
  goTo(0);
  Serial.print("-=[Time ");
  if(date.hour<10)
    Serial.print(0,DEC);
  Serial.print(date.hour,DEC);
  Serial.print(":");
  if(date.minute<10)
    Serial.print(0,DEC);
  Serial.print(date.minute,DEC);
  Serial.print("]=-");   
  goTo(16);
  Serial.print("   "); 
  
  if(date.month<10)
    Serial.print(0,DEC);
  Serial.print(date.month,DEC); 
  Serial.print("/");   
  
  if(date.day<10)
    Serial.print(0,DEC);
  Serial.print(date.day,DEC); 
  Serial.print("/");
  
  Serial.print("20");
  if(date.year<10)
    Serial.print(0,DEC);  
  Serial.print(date.year,DEC);  
  
  Serial.print("   "); 
  delay(500); 
}
//----------------------------------------------------------------------
void showRSQ(){ //Displays the Receive Signal Quality        
        Metrics RSQ;
        radio.getRSQ(&RSQ);
        clearLine1();                        
        Serial.print(" < ");
        Serial.print(RSQ.SNR,DEC);
        goTo(6); 
        Serial.print(":");
        Serial.print(RSQ.RSSI,DEC);
        goTo(10);
        Serial.print(":");
        Serial.print(RSQ.STBLEND,DEC);   
        goTo(14);
        Serial.print(">");
      
        clearLine2(); 
        Serial.print(" < ");
        Serial.print(RSQ.MULT,DEC);
        goTo(23); 
        Serial.print(": ");
        Serial.print(RSQ.FREQOFF,DEC);                        
        goTo(30);  
        Serial.print(">");
        delay(500);  
}
//----------------------------------------------------------------------
void showFREQ(){ //Displays the Freq information
        clearLine2();
        if(mode==FM){
	    Serial.print(" Freq: ");
	    Serial.print((frequency/100));
	    Serial.print(".");
	    Serial.print((frequency%100)/10);
	    Serial.print("MHz"); 
        }
        else{
            Serial.print(" Freq: ");
	    Serial.print((frequency));	    
	    Serial.print("kHz");  
        }
}
//----------------------------------------------------------------------
void showVOLUME(){ //Displays the Volume
        clearLine2();               
	Serial.print("  Volume: ");
	Serial.print(100*volume/63);
	Serial.print("%"); 
}
//----------------------------------------------------------------------
void showSEEK(){ //Displays the Seek information
        clearLine2();
        if(mode==FM){
	    Serial.print(" Seek: ");
	    Serial.print((frequency/100));
	    Serial.print(".");
	    Serial.print((frequency%100)/10);
	    Serial.print("MHz"); 
        }
        else{
            Serial.print(" Seek: ");
	    Serial.print((frequency));	    
	    Serial.print("kHz");  
        }
}
//----------------------------------------------------------------------
void showPS(){ //Displays the Program Service Information
        if (strlen(tuned.programService) == 8){
            if(ps_rdy){      
                if(!strcmp(tuned.programService,ps_prev,8)){         
      		        goTo(0);     
      		        Serial.print("-=[ ");
      		        Serial.print(tuned.programService); 
      		        Serial.print(" ]=-");     
                        strcpy(ps_prev,tuned.programService);
      		}  
            }    
      	}    
      	else {
                 //if(!strcmp("01234567",ps_prev,8)){ 
                        goTo(0); Serial.print("-=ArduinoRadio=-");
                 //       strcpy(ps_prev,"01234567");      
                 //}
        }
}
//----------------------------------------------------------------------
void showRadioText(){ //Displays the Radio Text Information
        if ((millis() - lastUpdate) > 500) {   
		if (strlen(tuned.radioText) == 64 & refresh_trigger==false) {
			//The refresh trigger cause the scrolling display to be delayed
			//this allows for the user to observe the new value they changed      
			goTo(16);
			if (radioText_pos < 64 - 16) {
				for (byte i=0; i<16; i++) { Serial.print(tuned.radioText[radioText_pos + i]); }
			} 
			else {
				byte nChars = 64 - radioText_pos;
				for (byte i=0; i<nChars; i++) { Serial.print(tuned.radioText[radioText_pos + i]); }
				for(byte i=0; i<(16 - nChars); i++) { Serial.print(tuned.radioText[i]); }
			}      
			radioText_pos++;
			if(radioText_pos >= 64) radioText_pos = 0;      
		}   
		lastUpdate = millis(); 
	}  
}
//#####################################################################
//                      REMOTE CONTROL FUNCTIONS
//#####################################################################

void remoteControl(){
 //Wait until a character comes in on the Serial port.
	if(Serial.available()>0){		
		//Decide what to do based on the character received.
		switch(Serial.read()){
		//If we get the number 8, turn the volume up.
		case '8':
			volume++;
			state=1;
			update=true;
			refresh=true;
			break;
			//If we get the number 2, turn the volume down.
		case '2':
			volume--;
			state=1;
			update=true;
			refresh=true;
			break;
			//If we get the number 4, seek down to the next channel in the current bandwidth (wrap to the top when the bottom is reached).
		case '4':
			if(mode==FM){
			        if(frequency-10<6400){ frequency=10800; }
			        else{ frequency-=10; }
                        }
                        else{
                                if(frequency-1<149){ frequency=23000; }
			        else{ frequency-=1; }
                        }
			state=0;
			update=true;
			refresh=true;
			break;
			//If we get the number 6, seek up to the next channel in the current bandwidth (wrap to the bottom when the top is reached).
		case '6':
			if(mode==FM){
			        if(frequency+10>10800){ frequency=6400; }
			        else{ frequency+=10; }
                        }
                        else{
                                if(frequency+1>23000){ frequency=149; }
			        else{ frequency+=1; }
                        }
			state=0;
			update=true;
			refresh=true;
			break;
                case '+': //Seek Up
			radio.seekUp();
			state=2;
			update=true;
			refresh=true;       
			break;
		case '-': //Seek Down
			radio.seekDown();
			state=2;
			update=true;
			refresh=true; 
			break;		
		case 'm': //Mute
                        radio.mute();
		        break;		
		case 'u': //Unmute
                        radio.unmute();
		        break;		
                case 'c': //Callsign
                        showCALLSIGN();
                        delay(500);
                        break;
                case 't': //Time
                        showTIME();
                        delay(500);                
                        break;
		case '~':
		case '`': //Switch mode			
			radio.end();
			clearLine2();
			Serial.print("SWITCH TO:");  
			if(mode==AM){ 
                                mode=FM;
                                Serial.print("FM");
                                frequency=10030;
                        }
			else{ 
                                mode=AM;
                                Serial.print("AM"); 
                                frequency=1270;
                        }  
			break;		
		case 'p': //Powerup
			radio.begin(mode);			
			break;
                case 's': //Sweep
			sweep();			
			break;
                case 'q': //Quality Check
                        showRSQ();
		default:
			break;
		}
	}   
	//Update the current settings
	if(update){
		refresh=true;//Refresh the LCD
		update=false; //The Si4735 should have been updated, turn off the flag
		switch(state){
		case 0://Increase and Decrease Tuned Frequency        
			radio.tuneFrequency(frequency);  
			break;
		case 1://Increase and Decrease Volume     
			volume=radio.setVolume(volume);     
			break;    
		case 2: //Seek UP and DOWN 
			//Seeking is done in the ROTATION function      
			break;
		}
	}
	//Display the current information
	if(refresh){   
		refresh_cnt=0;    
		switch(state){
		case 0://Display Tuned Frequency        
			showFREQ();   
			refresh_trigger=true; 
			break;
		case 1://Display Volume     
			showVOLUME();
			refresh_trigger=true;     
			break;    
		case 2: //Display Seek
			while(refresh){//Forces another refresh until frequency is valid
				frequency=radio.getFrequency(refresh);
				refresh=!refresh;    
				showSEEK();
			}   
			refresh_trigger=true;
			break;
		}
		//The display should have been updated. Turn the flag off
		refresh=false;
	}  
}

void sweep(){
  radio.mute();
  Metrics RSQ;
   Serial.print("SCAN_BEGIN:");
  for(word i=6400;i<=10800;i+=10){
     Serial.print(i,DEC);
     Serial.print(":");
     radio.tuneFrequency(i); 
     radio.getRSQ(&RSQ);
     Serial.print(RSQ.SNR,DEC);
     Serial.print(",");
  }  
  Serial.print(".");
  radio.unmute();
  radio.tuneFrequency(frequency);
}
