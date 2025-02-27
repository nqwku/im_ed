#include "image_utils.h"
#include "stb_include.h"
#include <omp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Filter filter[] = {
    {"--grayscale", grayscale, 0},
    {"--invert", invert, 0},
    {"--brightness", brightness, 1},
    {"--contrast", contrast, 1},
    {"--sepia", sepia, 0},
    {"--blur", gaussian_blur, 1},
    {"--edge", edge_detect, 1}
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
    fclose(input_file);

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
            stbi_image_free(image);
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

                    // free(image_copy); не освобождаю тут память - так как ее еще используем
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
