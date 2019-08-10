

#ifndef SRECORDER_CRC32_H
#define SRECORDER_CRC32_H

/**
    @function: unsigned int srecorder_calculate_crc32(unsigned char const* data, unsigned long len)
    @brief: Use CRC32 to do data check
    @param: data buffer to be checked
    @param: len  buffer length
    @return: CRC32 value
    @note: 
**/
unsigned int srecorder_calculate_crc32(unsigned char const* data, unsigned long len);

#endif
