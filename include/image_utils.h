#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <stdio.h>

#define JPEG_QUALITY 90
#define CLAMP(x) (((x) > 255) ? 255 : (((x) < 0) ? 0 : (x)))

#define BLOCK_SIZE 64
#define CACHE_LINE 64

#define CACHE_BLOCK_SIZE 32

typedef struct {
    const char *name;
    void (*func)(unsigned char*, int, int, int, float);
    int param;
} Filter;

const char* file_format(const char* filename);
int is_valid_expression(const char* filename);

double tmp_atof(const char s[]);
int is_number(const char *str);
double filter_time(void (*func)(unsigned char*, int, int, int, float),
                   unsigned char *image, int width, int height, int channels, float param);

void gaussian_blur(unsigned char *image, int width, int height, int channels, float radius);
void edge_detect(unsigned char *image, int width, int height, int channels, float threshold);

void grayscale(unsigned char *image, int width, int height, int channels, float param);
void invert(unsigned char *image, int width, int height, int channels, float param);
void brightness(unsigned char *image, int width, int height, int channels, float brightness);
void contrast(unsigned char *image, int width, int height, int channels, float factor);
void sepia(unsigned char *image, int width, int height, int channels, float param);

extern int use_thread;


#endif //IMAGE_UTILS_H
