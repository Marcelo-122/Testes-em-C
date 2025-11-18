[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/76FOJMNt)

# Sistema de Esteganografia com Compressão e Criptografia

Sistema unificado que combina compressão (zlib), criptografia (libsodium) e esteganografia em imagens BMP.

## Pré-requisitos

- `gcc`
- `zlib` (biblioteca de compressão)
- `libsodium` (biblioteca de criptografia)

### Compilação 

```bash
make
```

## Uso

### Compressão
```bash
./stegfs compress <arquivo> <saida.z>
./stegfs decompress <arquivo.z> <saida>
```

### Criptografia
```bash
./stegfs encrypt <senha> <arquivo> <saida.enc>
./stegfs decrypt <senha> <arquivo.enc> <saida>
```

⚠️ **Importante:** A senha deve ser idêntica para descriptografar.

### Esteganografia
```bash
./stegfs hide <imagem.bmp> <arquivo> <saida.bmp>
./stegfs extract <imagem.bmp> <saida>
./stegfs capacity <imagem.bmp>
```

### Processo Completo (Compressão + Criptografia + Esteganografia)
```bash
./stegfs full <imagem.bmp> <arquivo> <saida.bmp> <senha>
```

Este comando:
1. Comprime o arquivo
2. Criptografa os dados comprimidos com a senha fornecida
3. Esconde os dados criptografados na imagem BMP

## Exemplos

**Compressão simples:**
```bash
./stegfs compress documento.txt documento.txt.z
./stegfs decompress documento.txt.z documento_recuperado.txt
```

**Criptografia:**
```bash
./stegfs encrypt minhasenha123 secreto.txt secreto.enc
./stegfs decrypt minhasenha123 secreto.enc secreto_recuperado.txt
```

**Esteganografia:**
```bash
./stegfs hide foto.bmp secreto.txt foto_stego.bmp
./stegfs extract foto_stego.bmp secreto_recuperado.txt
```

**Processo completo:**
```bash
./stegfs full foto.bmp documento_secreto.txt foto_final.bmp minhasenha123
```

Para recuperar:
```bash
./stegfs extract foto_final.bmp dados_extraidos.enc
./stegfs decrypt minhasenha123 dados_extraidos.enc dados_recuperados.z
./stegfs decompress dados_recuperados.z documento_original.txt
```

## Comandos Disponíveis

- **compress** - Comprime um arquivo usando zlib
- **decompress** - Descomprime um arquivo
- **encrypt** - Criptografa um arquivo usando libsodium (XChaCha20-Poly1305)
- **decrypt** - Descriptografa um arquivo
- **hide** - Esconde arquivo em imagem BMP usando LSB
- **extract** - Extrai arquivo de imagem BMP
- **capacity** - Mostra capacidade de armazenamento da imagem
- **full** - Executa o processo completo (compressão + criptografia + esteganografia)