#include <omp.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifdef __x86_64__
#include <immintrin.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define JPEG_QUALITY 90
#define CLAMP(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))

#define BLOCK_SIZE 64
#define CACHE_LINE 64

#define CACHE_BLOCK_SIZE 32

int use_thread = 1;

const char* file_format(const char* filename);
int is_valid_expression(const char* filename);
double tmp_atof(const char s[]);
int is_number(const char *str);
double filter_time(void (*func)(unsigned char*, int, int, int, float),
                   unsigned char *image, int width, int height, int channels, float param);

void grayscale(unsigned char *image, int width, int height, int channels, float param);
void invert(unsigned char *image, int width, int height, int channels, float param);
void brightness(unsigned char *image, int width, int height, int channels, float brightness);
void contrast(unsigned char *image, int width, int height, int channels, float factor);
void sepia(unsigned char *image, int width, int height, int channels, float param);

void grayscale_simd(unsigned char *image, int width, int height, int channels, float param);

typedef struct {
    const char *name;
    void (*func)(unsigned char*, int, int, int, float);
    int param;
} Filter;

Filter filter[] = {
    {"--grayscale", grayscale, 0},
    {"--invert", invert, 0},
    {"--brightness", brightness, 1},
    {"--contrast", contrast, 1},
    {"--sepia", sepia, 0},
};

const int num_filters = sizeof(filter) / sizeof(Filter);

int main(int argc, char *argv[]) {
    printf("argc: %d\n%s\n", argc, file_format(argv[1]));

    printf("Processing with %d threads\n", omp_get_max_threads());

    int num_threads = omp_get_num_procs();
    omp_set_num_threads(num_threads);

    if(argc < 3) {
        fprintf(stderr, "Usage: %s input.jpg output.jpg [--filter] [param value]\n", argv[0]);
        for (int i = 0; i < num_filters; i++) {
            fprintf(stderr, "  %s\n", filter[i].name);
        }
        return 1;
    }

    if( !is_valid_expression(argv[1]) || !is_valid_expression(argv[2])) {
        fprintf(stderr, "Error: only .png & .jpg files support");
        return 1;
    }

    FILE* input_file = fopen(argv[1], "rb");
    if (!input_file) {
        fprintf(stderr, "Error: could not open file %s\n", argv[1]);
        return 1;
    }

    setvbuf(input_file, NULL, _IOFBF, 1024 * 1024);

    int width, height, channels;
    unsigned char *image = stbi_load(argv[1], &width, &height, &channels, 0);

    if (image == NULL) {
        fprintf(stderr, "Error: failed to load image %s. Reason: %s\n", argv[1], stbi_failure_reason());
        return 1;
    }
    printf("%s Image: %dx%d, Channels: %d\n", file_format(argv[1]), width, height, channels);

    int benchmark_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0) {
            benchmark_mode = 1;
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            break;
        }
    }

    unsigned char *image_copy = NULL;
    if (benchmark_mode) {
        image_copy = (unsigned char *)malloc(width * height * channels);
        if (!image_copy) {
            fprintf(stderr, "Error: failed to allocate memory for image benchmark\n");
            stbi_image_free(image_copy);
            return 1;
        }
    }

    for(int i = 3; i < argc; i++) {
        for (int j = 0; j < num_filters; j++) {
            if (strcmp(argv[i], filter[j].name) == 0) {
                float param = 1.0f;
                if (filter[j].param) {
                    if (i + 1 >= argc || !is_number(argv[i + 1])) {
                        fprintf(stderr, "Usage: %s [param value]\n", filter[j].name);
                        stbi_image_free(image);
                        if (image_copy) free(image_copy);
                        return 1;
                    }
                    param = tmp_atof(argv[i + 1]);
                    i++;
                }
                if (benchmark_mode) {
                    memcpy(image_copy, image, width * height * channels);

                    use_thread = 1;
                    double mt_time = filter_time(filter[j].func, image, width, height, channels, param);

                    use_thread = 0;
                    double st_time = filter_time(filter[j].func, image_copy, width, height, channels, param);

                    use_thread = 1;

                    printf("\n--- Performance Benchmark for %s ---\n", filter[j].name);
                    printf("Multi-threaded execution time: %.6f seconds\n", mt_time);
                    printf("Single-threaded execution time: %.6f seconds\n", st_time);
                    printf("Speedup: %.2fx\n", st_time / mt_time);
                    printf("------------------------------------------\n");

                    free(image_copy);
                }else
                    filter[j].func(image, width, height, channels, param);
                break;
            }
        }
    }

    const char *ext = strrchr(argv[2], '.');
    if (ext != NULL) {
        if (strstr(ext, ".jpg") || strstr(ext, ".jpeg")) {
            if (!stbi_write_jpg(argv[2], width, height, channels, image, JPEG_QUALITY)) {
                fprintf(stderr, "Failed to write JPEG.\n");
                stbi_image_free(image);
                if (image_copy) free(image_copy);
                return 1;
            }
        } else if (strstr(ext, ".png")) {
            if(!stbi_write_png(argv[2], width, height, channels, image, width * channels)) {
                fprintf(stderr, "Failed to write PNG.\n");
                stbi_image_free(image);
                if (image_copy) free(image_copy);
                return 1;
            }
        }
    } else {
        fprintf(stderr, "Failed to open: no extension \n");
        stbi_image_free(image);
        if (image_copy) free(image_copy);
        return 1;
    }
    stbi_image_free(image);
    if (image_copy) free(image_copy);
    return 0;
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

