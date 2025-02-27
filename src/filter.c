#include "image_utils.h"
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
int use_thread = 1;


double filter_time(void (*func)(unsigned char*, int, int, int, float),
                   unsigned char *image, int width, int height, int channels, float param) {
    double start_time = omp_get_wtime();
    func(image, width, height, channels, param);
    double end_time = omp_get_wtime();
    return end_time - start_time;
}

static void gaussian_kernel(float *kernel, int radius, float sigma) {
    float sum = 0.0f;
    int size = 2 * radius + 1;

    for (int i = -radius; i <= radius; i++) {
        float x = i;
        kernel[i + radius] = expf(-x * x / (2 * sigma * sigma)) / (sqrtf(2 * M_PI) * sigma);
        sum += kernel[i + radius];
    }

    for (int i = 0; i < size; i++) {
        kernel[i] /= sum;
    }
}

static void horizontal_blur(unsigned char *src, unsigned char *dst, int width, int height, int channels,
                            float *kernel, int radius) {
    int kernel_size = 2 * radius + 1;
    if (use_thread) {
        #pragma omp parallel for schedule(guided)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float sum[3] = {0};
                for (int k = -radius; k <= radius; k++) {
                    int px = x + k;
                    if (px < 0) px = 0;
                    if (px >= width) px = width - 1;
                    int idx = (y * width + px) * channels;
                    float weight = kernel[k + radius];
                    sum[0] += src[idx] * weight;
                    sum[1] += src[idx + 1] * weight;
                    sum[2] += src[idx + 2] * weight;
                }
                int idx = (y * width + x) * channels;
                dst[idx] = CLAMP(sum[0]);
                dst[idx + 1] = CLAMP(sum[1]);
                dst[idx + 2] = CLAMP(sum[2]);
            }
        }
    } else {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float sum[3] = {0};
                for (int k = -radius; k <= radius; k++) {
                    int px = x + k;
                    if (px < 0) px = 0;
                    if (px >= width) px = width - 1;
                    int idx = (y * width + px) * channels;
                    float weight = kernel[k + radius];
                    sum[0] += src[idx] * weight;
                    sum[1] += src[idx + 1] * weight;
                    sum[2] += src[idx + 2] * weight;
                }
                int idx = (y * width + x) * channels;
                dst[idx] = CLAMP(sum[0]);
                dst[idx + 1] = CLAMP(sum[1]);
                dst[idx + 2] = CLAMP(sum[2]);
            }
        }
    }
}

static void vertical_blur(unsigned char *src, unsigned char *dst, int width, int height, int channels,
                            float *kernel, int radius) {
    int kernel_size = 2 * radius + 1;
    if (use_thread) {
#pragma omp parallel for schedule(guided)
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                float sum[3] = {0};
                for (int k = -radius; k <= radius; k++) {
                    int py = y + k;
                    if (py < 0) py = 0;
                    if (py >= height) py = height - 1;
                    int idx = (py * width + x) * channels;
                    float weight = kernel[k + radius];
                    sum[0] += src[idx] * weight;
                    sum[1] += src[idx + 1] * weight;
                    sum[2] += src[idx + 2] * weight;
                }
                int idx = (y * width + x) * channels;
                dst[idx] = CLAMP(sum[0]);
                dst[idx + 1] = CLAMP(sum[1]);
                dst[idx + 2] = CLAMP(sum[2]);
            }
        }
    } else {
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                float sum[3] = {0};
                for (int k = -radius; k <= radius; k++) {
                    int py = y + k;
                    if (py < 0) py = 0;
                    if (py >= height) py = height - 1;
                    int idx = (py * width + x) * channels;
                    float weight = kernel[k + radius];
                    sum[0] += src[idx] * weight;
                    sum[1] += src[idx + 1] * weight;
                    sum[2] += src[idx + 2] * weight;
                }
                int idx = (y * width + x) * channels;
                dst[idx] = CLAMP(sum[0]);
                dst[idx + 1] = CLAMP(sum[1]);
                dst[idx + 2] = CLAMP(sum[2]);
            }
        }
    }
}

void gaussian_blur(unsigned char *image, int width, int height, int channels, float radius) {
    if (radius < 1.0f || radius > 10.0f) {
        fprintf(stderr, "Error: Radius must be between 1 and 10.\n");
        return;
    }
    int kernel_radius = (int)ceil(radius);
    int kernel_size = 2 * kernel_radius + 1;

    float *kernel = (float*)malloc(kernel_size * sizeof(float));
    if (!kernel) {
        fprintf(stderr, "Error allocating kernel.\n");
        return;
    }

    gaussian_kernel(kernel, kernel_radius, radius);

    unsigned char *temp = (unsigned char*)malloc(width * height * channels);
    if (!temp) {
        fprintf(stderr, "Error: Failed to allocate temporary buffer.\n");
        free(kernel);
        return;
    }

    horizontal_blur(image, temp, width, height, channels, kernel, kernel_radius);
    vertical_blur(temp, image, width, height, channels, kernel, kernel_radius);

    free(temp);
    free(kernel);
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