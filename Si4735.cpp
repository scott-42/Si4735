/* Arduino Si4735 Library
 * Written by Ryan Owens for SparkFun Electronics
 * 5/17/11
 * Altered by Wagner Sartori Junior <wsartori@gmail.com> on 09/13/11
 * Altered by Jon Carrier <jjcarrier@gmail.com> on 09/30/11
 *
 * This library is for use with the SparkFun Si4735 Shield
 * Released under the 'Buy Me a Beer' license
 * (If we ever meet, you buy me a beer)
 *
 * See the header file for better function documentation.
 *
 * See the example sketches to learn how to use the library in your code.
*/

#include "Si4735.h"
#include "WProgram.h"
#include "string.h"

//This is just a constructor.
Si4735::Si4735(){
	_mode 	= FM;
	_locale = NA;
	_volume = 63;
	_ab 	= 0;
	_hour 	= 0;
	_minute = 0;	
	clearRDS();
}

void Si4735::clearRDS(void) {
	byte i;
	for(i=0; i<64; i++)	_disp[i]	='\0';
	for(i=0; i<8; i++) 	_ps[i]		='\0';
	for(i=0; i<16; i++) 	_pty[i]		='\0';
	for(i=0; i<4; i++) 	_csign[i]	='\0';		
}

void Si4735::begin(char mode){
	_mode = mode;

	//Start by resetting the Si4735 and configuring the comm. protocol to SPI
	pinMode(POWER_PIN, OUTPUT);
	pinMode(RADIO_RESET_PIN, OUTPUT);
	pinMode(DATAIN, OUTPUT);  //Data In (GPO1) must be driven high after reset to select SPI
	pinMode(INT_PIN, OUTPUT);  //Int_Pin (GPO2) must be driven high after reset to select SPI

	//Sequence the power to the Si4735
	digitalWrite(RADIO_RESET_PIN, LOW);  
	digitalWrite(POWER_PIN, LOW);

	//Configure the device for SPI communication
	digitalWrite(DATAIN, HIGH);
	digitalWrite(INT_PIN, HIGH);
	delay(1);
	digitalWrite(POWER_PIN, HIGH);
	delay(100);
	digitalWrite(RADIO_RESET_PIN, HIGH);
	delay(100);

	//Now configure the I/O pins properly
	pinMode(DATAOUT, OUTPUT);
	pinMode(DATAIN, INPUT);
	pinMode(SPICLOCK,OUTPUT);
	pinMode(SS,OUTPUT); 
	pinMode(INT_PIN, INPUT); 
	digitalWrite(SS, HIGH);	

	//Configure the SPI hardware
	SPIClass::begin();
	SPIClass::setClockDivider(SPI_CLOCK_DIV32);
	
	//Send the POWER_UP command
	switch(_mode){
		case FM:
			sprintf(command, "%c%c%c", 0x01, 0x50, 0x05);
			break;
		case AM:
		case SW:
		case LW:
			sprintf(command, "%c%c%c", 0x01, 0x51, 0x05);
			break;
		default:
			return;
	}
	sendCommand(command, 3);
	delay(200);

	//Set the volume to the current value.
	sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x00, 0x00, _currentVolume);
	sendCommand(command, 6);
	delay(10);

	//Disable Mute
	sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x01, 0x00, 0x00);
	sendCommand(command, 6);
	delay(1);

	//Enable RDS
	//Only store good blocks and ones that have been corrected
	sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x15, 0x02, 0xAA, 0x01);
	//Only store good blocks
	//sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x15, 0x02, 0x00, 0x01);
	sendCommand(command, 6);
	delay(1);

	//Set the seek band for the desired mode (AM and FM can use default values)
	switch(_mode){
		case SW:
			//Set the lower band limit for Short Wave Radio to 2300 kHz
			sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x34, 0x00, 0x08, 0xFC);
			sendCommand(command, 6);
			delay(1);
			//Set the upper band limit for Short Wave Radio to 23000kHz
			sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x34, 0x01, 0x59, 0xD8);
			sendCommand(command, 6);			
			delay(1);
			break;
		case LW:
			//Set the lower band limit for Long Wave Radio to 152 kHz
			sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x34, 0x00, 0x00, 0x99);
			sendCommand(command, 6);
			//Set the upper band limit for Long Wave Radio to 279 kHz
			sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x34, 0x01, 0x01, 0x17);
			sendCommand(command, 6);			
			break;
		default:
			break;
	}
	
}
/*
* Description: Prepares myCommand to be sent over the SPI.
*/
void Si4735::sendCommand(char * myCommand){
	char tempValue=0;
	int index=0;
	//Convert the ascii string to a binary string
	while(*myCommand != '\0'){
		if(toupper(*myCommand) > '9')tempValue = toupper(*myCommand)-'A'+10;
		else tempValue = *myCommand - '0';
		command[index] = tempValue * 16;
		*myCommand++;
		
		if(toupper(*myCommand) > '9')tempValue = toupper(*myCommand)-'A'+10;
		else tempValue = *myCommand - '0';
		command[index++] += tempValue;
		*myCommand++;
	}
	//Now send the command to the radio
	sendCommand(command, index);
}

