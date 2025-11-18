#ifndef CRYPT_UTILS_H
#define CRYPT_UTILS_H

#include <stddef.h>

// Funcao de criptografia de arquivo
int encrypt_file(const char *target_file, const char *source_file,
                 const unsigned char *password, size_t password_len);

// Funcao de descriptografia de arquivo
int decrypt_file(const char *target_file, const char *source_file,
                 const unsigned char *password, size_t password_len);

// Funcao de criptografia em memoria
int encrypt_data(const unsigned char *input_data, size_t input_len,
                 unsigned char **output_data, size_t *output_len,
                 const unsigned char *password, size_t password_len);

// Funcao de descriptografia em memoria
int decrypt_data(const unsigned char *input_data, size_t input_len,
                 unsigned char **output_data, size_t *output_len,
                 const unsigned char *password, size_t password_len);

#endif
