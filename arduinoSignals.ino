/*
  PFC
*/
 

// PINS in use
int led = 13;
int DHT11_pin = 2;
int IR_pin = 4;
int RF_pin = 7;


// ERRORS
#define DHTLIB_OK		1
#define DHTLIB_ERROR_CHECKSUM	-1
#define DHTLIB_ERROR_TIMEOUT	-2




// Debug mode 0, 1
#define debug_ 0

// the setup routine runs once when you press reset:
void setup() 
{                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  
  pinMode(IR_pin, OUTPUT);
  digitalWrite(IR_pin, LOW);
  
  pinMode(RF_pin, OUTPUT);
  digitalWrite(RF_pin, LOW);
  
  
  
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for x miliseconds
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait for x miliseconds
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for x miliseconds
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait for x miliseconds
  
  Serial.begin(9600);

}


// Read temperature and humidity
// 1- MCU send start signal: 18ms low + 20-40ms high
// 2- DHT Response: 20us low + 80us high
// 3- DHT sends data: 50us low + (26-28us high for 0 or 70us high for 1)
void DHT11()
{
  
  if (debug_)
    Serial.println("DHT11 to the rescue");
  
  
  // BUFFER TO RECEIVE
  uint8_t bits[5];
  uint8_t cnt = 7;
  uint8_t idx = 0;

  // Variables
  int temperature_ = 0;
  int humidity_ = 0;
  int status_ = DHTLIB_OK;


  // EMPTY BUFFER
  for (int i=0; i< 5; i++) bits[i] = 0;
  
  // REQUEST SAMPLE
  pinMode(DHT11_pin, OUTPUT);
  digitalWrite(DHT11_pin, LOW);
  delay(18);
  digitalWrite(DHT11_pin, HIGH);
  delayMicroseconds(40);
  pinMode(DHT11_pin, INPUT);
  
  
  // ACKNOWLEDGE or TIMEOUT
  unsigned int loopCnt = 10000;
  while(digitalRead(DHT11_pin) == LOW && (status_ >= 0))
    if (loopCnt-- == 0) 
    {
      status_ = DHTLIB_ERROR_TIMEOUT;
    }
    
  loopCnt = 10000;
  while(digitalRead(DHT11_pin) == HIGH && (status_ >= 0))
    if (loopCnt-- == 0) 
    {
      status_ = DHTLIB_ERROR_TIMEOUT;
    }


  // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
  if (status_ >= 0)
  {
    for (int i=0; i<40; i++)
    {
      loopCnt = 10000;
      while(digitalRead(DHT11_pin) == LOW && (status_ >= 0))
      {
        if (loopCnt-- == 0) 
        {
          status_ = DHTLIB_ERROR_TIMEOUT;
        }
      }
        
      unsigned long t = micros();
  
      loopCnt = 10000;
      while(digitalRead(DHT11_pin) == HIGH && (status_ >= 0))
      {
        if (loopCnt-- == 0) 
        {
          status_ = DHTLIB_ERROR_TIMEOUT;
        }
      }
        
      if ((micros() - t) > 40) bits[idx] |= (1 << cnt);
      if (cnt == 0)   // next byte?read through that post and was unable to res
      {
        cnt = 7;    // restart at MSB
        idx++;      // next byte!
      }
      else cnt--;
    }
  }
  

  
  // WRITE TO RIGHT VARS
  // as bits[1] and bits[3] are allways zero they are omitted in formulas.
  humidity_    = bits[0]; 
  temperature_ = bits[2]; 

  uint8_t sum = bits[0] + bits[2];  

  if (bits[4] != sum) 
    status_ = DHTLIB_ERROR_CHECKSUM;
  
  
  
  // Raspi return
  // Ej:{"status":"1", "response":"OK", "payload":[{"temperature":"43", "humidity":"88"}]}
  if (status_ == DHTLIB_OK)
  {
    Serial.print("{\"status\": ");          // {"status":
    Serial.print(status_);                  // 1
    Serial.print(", \"response\": \"");     // , "response":"
    Serial.print("OK");                     // OK
    Serial.print("\", \"payload\": [{");    // ", "payload":[{
    Serial.print("\"temperature\": ");      // "temperature": 
    Serial.print(temperature_);             // 43
    Serial.print(", \"humidity\": ");       // , "humidity": 
    Serial.print(humidity_);                // 88
    Serial.println("}]}\r\n");                  // }]}
  }
  else if (status_ == DHTLIB_ERROR_CHECKSUM)
  {
    Serial.print("{\"status\": ");              // {"status":
    Serial.print(status_);                      // -1
    Serial.print(", \"response\": \"");         // , "response":"
    Serial.print("Error: Bad Checksum");        // Error: Bad Checksum
    Serial.print("\", \"payload\": []}\r\n");   // ", "payload":[None]}
  }
  else if (status_ == DHTLIB_ERROR_TIMEOUT)
  {
    Serial.print("{\"status\": ");              // {"status":
    Serial.print(status_);                      // -1
    Serial.print(", \"response\": \"");         // , "response":"
    Serial.print("Error: Timeout!");            // Error: Timeout
    Serial.print("\", \"payload\": []}\r\n");   // ", "payload":[None]}
  }
  else
  {
    Serial.print("{\"status\": ");              // {"status":
    Serial.print(status_);                      // -1
    Serial.print(", \"response\": \"");         // , "response":"
    Serial.print("Error: Unknown!");            // Error: Unknowkn
    Serial.print("\", \"payload\": []}\r\n");   // ", "payload":[None]}
  }
  
  
  
  return;
  
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------

// To send a '1', it must be in 38Khz carrier, so... the inverse is 26.31 microsec
void IR_NEC_CARRIER_ON(unsigned int timer_microseconds)
{
  const int carrier_delay = 9; // Should be 9 (+ 3/4 of changing would make 12/13)
  for(int i=0; i < (timer_microseconds / 26); i++)
  {
    
    digitalWrite(IR_pin, HIGH); //turn on the IR LED
    //NOTE: digitalWrite takes about 3.5us to execute, so we need to factor that into the timing.
    delayMicroseconds(carrier_delay); //delay for 13us (9us + digitalWrite), half the carrier frequnecy
    digitalWrite(IR_pin, LOW); //turn off the IR LED
    delayMicroseconds(carrier_delay); //delay for 13us (9us + digitalWrite), half the carrier frequnecy
  }
}
  
  
// NEC Protocol: (not extending the address)
// 1: leader code: ON 9ms + OFF 4,5ms
// 2: Payload: addr + ~addr + data + ~data
// times: addr + ~addr = 27ms, data + ~data = 27ms, total = 67,5ms(including leader code) , total + wait = 108ms (until beggining od next leader code)
// sizes: addr = 8 bits, data = 8 bits
// representation of 0: pulse distance of 1,125ms: 562us + 562us
// representation of 1: pulse distance of 2,250ms 562us + 1675us
void IR_NEC()
{
  
  const int leader_on = 8000; // Should be 9000
  const int leader_off = 4000; // Should be 4500
  const int bit_on = 560; // Should be 562
  const int zero_off = 565; // Should be 562
  const int one_off = 1690; // Should be 1686
  
  byte inByte = 0x00;
  int param_num = 4;
  
  //unsigned long code = 0b00000000111101110010000011011111; // 00 F7 20 DF Red?
  unsigned long code = 0x00000000;
  


  // Read id + channel + on/off
  for (int j=0; j<param_num; j++)
  {
    while (Serial.available() == 0)
    {
      delay(1);
    }
    
    if (Serial.available() > 0)
    {
      inByte = Serial.read();
      code <<= 8;
      code += inByte;
    } else {
      // Error
      Serial.print("{\"status\":-1, \"respose\":\"Error reading code\", \"payload\":[]}\r\n");
    }
  }
  


  
  // Initialization of protocol
  IR_NEC_CARRIER_ON(leader_on);
  digitalWrite(IR_pin, LOW); //ir_off();
  delayMicroseconds(leader_off);
  
  
  // Send a byte
  for (int i=0;i<32;i++) // MSB first (Most significatyve byte first)
  {
    // 
    IR_NEC_CARRIER_ON(bit_on);
    if (code & 0x80000000)
    {
      // A 1 is 
      delayMicroseconds(one_off);
    } else
    {
      // A 0 is 
      delayMicroseconds(zero_off);
    }
    code<<=1; // shift to the next bit
  }  
  // Stop bit (Just a low bit)
    IR_NEC_CARRIER_ON(bit_on);
    
    
    Serial.print("{\"status\":1, \"respose\":\"OK\", \"payload\":[");
    //Serial.print(0);
    Serial.print("]}\r\n");
    
    
}



// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------


// 4 bits (A, B, C or D) + 0101 + 4bits (Channel 1 or 2) + 0101 + 0001 + 4bits (ON/OFF)
// 0: ON (on_short) + OFF(off_long)
// 1: ON (on_long) + OFF (off_short)
void RF_B402B()
{
    // Sample = 22.676
  const int on_short = 394; // 18 samples at 44100Hz -> 408 us -- < 19 >17
  const int on_long  = 1025; //  45 samples at 44100Hz -> 1020 us -- < 46
  const int off_short = 316; // 13 samples at 44100Hz -> 249.4 us -- > 12
  const int off_long  = 816; // 36 samples at 44100Hz -> 884.4 us -- < 39
  const int end_delay  = 9720;
  const int repeat = 6;
  
  byte code[3] = {0x00, 0x00, 0x00};
  const byte code_test[3] =  {0b00000101, 0b00000101, 0b00010101}; // Ch 1 - A - ON (050515)
    
  byte inByte = 0b00000000;
  
  // Clean code
  code[0] = 0b00000000;
  code[1] = 0b00000000;
  code[2] = 0b00000000;


  // Read id + channel + on/off
  for (int j=0; j<3; j++)
  {
    while (Serial.available() == 0)
    {
      delay(1);
    }
    
    if (Serial.available() > 0)
    {
      inByte = Serial.read();
      code[j] = inByte;
    } else {
      // Error
      Serial.print("{\"status\":-1, \"respose\":\"Error reading code\", \"payload\":[]}\r\n");
    }
  }

  

  // Repeat n times
  for (int j = 0; j < repeat; j++)
  { 
    // send 3 bytes
    for (unsigned char i=0; i<3; i++) 
    {
      // Send each bit
      for (unsigned char k=0; k<8; k++)
      {
        if ((code[i]>>(7-k)) & 1) // If '1' bit
        {
          //Serial.print("1");
          digitalWrite(RF_pin, HIGH);
          delayMicroseconds(on_long);
          digitalWrite(RF_pin, LOW);    
          delayMicroseconds(off_short);
        } else { // If '0' bit
          //Serial.print("0");
          digitalWrite(RF_pin, HIGH);
          delayMicroseconds(on_short);
          digitalWrite(RF_pin, LOW);    
          delayMicroseconds(off_long);
        }
      }
    }

    // Final bit
    digitalWrite(RF_pin, HIGH);    
    delayMicroseconds(on_short);
    digitalWrite(RF_pin, LOW);
    delayMicroseconds(off_long);
    delayMicroseconds(end_delay);
    
  }
  Serial.print("{\"status\":1, \"respose\":\"OK\", \"payload\":[]}\r\n");
}



// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------

// the loop routine runs over and over again forever:
void loop() 
{

  char order = '0';
  
  // check if there is the order byte available
  if (Serial.available() > 0) 
  {
    order = Serial.read();
  
    switch(order)
    {
      case 0x01:
        DHT11();
        break;
        
      case 0x02:
        IR_NEC();
        break;
        
      case 0x03:
        RF_B402B();
        break;

      default:
        Serial.print("{\"status\":-1, \"respose\":\"Error command not found\", \"payload\":[]}\r\n");
        break;
    }
  
  
  }
  
  // Delay x ms
  delay(1000);

  

}