int is_valid_expression(const char* filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    return (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".png") == 0);
}

double tmp_atof(const char s[]) {
    if (s == NULL || strlen(s) == 0) {
        fprintf(stderr, "Error: Invalid input.\n");
        return 0.0;
    }
    double val, power;
    int i, sign;
    for(i = 0; isspace(s[i]); i++)
        ;

    sign = (s[i] == '-') ? -1 : 1;

    if(s[i] == '+' || s[i] == '-') i++;

    for(val = 0.0; isdigit(s[i]); i++)
        val = 10.0 * val + (s[i] - '0');

    if(s[i] == '.') i++;

    for(power = 1.0; isdigit(s[i]); i++){
        val = 10.0 * val + (s[i] - '0');
        power *= 10.0;
    }

    return sign * val / power;
}

int is_number(const char *str) {
    int has_dot = 0;
    for (int i = 0; str[i]; i++) {
        if (i == 0 && (str[i] == '-' || str[i] == '+')) continue;

        if (str[i] == '.') {
            if (has_dot) return 0;
            has_dot = 1;
            continue;
        }

        if (!isdigit(str[i])) return 0;
    }

    return 1;
}

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
#ifdef __x86_64__
void grayscale_simd(unsigned char *image, int width, int height, int channels, float param) {
    if(channels != 3 && channels != 4) {
        grayscale(image, width, height, channels, param);
        return;
    }

    __m128 r_weight = _mm_set1_ps(0.298f);
    __m128 g_weight = _mm_set1_ps(0.587f);
    __m128 b_weight = _mm_set1_ps(0.114f);

    if (use_thread) {
#pragma omp parallel for schedule(guided)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x += 4) {
                int idx = (y * width + x) * channels;

                __m128 r_vals = _mm_set_ps(
                    image[idx + 0], image[idx + channels + 0],
                    image[idx + 2 * channels + 0], image[idx + 3 * channels + 0]
                );

                __m128 g_vals = _mm_set_ps(
                    image[idx + 1], image[idx + channels + 1],
                    image[idx + 2 * channels + 1], image[idx + 3 * channels + 1]
                );

                __m128 b_vals = _mm_set_ps(
                    image[idx + 2], image[idx + channels + 2],
                    image[idx + 2 * channels + 2], image[idx + 3 * channels + 2]
                );

                __m128 gray_vals = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(r_vals, r_weight),
                        _mm_mul_ps(g_vals, g_weight)
                    ),
                    _mm_mul_ps(b_vals, b_weight)
                );

                float gray_array[4];
                _mm_storeu_ps(gray_array, gray_vals);

                for (int i = 0; i < 4; i++) {
                    int pixel_idx = idx + i * channels;
                    unsigned char gray_byte = (unsigned char)gray_array[i];
                    image[pixel_idx + 0] = gray_byte;
                    image[pixel_idx + 1] = gray_byte;
                    image[pixel_idx + 2] = gray_byte;

                }
            }

            for (int x = (width / 4) * 4; x < width; x++) {
                int idx = (y * width + x) * channels;
                float gray = 0.298f * image[idx + 0] + 0.587f * image[idx + 1] + 0.114f * image[idx + 2];
                unsigned char gray_byte = (unsigned char)gray;
                image[idx + 0] = gray_byte;
                image[idx + 1] = gray_byte;
                image[idx + 2] = gray_byte;
            }
        }
    } else {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x += 4) {
                int idx = (y * width + x) * channels;

                __m128 r_vals = _mm_set_ps(
                    image[idx + 0], image[idx + channels + 0],
                    image[idx + 2 * channels + 0], image[idx + 3 * channels + 0]
                );

                __m128 g_vals = _mm_set_ps(
                    image[idx + 1], image[idx + channels + 1],
                    image[idx + 2 * channels + 1], image[idx + 3 * channels + 1]
                );

                __m128 b_vals = _mm_set_ps(
                    image[idx + 2], image[idx + channels + 2],
                    image[idx + 2 * channels + 2], image[idx + 3 * channels + 2]
                );

                __m128 gray_vals = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(r_vals, r_weight),
                        _mm_mul_ps(g_vals, g_weight)
                    ),
                    _mm_mul_ps(b_vals, b_weight)
                );

                float gray_array[4];
                _mm_storeu_ps(gray_array, gray_vals);

                for (int i = 0; i < 4; i++) {
                    int pixel_idx = idx + i * channels;
                    unsigned char gray_byte = (unsigned char)gray_array[i];
                    image[pixel_idx + 0] = gray_byte;
                    image[pixel_idx + 1] = gray_byte;
                    image[pixel_idx + 2] = gray_byte;

                }
            }

            for (int x = (width / 4) * 4; x < width; x++) {
                int idx = (y * width + x) * channels;
                float gray = 0.298f * image[idx + 0] + 0.587f * image[idx + 1] + 0.114f * image[idx + 2];
                unsigned char gray_byte = (unsigned char)gray;
                image[idx + 0] = gray_byte;
                image[idx + 1] = gray_byte;
                image[idx + 2] = gray_byte;
            }
        }
    }

}
#endif
