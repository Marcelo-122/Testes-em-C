#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
 * ESTEGANOGRAFIA BÁSICA - LSB (Least Significant Bit)
 * 
 * Técnica: Modificar o bit menos significativo de cada byte da imagem
 * para armazenar nossos dados.
 * 
 * Exemplo:
 * Byte da imagem: 11010110 (214)
 * Nosso bit:      1
 * Resultado:      11010111 (215) - diferença quase imperceptível!
 */

#define MAGIC_NUMBER 0x53544547  // "STEG" em ASCII

// Cabeçalho que vamos guardar no início dos dados escondidos
typedef struct {
    uint32_t magic;      // Número mágico para validação
    uint32_t data_size;  // Tamanho dos dados escondidos
} StegoHeader;

/**
 * Calcula quantos bytes podemos esconder em uma imagem
 * Regra: 1 byte de dados precisa de 8 bytes da imagem (1 bit por byte)
 */
size_t calculate_capacity(size_t image_size) {
    // Subtraímos o espaço do cabeçalho
    size_t capacity = (image_size / 8) - sizeof(StegoHeader);
    return capacity;
}

/**
 * Esconde dados em uma imagem usando LSB
 */
int hide_data(const char *image_path, const char *data_path, const char *output_path) {
    FILE *img = fopen(image_path, "rb");
    FILE *data = fopen(data_path, "rb");
    FILE *output = fopen(output_path, "wb");
    
    if (!img || !data || !output) {
        perror("Erro ao abrir arquivos");
        if (img) fclose(img);
        if (data) fclose(data);
        if (output) fclose(output);
        return -1;
    }

    // Obter tamanhos
    fseek(img, 0, SEEK_END);
    size_t img_size = ftell(img);
    fseek(img, 0, SEEK_SET);

    fseek(data, 0, SEEK_END);
    size_t data_size = ftell(data);
    fseek(data, 0, SEEK_SET);

    printf("Tamanho da imagem: %zu bytes\n", img_size);
    printf("Tamanho dos dados: %zu bytes\n", data_size);
    printf("Capacidade disponível: %zu bytes\n", calculate_capacity(img_size));

    // Verifica se cabe
    if (data_size > calculate_capacity(img_size)) {
        fprintf(stderr, "ERRO: Dados muito grandes para a imagem!\n");
        fclose(img);
        fclose(data);
        fclose(output);
        return -1;
    }

    // Carrega imagem na memória
    unsigned char *image_buffer = malloc(img_size);
    if (!image_buffer) {
        perror("Erro ao alocar memória");
        fclose(img);
        fclose(data);
        fclose(output);
        return -1;
    }
    fread(image_buffer, 1, img_size, img);

    // Carrega dados na memória
    unsigned char *data_buffer = malloc(data_size);
    if (!data_buffer) {
        perror("Erro ao alocar memória");
        free(image_buffer);
        fclose(img);
        fclose(data);
        fclose(output);
        return -1;
    }
    fread(data_buffer, 1, data_size, data);

    // Prepara cabeçalho
    StegoHeader header;
    header.magic = MAGIC_NUMBER;
    header.data_size = data_size;

    printf("Escondendo dados...\n");

    // Esconde o cabeçalho
    unsigned char *header_bytes = (unsigned char *)&header;
    size_t img_offset = 0;
    
    for (size_t i = 0; i < sizeof(StegoHeader); i++) {
        unsigned char byte = header_bytes[i];
        // Cada byte precisa de 8 bytes da imagem
        for (int bit = 0; bit < 8; bit++) {
            // Pega o bit atual
            unsigned char data_bit = (byte >> bit) & 1;
            
            // Limpa o LSB do byte da imagem e insere nosso bit
            image_buffer[img_offset] = (image_buffer[img_offset] & 0xFE) | data_bit;
            img_offset++;
        }
    }

    // Esconde os dados
    for (size_t i = 0; i < data_size; i++) {
        unsigned char byte = data_buffer[i];
        
        // Cada byte precisa de 8 bytes da imagem
        for (int bit = 0; bit < 8; bit++) {
            unsigned char data_bit = (byte >> bit) & 1;
            image_buffer[img_offset] = (image_buffer[img_offset] & 0xFE) | data_bit;
            img_offset++;
        }

        // Progresso
        if (i % 1000 == 0) {
            printf("\rProgresso: %.1f%%", (i * 100.0) / data_size);
            fflush(stdout);
        }
    }
    printf("\rProgresso: 100.0%%\n");

    // Salva imagem modificada
    fwrite(image_buffer, 1, img_size, output);

    printf("✓ Dados escondidos com sucesso em: %s\n", output_path);

    // Limpeza
    free(image_buffer);
    free(data_buffer);
    fclose(img);
    fclose(data);
    fclose(output);

    return 0;
}

