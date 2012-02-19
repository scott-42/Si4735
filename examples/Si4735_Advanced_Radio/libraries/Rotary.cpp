//Coded By Jon Carrier
//Free to use however you please

//#####################################################################
//                          ROTARY FUNCTIONS
//#####################################################################
#include "Rotary.h"

Rotary::Rotary(){}

void Rotary::begin(int EncA, int EncB, int PB, void (*CALLBACK)(void)){
	//Initialize the private variables
	_ROT_FIFO_0=0; //Most Recent
	_ROT_FIFO_1=0;	
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
	attachInterrupt(0,CALLBACK,CHANGE);
	attachInterrupt(1,CALLBACK,CHANGE);

}

void Rotary::read(void){ //Read the Current Value
  int ROT_FIFO_0_TEST1=11;
  int ROT_FIFO_0_TEST2=21;
  //Perform a very basic debouncing process
  while(ROT_FIFO_0_TEST1 != ROT_FIFO_0_TEST2){
    ROT_FIFO_0_TEST2=ROT_FIFO_0_TEST1;
    delay(25);
    ROT_FIFO_0_TEST1 = (digitalRead(_EncA) * 2) + digitalRead(_EncB);    
  }  
  _ROT_FIFO_1 = _ROT_FIFO_0;
  _ROT_FIFO_0 = ROT_FIFO_0_TEST1;
}

int Rotary::isFwdRot(void){ //Check for forward rotation
  return ((_ROT_FIFO_0 == 2) && (_ROT_FIFO_1 == 0));// ||
         //((ROT_FIFO_0 == 1) && (ROT_FIFO_1 == 3)) ||
         //((ROT_FIFO_0 == 3) && (ROT_FIFO_1 == 2)) ||
         //((ROT_FIFO_0 == 0) && (ROT_FIFO_1 == 1));
}

int Rotary::isRevRot(void){ //Check for reverse rotation
  return ((_ROT_FIFO_0 == 0) && (_ROT_FIFO_1 == 2)); //||
         //((ROT_FIFO_0 == 2) && (ROT_FIFO_1 == 3)) ||
         //((ROT_FIFO_0 == 3) && (ROT_FIFO_1 == 1)) ||         
         //((ROT_FIFO_0 == 1) && (ROT_FIFO_1 == 0));

}

void Rotary::end(void){	
	//Detach the interrupts
  detachInterrupt(0);
  detachInterrupt(1);

}


/*void read_ROT(){ //Read the Current Value
  int ROT_FIFO_0_TEST1=11;
  int ROT_FIFO_0_TEST2=21;
  int ROT_FIFO_0_TEST3=31;
  int ROT_FIFO_0_TEST4=41;  
  //Perform a very basic debouncing process
  while(ROT_FIFO_0_TEST1 != ROT_FIFO_0_TEST2 ||  ROT_FIFO_0_TEST1 != ROT_FIFO_0_TEST3){
    ROT_FIFO_0_TEST4 = ROT_FIFO_0_TEST3;
    ROT_FIFO_0_TEST3 = ROT_FIFO_0_TEST2;
    ROT_FIFO_0_TEST2 = ROT_FIFO_0_TEST1;
    ROT_FIFO_0_TEST1 = (digitalRead(EncA) * 2) + digitalRead(EncB);    
  }  
  ROT_FIFO_1 = ROT_FIFO_0;
  ROT_FIFO_0 = ROT_FIFO_0_TEST1;
}*/

