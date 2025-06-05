#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "bmp.h"

#define EYE_SEPARATION  180.0f // = 2.5 in * 72 DPI
#define DEPTH_OF_FIELD  (1.0f / 3.0f)

typedef struct ZMap ZMap;
struct ZMap
{
    float *z;
    int32_t width;
    int32_t height;
};

static BmpImage
load_image(char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open the file: %s\n", path);
        exit(1);
    }
    BmpImage img = {};
    if (bmp_load(&img, file) != BMP_OK) {
        fprintf(stderr, "Failed to load BMP from file: %s\n", path);
        exit(1);
    }
    fclose(file);
    return img;
}

static void
save_image(char *path, BmpImage img)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open the output file: %s\n", path);
        exit(1);
    }
    if (bmp_save(img, file) != BMP_OK) {
        fprintf(stderr, "Failed to save SIRDS to file: %s\n", path);
        exit(1);
    }
    fclose(file);
}

static ZMap
load_zmap(char *path)
{
    BmpImage zmapImg = load_image(path);

    ZMap zmap = {};
    zmap.width = zmapImg.width;
    zmap.height = zmapImg.height;
    zmap.z = (float *)malloc(zmap.width * zmap.height * sizeof(float));
    ASSERT(zmap.z != NULL);

    for (int32_t row = 0; row < zmap.height; row++) {
        for (int32_t column = 0; column < zmap.width; column++) {
            uint8_t *pixel = zmapImg.pixels + row * zmapImg.stride + column * zmapImg.depth;
            zmap.z[row * zmap.width + column] = 1.0f - MIN((float)pixel[0] / 255.0f, 1.0f);
        }
    }

    bmp_free(&zmapImg);

    return zmap;
}

static int32_t
calc_separation(float z)
{
    float s = (1.0f - DEPTH_OF_FIELD * z) * EYE_SEPARATION / (2.0f - DEPTH_OF_FIELD * z);
    return NEAREST_INT(s);
}

static BmpImage
sirds_generate(ZMap zmap, BmpImage patternImg)
{
    BmpImage sirdsImg = {};
    sirdsImg.width = zmap.width;
    sirdsImg.height = zmap.height;
    sirdsImg.depth = 3;
    sirdsImg.stride = sirdsImg.width * sirdsImg.depth;
    sirdsImg.pixels = (uint8_t *)malloc(sirdsImg.stride * sirdsImg.height * sizeof(uint8_t));
    ASSERT(sirdsImg.pixels != NULL);

    for (int32_t row = 0; row < sirdsImg.height; row++) {
        int32_t rowOffset = row * sirdsImg.stride;
        for (int32_t column = 0; column < sirdsImg.width; column += patternImg.width) {
            int32_t columnOffset = column * sirdsImg.depth;

            uint8_t *src = sirdsImg.pixels + rowOffset + columnOffset;
            uint8_t *dst = patternImg.pixels + (row % patternImg.height) * patternImg.stride;
            uint32_t n = MIN((sirdsImg.width - column) * patternImg.depth, patternImg.stride);
            memcpy(src, dst, n);
        }
    }

    int32_t *same = (int32_t *)malloc(zmap.width * sizeof(int32_t));
    ASSERT(same != NULL);

    for (int32_t row = 0; row < zmap.height; row++) {
        int32_t rowOffset = row * sirdsImg.stride;

        int32_t separation = 0;
        int32_t leftColumn = 0;
        int32_t rightColumn = 0;

        for (int32_t column = 0; column < zmap.width; column++) {
            same[column] = column;
        }

        for (int32_t column = 0; column < zmap.width; column++) {
            float z = zmap.z[row * zmap.width + column];
            separation = calc_separation(z);
            leftColumn = column - (separation + (separation & row & 1)) / 2;
            rightColumn = leftColumn + separation;

            if (0 <= leftColumn && rightColumn < zmap.width) {
                bool visible;
                int t = 1;
                float zt;
                do {
                    zt = z + 2 * (2 - DEPTH_OF_FIELD * z) * (float)t / (DEPTH_OF_FIELD * EYE_SEPARATION);
                    visible = zmap.z[row * zmap.width + column - t] < zt && zmap.z[row * zmap.width + column + t] < zt;
                    t++;
                } while(visible && zt < 1.0f);
                if (visible) {
                    for(int32_t l = same[leftColumn]; l != leftColumn && l != rightColumn; l = same[leftColumn]) {
                        if (l < rightColumn) {
                            leftColumn = l;
                        }
                        else {
                            leftColumn = rightColumn;
                            rightColumn = l;
                        }
                    }
                }
                same[leftColumn] = rightColumn;
            }
        }

        for (int32_t column = zmap.width - 1; column >= 0; column--) {
            if (same[column] != column) {
                int32_t columnOffset = column * sirdsImg.depth;
                uint8_t *pixel = sirdsImg.pixels + rowOffset + columnOffset;
                pixel[0] = sirdsImg.pixels[rowOffset + same[column] * sirdsImg.depth + 0];
                pixel[1] = sirdsImg.pixels[rowOffset + same[column] * sirdsImg.depth + 1];
                pixel[2] = sirdsImg.pixels[rowOffset + same[column] * sirdsImg.depth + 2];
            }
        }
    }

    free(same);

    return sirdsImg;
}

int32_t
main(int32_t argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <zmap.bmp> <pattern.bmp> <output.bmp>\n", argv[0]);
        exit(1);
    }
    char *zmapPath = argv[1];
    char *patternPath = argv[2];
    char *outputPath = argv[3];

    ZMap zmap = load_zmap(zmapPath);
    BmpImage patternImg = load_image(patternPath);
    BmpImage sirdsImg = sirds_generate(zmap, patternImg);
    save_image(outputPath, sirdsImg);

    // Cleanup
    bmp_free(&sirdsImg);
    bmp_free(&patternImg);
    free(zmap.z);

    exit(0);
}
