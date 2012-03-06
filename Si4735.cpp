/* Arduino Si4735 Library
 * Written by Ryan Owens for SparkFun Electronics 5/17/11
 * Altered by Wagner Sartori Junior 09/13/11
 * Actively Being Developed by Jon Carrier
 *
 * This library is for use with the SparkFun Si4735 Shield
 * Released under the 'Buy Me a Beer' license
 * (If we ever meet, you buy me a beer)
 *
 * See the header file for better function documentation.
 *
 * See the example sketches to learn how to use the library in your code.
*/

/*

SPCR
| 7    | 6    | 5    | 4    | 3    | 2    | 1    | 0    |
| SPIE | SPE  | DORD | MSTR | CPOL | CPHA | SPR1 | SPR0 |

SPIE - Enables the SPI interrupt when 1
SPE - Enables the SPI when 1
DORD - Sends data least Significant Bit First when 1, most Significant Bit first when 0
MSTR - Sets the Arduino in master mode when 1, slave mode when 0
CPOL - Sets the data clock to be idle when high if set to 1, idle when low if set to 0
CPHA - Samples data on the falling edge of the data clock when 1, rising edge when 0
SPR1 and SPR0 - Sets the SPI speed, 00 is fastest (4MHz) 11 is slowest (250KHz)

*/
#include "Si4735.h"
#define READ_DELAY 10
//This is just a constructor.
//Default values are assigned to various private variables
Si4735::Si4735(){
	_mode		= FM;
	_locale	= NA;
	_volume	= 63;
	_ab		= 0;
	_year		= 0;
	_month	= 0;
	_day		= 0;
	_hour		= 0;
	_minute	= 0;	
	_newRadioText = 0;
	clearRDS();
}

void Si4735::clearRDS(void){
	byte i;
	for(i=0; i<64; i++)	_disp[i]	='\0';
	for(i=0; i<8; i++) 	_ps[i]	='\0';
	for(i=0; i<16; i++) 	_pty[i]	='\0';
	for(i=0; i<4; i++) 	_csign[i]='\0';		
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
	delay(1);
	digitalWrite(RADIO_RESET_PIN, HIGH);
	delay(1);

	//Now configure the I/O pins properly
	pinMode(DATAOUT, OUTPUT);
	pinMode(DATAIN, INPUT);
	pinMode(SPICLOCK,OUTPUT);
	pinMode(SS,OUTPUT); 
	pinMode(INT_PIN, INPUT); 
	digitalWrite(SS, HIGH);	

	//Configure the SPI hardware
	//SPIClass::begin();
	//SPIClass::setClockDivider(SPI_CLOCK_DIV32);
	SPCR = (1<<SPE)|(1<<MSTR);//|(1<<SPR1)|(1<<SPR0);	//Enable SPI HW, Master Mode	
	
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

	//Configure GPO lines to maximize stability
	sprintf(command, "%c%c", 0x80,0x06);
	sendCommand(command, 2);
	delay(10);
	sprintf(command, "%c%c", 0x81,0x04);
	sendCommand(command, 2);
	delay(10);

	//Set the volume to the current value.
	setVolume(_volume);

	//Disable Mute
	unmute();

	//Enable RDS
	//Only store good blocks and ones that have been corrected
	setProperty(0x1502, 0xAA01);	
	//Only store good blocks
	//setProperty(0x1502, 0x0001)
	

	//Set the seek band for the desired mode (AM and FM can use default values)
	switch(_mode){
		case SW:
			//Set the lower band limit for Short Wave Radio to 2300 kHz
			setProperty(0x3400, 0x08FC);			
			//Set the upper band limit for Short Wave Radio to 23000kHz
			setProperty(0x3401, 0x59D8);
			break;
		case LW:
			//Set the lower band limit for Long Wave Radio to 152 kHz
			setProperty(0x3400, 0x0099);			
			//Set the upper band limit for Long Wave Radio to 279 kHz
			setProperty(0x3401, 0x0117);							
			break;
		default:
			break;
	}	
}

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

