#include "image_utils.h"
#include "stb_include.h"
#include "logger.h"
#include <omp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_IO = 1,
    ERROR_INVALID_ARGS = 2
} ErrorCode;

Filter filter[] = {
    {"--grayscale", grayscale, 0,
        "Convert image to grayscale", 0.0f, 0.0f},
    {"--invert", invert, 0,
        "Invert image colors", 0.0f, 0.0f},
    {"--brightness", brightness, 1,
        "Adjust brightness", 0.1f, 2.0f},
    {"--contrast", contrast, 1,
        "Adjust contrast", 0.1f, 2.0f},
    {"--sepia", sepia, 0,
        "Apply sepia effect", 0.0f, 0.0f},
    {"--blur", gaussian_blur, 1,
        "Apply Gaussian blur", 1.0f, 10.0f},
    {"--edge", edge_detect, 1,
        "Apply edge detection", 0.0f, 255.0f}
    // {"--canny", canny_edge_detect_adapter, 1,
    // "Apply Canny edge detection", 20.0f, 200.0f}

};

const int num_filters = sizeof(filter) / sizeof(Filter);

void usage(const char* name) {
    printf(stderr, "Usage: %s input.jpg output.jpg [--filter] [param value] [--benchmark]\n", name);
    fprintf(stderr, "Available filters:\n");

    for (int i = 0; i < num_filters; i++) {
        if (filter[i].param) {
            if (strcmp(filter[i].name, "--brightness") == 0) {
                fprintf(stderr, "  %s [0.1-2.0] - %s\n", filter[i].name, filter[i].description);
            }
            else if (strcmp(filter[i].name, "--contrast") == 0) {
                fprintf(stderr, "  %s [0.1-2.0] - %s\n", filter[i].name, filter[i].description);
            }
            else if (strcmp(filter[i].name, "--blur") == 0) {
                fprintf(stderr, "  %s [1.0-10.0] - %s\n", filter[i].name, filter[i].description);
            }
            else if (strcmp(filter[i].name, "--edge") == 0) {
                fprintf(stderr, "  %s [0.0-255.0] - %s\n", filter[i].name, filter[i].description);
            }
            else {
                fprintf(stderr, "  %s [value] - %s\n", filter[i].name, filter[i].description);
            }
        } else {
            fprintf(stderr, "  %s - %s\n", filter[i].name, filter[i].description);
        }
    }

    fprintf(stderr, "  --benchmark - Compare single and multi-threaded execution\n");
}

int validate(const char* filter_name, float value) {
    for (int i = 0; i < num_filters; i++) {
        if (strcmp(filter_name, filter[i].name) == 0) {
            if (!filter[i].param) {
                return 1;
            }

            if (strcmp(filter_name, "--brightness") == 0) {
                if (value < filter[i].min || value > filter[i].max) {
                    fprintf(stderr, "Error: Brightness must be between %.1f and %.1f.\n",
                            filter[i].min, filter[i].max);
                    return 0;
                }
            }
            else if (strcmp(filter_name, "--contrast") == 0) {
                if (value < filter[i].min || value > filter[i].max) {
                    fprintf(stderr, "Error: Contrast must be between %.1f and %.1f.\n",
                            filter[i].min, filter[i].max);
                    return 0;
                }
            }
            else if (strcmp(filter_name, "--blur") == 0) {
                if (value < filter[i].min || value > filter[i].max) {
                    fprintf(stderr, "Error: Sigma must be between %.1f and %.1f\n",
                            filter[i].min, filter[i].max);
                    return 0;
                }
            }
            else if (strcmp(filter_name, "--edge") == 0) {
                if (value < filter[i].min || value > filter[i].max) {
                    fprintf(stderr, "Error: Threshold must be between %.1f and %.1f.\n",
                            filter[i].min, filter[i].max);
                    return 0;
                }
            }

            return 1;
        }
    }

    return 0;
}

void cleanup(unsigned char *image, unsigned char *image_copy) {
    if (image) stbi_image_free(image);
    if (image_copy) free(image_copy);
    log_close();
}

