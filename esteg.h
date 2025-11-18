#ifndef STEG_H
#define STEG_H

#include <stddef.h>

/**
 * Esconde dados em uma imagem BMP
 * 
 * @param image_path: caminho da imagem BMP original
 * @param data: buffer com os dados a esconder
 * @param data_size: tamanho dos dados
 * @param output_path: caminho da imagem de saída
 * @return: 0 em sucesso, -1 em erro
 */
int steg_hide(const char *image_path, const unsigned char *data, 
              size_t data_size, const char *output_path);

/**
 * Extrai dados escondidos de uma imagem BMP
 * 
 * @param image_path: caminho da imagem com dados escondidos
 * @param data: ponteiro para o buffer de saída (será alocado)
 * @param data_size: ponteiro para receber o tamanho dos dados
 * @return: 0 em sucesso, -1 em erro
 */
int steg_extract(const char *image_path, unsigned char **data, 
                 size_t *data_size);

/**
 * Esconde um arquivo em uma imagem BMP
 * 
 * @param image_path: caminho da imagem BMP original
 * @param file_path: caminho do arquivo a esconder
 * @param output_path: caminho da imagem de saída
 * @return: 0 em sucesso, -1 em erro
 */
int steg_hide_file(const char *image_path, const char *file_path, 
                   const char *output_path);

/**
 * Extrai dados escondidos de uma imagem e salva em arquivo
 * 
 * @param image_path: caminho da imagem com dados escondidos
 * @param output_path: caminho do arquivo de saída
 * @return: 0 em sucesso, -1 em erro
 */
int steg_extract_file(const char *image_path, const char *output_path);

/**
 * Calcula a capacidade de uma imagem BMP
 * 
 * @param image_path: caminho da imagem BMP
 * @return: capacidade em bytes, ou -1 em erro
 */
long steg_get_capacity(const char *image_path);

#endif /* STEG_H */