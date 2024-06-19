/*  This code is for controlling the OKI Driver VFD with the reference:
*   MSM9202-05
*   Control also the 2 pins with reference P1 and P2, which are general purpose pins.
*   This driver uses 2 bits as an ADRAM C0 & C1
* 
*   Please consider this code as a basis for evolution, this is not a clean or final code.
*   And is not careful as strong code.
*   For these reasons the code can be optimized and made robust, but that's not 
*   the purpose of this video.
*/

#include <util/delay.h>
// Standard Input/Output functions 1284
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//  Include following libraries to access IR sensor
#include <IRremote.hpp>
#define IR_RECEIVE_PIN 12

char tmp[14];
int dataOverflows[14];

byte data[12];
byte dataIndex;

#define VFD_in 8
#define VFD_clk 9
#define VFD_stb 10
#define VFD_rst 11 // You can use a pin of Arduino to do the Reset of driver... active at LOW value, normal running at HIGH

bool flag = true;  //This variable is use the go out of cycle while()
bool ledG = false; //LED green, please check the order of color... I not check it!
bool ledB = false; //LED Blue
bool ledR = false; //LED Red

void MSM9202_init(void){
  unsigned char i;
  _delay_ms(300); //power_up delay

  // set GPO pins to low
  cmd_with_stb(0b01000000); // GP1 & GP2 is bit B0 and B1
  _delay_us(60);
  
  // Configure VFD display (number of grids)
  cmd_with_stb(0b01100100); //(0b01100100); //12 grids  (0b01100111) //15 grids  //bin(01100001) 9grids
  _delay_us(60);
  
  // set DIMM/PWM to value
  cmd_with_stb((0b01010000) | 7); //(0b01010000) | 7); //0 min - 7 max  )(0b01010000)
  _delay_us(60);
  
  // switch off extra "ADRAM"
  cmd_with_stb(0b00110000); //
  for(i=0;i<12;i++)
  {
    cmd_with_stb(0x20);
  }
  _delay_us(60);
  
  // test mode: light all
  cmd_with_stb(0b01110011); //
  _delay_us(60);
  
  // normal mode
  cmd_with_stb(0b01110000); //((0b01110000); //test off-normal mode on  (0b01110000)
  _delay_us(60);
}
void cmd_without_stb(unsigned char a){
  // send without stb
  unsigned char transmit = 7; //define our transmit pin
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
   for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_in, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_in, LOW);
     }
    delayMicroseconds(5);
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(5);
   }
}
void cmd_with_stb(unsigned char a){
  // send with stb
  unsigned char transmit = 7; //define our transmit pin
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(1);
         for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
           digitalWrite(VFD_clk, LOW);
           delayMicroseconds(1);
                   if (data & mask){ // if bitwise AND resolves to true
                      digitalWrite(VFD_in, HIGH);
                   }
                   else{ //if bitwise and resolves to false
                     digitalWrite(VFD_in, LOW);
                   }
          digitalWrite(VFD_clk, HIGH);
          delayMicroseconds(1);
         }
   digitalWrite(VFD_stb, HIGH);
   delayMicroseconds(1);
}
void clear_VFD(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 3 bytes = 36 registers
  */
      for (int n=0; n < 12; n++){  // 
        cmd_with_stb(0b01100100); //       cmd 1 // 5 Grids & 16 Segments
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b00010000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table of 5 grids * 16 segm = 80 bits on the table
            //
            //cmd_with_stb((0b10001000) | 7); //cmd 4
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(10);
     }
}
void clear_VFD_ADRAM(void){
  // Only to clear the 2 additional bits of segments. See: (ADRAM DATA WRITE COMMAND ) pay attention to C0&C1
  /*
  Here I clean all registers of Aditional RAM
  Could be done only on the number of grid
  to be more fast. The 12 * 3 bytes = 36 registers
  This is the bit in front of matrix of 5*7 and the letter up of matrix
  */
      for (int n=0; n < 12; n++){  // 
          digitalWrite(VFD_stb, LOW);
          delayMicroseconds(1);
            cmd_without_stb((0b00110000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table 16 grids 
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(2);
      }
}
void MSM9202_print(unsigned char address, unsigned char *text){
  unsigned char c;
  digitalWrite(VFD_stb, LOW);
  cmd_without_stb((0b00010000) | (address & 0x0F)); //((0b00010000)) + (address & 0x0F) );//)(0b00010000
        while ( c = (*text++) )
        {
          cmd_without_stb(c); // & 0x7F); // 0x7F to 7bits ASCII code
        }
  digitalWrite(VFD_stb, HIGH);
  _delay_us(60);
}
void MSM9202_cls(void){
  unsigned char i;
  // cls DCRAM
 digitalWrite(VFD_clk, LOW);
  cmd_without_stb(0b00010000);//
      for(i=0;i<16;i++)
      {
      cmd_without_stb(' '); // Send space to clear the tube. Zone of DCRAM...
      }
 _delay_us(60); 
 digitalWrite(VFD_clk, HIGH);
  //
  // cls Data Write
  _delay_us(60);
 digitalWrite(VFD_clk, LOW);
  cmd_without_stb(0b00110000);//
      for(i=0;i<16;i++) 
      { 
      cmd_without_stb(' ');
      }
 _delay_us(60);
 digitalWrite(VFD_clk, HIGH);
 _delay_us(60);
}
void strrevert1(char *string){
   // Inverter the contents of pointer of string
   // and let it reverted until the next call of
   // function! exp: char letter[16] = "jogos.....tarde";
   // To do a declaration of pointer:  char* ptr=letter;
   // don't forget the null char "\0", this is a 1 more char
   // presente on the string.
   
   int len = 0;
   while (string[len])
   len++;

   int down = 0;
   int up = len - 1;

         while (up > down)
         {
           char c = string[down];
           string[down++] = string[up];
           string[up--] = c;
         }
}
void msgEmpty(){
          strcpy(data, "            "); // Fill the string with 12 spaces to stay empty
          strrevert1(data);             // Do the string reverting
          MSM9202_print(0,data);        // write a grid number 1
          delay(300);               
}
void msgHiFolks(){        
          strcpy(data, " Hi Folks!  "); // Fill the string
          strrevert1(data);             // Do the string reverting
          MSM9202_print(0,data);        // write a grid number 1
          delay(1000);                  // Give time to see the message on VFD
}
void msgNumbers(){
          strcpy(data, " 9876543210 ");   // Fill the string
          strrevert1(data);             // Do the string reverting
          MSM9202_print(0,data);        // write a grid number 1
          delay(1000);       
}
void msgOKI(){
          strcpy(data, " I'm driver ");
          strrevert1(data);
          MSM9202_print(0,data);
          delay(1000);
          strcpy(data, "  M9202-05");
          strrevert1(data);
          MSM9202_print(1,data);
          delay(1000);
          strcpy(data, " from OKI ");
          strrevert1(data);
          MSM9202_print(1,data);
          delay(1000);
}
void msgSony(){
          strcpy(data, "SONY Model");
          strrevert1(data);
          MSM9202_print(1, data);
          delay(1500);
          strcpy(data, " DAV-S888 ");
          strrevert1(data);
          MSM9202_print(1, data);
          delay(1500);
}
void msgLetters(){
          strcpy(data, "abcdefghij");
          strrevert1(data);
          MSM9202_print(1,data);
          delay(1000);
          strcpy(data, "klmnopqrst");
          strrevert1(data);
          MSM9202_print(1,data);
          delay(1000);
          strcpy(data, "uvxyzw... ");
          strrevert1(data);
          MSM9202_print(1,data);
          delay(1000);
}
void msgOnOff(){
  cmd_with_stb(0b01010111);  // set light
  delay(10);
  cmd_with_stb(0b01110011); //All on
  delay(1000);
  cmd_with_stb(0b01110001); //All off
  delay(500); 
  cmd_with_stb(0b01110011); //All on
  delay(1000);
  cmd_with_stb(0b01110001); //All off
  delay(500);
  cmd_with_stb(0b01110000); //Normal display
}
void ctrlGPIO(){
 // Zone of test to pins GPO
          cmd_with_stb(0b01000011); //Set P1 & P2 on
          delay(100);
          cmd_with_stb(0b01000000); //Set P1 & P2 off
          delay(100);
          cmd_with_stb(0b01000011); //Set P1 & P2 on
          delay(100);
          cmd_with_stb(0b01000000); //Set P1 & P2 off
          delay(100);
          cmd_with_stb(0b01000011); //Set P1 & P2 on
          delay(100);
          cmd_with_stb(0b01000000); //Set P1 & P2 off
          delay(100);
}
void infraRed(){
  if (IrReceiver.decode()) {
      Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); // Print "old" raw data
      //IrReceiver.printIRResultShort(&Serial); // Print complete received data in one line
      //IrReceiver.printIRSendUsage(&Serial);   // Print the statement required to send this data
      //...
        if (IrReceiver.decodedIRData.decodedRawData == 0x0880) {          //  Key '1' 
        msgSony();
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x0881) {          //  Key '2'  
        msgHiFolks();
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x0882) {          //  Key '3' 
        msgOKI();
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x0883) {          //  Key '4' 
          ctrlGPIO();  //This is bkinking of LED from GPIO 1 of M9202
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x0885) {          //  Key '6' 
            ledG=(digitalRead(2));delay(5);
          digitalWrite(2, !ledG);
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x0886) {          //  Key '7' 
            ledB=(digitalRead(3));delay(5);
          digitalWrite(3, !ledB);
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x0887) {          //  Key '8' 
            ledR=(digitalRead(4));delay(5);
          digitalWrite(4, !ledR);
        }
        if (IrReceiver.decodedIRData.decodedRawData == 0x08A0) {          //  Key '0' 
          flag = false;  //This is to force go out of cycle while!
        }
      IrReceiver.resume(); // Enable receiving of the next value
  }
}
void msgIR(){
          strcpy(data, "IR receive");
          strrevert1(data);
          MSM9202_print(1,data);
          delay(4);
}
void setup() {
  // put your setup code here, to run once:
  //initialize digital pin LED_BUILTIN as an output.
  //pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
 
digitalWrite(VFD_rst, LOW);
delay(5);
digitalWrite(VFD_rst, HIGH);
delay(100); //Only to protect the action of Reset time!

pinMode(12, INPUT_PULLUP); // Our infra red receiver pin is here through a resistor, like 1K

pinMode(2, OUTPUT);
pinMode(3, OUTPUT);
pinMode(4, OUTPUT);

pinMode(8, OUTPUT);
pinMode(9, OUTPUT);
pinMode(10, OUTPUT);
pinMode(11, OUTPUT);

//This is our set of Infra red receiver.
IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver of Infra Red pin, connected to the receptor IR.

MSM9202_init();
}
void loop(){ 
  flag = true;
msgOnOff();
MSM9202_cls();
clear_VFD();
clear_VFD_ADRAM();  // Only to clear the 2 additional bits of segments. 
                   //See: (ADRAM DATA WRITE COMMAND ) pay attention to C0&C1 (pins: AD1, AD2)        
  msgEmpty();

    for(uint8_t s = 0; s<5; s++){
      msgHiFolks();
      msgEmpty();
    }
 msgSony();
 msgNumbers();
 delay(500);  
 msgLetters(); 
 delay(500); 
 msgOKI();
 delay(100);
 msgEmpty();
 ctrlGPIO(); 
    while(flag){
      msgIR();
      infraRed();
      delay(100);
    }            
}
