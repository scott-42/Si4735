//=====================================================================
//                           SerLCD Class
//=====================================================================

#ifndef SerLCD_h
#define SerLCD_h
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  static uint8_t BYTE;
#else
  #include "WProgram.h"  
#endif

//SerLCD Helper Functions
class SerLCD
{
	public:
		SerLCD();
		void setBaud(int baud);
		void selectLine(int line);		
		void goTo(int position);
		void clear();
		void backlight(bool state);
		void serCommand(byte cmd);
		void clearLine(int line);

	private:
		
};
#endif
