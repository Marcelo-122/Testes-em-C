#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAGIC_NUMBER 0x53544547

typedef struct {
    uint32_t magic;
    uint32_t data_size;
} StegoHeader;

uint32_t get_bmp_data_offset(unsigned char *bmp_data) {
    uint32_t offset = bmp_data[10] | 
                     (bmp_data[11] << 8) | 
                     (bmp_data[12] << 16) | 
                     (bmp_data[13] << 24);
    return offset;
}

void print_first_bytes(unsigned char *buffer, size_t count, const char *label) {
    printf("%s: ", label);
    for (size_t i = 0; i < count && i < 20; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

int hide_data(const char *image_path, const char *data_path, const char *output_path) {
    FILE *img = fopen(image_path, "rb");
    FILE *data = fopen(data_path, "rb");
    
    if (!img || !data) {
        perror("Erro ao abrir arquivos");
        if (img) fclose(img);
        if (data) fclose(data);
        return -1;
    }

    // Tamanhos
    fseek(img, 0, SEEK_END);
    size_t img_size = ftell(img);
    fseek(img, 0, SEEK_SET);

    fseek(data, 0, SEEK_END);
    size_t data_size = ftell(data);
    fseek(data, 0, SEEK_SET);

    printf("\n=== INFORMAÇÕES ===\n");
    printf("Tamanho da imagem: %zu bytes\n", img_size);
    printf("Tamanho dos dados: %zu bytes\n", data_size);

    // Carrega imagem
    unsigned char *image_buffer = malloc(img_size);
    if (!image_buffer) {
        perror("Erro ao alocar memória");
        fclose(img);
        fclose(data);
        return -1;
    }
    fread(image_buffer, 1, img_size, img);

    printf("\n=== ANTES DE MODIFICAR ===\n");
    print_first_bytes(image_buffer, 20, "Primeiros 20 bytes");
    printf("Assinatura: '%c%c' (0x%02X 0x%02X)\n", 
           image_buffer[0], image_buffer[1],
           image_buffer[0], image_buffer[1]);

    uint32_t pixel_offset = get_bmp_data_offset(image_buffer);
    printf("Offset dos pixels no cabeçalho: %u bytes\n", pixel_offset);

    // Carrega dados
    unsigned char *data_buffer = malloc(data_size);
    if (!data_buffer) {
        perror("Erro ao alocar memória");
        free(image_buffer);
        fclose(img);
        fclose(data);
        return -1;
    }
    fread(data_buffer, 1, data_size, data);

    // Calcula capacidade
    size_t available = img_size - pixel_offset;
    size_t capacity = (available / 8) - sizeof(StegoHeader);
    printf("Capacidade disponível: %zu bytes\n", capacity);

    if (data_size > capacity) {
        fprintf(stderr, "ERRO: Dados muito grandes!\n");
        free(image_buffer);
        free(data_buffer);
        fclose(img);
        fclose(data);
        return -1;
    }

    // Prepara cabeçalho
    StegoHeader header;
    header.magic = MAGIC_NUMBER;
    header.data_size = data_size;

    printf("\n=== ESCONDENDO DADOS ===\n");
    printf("Começando no byte: %u\n", pixel_offset);
    
    size_t img_pos = pixel_offset;
    size_t initial_pos = img_pos;
    
    // Esconde cabeçalho
    unsigned char *header_bytes = (unsigned char *)&header;
    for (size_t i = 0; i < sizeof(StegoHeader); i++) {
        unsigned char byte = header_bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            if (img_pos >= img_size) {
                fprintf(stderr, "ERRO: Ultrapassou tamanho da imagem!\n");
                free(image_buffer);
                free(data_buffer);
                fclose(img);
                fclose(data);
                return -1;
            }
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_pos] = (image_buffer[img_pos] & 0xFE) | data_bit;
            img_pos++;
        }
    }

    printf("Cabeçalho escondido. Posição atual: %zu\n", img_pos);

    // Esconde dados
    for (size_t i = 0; i < data_size; i++) {
        unsigned char byte = data_buffer[i];
        for (int bit = 0; bit < 8; bit++) {
            if (img_pos >= img_size) {
                fprintf(stderr, "ERRO: Ultrapassou tamanho da imagem!\n");
                free(image_buffer);
                free(data_buffer);
                fclose(img);
                fclose(data);
                return -1;
            }
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_pos] = (image_buffer[img_pos] & 0xFE) | data_bit;
            img_pos++;
        }
    }

    printf("Dados escondidos. Posição final: %zu\n", img_pos);
    printf("Total de bytes modificados: %zu\n", img_pos - initial_pos);

    printf("\n=== DEPOIS DE MODIFICAR ===\n");
    print_first_bytes(image_buffer, 20, "Primeiros 20 bytes");
    printf("Assinatura: '%c%c' (0x%02X 0x%02X)\n", 
           image_buffer[0], image_buffer[1],
           image_buffer[0], image_buffer[1]);

    // Verifica se corrompeu
    if (image_buffer[0] != 0x42 || image_buffer[1] != 0x4D) {
        fprintf(stderr, "\n❌ ERRO CRÍTICO: Assinatura BMP corrompida!\n");
        fprintf(stderr, "Isso significa que escrevemos antes do offset %u\n", pixel_offset);
        
        // Não salva arquivo corrompido
        free(image_buffer);
        free(data_buffer);
        fclose(img);
        fclose(data);
        return -1;
    }

    // Salva arquivo
    FILE *output = fopen(output_path, "wb");
    if (!output) {
        perror("Erro ao criar arquivo de saída");
        free(image_buffer);
        free(data_buffer);
        fclose(img);
        fclose(data);
        return -1;
    }
    
    fwrite(image_buffer, 1, img_size, output);
    fclose(output);
    
    printf("\n✓ Arquivo salvo: %s\n", output_path);

    free(image_buffer);
    free(data_buffer);
    fclose(img);
    fclose(data);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s hide <imagem.bmp> <arquivo> <saida.bmp>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "hide") == 0) {
        return hide_data(argv[2], argv[3], argv[4]);
    }

    fprintf(stderr, "Comando inválido\n");
    return 1;
}