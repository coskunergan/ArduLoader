
/*
    8051 ICSP Programmer Loader

    Created on: Dec 25, 2020

    Author: Coskun ERGAN

    V-1.0
*/

#include <stdio.h>
#include <FastGPIO.h>
#include <CRC32.h>
#include "TimerOne.h"
#include <avr/wdt.h>

//------ Config ------
#define STREAM_TIMEOUT      75  // 750mS
#define FAIL_BLINK_PERIDOD  5   // 50mS
#define LOAD_BLINK_PERIDOD  20  // 500mS
#define BEEP_FAIL_PERIDOD   100 // 1S
#define BEEP_SUCCES_PERIDOD 5   // 50mS
#define RX_BUFFER_SIZE      1024 // bytes 
#define MTP_HEADER_BUFFER_SIZE  128 // bytes 
#define CLK_PIN             2   // O-PIN
#define DTA_PIN             3   // IO-PIN
#define VCC_PIN             4   // O-PIN 
#define BEEP_PIN            5   // O-PIN
#define GREEN_LED_PIN       6   // O-PIN
#define RED_LED_PIN         7   // O-PIN
#define FW_SELECT_PIN_ARRAY  {A0,A1,A2,A3,A4,A5,8,9,10,11}
#define START_BUTTON_PIN    12  // I-PIN
#define BOOT_KEY            "BOOT"  // 4 Char String 
#define BEEP_KEY            "Beep"  // 4 Char String 
#define VDD_ON_DELAY        30
#define VDD_TURN_OFF_DELAY  300 // 3sn
//--------------------
#define KEYBOARD_STREAM_TIMEOUT   2  // 20mS

#define RED_LED_OFF()  (FastGPIO::Pin<RED_LED_PIN>::setOutputValueHigh())
#define RED_LED_ON()  (FastGPIO::Pin<RED_LED_PIN>::setOutputValueLow())
#define GREEN_LED_OFF()  (FastGPIO::Pin<GREEN_LED_PIN>::setOutputValueHigh())
#define GREEN_LED_ON()  (FastGPIO::Pin<GREEN_LED_PIN>::setOutputValueLow())
#define RED_LED_TOGGLE() (FastGPIO::Pin<RED_LED_PIN>::setOutputValueToggle())
#define GREEN_LED_TOGGLE() (FastGPIO::Pin<GREEN_LED_PIN>::setOutputValueToggle())

#define CLK_HIGH()  (FastGPIO::Pin<CLK_PIN>::setOutputValueHigh())
#define CLK_LOW()  (FastGPIO::Pin<CLK_PIN>::setOutputValueLow())
#define DTA_HIGH()  (FastGPIO::Pin<DTA_PIN>::setOutputValueHigh())
#define DTA_LOW()  (FastGPIO::Pin<DTA_PIN>::setOutputValueLow())
#define DTA_READ() (FastGPIO::Pin<DTA_PIN>::isInputHigh())
#define CLK_READ() (FastGPIO::Pin<CLK_PIN>::isInputHigh())
#define DTA_INPUT() (pinMode(DTA_PIN, INPUT_PULLUP))
#define DTA_OUTPUT() (pinMode(DTA_PIN, OUTPUT))
#define CLK_INPUT() (pinMode(CLK_PIN, INPUT_PULLUP))
#define CLK_OUTPUT() (pinMode(CLK_PIN, OUTPUT))
#define CLK_TOGGLE() (FastGPIO::Pin<CLK_PIN>::setOutputValueToggle())
#define DTA_TOGGLE() (FastGPIO::Pin<DTA_PIN>::setOutputValueToggle())

#define VCC_ON()  (FastGPIO::Pin<VCC_PIN>::setOutputValueHigh())
#define VCC_OFF()  (FastGPIO::Pin<VCC_PIN>::setOutputValueLow())

#define BEEP_ON()  (FastGPIO::Pin<BEEP_PIN>::setOutputValueHigh())
#define BEEP_OFF()  (FastGPIO::Pin<BEEP_PIN>::setOutputValueLow())

#define NOP()  __asm__ __volatile__ ("nop\n\t");

typedef enum
{
  LED_OFF,
  LED_SUCCES,
  LED_FAIL,
  LED_BUSY
} Led_State_t;

typedef enum
{
  IDLE,
  BURN,
  CHECK
} Procces_State_t;

