#include "image_utils.h"
#include <math.h>

#ifdef __x86_64__
#include <immintrin.h>

void grayscale_simd(unsigned char *image, int width, int height, int channels, float param) {
    if (channels < 3) {
        fprintf(stderr, "Error: Grayscale SIMD filter requires at least 3 channels.\n");
        return;
    }

    const __m256 r_factor = _mm256_set1_ps(0.299f);
    const __m256 g_factor = _mm256_set1_ps(0.587f);
    const __m256 b_factor = _mm256_set1_ps(0.114f);

    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 8) {
            if (x + 8 <= width) {
                __m256 r_values = _mm256_setzero_ps();
                __m256 g_values = _mm256_setzero_ps();
                __m256 b_values = _mm256_setzero_ps();

                for (int i = 0; i < 8; i++) {
                    int pixel_idx = ((y * width) + (x + i)) * channels;
                    ((float*)&r_values)[i] = image[pixel_idx];
                    ((float*)&g_values)[i] = image[pixel_idx + 1];
                    ((float*)&b_values)[i] = image[pixel_idx + 2];
                }

                r_values = _mm256_mul_ps(r_values, r_factor);
                g_values = _mm256_mul_ps(g_values, g_factor);
                b_values = _mm256_mul_ps(b_values, b_factor);

                __m256 gray_values = _mm256_add_ps(r_values, g_values);
                gray_values = _mm256_add_ps(gray_values, b_values);

                for (int i = 0; i < 8; i++) {
                    int pixel_idx = ((y * width) + (x + i)) * channels;
                    unsigned char gray_byte = (unsigned char)((float*)&gray_values)[i];
                    image[pixel_idx] = gray_byte;
                    image[pixel_idx + 1] = gray_byte;
                    image[pixel_idx + 2] = gray_byte;
                }
            } else {
                for (int i = 0; i < (width - x); i++) {
                    int pixel_idx = ((y * width) + (x + i)) * channels;
                    const float gray = 0.299f * image[pixel_idx] + 0.587f * image[pixel_idx + 1] + 0.114f * image[pixel_idx + 2];
                    const unsigned char gray_byte = (unsigned char)gray;

                    image[pixel_idx] = gray_byte;
                    image[pixel_idx + 1] = gray_byte;
                    image[pixel_idx + 2] = gray_byte;
                }
            }
        }
    }
}
#endif



