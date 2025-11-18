#include "esteg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAGIC_NUMBER 0x53544547  // "STEG"

typedef struct {
    uint32_t magic;
    uint32_t data_size;
} StegoHeader;

static uint32_t get_bmp_pixel_offset(unsigned char *bmp_data) {
    return bmp_data[10] | (bmp_data[11] << 8) | 
           (bmp_data[12] << 16) | (bmp_data[13] << 24);
}

int steg_hide(const char *image_path, const unsigned char *data, 
              size_t data_size, const char *output_path) {
    
    FILE *img = fopen(image_path, "rb");
    if (!img) {
        perror("Erro ao abrir imagem");
        return -1;
    }

    // Lê imagem
    fseek(img, 0, SEEK_END);
    size_t img_size = ftell(img);
    fseek(img, 0, SEEK_SET);

    unsigned char *image_buffer = malloc(img_size);
    if (!image_buffer) {
        perror("Erro ao alocar memória");
        fclose(img);
        return -1;
    }
    fread(image_buffer, 1, img_size, img);
    fclose(img);

    // Verifica BMP
    if (image_buffer[0] != 0x42 || image_buffer[1] != 0x4D) {
        fprintf(stderr, "Erro: arquivo não é BMP válido\n");
        free(image_buffer);
        return -1;
    }

    uint32_t pixel_offset = get_bmp_pixel_offset(image_buffer);
    
    // Calcula capacidade
    size_t available = img_size - pixel_offset;
    size_t capacity = (available / 8) - sizeof(StegoHeader);

    if (data_size > capacity) {
        fprintf(stderr, "Erro: dados muito grandes para a imagem\n");
        fprintf(stderr, "Capacidade: %zu bytes, necessário: %zu bytes\n", 
                capacity, data_size);
        free(image_buffer);
        return -1;
    }

    // Prepara cabeçalho
    StegoHeader header;
    header.magic = MAGIC_NUMBER;
    header.data_size = data_size;

    size_t img_pos = pixel_offset;
    
    // Esconde cabeçalho
    unsigned char *header_bytes = (unsigned char *)&header;
    for (size_t i = 0; i < sizeof(StegoHeader); i++) {
        unsigned char byte = header_bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_pos] = (image_buffer[img_pos] & 0xFE) | data_bit;
            img_pos++;
        }
    }

    // Esconde dados
    for (size_t i = 0; i < data_size; i++) {
        unsigned char byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_pos] = (image_buffer[img_pos] & 0xFE) | data_bit;
            img_pos++;
        }
    }

    // Salva
    FILE *output = fopen(output_path, "wb");
    if (!output) {
        perror("Erro ao criar arquivo de saída");
        free(image_buffer);
        return -1;
    }
    
    fwrite(image_buffer, 1, img_size, output);
    fclose(output);
    free(image_buffer);

    return 0;
}

int steg_extract(const char *image_path, unsigned char **data, 
                 size_t *data_size) {
    
    FILE *img = fopen(image_path, "rb");
    if (!img) {
        perror("Erro ao abrir imagem");
        return -1;
    }

    fseek(img, 0, SEEK_END);
    size_t img_size = ftell(img);
    fseek(img, 0, SEEK_SET);

    unsigned char *image_buffer = malloc(img_size);
    if (!image_buffer) {
        perror("Erro ao alocar memória");
        fclose(img);
        return -1;
    }
    fread(image_buffer, 1, img_size, img);
    fclose(img);

    uint32_t pixel_offset = get_bmp_pixel_offset(image_buffer);
    size_t img_pos = pixel_offset;

    // Extrai cabeçalho
    StegoHeader header;
    unsigned char *header_bytes = (unsigned char *)&header;
    
    for (size_t i = 0; i < sizeof(StegoHeader); i++) {
        unsigned char byte = 0;
        for (int bit = 0; bit < 8; bit++) {
            unsigned char lsb = image_buffer[img_pos] & 1;
            byte |= (lsb << bit);
            img_pos++;
        }
        header_bytes[i] = byte;
    }

    if (header.magic != MAGIC_NUMBER) {
        fprintf(stderr, "Erro: dados não encontrados na imagem\n");
        free(image_buffer);
        return -1;
    }

    // Extrai dados
    *data = malloc(header.data_size);
    if (!*data) {
        perror("Erro ao alocar memória");
        free(image_buffer);
        return -1;
    }

    for (size_t i = 0; i < header.data_size; i++) {
        unsigned char byte = 0;
        for (int bit = 0; bit < 8; bit++) {
            unsigned char lsb = image_buffer[img_pos] & 1;
            byte |= (lsb << bit);
            img_pos++;
        }
        (*data)[i] = byte;
    }

    *data_size = header.data_size;
    free(image_buffer);

    return 0;
}

int steg_hide_file(const char *image_path, const char *file_path, 
                   const char *output_path) {
    
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        perror("Erro ao abrir arquivo");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *file_data = malloc(file_size);
    if (!file_data) {
        perror("Erro ao alocar memória");
        fclose(f);
        return -1;
    }

    fread(file_data, 1, file_size, f);
    fclose(f);

    int result = steg_hide(image_path, file_data, file_size, output_path);
    free(file_data);

    return result;
}

int steg_extract_file(const char *image_path, const char *output_path) {
    unsigned char *data = NULL;
    size_t data_size = 0;

    if (steg_extract(image_path, &data, &data_size) != 0) {
        return -1;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Erro ao criar arquivo de saída");
        free(data);
        return -1;
    }

    fwrite(data, 1, data_size, out);
    fclose(out);
    free(data);

    return 0;
}

long steg_get_capacity(const char *image_path) {
    FILE *img = fopen(image_path, "rb");
    if (!img) {
        return -1;
    }

    fseek(img, 0, SEEK_END);
    size_t img_size = ftell(img);
    fseek(img, 0, SEEK_SET);

    unsigned char header[14];
    fread(header, 1, 14, img);
    fclose(img);

    if (header[0] != 0x42 || header[1] != 0x4D) {
        return -1;
    }

    uint32_t pixel_offset = get_bmp_pixel_offset(header);
    size_t available = img_size - pixel_offset;
    size_t capacity = (available / 8) - sizeof(StegoHeader);

    return capacity;
}