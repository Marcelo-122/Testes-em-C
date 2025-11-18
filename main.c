#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compactar.h"
#include "esteg.h"
#include "crypt_utils.h"
#include "sodium.h"

void print_usage(const char *prog_name) {
    printf("Sistema de Esteganografia com Compressão e Criptografia\n\n");
    printf("Uso:\n");
    printf("  %s compress <arquivo> <saida.z>\n", prog_name);
    printf("  %s decompress <arquivo.z> <saida>\n", prog_name);
    printf("  %s encrypt <senha> <arquivo> <saida.enc>\n", prog_name);
    printf("  %s decrypt <senha> <arquivo.enc> <saida>\n", prog_name);
    printf("  %s hide <imagem.bmp> <arquivo> <saida.bmp>\n", prog_name);
    printf("  %s extract <imagem.bmp> <saida>\n", prog_name);
    printf("  %s capacity <imagem.bmp>\n", prog_name);
    printf("  %s full <imagem.bmp> <arquivo> <saida.bmp>\n", prog_name);
    printf("\nComandos:\n");
    printf("  compress   - Comprime um arquivo\n");
    printf("  decompress - Descomprime um arquivo\n");
    printf("  hide       - Esconde arquivo em imagem\n");
    printf("  extract    - Extrai arquivo de imagem\n");
    printf("  capacity   - Mostra capacidade da imagem\n");
    printf("  full       - Comprime + criptografa + esconde (completo)\n");
    printf("\nExemplos:\n");
    printf("  %s compress documento.txt documento.txt.z\n", prog_name);
    printf("  %s hide foto.bmp secreto.txt foto_stego.bmp\n", prog_name);
    printf("  %s extract foto_stego.bmp secreto_recuperado.txt\n", prog_name);
}

int cmd_compress(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s compress <arquivo> <saida.z>\n", argv[0]);
        return 1;
    }
    
    printf("Comprimindo arquivo...\n");
    if (compress_file(argv[2], argv[3]) == 0) {
        printf("✓ Arquivo comprimido com sucesso!\n");
        return 0;
    }
    return 1;
}

int cmd_decompress(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s decompress <arquivo.z> <saida>\n", argv[0]);
        return 1;
    }
    
    printf("Descomprimindo arquivo...\n");
    if (decompress_file(argv[2], argv[3]) == 0) {
        printf("✓ Arquivo descomprimido com sucesso!\n");
        return 0;
    }
    return 1;
}

int cmd_encrypt(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s encrypt <senha> <arquivo> <saida.enc>\n", argv[0]);
        return 1;
    }
    
    const unsigned char *password = (const unsigned char *)argv[2];
    size_t password_len = strlen(argv[2]);
    const char *input_file = argv[3];
    const char *output_file = argv[4];
    
    printf("Criptografando arquivo...\n");
    if (encrypt_file(output_file, input_file, password, password_len) == 0) {
        printf("✓ Arquivo criptografado com sucesso!\n");
        return 0;
    }
    return 1;
}

int cmd_decrypt(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s decrypt <senha> <arquivo.enc> <saida>\n", argv[0]);
        return 1;
    }
    
    const unsigned char *password = (const unsigned char *)argv[2];
    size_t password_len = strlen(argv[2]);
    const char *input_file = argv[3];
    const char *output_file = argv[4];
    
    printf("Descriptografando arquivo...\n");
    if (decrypt_file(output_file, input_file, password, password_len) == 0) {
        printf("✓ Arquivo descriptografado com sucesso!\n");
        return 0;
    }
    fprintf(stderr, "Erro: Senha incorreta ou arquivo corrompido\n");
    return 1;
}

int cmd_hide(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s hide <imagem.bmp> <arquivo> <saida.bmp>\n", argv[0]);
        return 1;
    }
    
    printf("Escondendo arquivo em imagem...\n");
    if (steg_hide_file(argv[2], argv[3], argv[4]) == 0) {
        printf("✓ Arquivo escondido com sucesso!\n");
        return 0;
    }
    return 1;
}

