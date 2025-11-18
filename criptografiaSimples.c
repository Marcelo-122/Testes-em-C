#include <stdio.h>
#include <stdlib.h>

void xor_cipher(const char *inputFile, const char *outputFile, char key) {
    FILE *fin = fopen(inputFile, "rb"); // "rb" = read binary (ler binario)
    FILE *fout = fopen(outputFile, "wb"); // "wb" = write binary (escrever binario)

    if (fin == NULL) {
        perror("Erro ao abrir arquivo de entrada");
        exit(EXIT_FAILURE);
    }

    if (fout == NULL) {
        perror("Erro ao criar arquivo de saida");
        fclose(fin);
        exit(EXIT_FAILURE);
    }

    // Le o arquivo de entrada byte a byte
    int byte;
    while ((byte = fgetc(fin)) != EOF) {
        // Aplica a operacao XOR com a chave
        byte = byte ^ key;
        
        // Escreve o byte transformado no arquivo de saida
        fputc(byte, fout);
    }

    printf("Operacao concluida com sucesso!\n");
    printf("Entrada: %s\n", inputFile);
    printf("Saida: %s\n", outputFile);

    // Fecha os arquivos
    fclose(fin);
    fclose(fout);
}

int main(int argc, char *argv[]) {
    // Verifica se o número correto de argumentos foi passado
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <arquivo_entrada> <arquivo_saida> <chave>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s original.txt criptografado.bin S\n", argv[0]);
        fprintf(stderr, "Nota: A chave deve ser um único caractere.\n");
        return EXIT_FAILURE;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];
    char key = argv[3][0]; // Pega o primeiro caractere da string da chave

    if (argv[3][1] != '\0') {
        fprintf(stderr, "Aviso: A chave deve ser um único caractere. Usando apenas '%c'.\n", key);
    }

    xor_cipher(inputFile, outputFile, key);

    return EXIT_SUCCESS;
}