/*
* Description: Tunes the Si4735 to a frequency
*
* Params: frequency
*			If(mode==FM) The desired frequency is in tens of kHz
*			If(mode==AM) The desired frequency is in kHz
*/
void Si4735::tuneFrequency(word frequency){	
	//Split the desired frequency into two character for use in the
	//set frequency command.
	char highByte = frequency >> 8;
	char lowByte = frequency & 0x00FF;
	
	//Depending on the current mode, set the new frequency.
	switch(_mode){
		// page 21 of AN332
		case FM:
			sprintf(command, "%c%c%c%c%c", 0x20, 0x00, highByte, lowByte, 0x00);
			break;
		// page 135 of AN332
		case AM:
		case LW:
			sprintf(command, "%c%c%c%c%c%c", 0x40, 0x00, highByte, lowByte, 0x00, 0x00);
			break;
		case SW:
			sprintf(command, "%c%c%c%c%c%c", 0x40, 0x00, highByte, lowByte, 0x00, 0xff);
			break;
		default:
			break;
	}
	sendCommand(command, 4);
	delay(100);
	clearRDS();
}

word Si4735::getFrequency(bool &valid){
	char response [16];
	word frequency;	
	byte highByte;
	byte lowByte;

	switch(_mode){
		case FM:			
			//The FM_TUNE_STATUS command
			sprintf(command, "%c%c", 0x22, 0x00);
			break;
		case AM:
		case SW:
		case LW:
			//The AM_TUNE_STATUS command
			sprintf(command, "%c%c", 0x42, 0x00);			
			break;
		default:
			break;
	}	
	
	//Send the command
	sendCommand(command, 2);

	//Now read the response	
	getResponse(response);	

	//Convert the bytes of the response to a frequency value	
	highByte=(((response[2]>>7)/(-1))<<7)+(response[2]&127);
	lowByte=(((response[3]>>7)/(-1))<<7)+(response[3]&127);
	frequency = (highByte<<8)+lowByte;

	//Check to see if the Si4735 is currently "busy"	
	valid=(response[0]&1)==1;

	return frequency;
}

void Si4735::seekUp(void){
	//Use the current mode selection to seek up.
	switch(_mode){
		case FM:
			sprintf(command, "%c%c", 0x21, 0x0C);
			sendCommand(command, 2);
			break;
		case AM:
		case SW:
		case LW:
			sprintf(command, "%c%c%c%c%c%c", 0x41, 0x0C, 0x00, 0x00, 0x00, 0x00);
			sendCommand(command, 6);
			break;
		default:
			break;
	}
	delay(1);
	clearRDS();
}

