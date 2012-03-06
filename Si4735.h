/* Arduino Si4735 Library
 * Written by Ryan Owens for SparkFun Electronics 5/17/11
 * Altered by Wagner Sartori Junior 09/13/11 
 * Actively Being Developed by Jon Carrier
 *
 * This library is for use with the SparkFun Si4735 Shield
 * Released under the 'Buy Me a Beer' license
 * (If we ever meet, you buy me a beer)
 *
 * See the example sketches to learn how to use the library in your code.
*/

#ifndef Si4735_h
#define Si4735_h

//Comment out these 'defines' to stip down the Si4735 library features.
//This will help you save memory space at the cost of features.
#define USE_SI4735_REV
#define USE_SI4735_FREQUENCY
#define USE_SI4735_SEEK

#define USE_SI4735_RDS
#define USE_SI4735_CALLSIGN
#define USE_SI4735_PTY
#define USE_SI4735_RADIOTEXT
#define USE_SI4735_DATE_TIME

#define USE_SI4735_RSQ
#define USE_SI4735_VOLUME
#define USE_SI4735_MUTE
#define USE_SI4735_LOCALE
#define USE_SI4735_MODE


#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"  
#else
  #include "WProgram.h"  
#endif

//#include "SPI.h"
#include "string.h"

//Assign the radio pin numbers
#define POWER_PIN	8
#define	RADIO_RESET_PIN	9
#define INT_PIN	2

//Define the SPI Pin Numbers
#if defined(MEGA)
	//DEFINE THE MEGA PINS
	#define DATAOUT 51		//MOSI
	#define DATAIN  50		//MISO 
	#define SPICLOCK  52		//sck
	#define SS 53	  			//ss
#else
	//DEFINE THE UNO PINS
	#define DATAOUT 11		//MOSI
	#define DATAIN  12		//MISO 
	#define SPICLOCK  13		//sck
	#define SS 7//10	  		//ss
#endif

//List of possible modes for the Si4735 Radio
#define AM	0
#define FM	1
#define SW	2
#define LW	3

//Define the Locale options
#define NA 0
#define EU 1

#define ON	true
#define OFF	false

#define MAKEINT(msb, lsb) (((msb) << 8) | (lsb))
typedef unsigned int u_int;
//typedef unsigned char byte;

typedef struct Today {
	byte year; //The 2-digit year
	byte month;
	byte day;
	byte hour;
	byte minute;
};

typedef struct RadioInfo {
	char mode;
	byte locale;
	Today date;
};

typedef struct Metrics {
	byte STBLEND;
	byte RSSI;
	byte SNR;
	byte MULT;
	byte FREQOFF;
};

typedef struct Station {
	char callSign[5];
	char programType[17];	
	char programService[9];
	char radioText[65];
	bool newRadioText;
	//Metrics signalQuality;
	//int frequency
};

class Si4735// : public SPIClass
{
	public:
		//This is just a constructor.		
		Si4735();
		
		/*
		* Description: 
		*	Initializes the Si4735, powers up the radio in the desired mode and limits the bandwidth appropriately.
		* 	This function must be called before any other radio command.
		*	The bands are set as follows:
		*	FM - 87.5 - 107.9 MHz
		*	AM - 520 - 1710 kHz
		*	SW - 2300 - 23000 khz
		*	LW - 152 - 279 kHz
		* Parameters:
		*	mode - The desired radio mode. Use AM(0), FM(1), SW(2) or LW(3).
		*/
		void begin(char mode);
		
		/*
		* Description: 
		*	Used to send an ascii command string to the radio.
		* Parameters:
		*	myCommand - A null terminated ascii string limited to hexidecimal characters
		*	to be sent to the radio module. Instructions for building commands can be found
		*	in the Si4735 Programmers Guide.
		*/
		void sendCommand(char * myCommand);

		/*
		* Description: 
		*	Acquires certain revision parameters from the Si4735 chip
		* Parameters:
		*	FW = Firmware and it is a 2 character array
		*	CMP = Component Revision and it is a 2 character array
		*	REV = Chip Revision and it is a single character
		*/
		#if defined(USE_SI4735_REV)
		void getREV(char*FW,char*CMP,char*REV);
		#endif

		/*
		* Description: 
		*	Used to to tune the radio to a desired frequency. The library uses the mode indicated in the
		* 	begin() function to determine how to set the frequency.
		* Parameters:
		*	frequency - The frequency to tune to, in kHz (or in 10kHz if using FM mode).
		*/
		#if defined(USE_SI4735_FREQUENCY)
		void tuneFrequency(word frequency);
		#endif

		/*
		* Description:
		*	Gets the frequency of the currently tuned station	
		*/
		#if defined(USE_SI4735_FREQUENCY)
		word getFrequency(bool &valid);
		#endif

		/*
		* Description:
		*	Commands the radio to seek up to the next valid channel. If the top of the band is reached, the seek
		*	will continue from the bottom of the band.
		*/
		#if defined(USE_SI4735_SEEK)
		void seekUp(void);
		#endif

		/*
		* Description:
		*	Commands the radio to seek down to the next valid channel. If the bottom of the band is reached, the seek
		*	will continue from the top of the band.
		*/
		#if defined(USE_SI4735_SEEK)
		void seekDown(void);
		#endif
		
		/*
		* Description:
		*	Adjust the threshold levels of the seek function.
		* FM Ranges:
		*	SNR=[0-127], FM_default=3 dB
		*	RSSI=[0-127], FM_default=20 dBuV
		* AM Ranges:
		*	SNR=[0-63], AM_default=5 dB
		*	RSSI=[0-63], AM_default=19 dBuV
		*/	
		#if defined(USE_SI4735_SEEK)
		void seekThresholds(byte SNR, byte RSSI);
		#endif

