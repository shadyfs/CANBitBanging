//Changable Information
const unsigned short baudrate = 250;//baudrate in thousands
const bool high_noise_tolerance = true;//Set to false to make bit capture timing more strict
const unsigned short RX = 0;//GPIO pin for receiving CAN messages
const unsigned short TX = 1;//GPIO pin to implement BitBanging technique on CAN line

//Global Variable Information
unsigned long ID = 0;//CAN message ID
unsigned long IDlst[100] = {0};
unsigned short IDcount = 0;
unsigned short bitpos = 0;//Bit Position
unsigned short temp_numBit = 0;//Used for samples taken without edge
unsigned short numBit = 0;//Used for calculations while keeping true numBits saved
volatile unsigned long Tdeltas[130] = {0};
unsigned long TdeltasTemp[130] = {0};
unsigned long TdeltasT[130] = {0};
unsigned long numbitarr[130] = {0};
unsigned long numbitarrtemp[130] = {0};
//Timestamps gathered, Maximum amount of changes with 10 added for buffer
bool bitlevel = 0;//Bit value of samples
volatile unsigned short pending = 0;//Bits waiting to be processed
volatile unsigned short processed = 0;//Bits processed
unsigned short tolerance = 0;//Changes bit capture tolerance
const unsigned short bitwidth = (1000 / baudrate);//Calculated Bitwidth in us depending on CAN baudrate
const unsigned short eofwidth = (11 * bitwidth) - (bitwidth / 2);//End of Frame will reset variables used for calculations
unsigned short noise = 0;

// Targeting
unsigned long *target = NULL;//Keeps track of message sections to construct
bool skip_msg = true;//Used to skip unneccessary message construction
elapsedMillis x;
void setup() { 
  Serial.begin(9600);
  if (high_noise_tolerance == false){tolerance = 1;}
  noise = (bitwidth / 2 + tolerance);
  
  //Pins for ISR and BitBanging
  pinMode(RX,INPUT);// sets the RX as input to read CAN traffic
  pinMode(TX,OUTPUT);//sets the TX as output to employ bitbang as needed
  attachInterrupt(digitalPinToInterrupt(RX),ISR_CAN,CHANGE);//Interrupt to detect edges of bit changes
}

void loop() {
  if (pending <= processed){return;}
  if (pending == 0){return;}
  unsigned short temp_processed = processed;
  if(temp_processed == 0){                                                      
    ID = 0;
    bitpos = 0;
    bitlevel = 1;
    target = &ID;
//    memcpy(numbitarrtemp, numbitarr, 130*sizeof(long));
//    memcpy(TdeltasTemp, Tdeltas, 130*sizeof(long));
  }  
  if(temp_processed != processed){      
    return;
  }
  bitlevel =! bitlevel;
  bool reduce = false;
  if(temp_numBit == 5 || temp_processed == 0){reduce = true;}                            
  temp_numBit=Tdeltas[temp_processed] / bitwidth;                                    
  if((Tdeltas[temp_processed] % bitwidth) >= (bitwidth / 2)){temp_numBit++;}  
  if(reduce == true){numBit = temp_numBit -1;}                                        
  else{numBit = temp_numBit;}
//  numbitarrtemp[temp_processed]= numBit;
  for(unsigned short j = 0; j < numBit; j++){      
    bitpos++;
    if(bitpos != 12 && bitpos != 13){
         if(bitpos <= 31){
            *(target) = (*(target) << 1) | bitlevel;
         }    
    }
    if(bitpos == 31){ 
      if(ID == 0XCF00400){
        digitalWrite(TX,HIGH);
        digitalWrite(TX,LOW);
      }
//         if ((ID &0x000000FF) != 0x00 &&(ID &0x000000FF) != 0x0F && (ID &0x000000FF) != 0x0B &&(ID &0x000000FF) != 0x31 &&(ID &0x000000FF) != 0x21){
//           //detachInterrupt(digitalPinToInterrupt(RX));
//           memcpy(TdeltasT, Tdeltas, 130*sizeof(long));
//           Serial.printf("%x ",ID);
//           Serial.println("Temp Array");
//           for(int i = 0; i<130;i++){
//            Serial.printf("%d", TdeltasTemp[i]);
//            Serial.println();
//           }
//           Serial.println("Actual Array:");
//           for(int i = 0; i<130;i++){
//            Serial.printf("%d", TdeltasT[i]);
//            Serial.println();
//           }
//           Serial.println("NumBit array:");
//           for(int i = 0; i<130;i++){
//            Serial.printf("%d", numbitarrtemp[i]);
//            Serial.println();
//           }
//           Serial.println();
//           Serial.println(bitpos);
//           Serial.println(bitlevel);
//           exit(1);
//         }
          //if ((ID &0x000000FF) != 0x00 &&(ID &0x000000FF) != 0xFA && (ID &0x000000FF) != 0x0B && (ID &0x000000FF) != 0x19 && (ID &0x000000FF) != 0x17 && (ID &0x000000FF) != 0x31 && (ID &0x000000FF) != 0x21 && (ID &0x000000FF) != 0x28){
//          IDcount ++;

    }
//    if (IDcount == 999){
//      Serial.println(IDcount);
//      IDcount ++;
//     }
  }
  temp_processed++;
    if (temp_processed != (processed + 1)){return;}                                   // If ISR changes globals, indicating new message has started transmitting   
    else{processed = temp_processed;} 
} 
    
void ISR_CAN(){
  //Serial.println("Interrupt");                                                                       
    static elapsedMicros dT;
   if (dT >= noise){
    if(dT >= eofwidth){
      pending = 0;
      processed = 0;
    }
    else{
      Tdeltas[pending] = dT;
      pending++;    
    }
   }
    dT = 0;   
}
