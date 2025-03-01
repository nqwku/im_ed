#include "image_utils.h"
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include <cpuid.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

int use_thread = 1;

double filter_time(void (*func)(unsigned char*, int, int, int, float),
                   unsigned char *image, int width, int height, int channels, float param) {
    double start_time = omp_get_wtime();
    func(image, width, height, channels, param);
    double end_time = omp_get_wtime();
    return end_time - start_time;
}

static void box_radii(int boxes[3], float sigma) {
    float p_width = sqrtf((12.0f * sigma * sigma / 3.0f) + 1.0f);
    int w = (int)floor(p_width);
    if (w % 2 == 0) w--;

    int n = 3;

    float wl = floor(p_width);
    float wu = wl + 2.0f;
    float m_p = (12.0f * sigma * sigma - n * wl * wl - 4.0f * n * wl - 3.0f * n) / (-4.0f * wl - 4.0f);
    float m = round(m_p);

    for(int i = 0; i < n; i++) boxes[i] = (int)((i < m) ? wl : wu);
    for(int i = 0; i < n; i++) boxes[i] = (boxes[i] - 1) / 2;
}

static void box_h_blur(unsigned char *src, unsigned char *dst, int width, int height, int channels, int radius) {
    float iarr = 1.0f / (radius + radius + 1);

    if (use_thread) {
        #pragma omp parallel for schedule(static)
        for (int y = 0; y < height; y++) {
            float val_r, val_g, val_b;
            int row_offset = y * width * channels;

            val_r = src[row_offset] * (radius + 1);
            val_g = src[row_offset + 1] * (radius + 1);
            val_b = src[row_offset + 2] * (radius + 1);

            for (int x = 0; x < radius; x++) {
                val_r += src[(row_offset + x * channels)];
                val_g += src[(row_offset + x * channels) + 1];
                val_b += src[(row_offset + x * channels) + 2];
            }

            for (int x = 0; x <= radius; x++) {
                val_r += src[(row_offset + (x + radius) * channels)] - src[row_offset];
                val_g += src[(row_offset + (x + radius) * channels) + 1] - src[row_offset + 1];
                val_b += src[(row_offset + (x + radius) * channels) + 2] - src[row_offset + 2];

                dst[(row_offset + x * channels)] = (unsigned char)(val_r * iarr);
                dst[(row_offset + x * channels) + 1] = (unsigned char)(val_g * iarr);
                dst[(row_offset + x * channels) + 2] = (unsigned char)(val_b * iarr);
            }

            for (int x = radius + 1; x < width - radius; x++) {
                val_r += src[(row_offset + (x + radius) * channels)] - src[(row_offset + (x - radius - 1) * channels)];
                val_g += src[(row_offset + (x + radius) * channels) + 1] - src[(row_offset + (x - radius - 1) * channels) + 1];
                val_b += src[(row_offset + (x + radius) * channels) + 2] - src[(row_offset + (x - radius - 1) * channels) + 2];

                dst[(row_offset + x * channels)] = (unsigned char)(val_r * iarr);
                dst[(row_offset + x * channels) + 1] = (unsigned char)(val_g * iarr);
                dst[(row_offset + x * channels) + 2] = (unsigned char)(val_b * iarr);
            }

            for (int x = width - radius; x < width; x++) {
                val_r += src[(row_offset + (width - 1) * channels)] - src[(row_offset + (x - radius -1) * channels)];
                val_g += src[(row_offset + (width - 1) * channels) + 1] - src[(row_offset + (x - radius -1) * channels) + 1];
                val_b += src[(row_offset + (width - 1) * channels) + 2] - src[(row_offset + (x - radius -1) * channels) + 2];

                dst[(row_offset + x * channels)] = (unsigned char)(val_r * iarr);
                dst[(row_offset + x * channels) + 1] = (unsigned char)(val_g * iarr);
                dst[(row_offset + x * channels) + 2] = (unsigned char)(val_b * iarr);
            }

            if (channels == 4) {
                for (int x = 0; x < width; x++) dst[(row_offset + x * channels) + 3] = src[(row_offset + x * channels) + 3];
            }
        }
    }
    else {
        for (int y = 0; y < height; y++) {
            float val_r, val_g, val_b;
            int row_offset = y * width * channels;

            val_r = src[row_offset] * (radius + 1);
            val_g = src[row_offset + 1] * (radius + 1);
            val_b = src[row_offset + 2] * (radius + 1);

            for (int x = 0; x < radius; x++) {
                val_r += src[(row_offset + x * channels)];
                val_g += src[(row_offset + x * channels) + 1];
                val_b += src[(row_offset + x * channels) + 2];
            }

            for (int x = 0; x <= radius; x++) {
                val_r += src[(row_offset + (x + radius) * channels)] - src[row_offset];
                val_g += src[(row_offset + (x + radius) * channels) + 1] - src[row_offset + 1];
                val_b += src[(row_offset + (x + radius) * channels) + 2] - src[row_offset + 2];

                dst[(row_offset + x * channels)] = (unsigned char)(val_r * iarr);
                dst[(row_offset + x * channels) + 1] = (unsigned char)(val_g * iarr);
                dst[(row_offset + x * channels) + 2] = (unsigned char)(val_b * iarr);
            }

            for (int x = radius + 1; x < width - radius; x++) {
                val_r += src[(row_offset + (x + radius) * channels)] - src[(row_offset + (x - radius - 1) * channels)];
                val_g += src[(row_offset + (x + radius) * channels) + 1] - src[(row_offset + (x - radius - 1) * channels) + 1];
                val_b += src[(row_offset + (x + radius) * channels) + 2] - src[(row_offset + (x - radius - 1) * channels) + 2];

                dst[(row_offset + x * channels)] = (unsigned char)(val_r * iarr);
                dst[(row_offset + x * channels) + 1] = (unsigned char)(val_g * iarr);
                dst[(row_offset + x * channels) + 2] = (unsigned char)(val_b * iarr);
            }

            for (int x = width - radius; x < width; x++) {
                val_r += src[(row_offset + (width - 1) * channels)] - src[(row_offset + (x - radius -1) * channels)];
                val_g += src[(row_offset + (width - 1) * channels) + 1] - src[(row_offset + (x - radius -1) * channels) + 1];
                val_b += src[(row_offset + (width - 1) * channels) + 2] - src[(row_offset + (x - radius -1) * channels) + 2];

                dst[(row_offset + x * channels)] = (unsigned char)(val_r * iarr);
                dst[(row_offset + x * channels) + 1] = (unsigned char)(val_g * iarr);
                dst[(row_offset + x * channels) + 2] = (unsigned char)(val_b * iarr);
            }

            if (channels == 4) {
                for (int x = 0; x < width; x++) dst[(row_offset + x * channels) + 3] = src[(row_offset + x * channels) + 3];
            }
        }
    }
}