void Si4735::seekDown(void){
	//Use the current mode selection to seek down.
	switch(_mode){
		case FM:
			sprintf(command, "%c%c", 0x21, 0x04);
			sendCommand(command, 2);
			break;
		case AM:
		case SW:
		case LW:
			sprintf(command, "%c%c%c%c%c%c", 0x41, 0x04, 0x00, 0x00, 0x00, 0x00);
			sendCommand(command, 6);
			break;
		default:
			break;
	}	
	delay(1);
	clearRDS();
}

bool Si4735::readRDS(void){
	char status;
	char response [16];
	bool ps_rdy=false;
	
	sprintf(command, "%c%c", 0x24, 0x00);
	sendCommand(command, 2);

	//Now read the response	
	getResponse(response);
	
	//response[4] = RDSA high BLOCK1
	//response[5] = RDSA low
	//response[6] = RDSB high BLOCK2
	//response[7] = RDSB low
	//response[8] = RDSC high BLOCK3
	//response[9] = RDSC low
	//response[10] = RDSD high BLOCK4
	//response[11] = RDSD low	
	byte pty = ((response[6]&3) << 3) | ((response[7] >> 5)&7);
	ptystr(pty);	
	
	int pi;	
	byte type = (response[6]>>4) & 0xF;
	bool version = bitRead(response[6], 4);
	bool tp = bitRead(response[6], 5);
	if (version == 0) {
		pi = MAKEINT(response[4], response[5]);
	} else {
		pi = MAKEINT(response[8], response[9]);
	}
	if(pi>=21672){
 		_csign[0]='W';
		pi-=21672;
	}
	else if(pi<21672 && pi>=4096){
		_csign[0]='K';
		pi-=4096;
	}
	else{
		pi=-1;
	}
	if(pi>=0){ //Convert the PI to the Callsign
		_csign[1]=char(pi/676+65);
		_csign[2]=char((pi - 676*int(pi/676))/26+65);
		_csign[3]=char(((pi - 676*int(pi/676))%26)+65);		
		_csign[4]='\0';		
	}
	else{ //Unknown/Unsupported Callsign
		_csign[0]='U';
		_csign[1]='N';
		_csign[2]='K';
		_csign[3]='N';
		_csign[4]='\0';
	}
	
	/*
	Serial.println("================================");
	Serial.print("Type:|");
	Serial.print(type, DEC);
	Serial.println("|");
	Serial.print("Version:|");
	Serial.print(version, BIN);
	Serial.println("|");
	Serial.print("TP:|");
	Serial.print(tp, BIN);
	Serial.println("|");
	Serial.print("PI:|");
	Serial.print(pi, BIN);
	Serial.println("|");
	*/
	
	// Groups 0A & 0B
	// Basic tuning and switching information only
	if (type == 0) {
		bool ta = bitRead(response[7], 4);
		bool ms = bitRead(response[7], 3);
		byte addr = response[7] & 3;
		bool diInfo = bitRead(response[7], 2);
		
		// Groups 0A & 0B: to extract PS segment we need blocks 1 and 3
		if (addr >= 0 && addr<= 3) {
			if (response[10] != '\0')
				_ps[addr*2] = response[10];
			if (response[11] != '\0')
				_ps[addr*2+1] = response[11];
			ps_rdy=(addr==3);
		}
		
		/*
		Serial.print("TA:|");
		Serial.print(ta, BIN);
		Serial.println("|");
		Serial.print("MS:|");
		Serial.print(ms, BIN);
		Serial.println("|");
		Serial.print("Addr:|");
		Serial.print(addr, DEC);
		Serial.println("|");
		Serial.print("diInfo:|");
		Serial.print(diInfo, BIN);
		Serial.println("|");
		Serial.print("_ps:|");
		Serial.print(_ps);
		Serial.println("|");
		Serial.print("_af:|");
		Serial.print(_af, DEC);
		Serial.println("|");
		*/
	}
	// Groups 2A & 2B
	else if (type == 2) {
		// Get their address
		int addressRT = response[7] & B1111; // Get rightmost 4 bits
		bool ab = bitRead(response[7], 4);
		
		if (version == 0) {
			if (addressRT >= 0 && addressRT <= 15) {
				if (response[8] != '\0')
					_disp[addressRT*4] = response[8];
				if (response[9] != '\0')
					_disp[addressRT*4+1] = response[9];
				if (response[10] != '\0')
					_disp[addressRT*4+2] = response[10]; 
				if (response[11] != '\0')
				_disp[addressRT*4+3] = response[11];
			}
		} else {
			if (addressRT >= 0 && addressRT <= 7) {
				if (response[10] != '\0')
					_disp[addressRT*2] = response[10];
				if (response[11] != '\0')
					_disp[addressRT*2+1] = response[11];
			}
		}
		if (ab != _ab) {
			for (byte i=0; i<64; i++) _disp[i] = ' ';
			_disp[64] = '\0';
		}
		_ab = ab;
		//Serial.println(_disp);
	}
	
	delay(40);
	//This is a simple way to indicate when the ps data has been fully refreshed.
	return ps_rdy; 
}

