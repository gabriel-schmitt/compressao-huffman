# Compressão com Huffman — texto e áudio

TDE de Estrutura de Dados II: escolher um método de compressão, implementar e
testar em dois ou mais tipos de dado. O método é a codificação de Huffman sobre
bytes. No áudio (WAV de 16 bits), as amostras passam antes por uma codificação
delta, que guarda a diferença entre amostras vizinhas.

## Compilar e rodar

```
make
./huffman c <entrada> <saida.huf>
./huffman d <entrada.huf> <saida>
```