typedef enum
{
  BS86D20A,
  BS66F360
} Device_Type_t;

typedef enum
{
  WAIT,
  DOWNLOADING,
  TIMEOUT
} Stream_State_t;

typedef enum
{
  BEGIN,
  DATA_WRITE
} Burn_Stage_t;

typedef union
{
  uint8_t Value;
  struct
  {
    unsigned Burn: 1;
    unsigned Check: 1;
    unsigned Vddon: 1;
    unsigned Beepon: 1;
    unsigned holtek: 1;
    unsigned VddTurnOff: 1;
  };
} Parameters_t;

typedef enum
{
  PIN_LOW = 0,
  PIN_HIGH,
  PIN_FLOAT
} PinState_t;

const char Key_Reg[] = BOOT_KEY;
const char Key_Beep[] = BEEP_KEY;

//---------------------------------
extern const char WriteStart[], WriteSeperate[], WriteFinish[], WriteInitalize1[], WriteInitalize2[], WriteTestData[];
extern const char ReadInit[], ReadInitalize[], ReadSeperate[];
Led_State_t Led_State;
Procces_State_t Procces_State;
Stream_State_t Stream_State;
Parameters_t Parameters;
Burn_Stage_t Burn_Stage;
CRC32 crc;
uint8_t BurnChapter;
uint8_t RxBuffer[RX_BUFFER_SIZE];
uint8_t MtpHeaderBuffer[MTP_HEADER_BUFFER_SIZE];
uint16_t RxStart;
uint16_t RxEnd;
uint8_t BeeperTimeout;
uint8_t StreamTimeout;
uint8_t LedTimeout;
uint32_t Image_Size;
uint32_t Crc32;
uint32_t Total_Counter;
uint32_t FileSize_Counter;
uint8_t KeyboadStreamTimeout;
uint8_t checksum;
uint8_t Mtp_Header_Size;
bool Mtp_Header_Flag;
uint16_t VddTurnOff_Timer;
Device_Type_t Select_Device;
unsigned int select_fw;
/***********************************************************/
/***********************************************************/
/***********************************************************/
void Recovery_Buffer(uint16_t size)
{
  if (RxStart >= size)
  {
    RxStart -= size;
  }
  else
  {
    RxStart += RX_BUFFER_SIZE;
    RxStart -= size;
  }
}
/***********************************************************/
bool Pop_Byte(uint8_t *byt)
{
  if (RxEnd == RxStart)
  {
    return false;
  }
  *byt = RxBuffer[RxStart];
  if (++RxStart >= RX_BUFFER_SIZE)
  {
    RxStart = 0;
  }
  return true;
}
/***********************************************************/
void Push_Byte(uint8_t byt)
{
  //if(((uint16_t)(RxEnd - RxStart)) % RX_BUFFER_SIZE == (RX_BUFFER_SIZE - 1))
  //{
  /* Avoid overflow */
  //return;
  //}
  RxBuffer[RxEnd] = byt;
  if (++RxEnd >= RX_BUFFER_SIZE)
  {
    RxEnd = 0;
  }
}
/***********************************************************/
void Fifo_Flush(void)
{
  RxEnd = RxStart = 0;
}
/***********************************************************/
void ISR_Time_Tick(void) // ISR per 10mS
{
  //----------------------
  if (KeyboadStreamTimeout > 0)
  {
    if (--KeyboadStreamTimeout  == 0)
    {
      if (checksum < 16)
      {
        Serial.print('0');
      }
      Serial.print((uint8_t)checksum, HEX);
      checksum = 0;
    }
  }
  //----------------------
  if (StreamTimeout > 0)
  {
    if (--StreamTimeout == 0)
    {
      Stream_State = TIMEOUT;
    }
  }
  //-----------------------
  if (BeeperTimeout)
  {
    if (--BeeperTimeout == 0)
    {
      BEEP_OFF();
    }
    else if (Parameters.Beepon == true)
    {
      BEEP_ON();
    }
  }
  //-----------------------
  if (VddTurnOff_Timer > 0)
  {
    if (--VddTurnOff_Timer == 0)
    {
      Parameters.Vddon = false;
    }
  }
  //-----------------------
  switch (Led_State)
  {
    case LED_OFF:
      RED_LED_ON();
      GREEN_LED_OFF();
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case LED_SUCCES:
      RED_LED_OFF();
      GREEN_LED_ON();
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    case LED_FAIL:
      if (LedTimeout == 0)
      {
        LedTimeout = FAIL_BLINK_PERIDOD;
        RED_LED_ON();
        GREEN_LED_OFF();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }
      else
      {
        LedTimeout--;
      }
      break;
    case LED_BUSY:
      if (LedTimeout == 0)
      {
        LedTimeout = LOAD_BLINK_PERIDOD;
        RED_LED_TOGGLE();
        GREEN_LED_TOGGLE();
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
void TurnOn_Device(void)
{
  pinMode(DTA_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  if (Parameters.holtek)
  {
    DTA_LOW();
    CLK_LOW();
    VCC_ON();
    delay(VDD_ON_DELAY);
  }
  else
  {
    CLK_HIGH();
    VCC_ON();
  }
}
/***********************************************************/
void TurnOff_Device(void)
{
  pinMode(DTA_PIN, INPUT);
  pinMode(CLK_PIN, INPUT);
  VCC_OFF();
  delay(300);
}
/***********************************************************/
void LoaderHandler(void)
{
  static uint8_t temp;
  static uint8_t byte_counter;

  switch (Procces_State)
  {
    case IDLE:
      if (Parameters.Burn)
      {
        Parameters.Burn = false;
        Burn_Stage = BEGIN;
        Procces_State = BURN;
        Led_State = LED_BUSY;
      }
      else if (Parameters.Check)
      {
        Parameters.Check = false;
        Procces_State = CHECK;
        Led_State = LED_BUSY;
      }
      else if (Parameters.Vddon)
      {
        VCC_ON();
      }
      else
      {
        VCC_OFF();
      }
      break;
    case BURN:
      switch (Burn_Stage)
      {
        case BEGIN:
          TurnOn_Device();
          if (Parameters.holtek)
          {
            EreaseFullChip_Holtek();
            WriteCalibration_Holtek(Select_Device);
            TurnOff_Device();
            TurnOn_Device();
            WritePrepare_Holtek();
            Mtp_Header_Size = 0;
            Mtp_Header_Flag = false;
          }
          else
          {
            SendPreamble_BYD();
            SendData_BYD(WriteInitalize1);
            delay(32);
            SendData_BYD(WriteInitalize2);
            delay(30);
            SendData_BYD(WriteStart);
          }
          Total_Counter = 0;
          byte_counter = 0;
          crc.reset();
          Burn_Stage = DATA_WRITE;
          break;
        case DATA_WRITE:
          if (Pop_Byte(&temp))
          {
            crc.update(temp);
            if (Parameters.holtek)
            {
              if (Mtp_Header_Flag == false)
              {
                MtpHeaderBuffer[Mtp_Header_Size] = temp;
                if (MtpHeaderBuffer[Mtp_Header_Size] == 0 && \
                    MtpHeaderBuffer[Mtp_Header_Size - 1] == 0 && \
                    MtpHeaderBuffer[Mtp_Header_Size - 2] == 0 && \
                    MtpHeaderBuffer[Mtp_Header_Size - 3] == 0)
                {
                  Mtp_Header_Flag = true;
                  FileSize_Counter = (16384 * (Image_Size / 16384)) + Mtp_Header_Size;
                }
                byte_counter = 255;
                Mtp_Header_Size++;
              }
              else
              {
                if (Total_Counter < FileSize_Counter)
                {
                  SendByte_Holtek(temp);
                }
              }
              if (++byte_counter >= 64)
              {
                byte_counter = 0;
                TwoBitSlow_Holtek_W();
              }
            }
            else
            {
              SendByte_BYD(temp);
              //delayMicroseconds(8);
              if (++byte_counter >= 64)
              {
                byte_counter = 0;
                SendData_BYD(WriteSeperate);
              }
            }
            Total_Counter++;
          }
          else if (Total_Counter >= Image_Size)
          {
            if (!Parameters.holtek)
            {
              while (byte_counter++ < 64)
              {
                SendByte_BYD(0xFF); // padding
                delayMicroseconds(30);
              }
              SendData_BYD(WriteFinish);
            }
            TurnOff_Device();
            StreamTimeout = 0;
            //Serial.print(F("CRC_LOAD:"));
            //Serial.println(crc.finalize(), HEX);
            if (Crc32 != crc.finalize())
            {
              Led_State = LED_FAIL;
              BeeperTimeout = BEEP_FAIL_PERIDOD;
              Parameters.Check = false;
              Parameters.Vddon = false;
            }
            else
            {
              Led_State = LED_SUCCES;
              Procces_State = IDLE;
            }
          }
          break;
      }
      break;
    case CHECK:
      TurnOn_Device();
      crc.reset();
      if (Parameters.holtek)
      {
        byte_counter = 0;
        while (byte_counter < Mtp_Header_Size)
        {
          crc.update(MtpHeaderBuffer[byte_counter++]);
        }
        ReadChip_Holtek(16384 * (Image_Size / 16384));
        Recovery_Buffer((Image_Size - Mtp_Header_Size) % 16384);
        while (Pop_Byte(&temp))
        {
          crc.update(temp);
        }
      }
      else
      {
        SendPreamble_BYD();
        SendData_BYD(ReadInit);
        uint16_t total_counter = 0;
        while (total_counter++ < Image_Size)
        {
          temp = ReadByte_BYD();
          crc.update(temp);
          if ((total_counter % 64) == 0)
          {
            SendData_BYD(ReadSeperate);
          }
        }
      }
      //Serial.print(F("CRC_READ:"));
      //Serial.println(crc.finalize(), HEX);
      if (Crc32 != crc.finalize())
      {
        Led_State = LED_FAIL;
        BeeperTimeout = BEEP_FAIL_PERIDOD;
        Parameters.Vddon = false;
        Serial.println(F("ERROR"));
      }
      else
      {
        if ((Parameters.holtek) && (ReadCalibration_Holtek(Select_Device) == false))
        {
          Led_State = LED_FAIL;
          BeeperTimeout = BEEP_FAIL_PERIDOD;
          Parameters.Vddon = false;
          Serial.println(F("ERROR"));
        }
        else
        {
          Led_State = LED_SUCCES;
          BeeperTimeout = BEEP_SUCCES_PERIDOD;
          Serial.println(F("SUCCES"));
          if (Parameters.VddTurnOff)
          {
            VddTurnOff_Timer = VDD_TURN_OFF_DELAY;
          }
        }
      }
      TurnOff_Device();
      Procces_State = IDLE;
      break;
  }
}
/***********************************************************/
void DataHandler(void)
{
  static uint8_t byte_counter = 0;
  uint8_t incomingByte;

  switch (Stream_State)
  {
    case WAIT:
      if (Pop_Byte(&incomingByte) == false)
      {
        break;
      }
      switch (byte_counter)
      {
        case 0:
        case 1:
        case 2:
        case 3:
          if (incomingByte != Key_Reg[byte_counter])
          {
            if (incomingByte != Key_Beep[byte_counter])
            {
              byte_counter = 0;
            }
            else if (byte_counter == 3)
            {
              byte_counter = 0;
              StreamTimeout = 0;
              BeeperTimeout = BEEP_SUCCES_PERIDOD;
            }
          }
          break;
        case 4:
          Crc32 = (uint32_t)incomingByte;
          break;
        case 5:
          Crc32 |= ((uint32_t)incomingByte << 8);
          break;
        case 6:
          Crc32 |= ((uint32_t)incomingByte << 16);
          break;
        case 7:
          Crc32 |= ((uint32_t)incomingByte << 24);
          break;
        case 8:
          Image_Size = (uint32_t)incomingByte;
          break;
        case 9:
          Image_Size |= ((uint32_t)incomingByte << 8);
          break;
        case 10:
          Image_Size |= ((uint32_t)incomingByte << 16);
          break;
        case 11:
          Image_Size |= ((uint32_t)incomingByte << 24);
          break;
        case 12:
          Parameters.Value = incomingByte;
          //Serial.print(F("Size:"));
          //Serial.println(Image_Size, DEC);
          //Serial.print(F("CRC:"));
          //Serial.println(Crc32, HEX);
          if (Parameters.holtek)
          {
            //Serial.println(F("Holtek"));
            if (Image_Size > 32768)
            {
              Select_Device = BS66F360;
            }
            else
            {
              Select_Device = BS86D20A;
            }
          }
          if (Parameters.Burn)
          {
            Stream_State = DOWNLOADING;
          }
          else
          {
            byte_counter = 0;
            StreamTimeout = 0;
            Led_State = LED_SUCCES;
          }
          break;
      }
      byte_counter++;
      break;
    case DOWNLOADING:
      if (Procces_State == IDLE)
      {
        byte_counter = 0;
        StreamTimeout = 0;
        Stream_State = WAIT;
      }
      break;
    case TIMEOUT:
      Led_State = LED_FAIL;
      BeeperTimeout = BEEP_FAIL_PERIDOD;
      byte_counter = 0;
      Stream_State = WAIT;
      Procces_State = IDLE;
      break;
  }
}
/***********************************************************/
void setup(void)
{
  wdt_enable(WDTO_4S);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(DTA_PIN, INPUT);
  pinMode(VCC_PIN, OUTPUT);
  pinMode(BEEP_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);

  const unsigned int pins[10] = FW_SELECT_PIN_ARRAY;

  for (int i = 0; i < 10; i++)
  {
    pinMode(pins[i], INPUT_PULLUP);
  }

  Serial.begin(115200);
  //Serial.println(F("Restart!\r\nV1.0\r\n\r\n\r\n"));

  Timer1.initialize(10000); // per 10mS
  Timer1.attachInterrupt(ISR_Time_Tick);

  Led_State = LED_OFF;

  BeeperTimeout = LedTimeout = StreamTimeout = 0;
  KeyboadStreamTimeout = checksum = 0;

  delay(200);

  Stream_State = WAIT;
  Parameters.Beepon = true;
  RxStart = RxEnd = 0;
  Procces_State = IDLE;

  PinState_t result;
  unsigned int count = 0x3FF;
  select_fw = 0x3FF;
  while (count)
  {
    for (int i = 0; i < 10; i++)
    {
      result = Get_PinState(pins[i]);
      if (result != PIN_FLOAT)
      {
        count &= ~(1 << i);
        select_fw &= ~((unsigned char)result << i);
      }
    }
  }
}
/***********************************************************/
PinState_t Get_PinState(int pin)
{
#define PIN_DEBOUNCE_VALUE 5000
  static int pin_count[21];
  static unsigned char first_step = true;

  if (first_step)
  {
    first_step = false;
    for (int i = 0; i < 21; i++ )
    {
      pin_count[i] = PIN_DEBOUNCE_VALUE / 2;
    }
  }
  if (digitalRead(pin))
  {
    if (pin_count[pin] < PIN_DEBOUNCE_VALUE)
    {
      pin_count[pin]++;
    }
  }
  else
  {
    if (pin_count[pin] > 0)
    {
      pin_count[pin]--;
    }
  }
  if (pin_count[pin] > (PIN_DEBOUNCE_VALUE / 10 ) * 9)
  {
    return PIN_HIGH;
  }
  else if (pin_count[pin] < PIN_DEBOUNCE_VALUE  / 10)
  {
    return PIN_LOW;
  }
  return PIN_FLOAT;
}
/***********************************************************/
void Send_Buff_WithChecksum(char* buff)
{
  unsigned char checksum = 0;
  while (*buff != '\0')
  {
    checksum ^= *buff;
    Serial.print(*buff++);
  }
  if (checksum < 16)
  {
    Serial.print('0');
  }
  Serial.print((unsigned char)checksum, HEX);
}
/***********************************************************/
void Send_Select_Fw(unsigned int sel)
{
  char buff[15] = "FSW0000";

  if (sel & 0x200)
  {
    buff[11] = 'T';
    buff[12] = 'K';
    buff[13] = '\0';
  }
  else
  {
    buff[11] = 'M';
    buff[12] = 'T';
    buff[13] = 'P';
    buff[14] = '\0';
  }

  sel &= ~0x200;
  buff[7] = (sel / 100) + '0';
  buff[8] = ((sel / 10) % 10) + '0';
  buff[9] = (sel % 10) + '0';
  buff[10] = '.';

  Send_Buff_WithChecksum(buff);
}
/***********************************************************/
void loop(void)
{
  //-----------------
  DataHandler();
  LoaderHandler();
  wdt_reset();
  //-----------------
  if (Get_PinState(START_BUTTON_PIN) == PIN_FLOAT)
  {
    if (Get_PinState(START_BUTTON_PIN) == PIN_LOW)
    {
      Send_Select_Fw(select_fw);
    }
  }
  //-----------------
}
/***********************************************************/
void serialEvent(void)
{
    while(Serial.available())
    {
        Push_Byte(Serial.read());
    }
    StreamTimeout = STREAM_TIMEOUT;
}
/***********************************************************/
