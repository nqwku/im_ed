#include "image_utils.h"
#include <stdio.h>
#include <omp.h>

int use_thread = 1;


double filter_time(void (*func)(unsigned char*, int, int, int, float),
                   unsigned char *image, int width, int height, int channels, float param) {
    double start_time = omp_get_wtime();
    func(image, width, height, channels, param);
    double end_time = omp_get_wtime();
    return end_time - start_time;
}


void grayscale(unsigned char *image, int width, int height, int channels, float param) {

    const float r_factor = 0.298f;
    const float g_factor = 0.587f;
    const float b_factor = 0.114f;

    const int total_pixels = width * height;

    if (use_thread){
#pragma omp parallel for schedule(guided)
        for (int block_y = 0; block_y < height; block_y += CACHE_BLOCK_SIZE) {
            for (int block_x = 0; block_x < width; block_x += CACHE_BLOCK_SIZE) {
                const int max_y = (block_y + CACHE_BLOCK_SIZE < height) ? block_y + CACHE_BLOCK_SIZE : height;
                const int max_x = (block_x + CACHE_BLOCK_SIZE < width) ? block_x + CACHE_BLOCK_SIZE : width;

                for (int y = block_y; y < max_y; y++) {
                    for (int x = block_x; x < max_x; x++) {
                        const int idx = (y * width + x) * channels;
                        const float gray = r_factor * image[idx] + g_factor * image[idx + 1] + b_factor * image[idx + 2];
                        const unsigned char gray_byte = (unsigned char)gray;

                        image[idx] = gray_byte;
                        image[idx + 1] = gray_byte;
                        image[idx + 2] = gray_byte;
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < total_pixels; i++) {
            const int idx = i * channels;
            const float gray = r_factor * image[idx] + g_factor * image[idx + 1] + b_factor * image[idx + 2];
            const unsigned char gray_byte = (unsigned char)gray;

            image[idx] = gray_byte;
            image[idx + 1] = gray_byte;
            image[idx + 2] = gray_byte;
        }
    }
}

void invert(unsigned char *image, int width, int height, int channels, float param) {
    const int total_size = width * height * channels;

    if (use_thread){
#pragma omp parallel for schedule(guided)
        for (int i = 0; i < total_size; i += channels) {
            for (int c = 0; c < 3 && c < channels; c++) {
                image[i + c] = 255 - image[i + c];
            }
        }
    } else {
        for (int i = 0; i < total_size; i += channels) {
            for (int c = 0; c < 3 && c < channels; c++) {
                image[i + c] = 255 - image[i + c];
            }
        }
    }
}

void brightness(unsigned char *image, int width, int height, int channels, float brightness) {
    if (brightness < 0.1 || brightness > 2.0) {
        fprintf(stderr, "Error: Brightness must be between 0 and 2.\n");
        return;
    }

    const int total_size = width * height * channels;

    if (use_thread) {
#pragma omp parallel for schedule(guided)
        for (int i = 0; i < total_size; i++) {
            float new_val = image[i] * brightness;
            image[i] = (new_val > 255.0f) ? 255 : (unsigned char)new_val;
        }
    } else {
        for (int i = 0; i < total_size; i++) {
            float new_val = image[i] * brightness;
            image[i] = (new_val > 255.0f) ? 255 : (unsigned char)new_val;
        }
    }
};

void contrast(unsigned char *image, int width, int height, int channels, float factor) {
    if (factor < 0.1 || factor > 2.0) {
        fprintf(stderr, "Error: Contrast must be between 0 and 2.\n");
        return;
    }

    if (use_thread) {
#pragma omp parallel for schedule(guided)
        for (int i = 0; i < width * height * channels; i++) {
            int tmp_image = (int)image[i];
            tmp_image = CLAMP(factor * (tmp_image - 128) + 128);
            image[i] = (unsigned char)tmp_image;
        }
    } else {
        for (int i = 0; i < width * height * channels; i++) {
            int tmp_image = (int)image[i];
            tmp_image = CLAMP(factor * (tmp_image - 128) + 128);
            image[i] = (unsigned char)tmp_image;
        }
    }
}

void sepia(unsigned char *image, int width, int height, int channels, float param) {
    if (channels != 3 && channels != 4) {
        fprintf(stderr, "Error: Sepia filter requires 3 or 4 channels.\n");
        return;
    }

    static const float c_red[3] = {0.393f, 0.769f, 0.189f};
    static const float c_green[3] = {0.349f, 0.686f, 0.168f};
    static const float c_blue[3] = {0.272f, 0.534f, 0.131f};

    const int total_pixels = width * height;

    if (use_thread) {
        #pragma omp parallel for schedule(guided)
        for (int i = 0; i < total_pixels; i++) {
            const int idx = i * channels;
            const int r = image[idx];
            const int g = image[idx + 1];
            const int b = image[idx + 2];

            const int sepia_red   = CLAMP((r * c_red[0] + g * c_red[1] + b * c_red[2]) * 1000.0f);
            const int sepia_green = CLAMP((r * c_green[0] + g * c_green[1] + b * c_green[2]) * 1000.0f);
            const int sepia_blue  = CLAMP((r * c_blue[0] + g * c_blue[1] + b * c_blue[2]) * 1000.0f);

            image[idx] = sepia_red;
            image[idx + 1] = sepia_green;
            image[idx + 2] = sepia_blue;
        }
    } else {
        for (int i = 0; i < total_pixels; i++) {
            const int idx = i * channels;
            const int r = image[idx];
            const int g = image[idx + 1];
            const int b = image[idx + 2];

            const int sepia_red   = CLAMP((r * c_red[0] + g * c_red[1] + b * c_red[2]) * 1000.0f);
            const int sepia_green = CLAMP((r * c_green[0] + g * c_green[1] + b * c_green[2]) * 1000.0f);
            const int sepia_blue  = CLAMP((r * c_blue[0] + g * c_blue[1] + b * c_blue[2]) * 1000.0f);

            image[idx] = sepia_red;
            image[idx + 1] = sepia_green;
            image[idx + 2] = sepia_blue;
        }
    }
}