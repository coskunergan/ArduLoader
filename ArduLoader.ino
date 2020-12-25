
#include <stdio.h>
#include <FastGPIO.h>
#include <CRC32.h>
#include "TimerOne.h"

#define STREAM_TIMEOUT 75 // 750mS

#define FAIL_BLINK_PERIDOD  5 // 50mS
#define LOAD_BLINK_PERIDOD  50 // 500mS

#define BEEP_FAIL_PERIDOD  100 // 1S
#define BEEP_SUCCES_PERIDOD  5 // 50mS

#define RX_BUFFER_SIZE  64

#define CLK_PIN  3
#define DTA_PIN  2
#define VCC_PIN  4
#define BEEP_PIN 5

#define CLK_HIGH()  (FastGPIO::Pin<CLK_PIN>::setOutputValueHigh())
#define CLK_LOW()  (FastGPIO::Pin<CLK_PIN>::setOutputValueLow())

#define DTA_HIGH()  (FastGPIO::Pin<DTA_PIN>::setOutputValueHigh())
#define DTA_LOW()  (FastGPIO::Pin<DTA_PIN>::setOutputValueLow())
#define DATA_READ() (FastGPIO::Pin<DTA_PIN>::isInputHigh())

#define VCC_HIGH()  (FastGPIO::Pin<VCC_PIN>::setOutputValueHigh())
#define VCC_LOW()  (FastGPIO::Pin<VCC_PIN>::setOutputValueLow())

#define BEEP_HIGH()  (FastGPIO::Pin<BEEP_PIN>::setOutputValueHigh())
#define BEEP_LOW()  (FastGPIO::Pin<BEEP_PIN>::setOutputValueLow())

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

typedef union
{
    uint8_t Value;
    struct
    {
        unsigned pageDone: 1;
        unsigned imageDone: 1;
    };
} Flag_t;


const char BOOT_KEY[] = "BOOT";

