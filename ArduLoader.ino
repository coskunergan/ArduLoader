
#include <stdio.h>
#include <FastGPIO.h>
#include <CRC32.h>
#include "TimerOne.h"

#define STREAM_TIMEOUT 50 // 500mS

#define FAIL_BLINK_PERIDOD  5 // 50mS
#define LOAD_BLINK_PERIDOD  50 // 500mS

#define CLK_PIN  3
#define DTA_PIN  2
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

typedef enum
{
  LED_OFF,
  LED_ON,
  LED_FAIL,
  LED_BURN
} Led_State_t;

typedef enum
{
  IDLE,
  BURN,
  CHECK
} Procces_State_t;

typedef enum
{
  WAIT,
  DOWNLOADING,
  TIMEOUT
} Stream_State_t;

const char BOOT_KEY[] = "BOOT";

//---------------------------------
extern const PROGMEM char WriteStart[], WriteSeperate[], WriteFinish[], WriteInitalizeData1[], WriteInitalizeData2[], WriteTestData[];
CRC32 crc;
uint8_t incomingByte;
uint8_t StreamTimeout;
uint8_t LedTimeout;
Led_State_t Led_State;
Procces_State_t Procces_State;
Stream_State_t Stream_State;
uint32_t image_length;
uint8_t BurnChapter;
uint8_t StreamBuffer[64];
bool pageDone;
bool incomingDone;
uint8_t WriteStep;
/***********************************************************/
/***********************************************************/
/***********************************************************/
void ISR_Time_Tick(void) // per 10mS
{
  //----------------------
  if (StreamTimeout > 0)
  {
    if (--StreamTimeout == 0)
    {
      Stream_State = TIMEOUT;
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
    case LED_BURN:
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
  int i, j;
  noInterrupts();
  DTA_HIGH();
  for (j = 0; j < 30; j++)
  {
    CLK_TOGGLE();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    delayMicroseconds(11); // 45KHz
  }
  VCC_HIGH();
  for (j = 0; j < 30; j++)
  {
    CLK_TOGGLE();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    delayMicroseconds(11); // 45KHz
  }
  for (i = 0; i < 315; i++)
  {
    DTA_TOGGLE(); // 750Hz
    for (j = 0; j < 60; j++)
    {
      CLK_TOGGLE();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      _NOP();
      delayMicroseconds(11); // 45KHz
    }
  }
  DTA_LOW();
  for (j = 0; j < 120; j++)
  {
    CLK_TOGGLE();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    _NOP();
    delayMicroseconds(11); // 45KHz
  }
  CLK_LOW();
  interrupts();
}
/***********************************************************/
void SendData(const char *databuffer)
{
  int i;
  char character;
  do
  {
    CLK_LOW();
    character = pgm_read_byte_near(databuffer + i);
    i++;
    if (character == '0')
    {
      DTA_LOW();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
      CLK_HIGH();
    }
    else if (character  == '1')
    {
      DTA_HIGH();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
      NOP();
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
void LoaderHandler(void)
{
  switch (Procces_State)
  {
    case IDLE:
      WriteStep = 0;
      break;
    case BURN:
      switch (WriteStep++)
      {
        case 0:
          SendPreamble();
          delay(1);
          SendData(WriteInitalizeData1);
          delay(32);
          SendData(WriteInitalizeData2);
          delay(30);
          SendData(WriteTestData);
          break;
        case 1:
          SendData(WriteSeperate);
          break;
        case 2:
          SendData(WriteTestData);
          break;
        case 3:
          SendData(WriteFinish);
          VCC_LOW();
          Procces_State = CHECK;
          break;
      }
      break;
    case CHECK:
      Procces_State = IDLE;
      break;
  }
}
/***********************************************************/
void DataHandler(void)
{
  static uint8_t byte_counter = 0;
  static uint32_t crc32, image_length, total_counter;

  StreamTimeout = STREAM_TIMEOUT;

  switch (Stream_State)
  {
    case WAIT:
      switch (byte_counter++)
      {
        case 0:
        case 1:
        case 2:
        case 3:
          if (incomingByte != BOOT_KEY[byte_counter-1])
          {
            byte_counter=0;
          }
          break;
        case 4:
          crc32 = incomingByte;
          break;
        case 5:
          crc32 |= ((uint32_t)incomingByte << 8);
          break;
        case 6:
          crc32 |= ((uint32_t)incomingByte << 16);
          break;
        case 7:
          crc32 |= ((uint32_t)incomingByte << 24);
          break;
        case 8:
          image_length = incomingByte;
          break;
        case 9:
          image_length |= ((uint32_t)incomingByte << 8);
          break;
        case 10:
          image_length |= ((uint32_t)incomingByte << 16);
          break;
        case 11:
          image_length |= ((uint32_t)incomingByte << 24);
          break;
        case 12:
          Procces_State = (Procces_State_t)incomingByte;
          switch (Procces_State)
          {
            case BURN:
              Stream_State = DOWNLOADING;
              break;
            case CHECK:
              break;
          }
          crc.reset();
          byte_counter = 0;
          total_counter = 0;
          break;
      }
      break;
    case DOWNLOADING:
      crc.update(incomingByte);
      StreamBuffer[byte_counter] = incomingByte;
      byte_counter++;
      total_counter++;
      if (total_counter >= image_length)
      {
        if (crc32 != crc.finalize())
        {
          Led_State = LED_FAIL;
          byte_counter = 0;
          Stream_State = WAIT;
        }
        else
        {
          while (byte_counter < 64)
          {
            StreamBuffer[byte_counter++] = 0xFF; // padding
          }
        }
      }
      if (byte_counter >= 64)
      {
        byte_counter = 0;
        pageDone = true;
      }
      break;
    case TIMEOUT:
      Led_State = LED_OFF;
      byte_counter = 0;
      Stream_State = WAIT;
      break;
  }
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

  StreamTimeout = 0;
  LedTimeout = 0;
  Led_State = LED_OFF;
  Procces_State = IDLE;
  Stream_State=WAIT;
  pageDone = false;
  incomingDone = false;
}
/***********************************************************/
void loop()
{
  if (incomingDone)
  {
    incomingDone = false;
    DataHandler();
  }
  LoaderHandler();
}
/***********************************************************/
void serialEvent()
{
  while (Serial.available())
  {
    incomingByte = Serial.read();
    incomingDone = true;
  }
}
/***********************************************************/
