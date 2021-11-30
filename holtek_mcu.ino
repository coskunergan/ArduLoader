/* Holtek MCU Subfunctions */
/***********************************************************/
/***********************************************************/
/***********************************************************/
void SendByte_Holtek(uint8_t temp)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        if(temp & 0x1)
        {
            DTA_HIGH();
        }
        else
        {
            DTA_LOW();
        }
        CLK_LOW();
        delayMicroseconds(2);
        CLK_HIGH();
        temp >>= 1;
    }
}
/***********************************************************/
uint8_t ReadByte_Holtek(void)
{
    uint8_t temp, i;
    for(i = 0; i < 8; i++)
    {
        delayMicroseconds(2);
        CLK_LOW();
        temp >>= 1;
        delayMicroseconds(2);
        CLK_HIGH();
        if(DTA_READ())
        {
            temp |= 0x80;
        }
    }
    return temp;
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
        if((i % 8) == 0)
        {
            delayMicroseconds(6);
        }
        i++;
        if(i == 33)
        {
            DTA_INPUT();
        }
        delayMicroseconds(3);
    }
    while(character != '\0');
    DTA_OUTPUT();
}
/***********************************************************/
void SendPreamble_Holtek(uint32_t bits)
{
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
        if(i != 20)
        {
            CLK_LOW();
        }
        delayMicroseconds(3);
    }
}
/***********************************************************/
void ReadFile_Holtek(uint32_t img_size)
{
    uint8_t temp;
    uint16_t total_counter = 0;
    DTA_INPUT();
    while(++total_counter <= img_size)
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
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(2);
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(2);
}
/***********************************************************/
void TwoBitSlow_Holtek(void)
{
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(2000);
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(2000);
}
/***********************************************************/
void TwoBitSlow_Holtek_W(void)
{
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(1925);//1925
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(20);//20
}
/***********************************************************/
void WritePrepare_Holtek(void)
{
    SendData_Holtek(WriteStep2);
    SendPreamble_Holtek(0);
}
/***********************************************************/
void ReadChip_Holtek(uint32_t file_size)
{
    SendData_Holtek(ReadStep1);
    SendPreamble_Holtek(0);
    DTA_INPUT();
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
    CLK_LOW();
    delayMicroseconds(300);
    SendData_Holtek(ReadInitalize);
    SendPreamble_Holtek(0);
    ReadFile_Holtek(file_size);
    CLK_LOW();
}
/***********************************************************/
void EreaseFullChip_Holtek(void)
{
    //------------------
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 4); // 5.bit
    DTA_INPUT();
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep1);
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep2);
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep1);
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep1);
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(WriteStep1);
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep1);
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
    CLK_LOW();
    delayMicroseconds(1000);
    //------------------
    SendData_Holtek(EraseStep3);
    SendPreamble_Holtek((3UL << 3) | 1); // 1-4-5.bit
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
    SendData_Holtek(EraseStep3);
    SendPreamble_Holtek(1); // 1.bit
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
    delayMicroseconds(50);
}
/***********************************************************/
bool WriteCalibration_Holtek(Device_Type_t device)
{
    uint8_t temp;
    SendData_Holtek(EraseStep1);
    SendPreamble_Holtek(1UL << 19);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    if(device == BS86D20A)
    {
        SendByte_Holtek(B00100000);// ??
    }
    else  //if(device==BS66F360)
    {
        SendByte_Holtek(B10000000);// ??
    }
    SendByte_Holtek(B00010101);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00101010);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00111111);
    TwoBitSlow_Holtek_W();
    if(device == BS86D20A)
    {
        SendByte_Holtek(B10110001);// ??
    }
    else  //if(device==BS66F360)
    {
        SendByte_Holtek(B00010010);// ??
    }
    SendByte_Holtek(B00000111);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00010010);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00101101);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00111000);
    TwoBitSlow_Holtek_W();
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00111000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00101101);
    if(device == BS86D20A)
    {
        SendByte_Holtek(B00000000);
    }
    else  //if(device==BS66F360)
    {
        SendByte_Holtek(B00000010);
    }
    SendByte_Holtek(B00010010);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000111);
    TwoBitSlow_Holtek_W();
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00111111);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00101010);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00010101);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    TwoBitSlow_Holtek_W();
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitSlow_Holtek_W();
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitSlow_Holtek_W();
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    if(device == BS86D20A)
    {
        SendByte_Holtek(B00000010);
    }
    else  //if(device==BS66F360)
    {
        SendByte_Holtek(B00111110);
    }
    SendByte_Holtek(B00000000);
    if(device == BS86D20A)
    {
        SendByte_Holtek(B00111101);
    }
    else  //if(device==BS66F360)
    {
        SendByte_Holtek(B00000001);
    }
    SendByte_Holtek(B00000000);
    TwoBitSlow_Holtek_W();
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B00000000);
    SendByte_Holtek(B11100000);
    SendByte_Holtek(B00011010);
    TwoBitSlow_Holtek_W();
    delayMicroseconds(100);
    CLK_LOW();
    DTA_LOW();
    delayMicroseconds(300);
    SendData_Holtek(ReadStep1);
    SendPreamble_Holtek(0);
    DTA_INPUT();
    for(int i = 0; i < 8; i++)
    {
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        if(i == 3)
        {
            temp = ReadByte_Holtek();
        }
        else
        {
            ReadByte_Holtek();
        }
        ReadByte_Holtek();
        ReadByte_Holtek();
        TwoBitFast_Holtek();
    }
    CLK_LOW();
    if(temp != 0x15)
    {
        return false;
    }
    return true;
}
/***********************************************************/
bool ReadCalibration_Holtek(void)
{
    uint8_t temp;
    CLK_LOW();
    DTA_LOW();
    delayMicroseconds(300);
    SendData_Holtek(ReadStep1);
    SendPreamble_Holtek(0);
    DTA_INPUT();
    for(int i = 0; i < 8; i++)
    {
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        ReadByte_Holtek();
        if(i == 3)
        {
            temp = ReadByte_Holtek();
        }
        else
        {
            ReadByte_Holtek();
        }
        ReadByte_Holtek();
        ReadByte_Holtek();
        TwoBitFast_Holtek();
    }
    CLK_LOW();
    if(temp != 0x15)
    {
        return false;
    }
    return true;
}
/***********************************************************/
/***********************************************************/
/***********************************************************/
