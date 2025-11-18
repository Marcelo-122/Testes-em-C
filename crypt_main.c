#include <stdio.h>
#include <string.h>
#include <sodium.h>

#include "crypt_utils.h" 

int main(int argc, char *argv[])
{
    // Verifica se a libsodium foi inicializada corretamente
    if (sodium_init() < 0) {
        fprintf(stderr, "Erro critico: Nao foi possivel inicializar a libsodium!\n");
        return 1;
    }

    // Verifica o numero de argumentos
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <encrypt/decrypt> <senha> <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const unsigned char *password = (const unsigned char *)argv[2];
    const size_t password_len = strlen(argv[2]);
    const char *source_file = argv[3];
    const char *target_file = argv[4];

    if (strcmp(mode, "encrypt") == 0) {
        if (encrypt_file(target_file, source_file, password, password_len) != 0) {
            fprintf(stderr, "Falha ao criptografar.\n");
            remove(target_file); 
            return 1;
        }
        printf("Arquivo '%s' criptografado com sucesso para '%s'.\n", source_file, target_file);

    } else if (strcmp(mode, "decrypt") == 0) {
        if (decrypt_file(target_file, source_file, password, password_len) != 0) {
            fprintf(stderr, "Falha ao descriptografar (senha incorreta ou arquivo corrompido).\n");
            remove(target_file);
            return 1;
        }
        printf("Arquivo '%s' descriptografado com sucesso para '%s'.\n", source_file, target_file);

    } else {
        fprintf(stderr, "Modo invalido. Use 'encrypt' ou 'decrypt'.\n");
        return 1;
    }

    return 0;
}
