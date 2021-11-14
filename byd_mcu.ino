/* BYD MCU Subfunctions */
/***********************************************************/
/***********************************************************/
/***********************************************************/
void SendPreamble_BYD(void)
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
void SendData_BYD(const char *databuffer)
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
void SendByte_BYD(uint8_t temp)
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
uint8_t ReadByte_BYD(void)
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
/***********************************************************/
/***********************************************************/