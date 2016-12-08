#define W0 P2_1
#define W1 P2_2
#define DoorP P1_6
#define DoorE P1_0

#include <codes.h>

//static unsigned long int cards[]={0,0,0,0};
char bits[80];
int bitcnt=0;
unsigned long long bitw=0;
unsigned int timeout=1000;
boolean valid = true;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
   
  pinMode(W0, INPUT_PULLUP); 
  pinMode(W1, INPUT_PULLUP); 
  pinMode(DoorP,OUTPUT);
  pinMode(DoorE,OUTPUT);
  digitalWrite(DoorP, LOW); 
  digitalWrite(DoorE, LOW); 
  
  // Set up edge interrupts on wiegand pins. 
  
  // Wiegand is used by many rfid card readers and mag stripe readers.
  
  // It has two wires. a '1' wire and a '0' wire. It will output a short pulse on
  //    one of these wires to indicate to you that it has read a 1 or a 0 bit.
  //    you need to count the pulses. It's a lot like rotary phone wheels
  //    only in binary and with 2 wires.


  // make sure W0 ans W1 trigger interrupts on a 1->0 transition.
  attachInterrupt((W0), W0ISR, FALLING);
  attachInterrupt((W1), W1ISR, FALLING);
  Serial.println("Hello World!");
  
  for(int i=0; i<sizeof(bits); i++) bits[i]=0;
}


void loop()
{
  digitalWrite(DoorP, LOW);
  digitalWrite(DoorE, LOW);
  if (timeout>0) timeout--; // Keep decrementing timeout. ISRs will set it when a bit is recieved.
  if (timeout == 0 && bitcnt != 0){ // The reader hasn't sent a bit in 2000 units of time. Process card.
    //Serial.print((long unsigned int)(bitw>>32),BIN);
    //Serial.print((long unsigned int)bitw,BIN);
      //  bitw = 0x122D9F628;
    for(int i=bitcnt; i!=0; i--) Serial.print((unsigned int)(bitw>>(i-1) & 0x00000001)); // Print the card number in binary
    Serial.print(" (");
    Serial.print(bitcnt);
    Serial.println(")");
    

    boolean ep,op;
    unsigned int      site;
    unsigned long int card;
    
    // Pick apart card. 
    
    site = (bitw>>25) & 0x7f;
    card = (bitw>>1)  & 0xffffff;
    op = (bitw>>0)  & 0x1;
    ep = (bitw>>32) & 0x1;
    
    // Check parity
    if (parity(site) != ep) valid=false;
    if (parity(card) == op) valid=false;
    
    // Print card info
    Serial.print("Site: "); Serial.println(site);
    Serial.print("Card: "); Serial.println(card);
    Serial.print("ep: "); Serial.print(parity(site));Serial.println(ep);
    Serial.print("op: "); Serial.print(parity(card));Serial.println(op);
    Serial.print("Parity Check: ");Serial.println(valid?"Valid":"Error");
    
    
    if (valid){ // Parity ok?
      if (site==17) // Site ok?
        for (int i=0; i<sizeof(cards); i++)
          if (cards[i] == card){ // Is it in the DB?
            Serial.println("Match!");
            digitalWrite(DoorP, HIGH); // Open door.
            delay(3000);
            goto done;
          }



   Serial.println("Error!");
   digitalWrite(DoorE, HIGH);
   delay(3000);
    }
 
   done:
   // cleanup and prep for next card.
   bitcnt=0;
   bitw = 0;
   valid = true;
  }

}




// Wiegand 0 bit ISR. Triggered by wiegand 0 wire.
void W0ISR(){
  bitw = (bitw<<1) | 0x0; // shift in a 0 bit.
  bitcnt++;               // Increment bit count
  timeout = 2000;         // Reset timeout
  
}

// Wiegand 1 bit ISR. Triggered by wiegand 1 wire.
void W1ISR(){
  bitw = (bitw<<1) | 0x1; // shift in a 1 bit
  bitcnt++;               // Increment bit count
  timeout = 2000;         // Reset Timeout
}

int parity(unsigned long int x) {
   unsigned long int y;
   y = x ^ (x >> 1);
   y = y ^ (y >> 2);
   y = y ^ (y >> 4);
   y = y ^ (y >> 8);
   y = y ^ (y >>16);
   return y & 1;
}