int main(int argc, char *argv[]) {
    if (!log_init("image_filter.log", LOG_DEBUG)) {
        fprintf(stderr, "Failed to initialize logging system\n");
        return ERROR_IO;
    }

    log_info("Program started with %d arguments", argc);

    if (argc < 3) {
        log_error("Not enough arguments. Provided: %d, minimum required: 3", argc);
        usage(argv[0]);
        log_close();
        return ERROR_INVALID_ARGS;
    }

    if (argc > 1) {
        printf("argc: %d\n%s\n", argc, file_format(argv[1]));
        log_debug("Input file format: %s", file_format(argv[1]));
    }

    int num_threads = omp_get_num_procs();
    omp_set_num_threads(num_threads);
    printf("Processing with %d threads\n", omp_get_max_threads());
    log_info("Processing with %d threads", omp_get_max_threads());

    if (!is_valid_expression(argv[1]) || !is_valid_expression(argv[2])) {
        log_error("Unsupported file format: %s or %s", argv[1], argv[2]);
        fprintf(stderr, "Error: only .png & .jpg files supported\n");
        log_close();
        return ERROR_INVALID_ARGS;
    }

    log_info("Attempting to open file: %s", argv[1]);
    FILE* input_file = fopen(argv[1], "rb");
    if (!input_file) {
        log_error("Cannot open file: %s", argv[1]);
        fprintf(stderr, "Error: could not open file %s\n", argv[1]);
        log_close();
        return ERROR_IO;
    }
    log_debug("File successfully opened: %s", argv[1]);

    setvbuf(input_file, NULL, _IOFBF, 1024 * 1024);
    fclose(input_file);

    int width, height, channels;
    log_info("Loading image: %s", argv[1]);
    unsigned char *image = stbi_load(argv[1], &width, &height, &channels, 0);

    if (image == NULL) {
        log_error("Failed to load image %s. Reason: %s", argv[1], stbi_failure_reason());
        fprintf(stderr, "Error: failed to load image %s. Reason: %s\n", argv[1], stbi_failure_reason());
        log_close();
        return ERROR_IO;
    }
    printf("%s Image: %dx%d, Channels: %d\n", file_format(argv[1]), width, height, channels);
    log_info("Image loaded: %s, %dx%d, %d channels", file_format(argv[1]), width, height, channels);

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
        log_info("Running in benchmark mode");
        log_debug("Allocating memory for image copy: %d bytes", width * height * channels);
        image_copy = (unsigned char *)malloc(width * height * channels);
        if (!image_copy) {
            log_error("Failed to allocate memory for image copy (%d bytes)", width * height * channels);
            fprintf(stderr, "Error: failed to allocate memory for image benchmark\n");
            stbi_image_free(image);
            log_close();
            return ERROR_IO;
        }
    }

    for (int i = 3; i < argc; i++) {
        int filter_found = 0;

        for (int j = 0; j < num_filters; j++) {
            if (strcmp(argv[i], filter[j].name) == 0) {
                filter_found = 1;
                float param = 1.0f;

                if (filter[j].param) {
                    if (i + 1 >= argc || !is_number(argv[i + 1])) {
                        log_error("Filter %s requires a numeric parameter", filter[j].name);
                        fprintf(stderr, "Error: %s requires a numeric parameter\n", filter[j].name);
                        cleanup(image, image_copy);
                        return ERROR_INVALID_ARGS;
                    }

                    param = tmp_atof(argv[i + 1]);

                    if (!validate(filter[j].name, param)) {
                        log_error("Invalid parameter value %.2f for filter %s", param, filter[j].name);
                        cleanup(image, image_copy);
                        return ERROR_INVALID_ARGS;
                    }

                    log_info("Applying filter %s with parameter %.2f", filter[j].name, param);
                    i++;
                } else {
                    log_info("Applying filter %s", filter[j].name);
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

                    log_info("Benchmark for filter %s: multi-threaded - %.6f s, single-threaded - %.6f s, speedup - %.2fx",
                              filter[j].name, mt_time, st_time, st_time / mt_time);
                } else {
                    filter[j].func(image, width, height, channels, param);
                }
                break;
            }
        }

        if (!filter_found) {
            log_error("Unknown filter: %s", argv[i]);
            fprintf(stderr, "Error: Unknown filter: %s\n", argv[i]);
            usage(argv[0]);
            cleanup(image, image_copy);
            return ERROR_INVALID_ARGS;
        }
    }

    log_info("Saving result to file: %s", argv[2]);
    const char *ext = strrchr(argv[2], '.');
    if (ext != NULL) {
        if (strstr(ext, ".jpg") || strstr(ext, ".jpeg")) {
            log_debug("Saving in JPEG format with quality %d", JPEG_QUALITY);
            if (!stbi_write_jpg(argv[2], width, height, channels, image, JPEG_QUALITY)) {
                log_error("Failed to write JPEG file: %s", argv[2]);
                fprintf(stderr, "Error: failed to write JPEG file %s\n", argv[2]);
                cleanup(image, image_copy);
                return ERROR_IO;
            }
        } else if (strstr(ext, ".png")) {
            log_debug("Saving in PNG format");
            if(!stbi_write_png(argv[2], width, height, channels, image, width * channels)) {
                log_error("Failed to write PNG file: %s", argv[2]);
                fprintf(stderr, "Error: failed to write PNG file %s\n", argv[2]);
                cleanup(image, image_copy);
                return ERROR_IO;
            }
        }
    } else {
        log_error("Output file has no extension: %s", argv[2]);
        fprintf(stderr, "Error: output file has no extension\n");
        cleanup(image, image_copy);
        return ERROR_INVALID_ARGS;
    }

    log_info("File successfully saved: %s", argv[2]);

    log_debug("Freeing image memory");
    stbi_image_free(image);
    if (image_copy) {
        log_debug("Freeing image copy memory");
        free(image_copy);
    }

    log_info("Program completed successfully");
    log_close();
    return ERROR_SUCCESS;
}