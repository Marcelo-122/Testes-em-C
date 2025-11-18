#include "compactar.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

/**
 * @brief Comprime um buffer de dados na memória usando a biblioteca zlib.
 */
int compress_data(const unsigned char *input, size_t input_size, 
                  unsigned char **output, size_t *output_size) {
    if (!input || !output || !output_size) {
        return -1;
    }

    // A função compressBound da zlib nos dá o tamanho máximo que os dados comprimidos podem ocupar no pior caso.
    // Isso garante que nosso buffer de saída seja grande o suficiente.
    unsigned long max_size = compressBound(input_size);
    
    // Aloca memória para o buffer que receberá os dados comprimidos.
    *output = (unsigned char *)malloc(max_size);
    if (!*output) {
        fprintf(stderr, "Erro ao alocar memória para compressão\n");
        return -1;
    }

    // Comprime
    unsigned long compressed_size = max_size;
    int ret = compress2(*output, &compressed_size, input, input_size, 
                        Z_DEFAULT_COMPRESSION);
    
    if (ret != Z_OK) {
        fprintf(stderr, "Erro na compressão: %d\n", ret);
        free(*output);
        *output = NULL;
        return -1;
    }

    *output_size = compressed_size;
    return 0;
}

/**
 * @brief Descomprime um buffer de dados na memória usando a biblioteca zlib.
 */
int decompress_data(const unsigned char *input, size_t input_size,
                    unsigned char **output, size_t *output_size) {
    if (!input || !output || !output_size) {
        return -1;
    }

    // Ao descomprimir, não sabemos o tamanho original exato. 
    // Uma estratégia comum é começar com um buffer de tamanho estimado (ex: 4x o tamanho comprimido) e aumentá-lo se necessário.
    unsigned long buffer_size = input_size * 4;
    *output = (unsigned char *)malloc(buffer_size);
    if (!*output) {
        fprintf(stderr, "Erro ao alocar memória para descompressão\n");
        return -1;
    }

    // Tenta descomprimir os dados. A função `uncompress` da zlib faz o trabalho.
    int ret = uncompress(*output, &buffer_size, input, input_size);
    
    // Se `uncompress` retornar `Z_BUF_ERROR`, significa que nosso buffer de saída não era grande o suficiente.
    // O loop `while` dobra o tamanho do buffer e tenta novamente até que a descompressão seja bem-sucedida.
    while (ret == Z_BUF_ERROR) {
        buffer_size *= 2;
        unsigned char *new_buffer = (unsigned char *)realloc(*output, buffer_size);
        if (!new_buffer) {
            fprintf(stderr, "Erro ao realocar memória\n");
            free(*output);
            *output = NULL;
            return -1;
        }
        *output = new_buffer;
        ret = uncompress(*output, &buffer_size, input, input_size);
    }

    if (ret != Z_OK) {
        fprintf(stderr, "Erro na descompressão: %d\n", ret);
        free(*output);
        *output = NULL;
        return -1;
    }

    *output_size = buffer_size;
    return 0;
}

/**
 * @brief Função de conveniência para comprimir um arquivo inteiro.
 *        Lê o arquivo, chama `compress_data` e salva o resultado.
 */
int compress_file(const char *input_path, const char *output_path) {
    FILE *in = fopen(input_path, "rb");
    if (!in) {
        perror("Erro ao abrir arquivo de entrada");
        return -1;
    }

    // Lê arquivo
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    unsigned char *input_data = (unsigned char *)malloc(file_size);
    if (!input_data) {
        perror("Erro ao alocar memória");
        fclose(in);
        return -1;
    }

    if (fread(input_data, 1, file_size, in) != (size_t)file_size) {
        fprintf(stderr, "Erro ao ler arquivo\n");
        free(input_data);
        fclose(in);
        return -1;
    }
    fclose(in);

    // Comprime
    unsigned char *output_data = NULL;
    size_t output_size = 0;
    
    if (compress_data(input_data, file_size, &output_data, &output_size) != 0) {
        free(input_data);
        return -1;
    }

    // Salva os dados comprimidos (que estão no `output_data`) em um novo arquivo.
    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Erro ao criar arquivo de saída");
        free(input_data);
        free(output_data);
        return -1;
    }

    if (fwrite(output_data, 1, output_size, out) != output_size) {
        fprintf(stderr, "Erro ao escrever arquivo\n");
        free(input_data);
        free(output_data);
        fclose(out);
        return -1;
    }

    fclose(out);
    free(input_data);
    free(output_data);

    printf("Arquivo comprimido: %ld -> %zu bytes (%.1f%%)\n", 
           file_size, output_size, 
           100.0 - (output_size * 100.0 / file_size));

    return 0;
}

/**
 * @brief Função de conveniência para descomprimir um arquivo inteiro.
 *        Lê o arquivo comprimido, chama `decompress_data` e salva o resultado.
 */
int decompress_file(const char *input_path, const char *output_path) {
    FILE *in = fopen(input_path, "rb");
    if (!in) {
        perror("Erro ao abrir arquivo comprimido");
        return -1;
    }

    // Lê arquivo
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    unsigned char *input_data = (unsigned char *)malloc(file_size);
    if (!input_data) {
        perror("Erro ao alocar memória");
        fclose(in);
        return -1;
    }

    if (fread(input_data, 1, file_size, in) != (size_t)file_size) {
        fprintf(stderr, "Erro ao ler arquivo\n");
        free(input_data);
        fclose(in);
        return -1;
    }
    fclose(in);

    // Chama a função `decompress_data` para descomprimir o conteúdo do arquivo.
    unsigned char *output_data = NULL;
    size_t output_size = 0;
    
    if (decompress_data(input_data, file_size, &output_data, &output_size) != 0) {
        free(input_data);
        return -1;
    }

    // Salva os dados descomprimidos em um novo arquivo.
    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Erro ao criar arquivo de saída");
        free(input_data);
        free(output_data);
        return -1;
    }

    if (fwrite(output_data, 1, output_size, out) != output_size) {
        fprintf(stderr, "Erro ao escrever arquivo\n");
        free(input_data);
        free(output_data);
        fclose(out);
        return -1;
    }

    fclose(out);
    free(input_data);
    free(output_data);

    printf("Arquivo descomprimido: %ld -> %zu bytes\n", file_size, output_size);

    return 0;
}