void Si4735::tuneFrequency(word frequency){
	//Split the desired frequency into two character for use in the
	//set frequency command.
	byte highByte = frequency >> 8;
	byte lowByte = frequency & 0x00FF;
	
	//Depending on the current mode, set the new frequency.
	switch(_mode){
		case FM:
			sprintf(command, "%c%c%c%c", 0x20, 0x00, highByte, lowByte);
			break;
		case AM:
		case SW:
		case LW:
			sprintf(command, "%c%c%c%c", 0x40, 0x00, highByte, lowByte);
			break;
		default:
			break;
	}
	sendCommand(command, 4);
	delay(100);	
	clearRDS();
}
#if defined(USE_SI4735_REV)
void Si4735::getREV(char*FW,char*CMP,char*REV){
	//FW = Firmware and it is a 2 character array
	//CMP = Component Revision and it is a 2 character array
	//REV = Chip Revision and it is a single character
	char response [16];
	sprintf(command, "%c", 0x10);
	
	//Send the command
	sendCommand(command, 1);

	//Now read the response	
	getResponse(response);	

	FW[0]=response[2];
	FW[1]=response[3];
	FW[2]='\0';
	CMP[0]=response[6];
	CMP[1]=response[7];
	CMP[2]='\0';
	*REV=response[8];	
}
#endif //USE_SI4735_REV
#if defined(USE_SI4735_FREQUENCY)
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
#endif //USE_SI4735_FREQUENCY
#if defined(USE_SI4735_SEEK)
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

void Si4735::seekThresholds(byte SNR, byte RSSI){
	//Use the current mode selection to set the threshold properties.	
	switch(_mode){
		case FM:
			if(SNR>127)SNR=127;
			else if(SNR<0)SNR=0;
			if(RSSI>127)RSSI=127;
			else if(RSSI<0)RSSI=0;
			setProperty(0x1403, (word)SNR);	
			setProperty(0x1404, (word)RSSI);				
			break;
		case AM:
		case SW:
		case LW:	
			if(SNR>63)SNR=63;
			else if(SNR<0)SNR=0;
			if(RSSI>63)RSSI=63;
			else if(RSSI<0)RSSI=0;
			setProperty(0x1403, (word)SNR);	
			setProperty(0x1404, (word)RSSI);				
			break;
		default:
			break;
	}
}

