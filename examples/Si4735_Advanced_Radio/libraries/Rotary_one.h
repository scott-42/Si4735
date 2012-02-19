//Coded By Jon Carrier
//Free to use however you please

//#####################################################################
//                          ROTARY FUNCTIONS
//#####################################################################
#ifndef Rotary_one_h
#define Rotary_one_h
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
class Rotary_one
{
	public:
		Rotary_one();
		void begin(int EncA, int EncB, int PB, void (*CALLBACK)(void));
		void read(void);
		int isFwdRot(void);
		int isRevRot(void);
		void end(void);
		volatile int ROTA; //Currently read value on A
		volatile int ROTB; //Currently read value on B
	private:
		int _ROTA_prev;
		int _ROTB_prev;
		int _EncA;
		int _EncB;
		int _PB;
};
#endif
