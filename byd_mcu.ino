/* BYD MCU Subfunctions */
#define CLK_HIGH_BYD  DTA_HIGH
#define CLK_LOW_BYD   DTA_LOW
#define DTA_HIGH_BYD  CLK_HIGH
#define DTA_LOW_BYD   CLK_LOW
#define DTA_READ_BYD  CLK_READ
#define DTA_INPUT_BYD  CLK_INPUT
#define DTA_OUTPUT_BYD CLK_OUTPUT
#define DTA_TOGGLE_BYD CLK_TOGGLE
#define CLK_TOGGLE_BYD DTA_TOGGLE
/***********************************************************/
/***********************************************************/
/***********************************************************/
void SendPreamble_BYD(void)
{
    int i, j;
    noInterrupts();
    for(i = 0; i < 315; i++)
    {
        DTA_TOGGLE_BYD(); // 750Hz
        for(j = 0; j < 60; j++)
        {
            CLK_TOGGLE_BYD();
            delayMicroseconds(12); // 45KHz
        }
    }
    DTA_LOW_BYD();
    for(j = 0; j < 120; j++)
    {
        CLK_TOGGLE_BYD();
        delayMicroseconds(12); // 45KHz
    }
    CLK_LOW_BYD();
    delay(1);
    interrupts();
}
/***********************************************************/
void SendData_BYD(const char *databuffer)
{
    int i = 0;
    char character;
    do
    {
        CLK_LOW_BYD();
        character = pgm_read_byte_near(databuffer + i);
        if(character == '0')
        {
            DTA_LOW_BYD();
            delayMicroseconds(2);
            CLK_HIGH_BYD();
        }
        else if(character  == '1')
        {
            DTA_HIGH_BYD();
            delayMicroseconds(2);
            CLK_HIGH_BYD();
        }
        else
        {
            DTA_LOW_BYD();
            delayMicroseconds(10);
        }
        i++;
        delayMicroseconds(2);
    }
    while(character != '\0');
    DTA_LOW_BYD();
    CLK_LOW_BYD();
}
/***********************************************************/
void SendByte_BYD(uint8_t temp)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        if(temp & 0x1)
        {
            DTA_HIGH_BYD();
        }
        else
        {
            DTA_LOW_BYD();
        }
        delayMicroseconds(2);
        CLK_HIGH_BYD();
        temp >>= 1;
        delayMicroseconds(2);
        CLK_LOW_BYD();
    }
    CLK_HIGH_BYD();
    delayMicroseconds(2);
    CLK_LOW_BYD();
    DTA_LOW_BYD();
}
/***********************************************************/
uint8_t ReadByte_BYD(void)
{
    uint8_t temp, i;
    DTA_INPUT_BYD();
    for(i = 0; i < 9; i++)
    {
        temp >>= 1;
        CLK_HIGH_BYD();
        if(DTA_READ_BYD())
        {
            temp |= 0x80;
        }
        delayMicroseconds(2);
        CLK_LOW_BYD();
        delayMicroseconds(2);
    }
    DTA_OUTPUT_BYD();
    DTA_LOW_BYD();
    return temp;
}
/***********************************************************/
/***********************************************************/
/***********************************************************/