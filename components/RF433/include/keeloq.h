#ifndef __KEELOG_H__
#define __KEELOG_H__

#define KEELOQ_HIGH_KEY 0x38b4b1ff
#define KEELOQ_LOW_KEY  0xd015238c


void keeloq_set_key(uint32_t keyHigh, uint32_t keyLow);

uint32_t keeloq_encrypt(uint32_t data);

uint32_t keeloq_decrypt(uint32_t data);

#define KeeLoq_NLF              0x3A5C742EUL

static uint32_t _keyHigh = 0;
static uint32_t _keyLow = 0;

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))


void keeloq_set_key(uint32_t keyHigh, uint32_t keyLow)
{
    _keyHigh = keyHigh;
    _keyLow = keyLow;
}


uint32_t keeloq_encrypt(uint32_t data)
{
    uint32_t keyBitVal,bitVal;
    int keyBitNo, index;
    uint32_t x = data;
    uint32_t r;

    for (r = 0; r < 528; r++)
    {
        keyBitNo = r & 63;
        if(keyBitNo < 32)
            keyBitVal = bitRead(_keyLow,keyBitNo);
        else
            keyBitVal = bitRead(_keyHigh, keyBitNo - 32);
        index = 1 * bitRead(x,1) + 2 * bitRead(x,9) + 4 * bitRead(x,20) + 8 * bitRead(x,26) + 16 * bitRead(x,31);
        bitVal = bitRead(x,0) ^ bitRead(x, 16) ^ bitRead(KeeLoq_NLF,index) ^ keyBitVal;
        x = (x>>1) ^ bitVal<<31;
    }

    return x;
}


uint32_t keeloq_decrypt(uint32_t data)
{
    uint32_t keyBitVal,bitVal;
    int keyBitNo, index;
    uint32_t x = data;
    uint32_t r;

    for (r = 0; r < 528; r++)
    {
        keyBitNo = (15-r) & 63;
        if(keyBitNo < 32)
            keyBitVal = bitRead(_keyLow,keyBitNo);
        else
            keyBitVal = bitRead(_keyHigh, keyBitNo - 32);
        index = 1 * bitRead(x,0) + 2 * bitRead(x,8) + 4 * bitRead(x,19) + 8 * bitRead(x,25) + 16 * bitRead(x,30);
        bitVal = bitRead(x,31) ^ bitRead(x, 15) ^ bitRead(KeeLoq_NLF,index) ^ keyBitVal;
        x = (x<<1) ^ bitVal;
    }

    return x;
}


#endif  /* __KEELOG_H__ */

