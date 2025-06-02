#include "utils.h"
#include "bmp.h"

#include <stdlib.h>
#include <string.h>

#define BMP_PADDING(w) ((w) % 4)

BmpError
bmp_load(BmpImage *img, FILE *srcFile)
{
    ASSERT(img != NULL && srcFile != NULL);

    uint8_t headerBuf[BMP_HEADER_SIZE] = {};

    if (fread(headerBuf, BMP_HEADER_SIZE, 1, srcFile) != 1) {
        return BMP_ERR_FILE_READ_FAILED;
    }

    if (headerBuf[BMP_HEADER_MAGIC_BYTE_1_OFFSET] != BMP_MAGIC_BYTE_1
        || headerBuf[BMP_HEADER_MAGIC_BYTE_2_OFFSET] != BMP_MAGIC_BYTE_2) {
        return BMP_ERR_FILE_INVALID;
    }

    img->width = *((int32_t *)(headerBuf + BMP_HEADER_WIDTH_OFFSET));
    img->height = *((int32_t *)(headerBuf + BMP_HEADER_HEIGHT_OFFSET));
    img->depth = *((int16_t *)(headerBuf + BMP_HEADER_BITS_PER_PIXEL_OFFSET)) / 8;
    img->stride = (img->width + BMP_PADDING(img->width)) * img->depth;

    if (img->depth != 3 || img->width <= 0 || img->height <= 0) {
        return BMP_ERR_FILE_INVALID;
    }

    int32_t offset = *((int32_t *)(headerBuf + BMP_HEADER_IMAGE_DATA_OFFSET));
    if (fseek(srcFile, offset, SEEK_SET) != 0) {
        return BMP_ERR_FILE_READ_FAILED;
    }

    img->pixels = (uint8_t *)malloc(img->stride * img->height);
    ASSERT(img->pixels != NULL);

    for (int32_t row = img->height - 1; row >= 0; row--) {
        if (fread(img->pixels + row * img->stride, img->stride, 1, srcFile) != 1) {
            return BMP_ERR_FILE_READ_FAILED;
        }
    }

    return BMP_OK;
}

BmpError
bmp_save(BmpImage img, FILE *destFile)
{
    ASSERT(destFile != NULL);

    if (img.depth != 3 || img.width <= 0 || img.height <= 0) {
        return BMP_ERR_FILE_INVALID;
    }

    int32_t imgSize = img.height * img.stride;

    uint8_t headerBuf[BMP_HEADER_SIZE] = {};
    *(headerBuf + BMP_HEADER_MAGIC_BYTE_1_OFFSET) = (uint8_t)BMP_MAGIC_BYTE_1;
    *(headerBuf + BMP_HEADER_MAGIC_BYTE_2_OFFSET) = (uint8_t)BMP_MAGIC_BYTE_2;
    *((int32_t *)(headerBuf + BMP_HEADER_FILE_SIZE_OFFSET)) = BMP_HEADER_SIZE + imgSize;
    *((int32_t *)(headerBuf + BMP_HEADER_RESERVED_OFFSET)) = 0;
    *((int32_t *)(headerBuf + BMP_HEADER_IMAGE_DATA_OFFSET)) = BMP_HEADER_SIZE;
    *((int32_t *)(headerBuf + BMP_HEADER_INFO_SIZE_OFFSET)) = BMP_HEADER_INFO_SIZE;
    *((int32_t *)(headerBuf + BMP_HEADER_WIDTH_OFFSET)) = img.width;
    *((int32_t *)(headerBuf + BMP_HEADER_HEIGHT_OFFSET)) = img.height;
    *((int16_t *)(headerBuf + BMP_HEADER_PLANES_OFFSET)) = (int16_t)1;
    *((int16_t *)(headerBuf + BMP_HEADER_BITS_PER_PIXEL_OFFSET)) = (int16_t)(img.depth * 8);
    *((int32_t *)(headerBuf + BMP_HEADER_COMPRESSION_OFFSET)) = 0;
    *((int32_t *)(headerBuf + BMP_HEADER_IMAGE_SIZE_OFFSET)) = imgSize;
    *((int32_t *)(headerBuf + BMP_HEADER_X_PIXELS_PER_METER_OFFSET)) = 0;
    *((int32_t *)(headerBuf + BMP_HEADER_Y_PIXELS_PER_METER_OFFSET)) = 0;
    *((int32_t *)(headerBuf + BMP_HEADER_COLORS_USED_OFFSET)) = 0;
    *((int32_t *)(headerBuf + BMP_HEADER_COLORS_IMPORTANT_OFFSET)) = 0;

    if (fwrite(headerBuf, BMP_HEADER_SIZE, 1, destFile) != 1) {
        return BMP_ERR_FILE_WRITE_FAILED;
    }

    for (int32_t row = img.height - 1; row >= 0; row--) {
        if (fwrite(img.pixels + row * img.stride, img.stride, 1, destFile) != 1) {
            return BMP_ERR_FILE_WRITE_FAILED;
        }
    }

    return BMP_OK;
}

void
bmp_free(BmpImage *image)
{
    ASSERT(image != NULL);
    free(image->pixels);
}
