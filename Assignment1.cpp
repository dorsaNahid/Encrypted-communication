/*
Assignment 1 part 2:
Names: Tymoore Jamal and Dorsa Nahid
ID's: 1452978 (Tymoore) and 1463449 (Dorsa)
Aknowlegements: We both worked on this assignment together. With no other outside
help other than the cmput 274 proffessors and the code they provided which we
used.
*/
#include <Arduino.h>
// Setup function that initializes the Arduino and its serial monitor.
void setup() {
  init();
  Serial.begin(9600);
  Serial3.begin(9600);
}
/*
A fucntion that multiplies two numbers a and b together and then mods the product
with mod. WARNING: errors may occur when mod is greater than 31 bits or a or b is
larger than 32 bits */
uint32_t multiply_mod(uint32_t a, uint32_t b, uint32_t mod){
  uint32_t sum=0 % mod;
  a= a % mod;
  b= b % mod;
  for(int i=0; i<32;++i){
    if ((a & (1ul<<i))!=0){
      sum = (sum+b) % mod;
    }
    b=(b<<1)% mod;
  }
  sum = sum % mod;
  return sum;
}
/*
A function that raises a to the power of b and then mods the answer with mod.
WARNING: errors may occur when a, b, or mod is greater than 31 bits.
*/
uint32_t pow_mod(uint32_t base, uint32_t exponent, uint32_t mod) {
  uint32_t result = 1 % mod;
  uint32_t p = base % mod;
  for (int i=0; i<32; ++i) {
    if ((exponent & (1ul<<i))!=0){
      result=multiply_mod(result,p,mod);
    }
    p = multiply_mod(p,p,mod);
  }
  return result;
}
/*
This function generates a random 32 bit unsigned integer by reading from anolog
pin 1 while nothing is plugged in to it 32 times, only selecting the least significant
bit each time.
*/
uint32_t random_generator(){
  int analogInPin=1;
  int read_value;
  uint32_t random_number=0;
  uint32_t bit;
  pinMode(analogInPin,INPUT);
  for (int i=0; i<32; i++){
    read_value= analogRead(analogInPin);
    bit = read_value & 1;
    random_number=random_number + (bit<<i);
    delay(50);
  }
  return random_number;
}
/*
Takes the user's private_key, and generates the users public key by raising the
generator to the power of the random private_key and them taking the mod of it
with respect to the prime.
*/
uint32_t get_my_public_key(uint32_t private_key,uint32_t generator, uint32_t prime){
  uint32_t my_public_key;
  my_public_key=pow_mod(generator,private_key,prime);
  return my_public_key;
}
/*
Writes an uint32_t to Serial3, starting from the least-significant
byte and finishing with the most significant byte.
*/
void uint32_to_serial3(uint32_t num) {
  Serial3.write((char) (num >> 0));
  Serial3.write((char) (num >> 8));
  Serial3.write((char) (num >> 16));
  Serial3.write((char) (num >> 24));
}
/*
Reads an uint32_t from Serial3, starting from the least-significant
byte and finishing with the most significant byte.
*/
uint32_t uint32_from_serial3() {
  uint32_t num = 0;
  num = num | ((uint32_t) Serial3.read()) << 0;
  num = num | ((uint32_t) Serial3.read()) << 8;
  num = num | ((uint32_t) Serial3.read()) << 16;
  num = num | ((uint32_t) Serial3.read()) << 24;
  return num;
}
/*
Waits for a certain number of bytes on Serial3 or timeout
param nbytes: the number of bytes we want
param timeout: timeout period (ms); specifying a negative number
turns off timeouts (the function waits indefinitely if timeouts are turned off).
return True if the required number of bytes have arrived.
*/
bool wait_on_serial3( uint8_t nbytes, long timeout ) {
  unsigned long deadline = millis() + timeout;//wraparound not a problem
  while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline))
  {
    delay(1); // be nice, no busy loop
  }
  return Serial3.available()>=nbytes;
}
/*
Implements the Park-Miller algorithm with 32 bit integer arithmetic
return ((current_key * 48271)) mod (2^31 - 1);
This is linear congruential generator, based on the multiplicative
group of integers modulo m = 2^31 - 1.
The generator has a long period and it is relatively efficient.
Most importantly, the generator's modulus is not a power of two
(as is for the built-in rng),
hence the keys mod 2^{s} cannot be obtained
by using a key with s bits.
Based on:
http://www.firstpr.com.au/dsp/rand31/rand31-park-miller-carta.cc.txt
*/
uint32_t next_key(uint32_t current_key) {
  const uint32_t modulus = 0x7FFFFFFF; // 2^31-1
  const uint32_t consta = 48271;  // we use that consta<2^16
  uint32_t lo = consta*(current_key & 0xFFFF);
  uint32_t hi = consta*(current_key >> 16);
  lo += (hi & 0x7FFF)<<16;
  lo += hi>>15;
  if (lo > modulus) lo -= modulus;
  return lo;
}
/*
This fucntion is only ran by the arduino whose pin 13 is connected to the ground.
It has multiple states, which is programmed through the enum data type. The first
state is the starting state and from this state the Arduino will send out both
the letter "C" and it's public key to the other serial monitor and then move on
to the second state. The second state (WaitingForAck) will wait for the other
Arduino to acknowlegde that it got the message by checking to see if the other
Arduino send an A through the serial monitor. If it gets no responce within one
second then it will return to the initial state, if it gets a responce then it will
read the value of the other persons public key, store it, and send one last
aknowlegement before moving on to the data exchange state where it returns the
recieved public key.
*/
uint32_t client(uint32_t my_public_key){
  uint32_t server_key;
  enum State { Start, WaitingForAck , DataExchange };
  State current_state = Start;
  while(true){
    if (current_state == Start){
      Serial3.write('C');
      uint32_to_serial3(my_public_key);
      current_state = WaitingForAck;
    }
    else if(current_state == WaitingForAck && wait_on_serial3(1,1000) == true && Serial3.read() == 'A'){
      if(wait_on_serial3(4,1000) == true){
        server_key = uint32_from_serial3();
        Serial3.write('A');
        current_state = DataExchange;
      }
      else {
        current_state = Start;
      }
    }
    else if(current_state == DataExchange){
      return server_key;
    }
    else {
      current_state = Start;
    }
  }
}
/*
This fucntion is only ran by the arduino whose pin 13 is connected to 5V.
It has multiple states, which is programmed through the enum data type. The first
state is the starting state called Listen and from this state the Arduino will
be constantly listening to see if it is recieving the letter "C" if it is then it
moves on the the second state, WaitingForKey1 where it waits for the other Arduino's
public key, and if it doesn't get it within 1 second then it timeouts and returns
to the start. However if it does recieve it then it stores it and writes the letter
"A" and then its public key to the other Arduino's serial mon to aknowlegde that
it got the information needed. From here it moves onto the state WaitForAck1 where
it listens to see if the other Arduino aknowlegded the aknowlegement (by sending "A")
and if so then it moves onto the DataExchange state and returns the other Arduino's
public key. However if it reads a "C" then it moves on to WaitingForKey2. If it doesnt read
anything in 1 second then it timeouts and returns to the beggining. If it reaches
WaitingForKey2 then it will behave almost exactly and it was in the state
WaitingForKey1 except that it will not send anything out and instead of moving on
to WaitForAck1 it will move on to WaitForAck2. If it reaches the state WaitForAck2
it will behave exactly like WaitForAck1. Finally when we reach data transfer then
it will return the value of the other Arduino's public key.
*/
uint32_t server(uint32_t your_public_key){
  uint32_t client_key;
  enum State { Listen, WaitingForKey1, WaitForAck1, WaitingForKey2, WaitForAck2, DataExchange };
  State current_state = Listen;
  while(true){
    if (current_state == Listen && wait_on_serial3(1,1000)==true && Serial3.read() == 'C'){
      current_state = WaitingForKey1;
    }
    else if (current_state == WaitingForKey1 && wait_on_serial3(4,1000) == true){
      client_key = uint32_from_serial3();
      Serial3.write ('A');
      uint32_to_serial3(your_public_key);
      current_state = WaitForAck1;
    }
    else if (current_state == WaitForAck1){
      if (wait_on_serial3(1,1000)==true && Serial3.read() == 'A'){
        current_state = DataExchange;
      }
      else if (wait_on_serial3(1,1000)==true && Serial3.read() == 'C'){
        current_state = WaitingForKey2;
      }
      else {
        current_state = Listen;
      }
    }
    else if (current_state == WaitingForKey2 && wait_on_serial3(4,1000) == true){
      client_key = uint32_from_serial3();
      current_state = WaitForAck2;
    }
    else if (current_state == WaitForAck2){
      if (wait_on_serial3(1,1000)==true && Serial3.read() == 'A'){
        current_state = DataExchange;
      }
      else if (wait_on_serial3(1,1000)==true && Serial3.read() == 'C'){
        current_state = WaitingForKey2;
      }
      else {
        current_state = Listen;
      }
    }
    else if (current_state == DataExchange){
      return client_key;
    }
    else {
      current_state = Listen;
    }
  }
}
/*
This fucntion determines whether the Arduino connected is the server or the reciever
by checking to see if pin 13 is connected to the ground or 5V. If it is connected
to 5V it will be the server and if it is connected to the ground then it will be
the client.
*/
uint32_t share_public_keys(uint32_t my_public_key){
  const int wire = 13;
  if (digitalRead (wire) == LOW){
    return client(my_public_key);
  }
  else if (digitalRead (wire) == HIGH){
    return server(my_public_key);
  }
}
/*
This fucntion is the chatting part of our program. It is constantly looping through a
state of checking to see if the Arduino is tansmitting or recieving. If it is transmitting
then in displays the character on the screen of the user and then encrypts the
character by XORing it with the least significant 8 bits of the shared_secret_key.
Then it changes the shared_secret_key using the function next_key.
If it is recieving it decryptes the character by XORing it once again
with the 8 least significant bits of shared_secret_key and then displaying it
and finally to stay in sync it then also changes its shared_secret_key through
the function next_key. This function will run indefinitely until stopped by the user.
*/
void chat(uint32_t shared_secret_key_recieved){
  uint32_t shared_secret_key_input = shared_secret_key_recieved;
  uint32_t shared_secret_key_output = shared_secret_key_recieved;
  uint32_t new_key_in=shared_secret_key_input;
  uint32_t new_key_out=shared_secret_key_output;
  char c;
  while (true) {
    if (Serial.available()>0) {
      c = Serial.read();
      if (c== 13){
        Serial.println();
      }
      Serial.write(c);
      c = c ^ (new_key_in & 0XFF);
      Serial3.write(c);
      new_key_in = next_key(shared_secret_key_input);
      shared_secret_key_input = new_key_in;
    }
    if (Serial3.available()>0) {
      c = Serial3.read();
      c = c ^ (new_key_out & 0XFF);
      if (c== 13){
        Serial.println();
      }
      Serial.write(c);
      new_key_out = next_key(shared_secret_key_output);
      shared_secret_key_output = new_key_out;
    }
  }
  return;
}
/*
This function puts together all other of the functions (except setup, finish, and main)
in order to run the program properly with the intended use. It also declares and
defines the prime and generator to be used.
*/
void run(){
  uint32_t prime = 2147483647;
  uint32_t generator = 16807;
  uint32_t private_key = random_generator();
  uint32_t my_public_key = get_my_public_key(private_key,generator,prime);
  uint32_t your_public_key=share_public_keys(my_public_key);
  uint32_t shared_secret_key = pow_mod(your_public_key, private_key,prime);
  chat(shared_secret_key);
  return;
}
/*
This function ensures that the serial monitors always have time to finsih printing
and never just end abruptly in the middle of communication. (Since chat runs
indefinitely until stopped this is not an issue but is still proper form.)
*/
void finish(){
  Serial3.end();
  Serial.end();
  return;
}
/*
Finally we have the main function which calls the setup function to setup the
Arduinos, the run fucntion to run the code for its intended purpose, and the finish
function to end the serial communication for the Arduinos.
*/
int main(){
  setup();
  run();
  finish();
  return 0;
}