void Si4735::getRDS(Station * tunedStation) {
	strcpy(tunedStation->programService, _ps);
	strcpy(tunedStation->radioText, _disp);	
	strcpy(tunedStation->programType, _pty);
	strcpy(tunedStation->callSign, _csign);		
}

void Si4735::getRSQ(Metrics * RSQ){
	//This function gets the Received Signal Quality Information
	char response [16];
	
	switch(_mode){
		case FM:			
			//The FM_RSQ_STATUS command
			sprintf(command, "%c%c", 0x23, 0x00);
			break;
		case AM:
		case SW:
		case LW:
			//The AM_RSQ_STATUS command
			sprintf(command, "%c%c", 0x43, 0x00);			
			break;
		default:
			break;
	}	
	
	//Send the command
	sendCommand(command, 2);

	//Now read the response	
	getResponse(response);	

	//Pull the response data into their respecive fields
	RSQ->RSSI=response[4];
	RSQ->SNR=response[5];

	if(_mode==FM){
		RSQ->STBLEND=response[3]&63;
		RSQ->MULT=response[6];
		RSQ->FREQOFF=response[7];
	}
	else{
		RSQ->STBLEND=0;
		RSQ->MULT=0;
		RSQ->FREQOFF=0;
	}
}

byte Si4735::volumeUp(void){
	//If we're not at the maximum volume yet, increase the volume
	if(_volume < 63){
		_volume+=1;
		//Set the volume to the current value.
		sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x00, 0x00, _volume);
		sendCommand(command, 6);
		delay(10);		
	}
	return _volume;
}

byte Si4735::volumeDown(void){
	//If we're not at the minimum volume yet, decrease the volume
	if(_volume > 0){
		_volume-=1;
		//Set the volume to the current value.
		sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x00, 0x00, _volume);
		sendCommand(command, 6);
		delay(10);		
	}
	return _volume;
}

byte Si4735::setVolume(byte value){
	if(value <= 63 && value >= 0){
		_volume=value;
		//Set the volume to the current value.
		sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x00, 0x00, _volume);
		sendCommand(command, 6);
		delay(10);		
	}
	return _volume;
}

byte Si4735::getVolume() {
	//Read the true value for volume from the Si4735
	char response [16];	
	sprintf(command, "%c%c%c%c", 0x13, 0x00, 0x40, 0x00);
	sendCommand(command, 4);	
	getResponse(response);
	_volume = response[3];
	return _volume; 
}

void Si4735::mute(void){
	//Disable Mute
	sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x01, 0x00, 0x03);
	sendCommand(command, 6);
	delay(1);
}

void Si4735::unmute(void){
	//Disable Mute
	sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, 0x40, 0x01, 0x00, 0x00);
	sendCommand(command, 6);
	delay(1);
}

char Si4735::getStatus(void){
	char response;
	digitalWrite(SS, LOW);
	delay(1);
	spiTransfer(0xA0);  //Set up to read a single byte
	delay(1);
	response = spiTransfer(0x00);  //Get the commands response
	digitalWrite(SS, HIGH);
	return response;
}

