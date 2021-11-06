
/*
 *  8051 ICSP Programmer Loader
 *
 *  Created on: Dec 25, 2020
 *
 *  Author: Coskun ERGAN
 *
 *  V-1.0
 */

#include <stdio.h>
#include <FastGPIO.h>
#include <CRC32.h>
#include "TimerOne.h"

//------ Config ------
#define STREAM_TIMEOUT      75  // 750mS
#define FAIL_BLINK_PERIDOD  5   // 50mS
#define LOAD_BLINK_PERIDOD  50  // 500mS
#define BEEP_FAIL_PERIDOD   100 // 1S
#define BEEP_SUCCES_PERIDOD 5   // 50mS
#define RX_BUFFER_SIZE      64  // bytes
#define CLK_PIN             3   // O-PIN
#define DTA_PIN             2   // IO-PIN
#define VCC_PIN             4   // O-PIN
#define BEEP_PIN            5   // O-PIN
#define BOOT_KEY            "BOOT"  // 4 Char String 
//--------------------

#define CLK_HIGH()  (FastGPIO::Pin<CLK_PIN>::setOutputValueHigh())
#define CLK_LOW()  (FastGPIO::Pin<CLK_PIN>::setOutputValueLow())

#define DTA_HIGH()  (FastGPIO::Pin<DTA_PIN>::setOutputValueHigh())
#define DTA_LOW()  (FastGPIO::Pin<DTA_PIN>::setOutputValueLow())
#define DTA_READ() (FastGPIO::Pin<DTA_PIN>::isInputHigh())
#define DTA_INPUT() (pinMode(DTA_PIN, INPUT))//(FastGPIO::Pin<DTA_PIN>::setInput())
#define DTA_OUTPUT() (pinMode(DTA_PIN, OUTPUT))//(FastGPIO::Pin<DTA_PIN>::setOutputValue(0))

#define VCC_ON()  (FastGPIO::Pin<VCC_PIN>::setOutputValueHigh())
#define VCC_OFF()  (FastGPIO::Pin<VCC_PIN>::setOutputValueLow())

#define BEEP_ON()  (FastGPIO::Pin<BEEP_PIN>::setOutputValueHigh())
#define BEEP_OFF()  (FastGPIO::Pin<BEEP_PIN>::setOutputValueLow())

#define CLK_TOGGLE() (FastGPIO::Pin<CLK_PIN>::setOutputValueToggle())
#define DTA_TOGGLE() (FastGPIO::Pin<DTA_PIN>::setOutputValueToggle())

#define NOP()  __asm__ __volatile__ ("nop\n\t");

typedef enum
{
    LED_OFF,
    LED_ON,
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
    };
} Parameters_t;

const char Key_Reg[] = BOOT_KEY;

