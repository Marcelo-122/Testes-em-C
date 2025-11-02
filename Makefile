CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = steg

all: $(TARGET)

$(TARGET): steg_basic.c
	$(CC) $(CFLAGS) -o $(TARGET) steg_basic.c

# Cria uma imagem de teste (simples arquivo binário)
test-image:
	@echo "Criando imagem de teste (10KB)..."
	@dd if=/dev/urandom of=test_image.bin bs=1024 count=10 2>/dev/null
	@echo "✓ Imagem criada: test_image.bin"

# Cria arquivo de teste para esconder
test-data:
	@echo "Este é um documento super secreto!" > secret.txt
	@echo "Contém informações confidenciais." >> secret.txt
	@echo "Apenas para seus olhos!" >> secret.txt
	@echo "✓ Arquivo secreto criado: secret.txt"

# Teste completo
test: $(TARGET) test-image test-data
	@echo ""
	@echo "=== TESTE DE ESTEGANOGRAFIA ==="
	@echo ""
	@echo "1. Escondendo dados na imagem..."
	./$(TARGET) hide test_image.bin secret.txt stego_image.bin
	@echo ""
	@echo "2. Comparando imagens (devem ser diferentes)..."
	@cmp -l test_image.bin stego_image.bin | head -5 || true
	@echo ""
	@echo "3. Extraindo dados da imagem..."
	./$(TARGET) extract stego_image.bin recovered_secret.txt
	@echo ""
	@echo "4. Comparando arquivo original com recuperado..."
	@diff secret.txt recovered_secret.txt && echo "✓ Arquivos idênticos! Sucesso!" || echo "✗ Arquivos diferentes!"
	@echo ""
	@echo "5. Mostrando conteúdo recuperado:"
	@cat recovered_secret.txt
	@echo ""
	@echo "=== TESTE CONCLUÍDO ==="

clean:
	rm -f $(TARGET) *.o
	rm -f test_image.bin stego_image.bin secret.txt recovered_secret.txt

.PHONY: all test clean test-image test-data