void Si4735::getResponse(char * response){
	digitalWrite(SS, LOW);
	delay(1);
	spiTransfer(0xE0);  //Set up to read the long response
	delay(1);
	for(int i=0; i<16; i++) {
		*response++ = spiTransfer(0x00);  //Assign the response to the string.
	}
	digitalWrite(SS, HIGH);
}

void Si4735::end(void){
	sprintf(command, "%c", 0x11);
	sendCommand(command, 1);
	delay(1);
}

void Si4735::setLocale(byte locale){
	_locale=locale;	
}
byte Si4735::getLocale(void){
	return _locale;
}
void Si4735::setMode(char mode){
	end();
	begin(mode);
}
char Si4735::getMode(void){
	return _mode;
}

/*******************************************
*
* Private Functions
*
*******************************************/
char Si4735::spiTransfer(char value){
	SPDR = value;                    // Start the transmission
	while (!(SPSR & (1<<SPIF)))     // Wait for the end of the transmission
	{
	};
	return SPDR;                    // return the received byte
	//return SPIClass::transfer(value);
}

void Si4735::sendCommand(char * command, int length){
  digitalWrite(SS, LOW);
  delay(1);
  spiTransfer(0x48);  //Contrl byte to write an SPI command (now send 8 bytes)
  for(int i=0; i<length; i++) {
	spiTransfer(command[i]);
  }
  for(int i=length; i<8; i++) {
	spiTransfer(0x00);  //Fill the rest of the command arguments with 0
  }
  digitalWrite(SS, HIGH);  //End the sequence
}

void Si4735::ptystr(byte pty){	
	// Translate the Program Type bits to the RBDS 16-character fields
	if(pty>=0 && pty<32){
		if(_locale==NA){		
			char* pty_LUT[32] = {	
				"      None      ",
				"      News      ",
				"  Information   ",
				"     Sports     ",
				"      Talk      ",
				"      Rock      ",
				"  Classic Rock  ",
				"   Adult Hits   ",
				"   Soft Rock    ",
				"     Top 40     ",
				"    Country     ",
				"     Oldies     ",
				"      Soft      ",
				"   Nostalgia    ",
				"      Jazz      ",
				"   Classical    ",
				"Rhythm and Blues",
				"   Soft R & B   ",
				"Foreign Language",
				"Religious Music ",
				" Religious Talk ",
				"  Personality   ",
				"     Public     ",
				"    College     ",
				"   None 11000   ",
				"   None 11001   ",
				"   None 11010   ",
				"   None 11011   ",
				"   None 11100   ",
				"     Weather    ",
				" Emergency Test ",
				" ALERT!  ALERT! "};
			strcpy(_pty, pty_LUT[pty]);			
		}
		else if(_locale==EU){
			char* pty_LUT[32] = {	
				"      None      ",
				"      News      ",
				"Current Affairs ",
				"  Information   ",
				"     Sports     ",
				"   Education    ",
				"     Drama      ",
				"    Cultures    ",
				"    Science     ",
				" Varied Speech  ",
				"   Pop Music    ",
				"  Rock Music    ",
				" Easy Listening ",
				"Light Classics M",
				"Serious Classics",
				"  Other Music   ",
				" Weather & Metr ",
				"    Finance     ",
				"Children's Progs",
				" Social Affairs ",
				"    Religion    ",
				"    Phone In    ",
				"Travel & Touring",
				"Leisure & Hobby ",
				"   Jazz Music   ",
				" Country Music  ",
				"  Oldies Music  ",
				"   Folk Music   ",
				"  Documentary   ",
				"   Alarm Test   ",
				" Alarm - Alarm! "};
			strcpy(_pty, pty_LUT[pty]);
		}		
		else{
			strcpy(_pty, " LOCALE UNKN0WN ");
		}
	}
	else{
		strcpy(_pty, "    PTY ERROR   ");
	}	
}