/**
 * Extrai dados escondidos de uma imagem
 */
int extract_data(const char *stego_image_path, const char *output_path) {
    FILE *img = fopen(stego_image_path, "rb");
    FILE *output = fopen(output_path, "wb");
    
    if (!img || !output) {
        perror("Erro ao abrir arquivos");
        if (img) fclose(img);
        if (output) fclose(output);
        return -1;
    }

    // Carrega imagem
    fseek(img, 0, SEEK_END);
    size_t img_size = ftell(img);
    fseek(img, 0, SEEK_SET);

    unsigned char *image_buffer = malloc(img_size);
    if (!image_buffer) {
        perror("Erro ao alocar memória");
        fclose(img);
        fclose(output);
        return -1;
    }
    fread(image_buffer, 1, img_size, img);

    printf("Extraindo cabeçalho...\n");

    // Extrai cabeçalho
    StegoHeader header;
    unsigned char *header_bytes = (unsigned char *)&header;
    size_t img_offset = 0;

    for (size_t i = 0; i < sizeof(StegoHeader); i++) {
        unsigned char byte = 0;
        
        // Reconstrói cada byte a partir de 8 bytes da imagem
        for (int bit = 0; bit < 8; bit++) {
            unsigned char lsb = image_buffer[img_offset] & 1;
            byte |= (lsb << bit);
            img_offset++;
        }
        
        header_bytes[i] = byte;
    }

    // Valida cabeçalho
    if (header.magic != MAGIC_NUMBER) {
        fprintf(stderr, "ERRO: Número mágico inválido! Imagem não contém dados escondidos.\n");
        free(image_buffer);
        fclose(img);
        fclose(output);
        return -1;
    }

    printf("Tamanho dos dados encontrados: %u bytes\n", header.data_size);
    printf("Extraindo dados...\n");

    // Extrai dados
    unsigned char *data_buffer = malloc(header.data_size);
    if (!data_buffer) {
        perror("Erro ao alocar memória");
        free(image_buffer);
        fclose(img);
        fclose(output);
        return -1;
    }

    for (size_t i = 0; i < header.data_size; i++) {
        unsigned char byte = 0;
        
        for (int bit = 0; bit < 8; bit++) {
            unsigned char lsb = image_buffer[img_offset] & 1;
            byte |= (lsb << bit);
            img_offset++;
        }
        
        data_buffer[i] = byte;

        // Progresso
        if (i % 1000 == 0) {
            printf("\rProgresso: %.1f%%", (i * 100.0) / header.data_size);
            fflush(stdout);
        }
    }
    printf("\rProgresso: 100.0%%\n");

    // Salva dados extraídos
    fwrite(data_buffer, 1, header.data_size, output);

    printf("✓ Dados extraídos com sucesso em: %s\n", output_path);

    // Limpeza
    free(image_buffer);
    free(data_buffer);
    fclose(img);
    fclose(output);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Uso:\n");
        printf("  Esconder: %s hide <imagem_original> <arquivo_secreto> <imagem_saida>\n", argv[0]);
        printf("  Extrair:  %s extract <imagem_com_dados> <arquivo_saida>\n", argv[0]);
        printf("\nExemplo:\n");
        printf("  %s hide foto.bmp secreto.txt foto_stego.bmp\n", argv[0]);
        printf("  %s extract foto_stego.bmp secreto_recuperado.txt\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "hide") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Erro: comando 'hide' precisa de 3 argumentos\n");
            return 1;
        }
        return hide_data(argv[2], argv[3], argv[4]);
    } 
    else if (strcmp(argv[1], "extract") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Erro: comando 'extract' precisa de 2 argumentos\n");
            return 1;
        }
        return extract_data(argv[2], argv[3]);
    }
    else {
        fprintf(stderr, "Comando inválido: %s\n", argv[1]);
        return 1;
    }

    return 0;
}