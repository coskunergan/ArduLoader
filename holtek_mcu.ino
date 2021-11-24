/* Holtek MCU Subfunctions */
/***********************************************************/
/***********************************************************/
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
        temp >>= 1;
        CLK_LOW();
        delayMicroseconds(2);
        CLK_HIGH();
        delayMicroseconds(2);
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
    total_counter = 0;
    crc.reset();
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
    delayMicroseconds(1920);//1925
    CLK_LOW();
    delayMicroseconds(2);
    CLK_HIGH();
    delayMicroseconds(20);//20
}
/***********************************************************/
void WritePrepare_Holtek(void)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    CLK_LOW();
    DTA_LOW();
    VCC_ON();
    delay(VDD_ON_DELAY);
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19); // 20 bit 1
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
    DTA_LOW();
    CLK_LOW();
    delay(20);
    DTA_HIGH();
    delayMicroseconds(50);
    SendData_Holtek(WriteStep2);
    SendPreamble_Holtek(0);
}
/***********************************************************/
void ReadChip_Holtek(uint32_t file_size)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    CLK_LOW();
    DTA_LOW();
    VCC_ON();
    delay(VDD_ON_DELAY);
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19); // 20 bit 1
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
    DTA_OUTPUT();
    DTA_LOW();
    CLK_LOW();
    delay(20);
    SendData_Holtek(ReadInitalize);
    SendPreamble_Holtek(0);
    ReadFile_Holtek(file_size);
    DTA_OUTPUT();
    DTA_LOW();
    CLK_LOW();
}
/***********************************************************/
void EreaseFullChip_Holtek(void)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);    
    DTA_LOW();
    VCC_ON();
    CLK_HIGH();
    delay(VDD_ON_DELAY);
    //------------------
    CLK_LOW();
    delayMicroseconds(300);
    SendData_Holtek(EraseStep4);
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
    CLK_LOW();
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
    CLK_LOW();
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
    CLK_LOW();
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
    CLK_LOW();
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
    CLK_LOW();
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
    CLK_LOW();
    delay(100);
    //------------------
    SendData_Holtek(EraseStep11);
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
    SendData_Holtek(EraseStep12);
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
    delayMicroseconds(1000);
    //------------------
    VCC_OFF();
    DTA_LOW();
    CLK_LOW();
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    delay(300);
}
/***********************************************************/
void WriteFinish_Holtek(void)
{
    // CLK_LOW();
    // delayMicroseconds(300);    
    // SendData_Holtek(WriteStep1);
    // SendPreamble_Holtek(1UL << 19); // 20 bit 1
    // DTA_INPUT();
    // for(int i = 0; i < 8; i++)
    // {
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     ReadByte_Holtek();
    //     TwoBitFast_Holtek();
    // }
    // //------------------
    // CLK_LOW();
    // delayMicroseconds(300);
    // SendData_Holtek(EraseStep12);
    // SendPreamble_Holtek(1); // 1.bit
    // DTA_LOW();
    // CLK_LOW();
    // delayMicroseconds(5);
    // CLK_HIGH();
    // delayMicroseconds(2500);
    // CLK_LOW();
    // delayMicroseconds(3);
    // CLK_HIGH();
    // delayMicroseconds(3);
    // DTA_HIGH();
    // delayMicroseconds(2500);
    // CLK_LOW();
    // delayMicroseconds(5);
    // CLK_HIGH();
    // delayMicroseconds(5);
    // DTA_LOW();
    // delayMicroseconds(2500);
    // CLK_LOW();
    // delayMicroseconds(1000);
    //------------------    
    CLK_LOW();
    delayMicroseconds(1000);
    SendData_Holtek(EraseStep10);
    SendPreamble_Holtek(1UL << 6);
    SendByte_Holtek(1);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    SendByte_Holtek(0);
    TwoBitFast_Holtek();
    delayMicroseconds(300);
    CLK_LOW();
    //------------------
    delayMicroseconds(300);
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(0);
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
    delayMicroseconds(300);
    CLK_LOW();
    //------------------
    delayMicroseconds(300);   
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 4);
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
    delay(50);
    CLK_LOW();    
    delayMicroseconds(300);  
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 4);
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
    delay(50);
    CLK_LOW();     
    delayMicroseconds(300);  
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 4);
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
    delayMicroseconds(700);      
    CLK_LOW();     
    delayMicroseconds(300);  
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19);
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
    delayMicroseconds(2800);   
    CLK_LOW();     
    delayMicroseconds(300);      
}
/***********************************************************/
void HIRC_Calibration_Holtek(void)
{
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    CLK_LOW();
    DTA_LOW();
    VCC_ON();
    delay(VDD_ON_DELAY);
    SendData_Holtek(WriteStep1);
    SendPreamble_Holtek(1UL << 19); // 20 bit 1
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
    //------------------
    delayMicroseconds(300);
    SendData_Holtek(EraseStep10);
    SendPreamble_Holtek((1UL << 6) | (1UL << 19));
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
    //------------------
    delayMicroseconds(300);
    SendData_Holtek(EraseStep10);
    SendPreamble_Holtek((1UL << 5) | (1UL << 19));
    for(int i = 0; i < 8; i++)
    {
        if(i == 7)
        {
            SendByte_Holtek(3);
        }
        else
        {
            SendByte_Holtek(0);
        }
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
    //------------------
    delayMicroseconds(300);
    SendData_Holtek(EraseStep10);
    SendPreamble_Holtek((1UL << 5) | (1UL << 19));
    for(int i = 0; i < 8; i++)
    {
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        if(i == 0)
        {
            SendByte_Holtek(0x20);
        }
        else
        {
            SendByte_Holtek(0);
        }
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        TwoBitFast_Holtek();
    }
    DTA_LOW();
    delayMicroseconds(2800);
    CLK_LOW();
    //------------------
    delayMicroseconds(300);
    SendData_Holtek(EraseStep10);
    SendPreamble_Holtek((1UL << 5) | (1UL << 19));
    for(int i = 0; i < 8; i++)
    {
        if(i == 7)
        {
            SendByte_Holtek(3);
        }
        else
        {
            SendByte_Holtek(0);
        }
        SendByte_Holtek(0);
        if(i == 0)
        {
            SendByte_Holtek(0x20);
        }
        else
        {
            SendByte_Holtek(0);
        }
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        SendByte_Holtek(0);
        TwoBitFast_Holtek();
    }
    DTA_INPUT();
    CLK_LOW();
    VCC_OFF();
    DTA_LOW();
    CLK_LOW();
    pinMode(DTA_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    delay(300);
}
/***********************************************************/
/***********************************************************/
/***********************************************************/
