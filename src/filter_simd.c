#include "image_utils.h"
#include <omp.h>

#ifdef __x86_64__
#include <immintrin.h>
#endif


#ifdef __x86_64__
void grayscale_simd(unsigned char *image, int width, int height, int channels, float param) {
    if (channels != 3 && channels != 4) {
        grayscale(image, width, height, channels, param);
        return;
    }

    const float r_factor = 0.298f;
    const float g_factor = 0.587f;
    const float b_factor = 0.114f;

    const int total_pixels = width * height;
    const int simd_width = 4;
    const int block_size = 1024;

    if (use_thread) {
        #pragma omp parallel for schedule(static, block_size)
        for (int i = 0; i < total_pixels; i += simd_width) {
            if (i + simd_width <= total_pixels) {
                float r_vals[4], g_vals[4], b_vals[4];

                for (int j = 0; j < simd_width; j++) {
                    const int idx = (i + j) * channels;
                    r_vals[j] = image[idx];
                    g_vals[j] = image[idx + 1];
                    b_vals[j] = image[idx + 2];
                }

                __m128 r = _mm_loadu_ps(r_vals);
                __m128 g = _mm_loadu_ps(g_vals);
                __m128 b = _mm_loadu_ps(b_vals);

                __m128 r_weight = _mm_set1_ps(r_factor);
                __m128 g_weight = _mm_set1_ps(g_factor);
                __m128 b_weight = _mm_set1_ps(b_factor);

                __m128 gray = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(r, r_weight),
                        _mm_mul_ps(g, g_weight)
                    ),
                    _mm_mul_ps(b, b_weight)
                );

                float gray_vals[4];
                _mm_storeu_ps(gray_vals, gray);

                for (int j = 0; j < simd_width; j++) {
                    const int idx = (i + j) * channels;
                    unsigned char gray_byte = (unsigned char)gray_vals[j];
                    image[idx] = gray_byte;
                    image[idx + 1] = gray_byte;
                    image[idx + 2] = gray_byte;
                }
            } else {
                for (int j = 0; i + j < total_pixels; j++) {
                    const int idx = (i + j) * channels;
                    const float gray = r_factor * image[idx] +
                                      g_factor * image[idx + 1] +
                                      b_factor * image[idx + 2];
                    const unsigned char gray_byte = (unsigned char)gray;
                    image[idx] = gray_byte;
                    image[idx + 1] = gray_byte;
                    image[idx + 2] = gray_byte;
                }
            }
        }
    } else {
        for (int i = 0; i < total_pixels; i += simd_width) {
            if (i + simd_width <= total_pixels) {
                float r_vals[4], g_vals[4], b_vals[4];

                for (int j = 0; j < simd_width; j++) {
                    const int idx = (i + j) * channels;
                    r_vals[j] = image[idx];
                    g_vals[j] = image[idx + 1];
                    b_vals[j] = image[idx + 2];
                }

                __m128 r = _mm_loadu_ps(r_vals);
                __m128 g = _mm_loadu_ps(g_vals);
                __m128 b = _mm_loadu_ps(b_vals);

                __m128 r_weight = _mm_set1_ps(r_factor);
                __m128 g_weight = _mm_set1_ps(g_factor);
                __m128 b_weight = _mm_set1_ps(b_factor);

                __m128 gray = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(r, r_weight),
                        _mm_mul_ps(g, g_weight)
                    ),
                    _mm_mul_ps(b, b_weight)
                );

                float gray_vals[4];
                _mm_storeu_ps(gray_vals, gray);

                for (int j = 0; j < simd_width; j++) {
                    const int idx = (i + j) * channels;
                    unsigned char gray_byte = (unsigned char)gray_vals[j];
                    image[idx] = gray_byte;
                    image[idx + 1] = gray_byte;
                    image[idx + 2] = gray_byte;
                }
            } else {
                for (int j = 0; i + j < total_pixels; j++) {
                    const int idx = (i + j) * channels;
                    const float gray = r_factor * image[idx] +
                                      g_factor * image[idx + 1] +
                                      b_factor * image[idx + 2];
                    const unsigned char gray_byte = (unsigned char)gray;
                    image[idx] = gray_byte;
                    image[idx + 1] = gray_byte;
                    image[idx + 2] = gray_byte;
                }
            }
        }
    }
}
#endif
