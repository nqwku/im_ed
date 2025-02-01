#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define JPEG_QUALITY 90

int clamp(int value, int min, int max);
const char* file_format(const char* filename);
void grayscale(unsigned char *image, int width, int height, int channels);
void invert(unsigned char *image, int width, int height, int channels);
void brightness(unsigned char *image, int width, int height, int channels, float brightness);
void contrast(unsigned char *image, int width, int height, int channels, float factor);
void sepia(unsigned char *image, int width, int height, int channels); // sepiaRed = .393*R + .769*G + .189B | sepiaGreen = .349*R + .686*G + .168B | sepiaBlue= .272*R + .534*G + .131B

int main(void) {
    int width, height, channels;
    unsigned char *image = stbi_load("image.jpg", &width, &height, &channels, 0);

    if (image == NULL) {
        printf("Failed to load image.\n");
        return 1;
    }

    printf("%s Image: %dx%d, Channels: %d\n", file_format("images.jpeg"), width, height, channels);

    contrast(image, width, height, channels, 1.2);

    stbi_write_jpg("output.jpg", width, height, channels, image, JPEG_QUALITY);

    stbi_image_free(image);

    return 0;
}

int clamp(int value, int min, int max) {
    return (value < min) ? min : (value > max) ? max : value;
}

const char* file_format(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return "Unknown";

    unsigned char header[8];
    fread(header, 1, 8, file);
    fclose(file);

    if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
        return "JPEG";
    }

    if (memcmp(header, "\x89PNG\r\n\x1a\n", 8) == 0 ){
        return "PNG";
    }

    return "Unknown";
}

void grayscale(unsigned char *image, int width, int height, int channels) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * channels;
            image[index + 0] = 0.298f * image[index + 0] + 0.587f * image[index + 1] + 0.114f * image[index + 2];
            image[index + 1] = 0.298f * image[index + 0] + 0.587f * image[index + 1] + 0.114f * image[index + 2];
            image[index + 2] = 0.298f * image[index + 0] + 0.587f * image[index + 1] + 0.114f * image[index + 2];
        }
    }
}

void invert(unsigned char *image, int width, int height, int channels) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * channels;
            image[index + 0] = 255 - image[index + 0];
            image[index + 1] = 255 - image[index + 1];
            image[index + 2] = 255 - image[index + 2];
        }
    }
}

void brightness(unsigned char *image, int width, int height, int channels, float brightness) {
    for (int i = 0; i < width * height * channels; i++) {
        image[i] = (image[i] * brightness > 255) ? 255 : image[i] * brightness;
    }
};
void contrast(unsigned char *image, int width, int height, int channels, float factor) {
    for (int i = 0; i < width * height * channels; i++) {
        int tmp_image = (int)image[i];
        tmp_image = clamp(factor * (tmp_image - 128) + 128, 0, 255);
        image[i] = (unsigned char)tmp_image;
    }
}

void sepia(); // sepiaRed = .393*R + .769*G + .189B | sepiaGreen = .349*R + .686*G + .168B | sepiaBlue= .272*R + .534*G + .131B

