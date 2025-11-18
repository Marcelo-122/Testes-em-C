#ifndef COMPACTAR_H
#define COMPACTAR_H

#include <stddef.h>

/**
 * Comprime dados usando zlib
 * 
 * @param input: buffer de entrada
 * @param input_size: tamanho dos dados de entrada
 * @param output: ponteiro para o buffer de saída (será alocado)
 * @param output_size: ponteiro para receber o tamanho dos dados comprimidos
 * @return: 0 em sucesso, -1 em erro
 */
int compress_data(const unsigned char *input, size_t input_size, 
                  unsigned char **output, size_t *output_size);

/**
 * Descomprime dados usando zlib
 * 
 * @param input: buffer com dados comprimidos
 * @param input_size: tamanho dos dados comprimidos
 * @param output: ponteiro para o buffer de saída (será alocado)
 * @param output_size: ponteiro para receber o tamanho dos dados descomprimidos
 * @return: 0 em sucesso, -1 em erro
 */
int decompress_data(const unsigned char *input, size_t input_size,
                    unsigned char **output, size_t *output_size);

/**
 * Comprime um arquivo
 * 
 * @param input_path: caminho do arquivo original
 * @param output_path: caminho do arquivo comprimido
 * @return: 0 em sucesso, -1 em erro
 */
int compress_file(const char *input_path, const char *output_path);

/**
 * Descomprime um arquivo
 * 
 * @param input_path: caminho do arquivo comprimido
 * @param output_path: caminho do arquivo descomprimido
 * @return: 0 em sucesso, -1 em erro
 */
int decompress_file(const char *input_path, const char *output_path);

#endif // COMPACTAR_H