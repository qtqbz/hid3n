#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdio.h>

#define BMP_HEADER_FILE_SIZE 14
#define BMP_HEADER_INFO_SIZE 40
#define BMP_HEADER_SIZE (BMP_HEADER_FILE_SIZE + BMP_HEADER_INFO_SIZE)

#define BMP_HEADER_MAGIC_BYTE_1_OFFSET 0
#define BMP_HEADER_MAGIC_BYTE_2_OFFSET 1
#define BMP_HEADER_FILE_SIZE_OFFSET 2
#define BMP_HEADER_RESERVED_OFFSET 6
#define BMP_HEADER_IMAGE_DATA_OFFSET 10
#define BMP_HEADER_INFO_SIZE_OFFSET 14
#define BMP_HEADER_WIDTH_OFFSET 18
#define BMP_HEADER_HEIGHT_OFFSET 22
#define BMP_HEADER_PLANES_OFFSET 26
#define BMP_HEADER_BITS_PER_PIXEL_OFFSET 28
#define BMP_HEADER_COMPRESSION_OFFSET 30
#define BMP_HEADER_IMAGE_SIZE_OFFSET 34
#define BMP_HEADER_X_PIXELS_PER_METER_OFFSET 38
#define BMP_HEADER_Y_PIXELS_PER_METER_OFFSET 42
#define BMP_HEADER_COLORS_USED_OFFSET 46
#define BMP_HEADER_COLORS_IMPORTANT_OFFSET 50

#define BMP_MAGIC_BYTE_1 0x42
#define BMP_MAGIC_BYTE_2 0x4D

typedef uint32_t BmpError;
enum
{
    BMP_OK,
    BMP_ERR_FILE_INVALID,
    BMP_ERR_FILE_READ_FAILED,
    BMP_ERR_FILE_WRITE_FAILED,
};

typedef struct BmpImage BmpImage;
struct BmpImage
{
    int32_t width;
    int32_t height;
    int32_t depth;
    int32_t stride;
    uint8_t *pixels;
};

BmpError bmp_load(BmpImage *img, FILE *srcFile);
BmpError bmp_save(BmpImage img, FILE *destFile);
void bmp_free(BmpImage *image);

#endif //BMP_H
