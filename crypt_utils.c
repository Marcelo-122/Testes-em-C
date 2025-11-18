#include "crypt_utils.h"
#include <stdio.h>
#include <sodium.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 4096


int encrypt_file(const char *target_file, const char *source_file,
                 const unsigned char *password, size_t password_len)
{
    unsigned char  buf_in[CHUNK_SIZE];
    unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    unsigned char  salt[crypto_pwhash_SALTBYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *source_fp, *target_fp;
    unsigned long long out_len;
    size_t         in_len;
    int            eof;
    unsigned char  tag;

    // Abrir os arquivos
    source_fp = fopen(source_file, "rb");
    if (!source_fp) {
        fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo fonte '%s'\n", source_file);
        return 1;
    }
    target_fp = fopen(target_file, "wb");
    if (!target_fp) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo destino '%s'\n", target_file);
        fclose(source_fp);
        return 1;
    }

    // Gera um Salt aleatorio
    randombytes_buf(salt, sizeof salt);

    // Deriva a chave da senha usando o Salt
    if (crypto_pwhash(key, sizeof key, (const char *)password, password_len, salt,
                  	crypto_pwhash_OPSLIMIT_INTERACTIVE, 
                  	crypto_pwhash_MEMLIMIT_INTERACTIVE,
                  	crypto_pwhash_ALG_DEFAULT) != 0) {
        fprintf(stderr, "Erro: Falha ao derivar a chave (possivelmente pouca memoria)\n");
        return 1;
    }

    // Salva o Salt no inicio do arquivo de saida
    if (fwrite(salt, 1, sizeof salt, target_fp) != sizeof salt) {
        fprintf(stderr, "Erro: Falha ao escrever o salt no arquivo de saida.\n");
        fclose(source_fp);
        fclose(target_fp);
        return 1;
    }

    // Inicializa o fluxo de criptografia e gerar o header
    crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);

    // Salva o Header no arquivo de saida
    if (fwrite(header, 1, sizeof header, target_fp) != sizeof header) {
        fprintf(stderr, "Erro: Falha ao escrever o header no arquivo de saida.\n");
        fclose(source_fp);
        fclose(target_fp);
        return 1;
    }

    // Loop de criptografia (chunk por chunk)
    do {
        in_len = fread(buf_in, 1, sizeof buf_in, source_fp);
        eof = feof(source_fp);
        tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
        crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, in_len,
                                                  NULL, 0, tag);
        if (fwrite(buf_out, 1, (size_t) out_len, target_fp) != (size_t) out_len) {
            fprintf(stderr, "Erro: Falha ao escrever dados criptografados.\n");
            fclose(source_fp);
            fclose(target_fp);
            return 1;
        }
    } while (!eof);

    fclose(source_fp);
    fclose(target_fp);
    return 0;
}


int decrypt_file(const char *target_file, const char *source_file,
                 const unsigned char *password, size_t password_len)
{
    unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  buf_out[CHUNK_SIZE];
    unsigned char  key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    unsigned char  salt[crypto_pwhash_SALTBYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *source_fp, *target_fp;
    unsigned long long out_len;
    size_t         in_len;
    int            eof;
    unsigned char  tag;
    int            ret = -1; 

    source_fp = fopen(source_file, "rb");
    if (!source_fp) {
        fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo fonte '%s'\n", source_file);
        return 1;
    }
    target_fp = fopen(target_file, "wb");
    if (!target_fp) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo destino '%s'\n", target_file);
        fclose(source_fp);
        return 1;
    }

    // Le o Salt
    if (fread(salt, 1, sizeof salt, source_fp) != sizeof salt) {
        fprintf(stderr, "Erro: Falha ao ler o salt (arquivo corrompido ou invalido).\n");
        goto cleanup;
    }

    // Deriva a chave
    if (crypto_pwhash(key, sizeof key, (const char *)password, password_len, salt,
                  	crypto_pwhash_OPSLIMIT_INTERACTIVE, 
                  	crypto_pwhash_MEMLIMIT_INTERACTIVE,
                  	crypto_pwhash_ALG_DEFAULT) != 0) {
        fprintf(stderr, "Erro: Falha ao derivar a chave.\n");
        goto cleanup;
    }

    // Le o Header
    if (fread(header, 1, sizeof header, source_fp) != sizeof header) {
        fprintf(stderr, "Erro: Falha ao ler o header (arquivo corrompido ou invalido).\n");
        goto cleanup;
    }

    // Inicializa o fluxo de descriptografia
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
        fprintf(stderr, "Erro: Header invalido (arquivo corrompido?).\n");
        goto cleanup;
    }

    // Loop de descriptografia
    do {
        in_len = fread(buf_in, 1, sizeof buf_in, source_fp);
        eof = feof(source_fp);
        if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
                                                      buf_in, in_len, NULL, 0) != 0) {
            fprintf(stderr, "ERRO: MENSAGEM CORROMPIDA OU SENHA INCORRETA.\n");
            goto cleanup;
        }
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
            if (!eof) {
                fprintf(stderr, "Erro: Fim do fluxo alcancado, mas nao e o fim do arquivo.\n");
                goto cleanup;
            }
        }
        if (fwrite(buf_out, 1, (size_t) out_len, target_fp) != (size_t) out_len) {
            fprintf(stderr, "Erro: Falha ao escrever dados descriptografados.\n");
            goto cleanup;
        }
    } while (tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    ret = 0; 

cleanup:
    fclose(target_fp);
    fclose(source_fp);
    return ret;
}