int cmd_extract(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s extract <imagem.bmp> <saida>\n", argv[0]);
        return 1;
    }
    
    printf("Extraindo arquivo da imagem...\n");
    if (steg_extract_file(argv[2], argv[3]) == 0) {
        printf("✓ Arquivo extraído com sucesso!\n");
        return 0;
    }
    return 1;
}

int cmd_capacity(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s capacity <imagem.bmp>\n", argv[0]);
        return 1;
    }
    
    long capacity = steg_get_capacity(argv[2]);
    if (capacity >= 0) {
        printf("Capacidade da imagem: %ld bytes (%.2f KB)\n", 
               capacity, capacity / 1024.0);
        return 0;
    }
    return 1;
}

int cmd_full(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s full <imagem.bmp> <arquivo> <saida.bmp> <senha>\n", argv[0]);
        return 1;
    }
    
    const char *image_path = argv[2];
    const char *file_path = argv[3];
    const char *output_path = argv[4];
    const unsigned char *password = (const unsigned char *)argv[5];
    size_t password_len = strlen(argv[5]);
    
    printf("=== Processo Completo (Compressão + Criptografia + Esteganografia) ===\n");
    
    // 1. Lê arquivo original
    printf("\n1. Lendo arquivo...\n");
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        perror("Erro ao abrir arquivo");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *original_data = malloc(file_size);
    if (!original_data) {
        perror("Erro ao alocar memória");
        fclose(f);
        return 1;
    }
    fread(original_data, 1, file_size, f);
    fclose(f);
    
    printf("   Tamanho original: %ld bytes\n", file_size);
    
    // 2. Comprime
    printf("\n2. Comprimindo dados...\n");
    unsigned char *compressed_data = NULL;
    size_t compressed_size = 0;
    
    if (compress_data(original_data, file_size, 
                      &compressed_data, &compressed_size) != 0) {
        free(original_data);
        return 1;
    }
    
    printf("   Tamanho comprimido: %zu bytes (%.1f%% de redução)\n", 
           compressed_size, 
           100.0 - (compressed_size * 100.0 / file_size));
    
    // 3. Criptografa
    printf("\n3. Criptografando dados...\n");
    unsigned char *encrypted_data = NULL;
    size_t encrypted_size = 0;
    
    if (encrypt_data(compressed_data, compressed_size,
                     &encrypted_data, &encrypted_size,
                     password, password_len) != 0) {
        free(original_data);
        free(compressed_data);
        return 1;
    }
    
    printf("   Tamanho criptografado: %zu bytes\n", encrypted_size);
    
    // 4. Esconde na imagem
    printf("\n4. Escondendo na imagem...\n");
    if (steg_hide(image_path, encrypted_data, encrypted_size, output_path) != 0) {
        free(original_data);
        free(compressed_data);
        free(encrypted_data);
        return 1;
    }
    
    printf("   Dados escondidos com sucesso!\n");
    
    free(original_data);
    free(compressed_data);
    free(encrypted_data);
    
    printf("\n✓ Processo completo finalizado!\n");
    printf("Imagem salva em: %s\n", output_path);
    printf("\nPara extrair: use 'extract' e depois 'decrypt' com a mesma senha\n");
    
    return 0;
}

int main(int argc, char *argv[]) {
    // Inicializa libsodium
    if (sodium_init() < 0) {
        fprintf(stderr, "Erro critico: Nao foi possivel inicializar a libsodium!\n");
        return 1;
    }
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "compress") == 0) {
        return cmd_compress(argc, argv);
    }
    else if (strcmp(command, "decompress") == 0) {
        return cmd_decompress(argc, argv);
    }
    else if (strcmp(command, "encrypt") == 0) {
        return cmd_encrypt(argc, argv);
    }
    else if (strcmp(command, "decrypt") == 0) {
        return cmd_decrypt(argc, argv);
    }
    else if (strcmp(command, "hide") == 0) {
        return cmd_hide(argc, argv);
    }
    else if (strcmp(command, "extract") == 0) {
        return cmd_extract(argc, argv);
    }
    else if (strcmp(command, "capacity") == 0) {
        return cmd_capacity(argc, argv);
    }
    else if (strcmp(command, "full") == 0) {
        return cmd_full(argc, argv);
    }
    else {
        fprintf(stderr, "Comando inválido: %s\n\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}