		/*
		*  Description:
		*	Collects the RDS information. 
		*	This function needs to be actively called in order to see sensible information
		*/
		#if defined(USE_SI4735_RDS)
		bool readRDS(void);
		#endif

		/*
		*  Description:
		*	Pulls the RDS information from the private variable and copies them locally. 
		*/
		#if defined(USE_SI4735_RDS)
		void getRDS(Station * tunedStation);
		#endif

		/*
		*  Description:
		*	Clears _disp and _ps so that data from other stations are not overlayed on the current station.
		*/
		void clearRDS(void);

		/*
		*  Description:
		*	Retreives the Time time that is broadcasted from the tuned station.
		*/
		#if defined(USE_SI4735_RDS) && defined(USE_SI4735_DATE_TIME)
		void getTime(Today * date);
		#endif

		/*
		*  Description:
		*	Retreives the Received Signal Quality Parameters/Metrics.
		*/
		#if defined(USE_SI4735_RSQ)
		void getRSQ(Metrics * RSQ);
		#endif	

		/*
		* Description:
		*	Sets the volume. If of of the 0 - 63 range, no change will be made.
		*/
		#if defined(USE_SI4735_VOLUME)
		byte setVolume(byte value);
		#endif

		/*
		* Description:
		*	Gets the current volume.
		*/
		#if defined(USE_SI4735_VOLUME)
		byte getVolume(void);
		#endif

		/*
		* Description:
		*	Increasese the volume by 1. If the maximum volume has been reached, no increase will take place.
		*/
		#if defined(USE_SI4735_VOLUME)
		byte volumeUp(void);
		#endif
		
		/*
		* Description:
		*	Decreases the volume by 1. If the minimum volume has been reached, no decrease will take place.
		*/
		#if defined(USE_SI4735_VOLUME)
		byte volumeDown(void);
		#endif
		
		/*
		* Description:
		*	Mutes the audio output
		*/
		#if defined(USE_SI4735_MUTE)
		void mute(void);
		#endif

		/*
		* Description:
		*	Disables the mute.
		*/
		#if defined(USE_SI4735_MUTE)
		void unmute(void);
		#endif

		/*
		* Description:
		*	Gets the current status of the radio. Learn more about the status in the Si4735 datasheet.
		* Returns:
		*	The status of the radio.
		*/
		char getStatus(void);
		
		/*
		* Description:
		*	Gets the long response (16 characters) from the radio. Learn more about the long response in the Si4735 datasheet.
		* Parameters:
		*	response - A string for the response from the radio to be stored in.
		*/
		void getResponse(char * response);

		/*
		* Description:
		*	Powers down the radio
		*/
		void end(void);

		/*
		* Description:
		*	Sets the Locale. This determines what Lookup Table (LUT) to use for the pyt_LUT.
		*/
		#if defined(USE_SI4735_LOCALE)
		void setLocale(byte locale);
		#endif

		/*
		* Description:
		*	Gets the Locale.
		*/
		#if defined(USE_SI4735_LOCALE)
		byte getLocale(void);
		#endif	

		/*
		* Description:
		*	Gets the Mode of the radio [AM,FM,SW,LW].
		*/
		#if defined(USE_SI4735_MODE)
		char getMode(void);
		#endif

		/*
		* Description:
		*	Sets the Mode of the radio [AM,FM,SW,LW]. This also performs a powerdown operation.
		*	The user is responsible for reissuing the begin method after this method has been called.
		*/
		#if defined(USE_SI4735_MODE)
		void setMode(char mode);
		#endif

		/*
		* Description:
		*	Sets a property value.
		*/
		void setProperty(word address, word value);
		
		/*
		* Description:
		*	Gets a property value.
		* Returns:
		*	The value stored in address.
		*/
		word getProperty(word address);

	private:
	
		char _mode; 			//Contains the Current Radio mode [AM,FM,SW,LW]		
		char _volume;				//Current Volume
		//word _frequency;			//Current Frequency
		char _disp[65]; 			//Radio Text String
		char _ps[9]; 				//Program Service String
		char _csign[5]; 			//Call Sign
		bool _ab; 					//Indicates new radioText		
		char _pty[17];				//Program Type String
		byte _year;					//Contains the month
		byte _month;				//Contains the year
		byte _day;					//Contains the day
		byte _hour;					//Contains the hour
		byte _minute; 				//Contains the minute
		byte _locale; 				//Contains the locale [NA, EU]	
		bool _newRadioText;		//Indicates that a new RadioText has been received
		
		/*
		* Command string that holds the binary command string to be sent to the Si4735.
		*/
		char command[9];	
		
		/*
		* Description:
		*	Sends a binary command string to the Si4735.
		* Parameters:
		*	command - Binary command to be sent to the radio.
		*	length - The number of characters in the command string (since it can't be null terminated!)
		* TODO:
		*	Make the command wait for a valid CTS response from the radio before releasing 				control of the CPU.
		*/
		void sendCommand(char * command, int length);
		
		/*
		* Description:
		*	Sends/Receives a character from the SPI bus.
		* Parameters:
		*	value - The character to be sent to the SPI bus.
		* Returns:
		*	The character read from the SPI bus during the transfer.
		*/
		char spiTransfer(char value);	
		
		/*
		*  Description:
		*	converts the integer pty value to the 16 character string Program Type.
		*/
		#if defined(USE_SI4735_RDS) && defined(USE_SI4735_PTY)
		void ptystr(byte);
		#endif
		
		/*
		*  Description:
		*	Filters the sting str to only contain printable characters.
		*	Any character that is not a normal character is converted to a space.
		* 	This helps with filtering out noisy strings.

		*/
		void printable_str(char * str, int length);		
};

#endif