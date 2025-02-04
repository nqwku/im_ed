#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define JPEG_QUALITY 90
#define CLAMP(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))

const char* file_format(const char* filename);
int is_valid_expression(const char* filename);
double tmp_atof(char s[]);
int is_number(const char *str);

void grayscale(unsigned char *image, int width, int height, int channels);
void invert(unsigned char *image, int width, int height, int channels);
void brightness(unsigned char *image, int width, int height, int channels, float brightness);
void contrast(unsigned char *image, int width, int height, int channels, float factor);
void sepia(unsigned char *image, int width, int height, int channels);

int main(int argc, char *argv[]) {
    int width, height, channels;

    printf("argc: %d\n%s\n", argc, file_format(argv[1]));

    int gr_stat = 0; // grayscale status
    int in_stat = 0; // invert status
    int br_stat = 0; // brightness status
    int co_stat = 0; // contrast status
    int se_stat = 0; // sepia status

    double brightness_value = 1.0;
    double contrast_value = 1.0;

    if(argc < 3) {
        fprintf(stderr, "Usage: %s input.jpg output.jpg [--filter] [param value (0.5 - 2.0)]\n", argv[0]);
        return 1;
    }

    if(!is_valid_expression(argv[2])) {
        fprintf(stderr, "Error: only .png & .jpg files support");
        return 1;
    }

    if((strcmp(file_format(argv[1]), "JPEG") == 0 || strcmp(file_format(argv[1]), "PNG") == 0)){
        unsigned char *image = stbi_load(argv[1], &width, &height, &channels, 0);
        printf("%s Image: %dx%d, Channels: %d\n", file_format(argv[1]), width, height, channels);

        if (image == NULL) {
            fprintf(stderr, "Failed to load image.\n");
            return 1;
        }

        for(int i = 3; i < argc; i++){
            if(argv[i][0] == '-'){
                if(strcmp(argv[i], "--grayscale") == 0) gr_stat = 1;

                else if(strcmp(argv[i], "--invert") == 0) in_stat = 1;

                else if(strcmp(argv[i], "--brightness") == 0) {
                    if(i + 1 >= argc || !is_number(argv[i + 1])){
                        fprintf(stderr, "Usage: --brightness [param value (0.5 - 2.0)]\n");
                        return 1;
                    }

                    brightness_value = tmp_atof(argv[i + 1]);

                    if(brightness_value < 0.5 || brightness_value > 2.0){
                        fprintf(stderr, "Error, %.1f parameter is out of range\n", brightness_value);
                        return 1;
                    }
                    i++;
                    br_stat = 1;
                }

                else if(strcmp(argv[i], "--contrast") == 0) {
                    if(i + 1 >= argc || !is_number(argv[i + 1])){
                        fprintf(stderr, "Usage: --contrast [param value (0.5 - 2.0)]\n");
                        return 1;
                    }

                    contrast_value = tmp_atof(argv[i + 1]);

                    if(contrast_value < 0.5 || contrast_value > 2.0){
                        fprintf(stderr, "Error, %.1f parameter is out of range\n", contrast_value);
                        return 1;
                    }
                    i++;
                    co_stat = 1;
                }

                else if(strcmp(argv[i], "--sepia") == 0) se_stat = 1;

                else{
                    fprintf(stderr, "Unknown flag %s\n", argv[i]);
                    return 1;
                }
            }
            else{
                fprintf(stderr, "Unsupported flag %s\n", argv[i]);
                return 1;
            }
        }

        if (gr_stat) grayscale(image, width, height, channels);
        if (in_stat) invert(image, width, height, channels);
        if (br_stat) brightness(image, width, height, channels, brightness_value);
        if (co_stat) contrast(image, width, height, channels, contrast_value);
        if (se_stat) sepia(image, width, height, channels);

        const char *ext = strrchr(argv[2], '.');
        if (ext !=NULL) {
            if (strstr(ext, ".jpg") || strstr(ext, ".jpeg")) {
                if (!stbi_write_jpg(argv[2], width, height, channels, image, 100)) {
                    fprintf(stderr, "Failed to write JPEG.\n");
                    return 1;
                }
            } else if (strstr(ext, ".png")) {
                if(!stbi_write_png(argv[2], width, height, channels, image, width * channels)) {
                    fprintf(stderr, "Failed to write PNG.\n");
                    return 1;
                }
            }
        } else {
            fprintf(stderr, "Failed to open: no extencion \n");
            return 1;
        }
        stbi_image_free(image);
    }

    else{
        fprintf(stderr, "Only .png & .jpg files support\n");
        return 1;
    }
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

double tmp_atof(char s[]) {
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
        tmp_image = CLAMP(factor * (tmp_image - 128) + 128);
        image[i] = (unsigned char)tmp_image;
    }
}

// sepiaRed = .393*R + .769*G + .189B | sepiaGreen = .349*R + .686*G + .168B | sepiaBlue= .272*R + .534*G + .131B
// (393, 769, 189)
// (349, 686, 168)
// (272, 534, 131)

void sepia(unsigned char *image, int width, int height, int channels) {
    if (channels != 3 && channels != 4) return;

    static const int c_red[3] = {393, 769, 189};
    static const int c_green[3] = {349, 686, 168};
    static const int c_blue[3] = {272, 534, 131};

    for (size_t i = 0; i < width * height * channels; i += channels) {
        const int r = image[i];
        const int g = image[i + 1];
        const int b = image[i + 2];

        const int sepia_red   = CLAMP((r * c_red[0] + g * c_red[1] + b * c_red[2] + 500)  / 1000);
        const int sepia_green = CLAMP((r * c_green[0] + g * c_green[1] + b * c_green[2] + 500)  / 1000);
        const int sepia_blue  = CLAMP((r * c_blue[0] + g * c_blue[1] + b * c_blue[2] + 500)  / 1000);

        image[i] = sepia_red;
        image[i + 1] = sepia_green;
        image[i + 2] = sepia_blue;
    }
}

