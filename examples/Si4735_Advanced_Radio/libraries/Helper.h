//Coded By Jon Carrier
//Free to use however you please

//#####################################################################
//#####################################################################
//                          HELPER FUNCTIONS
//#####################################################################
//#####################################################################

//#####################################################################
//                          STRING FUNCTIONS
//#####################################################################

bool strcmp(char* str1, char* str2, int length){
  bool same=true;
  for(int i=0;i<length;i++){
    if(str1[i]!=str2[i]){
      same=false;
      break;
    }    
  }  
  return same;
}

void printable_str(char * str, int length){	
	for(int i=0;i<length;i++){		
		if( (str[i]!=0 && str[i]<32) || str[i]>126 ) str[i]=' ';	
	}
}

//#####################################################################
//                          DEBOUNCER FUNCTION
//#####################################################################
int debounce(int signal, int debounceTime){
  int state = digitalRead(signal);
  int lastState = !(state);
  while (state != lastState) {
    lastState=state;
    delay(debounceTime);
    state = digitalRead(signal);
  } 
  return state;
}