static void box_v_blur(unsigned char *src, unsigned char *dst, int width, int height, int channels, int radius) {
    float iarr = 1.0f / (radius + radius + 1);

    if (use_thread) {
        #pragma omp parallel for schedule(static)
        for (int x = 0; x < width; x++) {

            float val_r, val_g, val_b;
            int col_offset = x * channels;

            val_r = src[col_offset] * (radius + 1);
            val_g = src[col_offset + 1] * (radius + 1);
            val_b = src[col_offset + 2] * (radius + 1);

            for (int y = 0; y < radius; y++) {
                val_r += src[(y * width + x) * channels];
                val_g += src[(y * width + x) * channels + 1];
                val_b += src[(y * width + x) * channels + 2];
            }

            for (int y = 0; y <= radius; y++) {
                val_r += src[((y + radius) * width + x) * channels] - src[col_offset];
                val_g += src[((y + radius) * width + x) * channels + 1] - src[col_offset + 1];
                val_b += src[((y + radius) * width + x) * channels + 2] - src[col_offset + 2];

                dst[(y * width + x) * channels] = (unsigned char)(val_r * iarr);
                dst[(y * width + x) * channels + 1] = (unsigned char)(val_g * iarr);
                dst[(y * width + x) * channels + 2] = (unsigned char)(val_b * iarr);
            }

            for (int y = radius + 1; y < height - radius; y++) {
                val_r += src[((y + radius) * width + x) * channels] - src[((y - radius - 1) * width + x) * channels];
                val_g += src[((y + radius) * width + x) * channels + 1] - src[((y - radius - 1) * width + x) * channels + 1];
                val_b += src[((y + radius) * width + x) * channels + 2] - src[((y - radius - 1) * width + x) * channels + 2];

                dst[(y * width + x) * channels] = (unsigned char)(val_r * iarr);
                dst[(y * width + x) * channels + 1] = (unsigned char)(val_g * iarr);
                dst[(y * width + x) * channels + 2] = (unsigned char)(val_b * iarr);
            }

            for (int y = height - radius; y < height; y++) {
                val_r += src[((height - 1) * width + x) * channels] - src[((y - radius - 1) * width + x) * channels];
                val_g += src[((height - 1) * width + x) * channels + 1] - src[((y - radius - 1) * width + x) * channels + 1];
                val_b += src[((height - 1) * width + x) * channels + 2] - src[((y - radius - 1) * width + x) * channels + 2];

                dst[(y * width + x) * channels] = (unsigned char)(val_r * iarr);
                dst[(y * width + x) * channels + 1] = (unsigned char)(val_g * iarr);
                dst[(y * width + x) * channels + 2] = (unsigned char)(val_b * iarr);
            }

            if (channels == 4) {
                for (int y = 0; y < height; y++) {
                    dst[(y * width + x) * channels + 3] = src[(y * width + x) * channels + 3];
                }
            }
        }
    } else {
        for (int x = 0; x < width; x++) {

            float val_r, val_g, val_b;
            int col_offset = x * channels;

            val_r = src[col_offset] * (radius + 1);
            val_g = src[col_offset + 1] * (radius + 1);
            val_b = src[col_offset + 2] * (radius + 1);

            for (int y = 0; y < radius; y++) {
                val_r += src[(y * width + x) * channels];
                val_g += src[(y * width + x) * channels + 1];
                val_b += src[(y * width + x) * channels + 2];
            }

            for (int y = 0; y <= radius; y++) {
                val_r += src[((y + radius) * width + x) * channels] - src[col_offset];
                val_g += src[((y + radius) * width + x) * channels + 1] - src[col_offset + 1];
                val_b += src[((y + radius) * width + x) * channels + 2] - src[col_offset + 2];

                dst[(y * width + x) * channels] = (unsigned char)(val_r * iarr);
                dst[(y * width + x) * channels + 1] = (unsigned char)(val_g * iarr);
                dst[(y * width + x) * channels + 2] = (unsigned char)(val_b * iarr);
            }

            for (int y = radius + 1; y < height - radius; y++) {
                val_r += src[((y + radius) * width + x) * channels] - src[((y - radius - 1) * width + x) * channels];
                val_g += src[((y + radius) * width + x) * channels + 1] - src[((y - radius - 1) * width + x) * channels + 1];
                val_b += src[((y + radius) * width + x) * channels + 2] - src[((y - radius - 1) * width + x) * channels + 2];

                dst[(y * width + x) * channels] = (unsigned char)(val_r * iarr);
                dst[(y * width + x) * channels + 1] = (unsigned char)(val_g * iarr);
                dst[(y * width + x) * channels + 2] = (unsigned char)(val_b * iarr);
            }

            for (int y = height - radius; y < height; y++) {
                val_r += src[((height - 1) * width + x) * channels] - src[((y - radius - 1) * width + x) * channels];
                val_g += src[((height - 1) * width + x) * channels + 1] - src[((y - radius - 1) * width + x) * channels + 1];
                val_b += src[((height - 1) * width + x) * channels + 2] - src[((y - radius - 1) * width + x) * channels + 2];

                dst[(y * width + x) * channels] = (unsigned char)(val_r * iarr);
                dst[(y * width + x) * channels + 1] = (unsigned char)(val_g * iarr);
                dst[(y * width + x) * channels + 2] = (unsigned char)(val_b * iarr);
            }

            if (channels == 4) {
                for (int y = 0; y < height; y++) {
                    dst[(y * width + x) * channels + 3] = src[(y * width + x) * channels + 3];
                }
            }
        }
    }
}

