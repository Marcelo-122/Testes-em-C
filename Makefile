CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lz -lsodium

# Nome do executável
TARGET = stegfs

# Arquivos objeto
OBJS = main.o compactar.o esteg.o crypt_utils.o

# Regra padrão
all: $(TARGET)

# Compila o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@echo "✓ Compilado com sucesso: $(TARGET)"

# Compila cada arquivo .c em .o
main.o: main.c compactar.h esteg.h crypt_utils.h
	$(CC) $(CFLAGS) -c main.c

compactar.o: compactar.c compactar.h
	$(CC) $(CFLAGS) -c compactar.c

esteg.o: esteg.c esteg.h
	$(CC) $(CFLAGS) -c esteg.c

crypt_utils.o: crypt_utils.c crypt_utils.h
	$(CC) $(CFLAGS) -c crypt_utils.c

# Teste completo
test: $(TARGET)
	@echo "\n=== Teste do Sistema ==="
	@echo "\n1. Criando arquivo de teste..."
	@echo "Este é um documento secreto para testar!" > test_file.txt
	@echo "Linha 2 do documento" >> test_file.txt
	@echo "Linha 3 do documento" >> test_file.txt
	
	@echo "\n2. Testando compressão..."
	./$(TARGET) compactar test_file.txt test_file.txt.z
	./$(TARGET) decompactar test_file.txt.z test_recovered.txt
	@diff test_file.txt test_recovered.txt && echo "✓ Compressão OK" || echo "✗ Erro na compressão"
	
	@echo "\n3. Verificando capacidade da imagem..."
	@if [ -f teste.bmp ]; then \
		./$(TARGET) capacity teste.bmp; \
	else \
		echo "Aviso: teste.bmp não encontrado. Criando imagem de teste..."; \
		dd if=/dev/urandom of=test_image.bmp bs=1024 count=100 2>/dev/null; \
		printf '\x42\x4d' | dd of=test_image.bmp bs=1 count=2 conv=notrunc 2>/dev/null; \
		./$(TARGET) capacity test_image.bmp; \
	fi
	
	@echo "\n4. Testando esteganografia..."
	@if [ -f teste.bmp ]; then \
		./$(TARGET) hide teste.bmp test_file.txt test_stego.bmp; \
		./$(TARGET) extract test_stego.bmp test_extracted.txt; \
		diff test_file.txt test_extracted.txt && echo "✓ Esteganografia OK" || echo "✗ Erro na esteganografia"; \
	else \
		echo "Pulando teste de esteganografia (sem teste.bmp válido)"; \
	fi
	
	@echo "\n=== Testes concluídos ==="

# Teste do fluxo completo
test-full: $(TARGET)
	@echo "\n=== Teste Completo (compressão + esteganografia) ==="
	@echo "Este teste precisa de teste.bmp"
	@if [ -f teste.bmp ]; then \
		echo "Criando arquivo de teste..."; \
		echo "Documento ultra secreto!" > secret.txt; \
		./$(TARGET) full teste.bmp secret.txt output_full.bmp; \
		echo "✓ Teste completo finalizado"; \
	else \
		echo "❌ Erro: teste.bmp não encontrado"; \
	fi

# Limpeza
clean:
	rm -f $(TARGET) $(OBJS)
	rm -f test_file.txt test_file.txt.z test_recovered.txt
	rm -f test_image.bmp test_stego.bmp test_extracted.txt
	rm -f secret.txt output_full.bmp
	@echo "✓ Arquivos limpos"

# Ajuda
help:
	@echo "Makefile para Sistema de Esteganografia"
	@echo ""
	@echo "Alvos disponíveis:"
	@echo "  make          - Compila o programa"
	@echo "  make test     - Executa testes"
	@echo "  make test-full - Teste completo (compressão + esteganografia)"
	@echo "  make clean    - Remove arquivos gerados"
	@echo "  make help     - Mostra esta ajuda"

.PHONY: all test test-full clean help