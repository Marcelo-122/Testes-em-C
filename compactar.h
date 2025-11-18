#ifndef COMPACTAR_H
#define COMPACTAR_H

#include <stddef.h>

int compress_data(const unsigned char *input, size_t input_size, 
                  unsigned char **output, size_t *output_size);

int decompress_data(const unsigned char *input, size_t input_size,
                    unsigned char **output, size_t *output_size);

int compress_file(const char *input_path, const char *output_path);

int decompress_file(const char *input_path, const char *output_path);

#endif // COMPACTAR_H