static void box_blur(unsigned char *src, unsigned char *dst, unsigned char *temp,
                     int width, int height, int channels, int radius) {
    box_h_blur(src, temp, width, height, channels, radius);
    box_v_blur(temp, dst, width, height, channels, radius);
}

void gaussian_blur(unsigned char *image, int width, int height, int channels, float sigma) {
    if (sigma < 1 || sigma > 10.0f) {
        fprintf(stderr, "Error: Sigma must be between 1 and 10\n");
        return;
    }

    int boxes[3];
    box_radii(boxes, sigma);

    unsigned char *temp = (unsigned char *)malloc(width * height * channels);
    unsigned char *buffer = (unsigned char *)malloc(width * height * channels);

    if (!temp || !buffer) {
        fprintf(stderr, "Error: Failed tp allocate temporary buffer\n");
        if (temp) free(temp);
        if (buffer) free(buffer);
        return;
    }

    memcpy(buffer, image, width * height * channels);
    for (int i = 0; i < 3; i++) {
        box_blur(buffer, image, temp, width, height, channels, boxes[i]);
        if (i < 2) memcpy(buffer, image, width * height * channels);
    }

    free(temp);
    free(buffer);
}


void edge_detect(unsigned char *image, int width, int height, int channels, float threshold) {
    if (threshold < 0.0f || threshold > 255.0f) {
        fprintf(stderr, "Error: Threshold must be between 0 and 255.\n");
        return;
    }

    unsigned char *temp = (unsigned char*)malloc(width * height * channels);
    unsigned char *gray = (unsigned char*)malloc(width * height);

    if (!temp || !gray) {
        fprintf(stderr, "Error: Failed to allocate temporary buffers for edge detection.\n");
        if (temp) free(temp);
        if (gray) free(gray);
        return;
    }

    memcpy(temp, image, width * height * channels);

    if (use_thread) {
        #pragma omp parallel for schedule(guided)
        for (int i = 0; i < width * height; i++) {
            int idx = i * channels;
            gray[i] = (unsigned char)(0.299f * temp[idx] + 0.587f * temp[idx + 1] + 0.114f * temp[idx + 2]);
        }
    } else {
        for (int i = 0; i < width * height; i++) {
            int idx = i * channels;
            gray[i] = (unsigned char)(0.299f * temp[idx] + 0.587f * temp[idx + 1] + 0.114f * temp[idx + 2]);
        }
    }

    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    int Gy[3][3] = {
        { 1,  2,  1},
        { 0,  0,  0},
        {-1, -2, -1}
    };

    if (use_thread) {
        #pragma omp parallel for schedule(guided)
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                float gx = 0.0f, gy = 0.0f;

                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int pixel = gray[(y + ky) * width + (x + kx)];
                        gx += pixel * Gx[ky + 1][kx + 1];
                        gy += pixel * Gy[ky + 1][kx + 1];
                    }
                }

                float magnitude = sqrtf(gx * gx + gy * gy);

                int idx = (y * width + x) * channels;
                unsigned char edge_value = (magnitude > threshold) ? 255 : 0;

                for (int c = 0; c < channels; c++) {
                    if (channels == 4 && c == 3) {
                        image[idx + c] = temp[idx + c];
                    } else {
                        image[idx + c] = edge_value;
                    }
                }
            }
        }
    } else {
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                float gx = 0.0f, gy = 0.0f;

                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int pixel = gray[(y + ky) * width + (x + kx)];
                        gx += pixel * Gx[ky + 1][kx + 1];
                        gy += pixel * Gy[ky + 1][kx + 1];
                    }
                }

                float magnitude = sqrtf(gx * gx + gy * gy);

                int idx = (y * width + x) * channels;
                unsigned char edge_value = (magnitude > threshold) ? 255 : 0;

                for (int c = 0; c < channels; c++) {
                    if (channels == 4 && c == 3) {
                        image[idx + c] = temp[idx + c];
                    } else {
                        image[idx + c] = edge_value;
                    }
                }
            }
        }
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                for (int c = 0; c < channels; c++) {
                    if (channels == 4 && c == 3) {
                        continue;
                    }
                    int idx = (y * width + x) * channels + c;
                    image[idx] = 0;
                }
            }
        }
    }

    free(temp);
    free(gray);
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
                    #pragma omp simd
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
        for (int block_y = 0; block_y < height; block_y += CACHE_BLOCK_SIZE) {
            for (int block_x = 0; block_x < width; block_x += CACHE_BLOCK_SIZE) {
                const int max_y = (block_y + CACHE_BLOCK_SIZE < height) ? block_y + CACHE_BLOCK_SIZE : height;
                const int max_x = (block_x + CACHE_BLOCK_SIZE < width) ? block_x + CACHE_BLOCK_SIZE : width;

                for (int y = block_y; y < max_y; y++) {
                    #pragma omp simd
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

    const int min_pixels_per_thread = 10000;

    if (use_thread && total_pixels > min_pixels_per_thread) {
        #pragma omp parallel for schedule(static, 8192)
        for (int i = 0; i < total_pixels; i++) {
            const int idx = i * channels;
            const int r = image[idx];
            const int g = image[idx + 1];
            const int b = image[idx + 2];

            const int sepia_red   = CLAMP((r * c_red[0] + g * c_red[1] + b * c_red[2]) );
            const int sepia_green = CLAMP((r * c_green[0] + g * c_green[1] + b * c_green[2]));
            const int sepia_blue  = CLAMP((r * c_blue[0] + g * c_blue[1] + b * c_blue[2]));

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

            const int sepia_red   = CLAMP((r * c_red[0] + g * c_red[1] + b * c_red[2]) );
            const int sepia_green = CLAMP((r * c_green[0] + g * c_green[1] + b * c_green[2]) );
            const int sepia_blue  = CLAMP((r * c_blue[0] + g * c_blue[1] + b * c_blue[2]) );

            image[idx] = sepia_red;
            image[idx + 1] = sepia_green;
            image[idx + 2] = sepia_blue;
        }
    }
}