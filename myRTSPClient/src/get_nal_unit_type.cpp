#include <cstdint>
#include <cmath>

uint32_t Ue(uint8_t* pBuff, uint32_t nLen, uint32_t& nStartBit)
{
    // count 0 bit
    uint32_t nZeroNum = 0;
    while (nStartBit < nLen * 8)
    {
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
            break;
        }
        nZeroNum++;
        nStartBit++;
    }
    nStartBit++;


    // count
    uint32_t dwRet = 0;
    for (auto i = 0; i < nZeroNum; i++)
    {
        dwRet <<= 1;
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
            dwRet += 1;
        }
        nStartBit++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}


int Se(uint8_t* pBuff, uint32_t nLen, uint32_t& nStartBit)
{
    int UeVal = Ue(pBuff, nLen, nStartBit);
    double k = UeVal;
    int nValue = ceil(k / 2);
    if (UeVal % 2 == 0)
    {
        nValue = -nValue;
    }
    return nValue;
}


uint32_t u(uint32_t BitCount, uint8_t* buf, uint32_t& nStartBit)
{
    uint32_t dwRet = 0;
    for (auto i = 0; i < BitCount; i++)
    {
        dwRet <<= 1;
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
            dwRet += 1;
        }
        nStartBit++;
    }
    return dwRet;
}

void de_emulation_prevention(uint8_t* buf, unsigned int* buf_size)
{
    int i = 0, j = 0;
    uint8_t* tmp_ptr = NULL;
    unsigned int tmp_buf_size = 0;
    int val = 0;

    tmp_ptr = buf;
    tmp_buf_size = *buf_size;
    for (i = 0; i < (tmp_buf_size - 2); i++)
    {
        //check for 0x000003
        val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
        if (val == 0) {
            //kick out 0x03
            for (j = i + 2; j < tmp_buf_size - 1; j++)
                tmp_ptr[j] = tmp_ptr[j + 1];

            //and so we should devrease bufsize
            (*buf_size)--;
        }
    }
}

uint32_t get_nal_unit_type(uint8_t* buf, unsigned int nLen)
{
    uint32_t StartBit = 0;
    de_emulation_prevention(buf, &nLen);
    uint32_t forbidden_zero_bit = u(1, buf, StartBit);
    uint32_t nal_ref_idc = u(2, buf, StartBit);
    uint32_t nal_unit_type = u(5, buf, StartBit);
    return nal_unit_type;
}