int encrypt_data(const unsigned char *input_data, size_t input_len,
                 unsigned char **output_data, size_t *output_len,
                 const unsigned char *password, size_t password_len)
{
    unsigned char  key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    unsigned char  salt[crypto_pwhash_SALTBYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    unsigned long long encrypted_len;
    size_t total_size;
    unsigned char *result;
    unsigned char *ptr;

    // Gera um Salt aleatorio
    randombytes_buf(salt, sizeof salt);

    // Deriva a chave da senha usando o Salt
    if (crypto_pwhash(key, sizeof key, (const char *)password, password_len, salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, 
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        fprintf(stderr, "Erro: Falha ao derivar a chave (possivelmente pouca memoria)\n");
        return 1;
    }

    // Inicializa o fluxo de criptografia e gerar o header
    crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);

    // Calcula tamanho total: salt + header + dados criptografados
    total_size = sizeof(salt) + sizeof(header) + input_len + 
                 crypto_secretstream_xchacha20poly1305_ABYTES;

    // Aloca memoria para o resultado
    result = malloc(total_size);
    if (!result) {
        fprintf(stderr, "Erro: Falha ao alocar memoria para criptografia\n");
        return 1;
    }

    ptr = result;

    // Copia salt
    memcpy(ptr, salt, sizeof(salt));
    ptr += sizeof(salt);

    // Copia header
    memcpy(ptr, header, sizeof(header));
    ptr += sizeof(header);

    // Criptografa os dados
    crypto_secretstream_xchacha20poly1305_push(&st, ptr, &encrypted_len,
                                              input_data, input_len,
                                              NULL, 0, 
                                              crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    *output_data = result;
    *output_len = sizeof(salt) + sizeof(header) + encrypted_len;

    return 0;
}


int decrypt_data(const unsigned char *input_data, size_t input_len,
                 unsigned char **output_data, size_t *output_len,
                 const unsigned char *password, size_t password_len)
{
    unsigned char  key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    unsigned char  salt[crypto_pwhash_SALTBYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    unsigned long long decrypted_len;
    unsigned char *result;
    const unsigned char *ptr;
    unsigned char tag;
    size_t encrypted_size;

    // Verifica tamanho minimo
    if (input_len < sizeof(salt) + sizeof(header) + 
                    crypto_secretstream_xchacha20poly1305_ABYTES) {
        fprintf(stderr, "Erro: Dados criptografados invalidos (muito pequenos)\n");
        return 1;
    }

    ptr = input_data;

    // Le o Salt
    memcpy(salt, ptr, sizeof(salt));
    ptr += sizeof(salt);

    // Deriva a chave
    if (crypto_pwhash(key, sizeof key, (const char *)password, password_len, salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, 
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        fprintf(stderr, "Erro: Falha ao derivar a chave.\n");
        return 1;
    }

    // Le o Header
    memcpy(header, ptr, sizeof(header));
    ptr += sizeof(header);

    // Inicializa o fluxo de descriptografia
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
        fprintf(stderr, "Erro: Header invalido (arquivo corrompido?).\n");
        return 1;
    }

    // Aloca memoria para o resultado (tamanho maximo possivel)
    encrypted_size = input_len - sizeof(salt) - sizeof(header);
    result = malloc(encrypted_size);
    if (!result) {
        fprintf(stderr, "Erro: Falha ao alocar memoria para descriptografia\n");
        return 1;
    }

    // Descriptografa os dados
    if (crypto_secretstream_xchacha20poly1305_pull(&st, result, &decrypted_len, &tag,
                                                  ptr, encrypted_size, NULL, 0) != 0) {
        fprintf(stderr, "ERRO: MENSAGEM CORROMPIDA OU SENHA INCORRETA.\n");
        free(result);
        return 1;
    }

    if (tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
        fprintf(stderr, "Erro: Tag final nao encontrada\n");
        free(result);
        return 1;
    }

    *output_data = result;
    *output_len = decrypted_len;

    return 0;
}