//---------------------------------
extern const char WriteStart[], WriteSeperate[], WriteFinish[], WriteInitalize1[], WriteInitalize2[], WriteTestData[];
extern const char ReadInitalize[], ReadSeperate[], EraseStep4[], EraseStep5[], EraseStep6[], EraseStep7[], EraseStep8[], EraseStep9[], EraseStep10[], EraseStep11[], EraseStep12[];
extern const char WriteStep1[], WriteStep2[];
Led_State_t Led_State;
Procces_State_t Procces_State;
Stream_State_t Stream_State;
Parameters_t Parameters;
Burn_Stage_t Burn_Stage;
CRC32 crc;
uint8_t BurnChapter;
uint8_t RxBuffer[RX_BUFFER_SIZE];
uint8_t RxStart;
uint8_t RxEnd;
uint8_t BeeperTimeout;
uint8_t StreamTimeout;
uint8_t LedTimeout;
uint32_t Image_Size;
uint32_t Crc32;
/***********************************************************/
/***********************************************************/
/***********************************************************/
bool Pop_Byte(uint8_t *byt)
{
    if(RxEnd - RxStart == 0)
    {
        return false;
    }
    *byt = RxBuffer[RxStart];
    if(++RxStart >= RX_BUFFER_SIZE)
    {
        RxStart = 0;
    }
    return true;
}
/***********************************************************/
void Push_Byte(uint8_t byt)
{
    if((RxEnd - RxStart) % RX_BUFFER_SIZE == (RX_BUFFER_SIZE - 1))
    {
        /* Avoid overflow */
        return;
    }
    RxBuffer[RxEnd] = byt;
    if(++RxEnd >= RX_BUFFER_SIZE)
    {
        RxEnd = 0;
    }
}
/***********************************************************/
void ISR_Time_Tick(void) // ISR per 10mS
{
    //----------------------
    if(StreamTimeout > 0)
    {
        if(--StreamTimeout == 0)
        {
            Stream_State = TIMEOUT;
        }
    }
    //-----------------------
    if(BeeperTimeout)
    {
        if(--BeeperTimeout == 0)
        {
            BEEP_OFF();
        }
        else if(Parameters.Beepon == true)
        {
            BEEP_ON();
        }
    }
    //-----------------------
    switch(Led_State)
    {
        case LED_OFF:
            digitalWrite(LED_BUILTIN, LOW);
            break;
        case LED_ON:
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        case LED_FAIL:
            if(LedTimeout == 0)
            {
                LedTimeout = FAIL_BLINK_PERIDOD;
                digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            }
            else
            {
                LedTimeout--;
            }
            break;
        case LED_BUSY:
            if(LedTimeout == 0)
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
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    DTA_HIGH();
    VCC_ON();
    for(i = 0; i < 315; i++)
    {
        DTA_TOGGLE(); // 750Hz
        for(j = 0; j < 60; j++)
        {
            CLK_TOGGLE();
            delayMicroseconds(12); // 45KHz
        }
    }
    DTA_LOW();
    for(j = 0; j < 120; j++)
    {
        CLK_TOGGLE();
        delayMicroseconds(12); // 45KHz
    }
    CLK_LOW();
    delay(1);
    interrupts();
}
/***********************************************************/
void SendData(const char *databuffer)
{
    int i = 0;
    char character;
    do
    {
        CLK_LOW();
        character = pgm_read_byte_near(databuffer + i);
        if(character == '0')
        {
            DTA_LOW();
            delayMicroseconds(2);
            CLK_HIGH();
        }
        else if(character  == '1')
        {
            DTA_HIGH();
            delayMicroseconds(2);
            CLK_HIGH();
        }
        else
        {
            DTA_LOW();
            delayMicroseconds(10);
        }
        i++;
        delayMicroseconds(2);
    }
    while(character != '\0');
    DTA_LOW();
    CLK_LOW();
}
/***********************************************************/
void SendByte(uint8_t temp)
{
    uint8_t i;

    for(i = 0; i < 8; i++)
    {
        if(temp & 0x1)
        {
            DTA_HIGH();
        }
        else
        {
            DTA_LOW();
        }
        delayMicroseconds(2);
        CLK_HIGH();
        temp >>= 1;
        delayMicroseconds(2);
        CLK_LOW();
    }
    CLK_HIGH();
    delayMicroseconds(2);
    CLK_LOW();
    DTA_LOW();
}
/***********************************************************/
uint8_t ReadByte(void)
{
    uint8_t temp, i;
    DTA_INPUT();
    for(i = 0; i < 9; i++)
    {
        temp >>= 1;
        CLK_HIGH();
        if(DTA_READ())
        {
            temp |= 0x80;
        }
        delayMicroseconds(2);
        CLK_LOW();
        delayMicroseconds(2);
    }
    DTA_OUTPUT();
    DTA_LOW();
    return temp;
}
/***********************************************************/
void SendPreamble_Holtek(uint32_t bits)
{
    DTA_OUTPUT();
    DTA_HIGH();
    delayMicroseconds(2);
    for(int i = 0; i < 511; i++)
    {
        CLK_HIGH();
        delayMicroseconds(4);
        CLK_LOW();
        delayMicroseconds(4);
    }
    for(int i = 0; i < 21; i++)
    {
        CLK_HIGH();
        delayMicroseconds(2);
        if(bits & (1UL << i))
        {
            DTA_HIGH();
        }
        else
        {
            DTA_LOW();
        }
        delayMicroseconds(6);
        CLK_LOW();
        delayMicroseconds(3);
    }
}
/***********************************************************/
void SendData_Holtek(const char *databuffer)
{
    int i = 0;
    char character;
    DTA_OUTPUT();
    DTA_HIGH();
    delayMicroseconds(200);
    CLK_HIGH();
    delayMicroseconds(45);
    do
    {
        CLK_HIGH();
        delayMicroseconds(3);
        character = pgm_read_byte_near(databuffer + i);
        if(character == '0')
        {
            DTA_LOW();
        }
        else //if(character  == '1')
        {
            DTA_HIGH();
        }
        delayMicroseconds(3);
        CLK_LOW();
        i++;
        if(i == 32)
        {
            DTA_INPUT();
        }
        if((i % 8) == 0)
        {
            delayMicroseconds(6);
        }
        delayMicroseconds(3);
    }
    while(character != '\0');
    DTA_OUTPUT();
}
/***********************************************************/
void SendByte_Holtek(uint8_t temp)
{
    uint8_t i;
    DTA_OUTPUT();
    for(i = 0; i < 8; i++)
    {
        if(temp & 0x1)
        {
            DTA_HIGH();
        }
        else
        {
            DTA_LOW();
        }
        delayMicroseconds(3);
        CLK_HIGH();
        temp >>= 1;
        delayMicroseconds(3);
        CLK_LOW();
        delayMicroseconds(3);
    }
    DTA_LOW();
}
/***********************************************************/
uint8_t ReadByte_Holtek(void)
{
    uint8_t temp, i;
    DTA_INPUT();
    for(i = 0; i < 8; i++)
    {
        temp >>= 1;
        CLK_HIGH();
        if(DTA_READ())
        {
            temp |= 0x80;
        }
        delayMicroseconds(3);
        CLK_LOW();
        delayMicroseconds(3);
    }
    DTA_LOW();
    return temp;
}
/***********************************************************/
void ReadFile_Holtek(uint32_t img_size)
{
    uint32_t total_counter = 0;
    uint8_t temp;

    crc.reset();
    while(total_counter++ < img_size)
    {
        temp = ReadByte_Holtek();
        crc.update(temp);
        if((total_counter % 8) == 0)
        {
            TwoBitFast_Holtek();
        }
    }
}
/***********************************************************/
void TwoBitFast_Holtek(void)
{
    CLK_HIGH();
    delayMicroseconds(3);
    CLK_LOW();
    delayMicroseconds(3);
    CLK_HIGH();
    delayMicroseconds(3);
    CLK_LOW();
    delayMicroseconds(3);
}
/***********************************************************/
void TwoBitSlow_Holtek(void)
{
    CLK_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(3);
    CLK_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(3);
}
/***********************************************************/
/***********************************************************/
/***********************************************************/
void WritePrepare_Holtek(void)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    CLK_LOW();
    DTA_LOW();
    VCC_ON();
    delay(20);
    noInterrupts();
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19); // 20 bit 1
    for(int i = 0; i < 8; i++)
    {
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        TwoBitFast_Holtek();
    }
    interrupts();
    delay(20);
    noInterrupts();
    SendData_Holtek(WriteStep2);
    SendPreamble_Holtek(0);
    interrupts();
}
/***********************************************************/
void WriteFinish_Holtek(void)
{
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19); // 20 bit 1
    for(int i = 0; i < 8; i++)
    {
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        TwoBitFast_Holtek();
    }

    //------------------
    SendData_Holtek(EraseStep12);
    SendPreamble_Holtek(1); // 1.bit
    DTA_OUTPUT();
    DTA_LOW();
    delayMicroseconds(5);
    CLK_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(3);
    CLK_HIGH();
    delayMicroseconds(3);
    DTA_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(5);
    CLK_HIGH();
    delayMicroseconds(5);
    DTA_LOW();
    delayMicroseconds(400);
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    interrupts();
}
/***********************************************************/
/***********************************************************/
/***********************************************************/
void ReadChip_Holtek(uint32_t file_size)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    CLK_LOW();
    DTA_LOW();
    VCC_ON();
    delay(20);
    noInterrupts();
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19); // 20 bit 1
    for(int i = 0; i < 8; i++)
    {
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        TwoBitFast_Holtek();
    }
    interrupts();
    delay(20);
    noInterrupts();
    SendData_Holtek(ReadInitalize);
    SendPreamble_Holtek(0);
    DTA_INPUT();
    CLK_HIGH();
    delayMicroseconds(3);
    CLK_LOW();
    delayMicroseconds(3);
    ReadFile_Holtek(file_size);
    interrupts();
}
/***********************************************************/
/***********************************************************/
/***********************************************************/
void EreaseFullChip_Holtek(void)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    CLK_LOW();
    DTA_LOW();
    VCC_ON();
    delay(20);
    noInterrupts();
    //------------------
    SendData_Holtek(EraseStep4);
    SendPreamble_Holtek(1UL << 4); // 5.bit
    for(int i = 0; i < 4; i++)
    {
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        TwoBitFast_Holtek();
    }
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep5);
    SendPreamble_Holtek(1UL << 6); // 7.bit
    SendByte_Holtek(1);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitFast_Holtek();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep6);
    SendPreamble_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitSlow_Holtek();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep7);
    SendPreamble_Holtek(1UL << 6); // 7.bit
    SendByte_Holtek(1);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitFast_Holtek();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep8);
    SendPreamble_Holtek(1UL << 4); // 5. bit
    for(int i = 0; i < 4; i++)
    {
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        TwoBitSlow_Holtek();
    }
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep9);
    SendPreamble_Holtek(1UL << 4); // 5. bit
    for(int i = 0; i < 4; i++)
    {
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        TwoBitFast_Holtek();
    }
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep10);
    SendPreamble_Holtek(1UL << 6); // 7.bit
    SendByte_Holtek(1);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitFast_Holtek();
    delay(100);
    //------------------
    SendData_Holtek(EraseStep11);
    SendPreamble_Holtek((3UL << 3) | 1); // 1-4-5.bit
    DTA_OUTPUT();
    DTA_LOW();
    CLK_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(3);
    CLK_HIGH();
    DTA_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(5);
    CLK_HIGH();
    delayMicroseconds(5);
    CLK_LOW();
    SendByte_Holtek(9);
    SendByte_Holtek(0);
    TwoBitSlow_Holtek();
    TwoBitSlow_Holtek();
    TwoBitSlow_Holtek();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep12);
    SendPreamble_Holtek(1); // 1.bit
    DTA_OUTPUT();
    DTA_LOW();
    delayMicroseconds(5);
    CLK_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(3);
    CLK_HIGH();
    delayMicroseconds(3);
    DTA_HIGH();
    delayMicroseconds(2500);
    CLK_LOW();
    delayMicroseconds(5);
    CLK_HIGH();
    delayMicroseconds(5);
    DTA_LOW();
    delayMicroseconds(400);
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    interrupts();
    VCC_OFF();
    DTA_LOW();
    CLK_LOW();
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
}
/***********************************************************/
void LoaderHandler(void)
{
    static uint8_t temp;
    static uint8_t byte_counter;
    static uint32_t total_counter;

    switch(Procces_State)
    {
        case IDLE:
            if(Parameters.Burn)
            {
                Parameters.Burn = false;
                Burn_Stage = BEGIN;
                Procces_State = BURN;
                Led_State = LED_BUSY;
            }
            else if(Parameters.Check)
            {
                Parameters.Check = false;
                Procces_State = CHECK;
                Led_State = LED_BUSY;
            }
            else if(Parameters.Vddon)
            {
                VCC_ON();
            }
            else
            {
                VCC_OFF();
            }
            break;
        case BURN:
            switch(Burn_Stage)
            {
                case BEGIN:
                    if(Parameters.holtek)
                    {
                        EreaseFullChip_Holtek();
                        delay(100);
                        WritePrepare_Holtek();
                    }
                    else
                    {
                        SendPreamble();
                        SendData(WriteInitalize1);
                        delay(32);
                        SendData(WriteInitalize2);
                        delay(30);
                        SendData(WriteStart);
                    }
                    total_counter = 0;
                    byte_counter = 0;
                    crc.reset();
                    Burn_Stage = DATA_WRITE;
                    break;
                case DATA_WRITE:
                    if(Pop_Byte(&temp) == true)
                    {
                        if(Parameters.holtek)
                        {
                            SendByte_Holtek(temp);
                        }
                        else
                        {
                            SendByte(temp);
                            delayMicroseconds(8);
                        }
                        crc.update(temp);
                        if(++byte_counter >= 64)
                        {
                            byte_counter = 0;
                            if(Parameters.holtek)
                            {
                                TwoBitSlow_Holtek();
                            }
                            else
                            {
                                SendData(WriteSeperate);
                            }
                        }
                        total_counter++;
                    }
                    else if(total_counter >= Image_Size)
                    {
                        if(Parameters.holtek)
                        {
                            WriteFinish_Holtek();
                        }
                        else
                        {
                            while(byte_counter++ < 64)
                            {
                                SendByte(0xFF); // padding
                                delayMicroseconds(30);
                            }
                            SendData(WriteFinish);
                        }
                        VCC_OFF();
                        pinMode(DTA_PIN, INPUT);
                        pinMode(CLK_PIN, INPUT);
                        delay(500);
                        Led_State = LED_ON;
                        Procces_State = IDLE;
                    }
                    break;
            }
            break;
        case CHECK:
            if(Crc32 != crc.finalize())
            {
                Led_State = LED_FAIL;
                BeeperTimeout = BEEP_FAIL_PERIDOD;
            }
            else
            {
                if(Parameters.holtek)
                {
                    ReadChip_Holtek(Image_Size);
                }
                else
                {
                    SendPreamble();
                    SendData(ReadInitalize);
                    crc.reset();
                    total_counter = 0;
                    while(total_counter++ < Image_Size)
                    {
                        temp = ReadByte();
                        crc.update(temp);
                        if((total_counter % 64) == 0)
                        {
                            SendData(ReadSeperate);
                        }
                    }
                }
                if(Crc32 != crc.finalize())
                {
                    Led_State = LED_FAIL;
                    BeeperTimeout = BEEP_FAIL_PERIDOD;
                }
                else
                {
                    Led_State = LED_ON;
                    BeeperTimeout = BEEP_SUCCES_PERIDOD;
                }
            }
            VCC_OFF();
            pinMode(DTA_PIN, INPUT);
            pinMode(CLK_PIN, INPUT);
            delay(500);
            Procces_State = IDLE;
            break;
    }
}
/***********************************************************/
void DataHandler(void)
{
    static uint8_t byte_counter = 0;
    uint8_t incomingByte;

    switch(Stream_State)
    {
        case WAIT:
            if(Pop_Byte(&incomingByte) == false)
            {
                break;
            }
            switch(byte_counter)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    if(incomingByte != Key_Reg[byte_counter])
                    {
                        byte_counter = 0;
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
                    Serial.print(F("Size:"));
                    Serial.println(Image_Size, DEC);
                    Serial.print(F("CRC:"));
                    Serial.println(Crc32, HEX);
                    if(Parameters.Burn)
                    {
                        Stream_State = DOWNLOADING;
                    }
                    else
                    {
                        byte_counter = 0;
                        StreamTimeout = 0;
                        Led_State = LED_ON;
                    }
                    break;
            }
            byte_counter++;
            break;
        case DOWNLOADING:
            if(Procces_State == IDLE)
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
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(CLK_PIN, INPUT);
    pinMode(DTA_PIN, INPUT);
    pinMode(VCC_PIN, OUTPUT);
    pinMode(BEEP_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println(F("Restart!\r\n\r\n\r\n\r\n"));

    Timer1.initialize(10000); // per 10mS
    Timer1.attachInterrupt(ISR_Time_Tick);

    Led_State = LED_OFF;
    Procces_State = IDLE;
    Stream_State = WAIT;
    Parameters.Value = 0;
    RxStart = RxEnd = 0;
    BeeperTimeout = LedTimeout = StreamTimeout = 0;
}
/***********************************************************/
void loop(void)
{
    DataHandler();
    LoaderHandler();
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
