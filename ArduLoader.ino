
#include <CRC32.h>
#include "TimerOne.h"

#define UART_TIMEOUT 10 // 100mS

#define FAIL_BLINK_PERIDOD  5 // 50mS
#define LOAD_BLINK_PERIDOD  50 // 500mS

CRC32 crc;

typedef enum
{
  LED_OFF,
  LED_ON,
  LED_FAIL,
  LED_LOAD
} Led_State_t;

//---------------------------------
uint8_t incomingByte;
uint8_t UartTimeout;
uint8_t LedTimeout;
bool TimeoutFlag;
Led_State_t Led_State;
uint16_t bytes;
uint32_t crc32;
/***********************************************************/
/***********************************************************/
/***********************************************************/
void ISR_Time_Tick(void) // per 10mS
{
  //----------------------
  if (UartTimeout > 0)
  {
    if (--UartTimeout == 0)
    {
      TimeoutFlag = true;
    }
  }
  //-----------------------
  switch (Led_State)
  {
    case LED_OFF:
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case LED_ON:
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    case LED_FAIL:
      if (LedTimeout == 0)
      {
        LedTimeout = FAIL_BLINK_PERIDOD;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }
      else
      {
        LedTimeout--;
      }
      break;
    case LED_LOAD:
      if (LedTimeout == 0)
      {
        LedTimeout = LOAD_BLINK_PERIDOD;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }
      else
      {
        LedTimeout--;
      }
      break;
  }
  //-----------------------
}
/***********************************************************/
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println(F("Restart!"));

  Timer1.initialize(10000); // per 10mS
  Timer1.attachInterrupt(ISR_Time_Tick);

  bytes = 0;
  UartTimeout = 0;
  LedTimeout = 0;
  TimeoutFlag = false;
  Led_State = LED_OFF;
}
/***********************************************************/
void loop()
{
  if (TimeoutFlag == true)
  {
    TimeoutFlag = false;
    bytes = 0;
    if (crc32 == crc.finalize())
    {
      Led_State = LED_ON;
    }
    else
    {
      Led_State = LED_FAIL;
    }
  }
}
/***********************************************************/
void serialEvent()
{
  while(Serial.available())
  {    
    incomingByte = Serial.read();
    UartTimeout = UART_TIMEOUT;
    Led_State = LED_LOAD;
    switch (bytes++)
    {
        case 0:
          crc32 = incomingByte;
          break;
        case 1:
          crc32 |= ((uint32_t)incomingByte << 8);
          break;
        case 2:
          crc32 |= ((uint32_t)incomingByte << 16);
          break;
        case 3:
          crc32 |= ((uint32_t)incomingByte << 24);
          crc.reset();
          break;
      default:        
        crc.update(incomingByte);
        break;
    }
  }
}
/***********************************************************/