#endif //USE_SI4735_SEEK
#if defined(USE_SI4735_RDS)
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

	byte type = (response[6]>>4) & 15;
	bool version = bitRead(response[6], 4);
	bool tp = bitRead(response[6], 5);	
	int pi;

	if (version == 0) {
		pi = MAKEINT(response[4], response[5]);
	} else {
		pi = MAKEINT(response[8], response[9]);
	}
	#if defined(USE_SI4735_CALLSIGN)
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
	if(pi>=0){
		_csign[1]=char(pi/676+65);//char(pi/676);
		_csign[2]=char((pi - 676*int(pi/676))/26+65);//char((pi-676*(_csign[1]))/26+65);
		_csign[3]=char(((pi - 676*int(pi/676))%26)+65);//char((pi-676*(_csign[1]))%26+65);
		//_csign[1]+=65;
		_csign[4]='\0';		
	}
	else{
		_csign[0]='U';
		_csign[1]='N';
		_csign[2]='K';
		_csign[3]='N';
		_csign[4]='\0';
	}
	#endif //USE_SI4735_CALLSIGN

	// Groups 0A & 0B
	// Basic tuning and switching information only
	if (type == 0) {
	#if defined(USE_SI4735_PTY)
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
		printable_str(_ps, 8);
	#endif //USE_SI4735_PTY
	}
	// Groups 2A & 2B
	// Radio Text
	else if (type == 2) {
		#if defined(USE_SI4735_RADIOTEXT)
		// Get their address
		int addressRT = response[7] & 15; // Get rightmost 4 bits
		bool ab = bitRead(response[7], 4);
 		bool cr = 0; //indicates that a carriage return was received
		byte len = 64;
		if (version == 0) {
			if (addressRT >= 0 && addressRT <= 15) {
				if (response[8] != 0x0D )
					_disp[addressRT*4] = response[8];
				else{
					len=addressRT*4;
					cr=1;
				}
				if (response[9] != 0x0D)
					_disp[addressRT*4+1] = response[9];
				else{
					len=addressRT*4+1;
					cr=1;
				}
				if (response[10] != 0x0D)
					_disp[addressRT*4+2] = response[10]; 
				else{
					len=addressRT*4+2;
					cr=1;
				}
				if (response[11] != 0x0D)
					_disp[addressRT*4+3] = response[11];
				else{
					len=addressRT*4+3;
					cr=1;
				}
			}
		} else {
			if (addressRT >= 0 && addressRT <= 7) {
				if (response[10] != '\0')
					_disp[addressRT*2] = response[10];
				if (response[11] != '\0')
					_disp[addressRT*2+1] = response[11];
			}
		}
		if(cr){
			for (byte i=len; i<64; i++) _disp[i] = ' ';
		}
		if (ab != _ab) {			
			for (byte i=0; i<64; i++) _disp[i] = ' ';
			_disp[64] = '\0';			
			_newRadioText=1;
		}
		else{
			_newRadioText=0;
		}
		_ab = ab;
		printable_str(_disp, 64);
		#endif //USE_SI4735_RADIOTEXT
	}
	// Group 4A	Clock-time and Date
	//Note the time is localized but the date is the UTC date
	//Setting offset to 0 will make the time referenced to UTC
	else if (type == 4 && version == 0){	
		#if defined(USE_SI4735_DATE_TIME)
		unsigned long MJD = (response[7]&3<<15 | u_int(response[8])<<7 | (response[9]>>1)&127)&0x01FFFF;
		u_int Y=(MJD-15078.2)/365.25;
		u_int M=(MJD-14956.1-u_int(Y*365.25))/30.6001;		
		byte K;
		if(M==14||M==15)
			K=1;
		else
			K=0;
		_year=(Y+K)%100;
		_month=M-1-K*12;
		_day=MJD-14956-u_int(Y*365.25)-u_int(M*30.6001);		
		
		char sign=(((response[11]>>5)&1)/(-1));
		if(sign==0)sign=1; //Make sign a bipolar variable
		char offset=sign*(response[11]&31);

		_hour=((response[9]&1)<<4) | ((response[10]>>4)&15);
		_minute=((response[10]&15)<<2 | (response[11]>>6)&3);			
		_hour= (_hour + (offset/2) + 24)%24;		
		_minute=(_minute + (offset)%2*30 + 60)%60;
		#endif //USE_SI4735_DATE_TIME
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
	tunedStation->newRadioText=_newRadioText;		
}
#if defined(USE_SI4735_DATE_TIME)
void Si4735::getTime(Today * date){
	date->year=_year;
	date->month=_month;
	date->day=_day;
	date->hour=_hour;
	date->minute=_minute;
}
#endif //USE_SI4735_DATE_TIME
#endif //USE_SI4735_RDS
#if defined(USE_SI4735_RSQ) 
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
#endif
#if defined(USE_SI4735_VOLUME)
byte Si4735::volumeUp(void){
	//If we're not at the maximum volume yet, increase the volume
	if(_volume < 63){
		_volume+=1;
		//Set the volume to the current value.
		setProperty(0x4000, (word)_volume);	
	}
	return _volume;
}

byte Si4735::volumeDown(void){
	//If we're not at the minimum volume yet, decrease the volume
	if(_volume > 0){
		_volume-=1;
		//Set the volume to the current value.
		setProperty(0x4000, (word)_volume);	
	}
	return _volume;
}

byte Si4735::setVolume(byte value){
	if(value <= 63 && value >= 0){
		_volume=value;
		//Set the volume to the current value.
		setProperty(0x4000, (word)_volume);
	}
	return _volume;
}

byte Si4735::getVolume(void){	
	return _volume;
}
#endif //USE_SI4735_VOLUME
#if defined(USE_SI4735_MUTE)
void Si4735::mute(void){
	//Disable Mute
	setProperty(0x4001, 0x0003);
}

