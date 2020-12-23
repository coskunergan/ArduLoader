
#include <stdio.h>
#include <FastGPIO.h>
#include <CRC32.h>
#include "TimerOne.h"

#define UART_TIMEOUT 10 // 100mS

#define FAIL_BLINK_PERIDOD  5 // 50mS
#define LOAD_BLINK_PERIDOD  50 // 500mS

#define CLK_PIN  2
#define DTA_PIN  3
#define VCC_PIN  4

#define CLK_HIGH()  (FastGPIO::Pin<CLK_PIN>::setOutputValueHigh())
#define CLK_LOW()  (FastGPIO::Pin<CLK_PIN>::setOutputValueLow())

#define DTA_HIGH()  (FastGPIO::Pin<DTA_PIN>::setOutputValueHigh())
#define DTA_LOW()  (FastGPIO::Pin<DTA_PIN>::setOutputValueLow())

#define VCC_HIGH()  (FastGPIO::Pin<VCC_PIN>::setOutputValueHigh())
#define VCC_LOW()  (FastGPIO::Pin<VCC_PIN>::setOutputValueLow())

#define CLK_TOGGLE() (FastGPIO::Pin<CLK_PIN>::setOutputValueToggle())
#define DTA_TOGGLE() (FastGPIO::Pin<DTA_PIN>::setOutputValueToggle())

#define NOP()  __asm__ __volatile__ ("nop\n\t");

CRC32 crc;

typedef enum
{
  LED_OFF,
  LED_ON,
  LED_FAIL,
  LED_LOAD
} Led_State_t;

//---------------------------------

extern const PROGMEM char Array[];


const char Array2[] PROGMEM = "0101010101010";

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
void SendPreamble(void)
{
  uint8_t i, j;

  for (i = 0; i < 250; i++)
  {
    DTA_TOGGLE(); // 750Hz
    for (j = 0; j < 59; j++)
    {
      CLK_TOGGLE();
      delayMicroseconds(12); // 45KHz
    }
    VCC_HIGH();
  }
  DTA_LOW();
  CLK_LOW();
}
/***********************************************************/
void SendArray(void)
{
  int i;
  char character;
  do
  {
    CLK_LOW();
    character = pgm_read_byte_near(Array + i);
    i++;
    if (character == '0')
    {
      DTA_LOW();
      NOP();
      CLK_HIGH();
    }
    else if (character  == '1')
    {
      DTA_HIGH();
      NOP();
      CLK_HIGH();
    }
    else
    {
      DTA_LOW();
      CLK_LOW();
      delayMicroseconds(10);
    }
    NOP();
    NOP();
  } while (character != '\0');

}
/***********************************************************/
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(DTA_PIN, OUTPUT);
  pinMode(VCC_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println(F("Restart!"));

  Timer1.initialize(10000); // per 10mS
  Timer1.attachInterrupt(ISR_Time_Tick);

  bytes = 0;
  UartTimeout = 0;
  LedTimeout = 0;
  TimeoutFlag = false;
  Led_State = LED_OFF;

  VCC_LOW();

  delay(1000);
  SendPreamble();
  SendArray();
  delay(500);
  VCC_LOW();
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
  while (Serial.available())
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
