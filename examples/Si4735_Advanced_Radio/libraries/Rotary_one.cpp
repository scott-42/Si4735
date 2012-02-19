//Coded By Jon Carrier
//Free to use however you please

//#####################################################################
//                          ROTARY FUNCTIONS
//#####################################################################
#include "Rotary_one.h"
//#include "WProgram.h"
Rotary_one::Rotary_one(){}

void Rotary_one::begin(int EncA, int EncB, int PB, void (*CALLBACK)(void)){
	//Initialize the private variables
	ROTA=0;
	ROTB=0;	
	_EncA=EncA;
	_EncB=EncB;
	_PB=PB;

	//Setup the rotary encoder and pushbutton
	pinMode(PB, INPUT);
	pinMode(EncA, INPUT);
	pinMode(EncB, INPUT);	
	digitalWrite(PB, LOW);
	digitalWrite(EncA, HIGH);
	digitalWrite(EncB, HIGH);	

	//We use an interrupt to improve the performance of the encoder
	//attachInterrupt(0,CALLBACK,CHANGE);//Pin2 on UNO
	attachInterrupt(1,CALLBACK,CHANGE);//Pin3 on UNO
}

void Rotary_one::read(void){ //Read the Current Value	
  	int ROTA2=2;
	int ROTA3=3;
  	int ROTB2=2;
	int ROTB3=3;
  	bool done=false;
	_ROTA_prev = ROTA;
	_ROTB_prev = ROTB;
	ROTA=digitalRead(_EncA);
	ROTB=digitalRead(_EncB); 	
  //Perform a very basic debouncing process
  while(!done){  
	ROTA3=ROTA2;
	ROTA2=ROTA;
	ROTB3=ROTB2;
  	ROTB2=ROTB;
   delayMicroseconds(50);
   ROTA=digitalRead(_EncA);
	ROTB=digitalRead(_EncB);    
	done=(ROTA3 == ROTA2 && ROTA2 == ROTA && ROTB3 == ROTB2 && ROTB2 == ROTB);
	//done=(ROTA2 == ROTA && ROTB2 == ROTB);
	
	
  }
	
	/*Serial.print(ROTA);
	Serial.print(ROTA2);
	Serial.print(ROTA3);
	Serial.print(",");
	Serial.print(ROTB);
	Serial.print(ROTB2);
	Serial.println(ROTB3);*/
}

int Rotary_one::isFwdRot(void){ //Check for forward rotation
	//_ROTA_prev = ROTA;
	//_ROTB_prev = ROTB;
	if( !((_ROTA_prev != ROTA) && (_ROTB_prev != ROTB)) ){ //We use AND to avoid bad state changes
		return (ROTB&&ROTA); //Only count when both are HI, this avoids counting 2x per step
	}
	else
		return 0;
}

int Rotary_one::isRevRot(void){ //Check for reverse rotation
	//_ROTA_prev = ROTA;
	//_ROTB_prev = ROTB;
	if( !((_ROTA_prev != ROTA) && (_ROTB_prev != ROTB)) ){ //We use AND to avoid bad state changes				
		return ((!ROTA)&&ROTB); //Only count when A=1 and B=0, this avoids counting 2x per step
	}
	else		
		return 0;  
}

void Rotary_one::end(void){	
	//Detach the interrupts
  //detachInterrupt(0);
  detachInterrupt(1);

}