void Si4735::unmute(void){
	//Disable Mute
	setProperty(0x4001, 0x0000);
}
#endif //USE_SI4735_MUTE

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
	for(int i=0; i<16; i++)*response++ = spiTransfer(0x00);  //Assign the response to the string.
	digitalWrite(SS, HIGH);
}

void Si4735::end(void){
	sprintf(command, "%c", 0x11);
	sendCommand(command, 1);
	delay(1);
}
#if defined(USE_SI4735_LOCALE)
void Si4735::setLocale(byte locale){
	_locale=locale;	
	//Set the deemphasis to match the locale
	switch(_locale){
		case NA:			
			setProperty(0x1100, 0x0002);		
			break;
		case EU:
			setProperty(0x1100, 0x0001);
			break;
		default:
			break;
	}
}

byte Si4735::getLocale(void){
	return _locale;
}
#endif //USE_SI4735_LOCALE

#if defined(USE_SI4735_MODE)
void Si4735::setMode(char mode){
	end();
	begin(mode);
}

char Si4735::getMode(void){
	return _mode;
}
#endif //USE_SI4735_MODE

void Si4735::setProperty(word address, word value){	
	sprintf(command, "%c%c%c%c%c%c", 0x12, 0x00, (address>>8)&255, address&255, (value>>8)&255, value&255);
	sendCommand(command, 6);
	delay(1);
}

word Si4735::getProperty(word address){	
	char response [16];	
	sprintf(command, "%c%c%c%c", 0x13, 0x00, (address>>8)&255, address&255);
	sendCommand(command, 4);
	getResponse(response);
	return response[2]<<8 | response[3];
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
}

void Si4735::sendCommand(char * command, int length){
  digitalWrite(SS, LOW);
  delay(1);
  spiTransfer(0x48);  //Contrl byte to write an SPI command (now send 8 bytes)
  for(int i=0; i<length; i++)spiTransfer(command[i]);
  for(int i=length; i<8; i++)spiTransfer(0x00);  //Fill the rest of the command arguments with 0
  digitalWrite(SS, HIGH);  //End the sequence
}
#if defined(USE_SI4735_PTY)
void Si4735::ptystr(byte pty){	
	// Translate the Program Type bits to the RBDS 16-character fields	
	if(pty>=0 && pty<32){
		char* pty_LUT[51] = {	
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
			" Reserved  -24- ",
			" Reserved  -25- ",
			" Reserved  -26- ",
			" Reserved  -27- ",
			" Reserved  -28- ",
			"     Weather    ",
			" Emergency Test ",
			"  !!!ALERT!!!   ",
			"Current Affairs ",
			"   Education    ",
			"     Drama      ",
			"    Cultures    ",
			"    Science     ",
			" Varied Speech  ",
			" Easy Listening ",
			" Light Classics ",
			"Serious Classics",
			"  Other Music   ",
			"    Finance     ",
			"Children's Progs",
			" Social Affairs ",
			"    Phone In    ",
			"Travel & Touring",
			"Leisure & Hobby ",
			" National Music ",
			"   Folk Music   ",
			"  Documentary   "};
		if(_locale==NA){		
			strcpy(_pty, pty_LUT[pty]);
		}
		else if(_locale==EU){
			byte LUT[32] = {0, 1, 32, 2, 
					3, 33, 34, 35,
					36, 37, 9, 5, 
					38, 39, 40, 41,
					29, 42, 43, 44, 
					20, 45, 46, 47,
					14, 10, 48, 11, 
					49, 50, 30, 31 };
			strcpy(_pty, pty_LUT[LUT[pty]]);
		}
		else{
			strcpy(_pty, " LOCALE UNKN0WN ");
		}
	}
	else{
		strcpy(_pty, "    PTY ERROR   ");
	}	
}
#endif //USE_SI4735_PTY

void Si4735::printable_str(char * str, int length){
	for(int i=0;i<length;i++){
		if( (str[i]!=0 && str[i]<32) || str[i]>126 ) str[i]=' ';	
	}
}
