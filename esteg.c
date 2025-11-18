#include "esteg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Define um "número mágico" (a sequência de caracteres "STEG").
// Isso serve como uma assinatura para identificar rapidamente se uma imagem contém dados escondidos por este programa.
#define MAGIC_NUMBER 0x53544547  // "STEG"

/**
 * @brief Define o cabeçalho que será escondido na imagem antes dos dados.
 * Este cabeçalho contém o número mágico e o tamanho dos dados escondidos.
 */
typedef struct {
    uint32_t magic;
    uint32_t data_size;
} StegoHeader;

/**
 * @brief Lê o cabeçalho de um arquivo BMP para descobrir onde os dados dos pixels começam.
 * O offset (deslocamento) está armazenado a partir do 10º byte do arquivo.
 * @param bmp_data Ponteiro para os dados brutos do arquivo BMP.
 * @return O deslocamento em bytes até a área de pixels.
 */
static uint32_t get_bmp_pixel_offset(unsigned char *bmp_data) {
    // Lê os 4 bytes que representam o offset para a área de pixels no arquivo BMP.
    return bmp_data[10] | (bmp_data[11] << 8) | 
           (bmp_data[12] << 16) | (bmp_data[13] << 24);
}

/**
 * @brief Esconde um buffer de dados dentro de uma imagem BMP.
 * 
 * @param image_path Caminho para a imagem BMP original (cover image).
 * @param data Ponteiro para os dados que serão escondidos.
 * @param data_size Tamanho dos dados a serem escondidos.
 * @param output_path Caminho para salvar a nova imagem com os dados escondidos (stego image).
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int steg_hide(const char *image_path, const unsigned char *data, 
              size_t data_size, const char *output_path) {
    
    // Abre a imagem original em modo binário para leitura.
    FILE *img = fopen(image_path, "rb");
    if (!img) {
        perror("Erro ao abrir imagem");
        return -1;
    }

    // Lê a imagem inteira para um buffer na memória para facilitar a manipulação.
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

    // Verifica se o arquivo é um BMP válido checando os dois primeiros bytes (assinatura "BM").
    if (image_buffer[0] != 0x42 || image_buffer[1] != 0x4D) {
        fprintf(stderr, "Erro: arquivo não é BMP válido\n");
        free(image_buffer);
        return -1;
    }

    // Obtém o offset para a área de pixels no arquivo BMP.
    uint32_t pixel_offset = get_bmp_pixel_offset(image_buffer);
    
    // Calcula a capacidade de armazenamento da imagem. 
    // Cada byte de dado requer 8 bytes na imagem (1 bit por byte, no bit menos significativo - LSB).
    // Também subtrai o espaço necessário para o nosso próprio cabeçalho (StegoHeader).
    size_t available = img_size - pixel_offset;
    size_t capacity = (available / 8) - sizeof(StegoHeader);

    if (data_size > capacity) {
        fprintf(stderr, "Erro: dados muito grandes para a imagem\n");
        fprintf(stderr, "Capacidade: %zu bytes, necessário: %zu bytes\n", 
                capacity, data_size);
        free(image_buffer);
        return -1;
    }

    // Prepara o nosso cabeçalho com o número mágico e o tamanho dos dados.
    StegoHeader header;
    header.magic = MAGIC_NUMBER;
    header.data_size = data_size;

    size_t img_pos = pixel_offset;
    
    // Itera sobre cada byte do nosso cabeçalho (StegoHeader).
    // Para cada byte, esconde seus 8 bits nos 8 bytes seguintes da imagem.
    unsigned char *header_bytes = (unsigned char *)&header;
    for (size_t i = 0; i < sizeof(StegoHeader); i++) {
        unsigned char byte = header_bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_pos] = (image_buffer[img_pos] & 0xFE) | data_bit;
            img_pos++;
        }
    }

    // Faz o mesmo processo para os dados do arquivo, escondendo cada bit de cada byte.
    for (size_t i = 0; i < data_size; i++) {
        unsigned char byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_pos] = (image_buffer[img_pos] & 0xFE) | data_bit;
            img_pos++;
        }
    }

    // Salva o buffer da imagem (agora modificado) em um novo arquivo.
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

/**
 * @brief Extrai dados escondidos de uma imagem BMP.
 * 
 * @param image_path Caminho para a imagem que contém os dados (stego image).
 * @param data Ponteiro para um buffer que será alocado para armazenar os dados extraídos.
 * @param data_size Ponteiro para uma variável que receberá o tamanho dos dados extraídos.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int steg_extract(const char *image_path, unsigned char **data, 
                 size_t *data_size) {
    
    // Abre a imagem que contém os dados escondidos em modo binário para leitura.
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

    // Obtém o offset para a área de pixels no arquivo BMP.
    uint32_t pixel_offset = get_bmp_pixel_offset(image_buffer);
    size_t img_pos = pixel_offset;

    // Extrai o cabeçalho (StegoHeader) da imagem. 
    // O processo é o inverso de esconder: lê 8 bytes da imagem para reconstruir 1 byte do cabeçalho.
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

    // Verifica se o número mágico corresponde. Se não, a imagem não contém nossos dados.
    if (header.magic != MAGIC_NUMBER) {
        fprintf(stderr, "Erro: dados não encontrados na imagem\n");
        free(image_buffer);
        return -1;
    }

    // Uma vez que o cabeçalho é válido, aloca memória e extrai o restante dos dados.
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

/**
 * @brief Função de conveniência para esconder um arquivo inteiro.
 *        Lê o arquivo para um buffer e chama a função steg_hide.
 */
int steg_hide_file(const char *image_path, const char *file_path, 
                   const char *output_path) {
    
    // Abre o arquivo que será escondido em modo binário para leitura.
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

/**
 * @brief Função de conveniência para extrair dados para um arquivo.
 *        Chama steg_extract e salva o buffer resultante em um arquivo.
 */
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

/**
 * @brief Calcula a capacidade de armazenamento de uma imagem BMP em bytes.
 * 
 * @param image_path Caminho para a imagem BMP.
 * @return A capacidade em bytes, ou -1 em caso de erro.
 */
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