//---------------------------------
extern const char WriteStart[], WriteSeperate[], WriteFinish[], WriteInitalizeData1[], WriteInitalizeData2[], WriteTestData[];
extern const char ReadInitalize[], ReadSeperate[];
CRC32 crc;
uint8_t StreamTimeout;
uint8_t LedTimeout;
Led_State_t Led_State;
Procces_State_t Procces_State;
Stream_State_t Stream_State;
uint32_t image_length;
uint8_t BurnChapter;
uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxstart = 0;
uint8_t rxend = 0;
Flag_t Flags;
uint8_t WriteStep;
uint32_t Image_Size;
uint32_t Crc32;
uint8_t BeeperTimeout;
/***********************************************************/
/***********************************************************/
/***********************************************************/
bool rx_pop(uint8_t *b)
{
    if(rxend - rxstart == 0)
    {
        return false;
    }

    *b = rxBuffer[rxstart];

    if(++rxstart >= RX_BUFFER_SIZE)
    {
        rxstart = 0;
    }
    return true;
}
/***********************************************************/
void rx_push(uint8_t b)
{
    if((rxend - rxstart) % RX_BUFFER_SIZE == (RX_BUFFER_SIZE - 1))
    {
        /* Avoid overflow */
        return;
    }
    rxBuffer[rxend] = b;
    if(++rxend >= RX_BUFFER_SIZE)
    {
        rxend = 0;
    }
}
/***********************************************************/
void ISR_Time_Tick(void) // per 10mS
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
    if(BeeperTimeout > 0)
    {
        if(--BeeperTimeout == 0)
        {
            BEEP_LOW();
        }
        else
        {
            BEEP_HIGH();
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
        case LED_BURN:
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
    DTA_HIGH();
    for(j = 0; j < 30; j++)
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
    for(j = 0; j < 30; j++)
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
    for(i = 0; i < 315; i++)
    {
        DTA_TOGGLE(); // 750Hz
        for(j = 0; j < 60; j++)
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
    for(j = 0; j < 120; j++)
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
        if(character == '0')
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
        else if(character  == '1')
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
    }
    while(character != '\0');
}
/***********************************************************/
void SendByte(uint8_t temp)
{
    uint8_t i;
    for(i = 0; i < 9; i++)
    {
        if(temp & 0x1)
        {
            DTA_HIGH();
        }
        else
        {
            DTA_LOW();
        }
        temp >>= 1;
        delayMicroseconds(2);
        CLK_HIGH();
        delayMicroseconds(2);
        CLK_LOW();
    }
    delayMicroseconds(10);
}
/***********************************************************/
uint8_t ReadByte(void)
{
    uint8_t temp, i;
    pinMode(DTA_PIN, INPUT);
    for(i = 0; i < 9; i++)
    {
        temp >>= 1;
        CLK_HIGH();
        if(DATA_READ())
        {
            temp |= 0x80;
        }
        delayMicroseconds(2);
        CLK_LOW();
        delayMicroseconds(2);
    }
    pinMode(DTA_PIN, OUTPUT);
    return temp;
}
/***********************************************************/
void LoaderHandler(void)
{
    static uint8_t byte_counter;
    static uint32_t total_counter;
    static uint8_t temp;

    switch(Procces_State)
    {
        case IDLE:
            VCC_LOW();
            WriteStep = 0;
            break;
        case BURN:
            Led_State = LED_BURN;
            switch(WriteStep)
            {
                case 0:
                    SendPreamble();
                    delay(1);
                    SendData(WriteInitalizeData1);
                    delay(32);
                    SendData(WriteInitalizeData2);
                    delay(30);
                    SendData(WriteStart);
                    WriteStep = 1;
                    total_counter = 0;
                    byte_counter = 0;
                    crc.reset();
                    break;
                case 1:
                    if(rx_pop(&temp) == true)
                    {
                        SendByte(temp);
                        crc.update(temp);
                        if(++byte_counter >= 64)
                        {
                            byte_counter = 0;
                            SendData(WriteSeperate);
                        }
                        total_counter++;
                    }
                    else if(total_counter >= Image_Size)
                    {
                        while(byte_counter++ < 64)
                        {
                            SendByte(0xFF); // padding
                            delayMicroseconds(16);
                        }
                        SendData(WriteFinish);
                        Procces_State = CHECK;
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
                VCC_LOW();
                delay(500);
                SendPreamble();
                delay(1);
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
            if(rx_pop(&incomingByte) == false)
            {
                break;
            }
            switch(byte_counter++)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    if(incomingByte != BOOT_KEY[byte_counter - 1])
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
                    Procces_State = (Procces_State_t)incomingByte;
                    Serial.print(F("Size:"));
                    Serial.println(Image_Size, DEC);
                    Serial.print(F("CRC:"));
                    Serial.println(Crc32, HEX);
                    switch(Procces_State)
                    {
                        case BURN:
                            Stream_State = DOWNLOADING;
                            break;
                        case CHECK:
                            break;
                    }
                    byte_counter = 0;
                    break;
            }
            break;
        case DOWNLOADING:
            break;
        case TIMEOUT:            
            byte_counter = 0;
            Stream_State = WAIT;
            Procces_State = IDLE;
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
    pinMode(BEEP_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println(F("Restart!"));

    Timer1.initialize(10000); // per 10mS
    Timer1.attachInterrupt(ISR_Time_Tick);

    StreamTimeout = 0;
    LedTimeout = 0;
    Led_State = LED_OFF;
    Procces_State = IDLE;
    Stream_State = WAIT;
    Flags.Value = 0;
}
/***********************************************************/
void loop()
{
    DataHandler();
    LoaderHandler();
}
/***********************************************************/
void serialEvent()
{
    while(Serial.available())
    {
        rx_push(Serial.read());
    }
    StreamTimeout = STREAM_TIMEOUT;
}
/***********************************************************/
