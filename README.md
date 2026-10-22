# Compressão com Huffman — texto e áudio

Compressor sem perda baseado na codificação de Huffman sobre bytes, testado em
um arquivo de texto e em um áudio WAV. No áudio há um segundo modo, que passa
as amostras por uma codificação delta (diferença entre amostras vizinhas) antes
do Huffman.

## Compilar e rodar

```
make
./huffman c  <entrada> <saida.huf>    # Huffman sobre bytes
./huffman cd <entrada.wav> <saida.huf> # delta entre amostras + Huffman
./huffman d  <entrada.huf> <saida>    # descomprime (o modo vem no .huf)
```

## O método

**Huffman.** Conta quantas vezes cada um dos 256 valores de byte aparece no
arquivo e monta uma árvore binária juntando, a cada passo, os dois nós de menor
frequência. O código de cada byte é o caminho da raiz até a folha dele (0 para
a esquerda, 1 para a direita). Bytes frequentes ficam perto da raiz e ganham
códigos curtos; bytes raros, códigos longos. Nenhum código é prefixo de outro,
então a descompressão lê bit a bit, desce na árvore e escreve o byte ao chegar
numa folha.

**Delta (só no áudio).** Um WAV guarda a onda como amostras de 16 bits, e
amostras vizinhas costumam ter valores próximos. Os valores absolutos variam
por milhares de níveis, o que deixa os bytes bem distribuídos e o Huffman sem
ter o que encurtar. O delta troca cada amostra pela diferença para a anterior:

```
amostras: 5000  5012  5020  5021  5015
delta:    5000   +12    +8    +1    -6
```

As diferenças ficam concentradas perto de zero e se repetem, e é isso que o
Huffman aproveita. Na volta, cada amostra é a soma do delta com a amostra
anterior já restaurada, então nada se perde.

## Decisões de implementação

- **Arquivo `.huf`**: 1 byte com o modo (`c` ou `cd`), o tamanho original e a
  tabela das 256 frequências, seguidos dos bits. A descompressão remonta a
  mesma árvore a partir da tabela. São 2057 bytes fixos, o que torna a
  compressão desvantajosa para arquivos muito pequenos.
- **Escolha dos dois menores** por busca linear num vetor. Com no máximo 256
  símbolos, um heap não traria ganho perceptível.
- **Último byte** completado com zeros. Como o tamanho original está no
  cabeçalho, a descompressão para ao restaurar o último byte e ignora o resto.
- **Arquivo com um só valor de byte**: a árvore é uma folha e o código tem zero
  bits. A descompressão repete esse byte pelo tamanho original.
- **WAV aceito pelo `cd`**: PCM mono de 16 bits com cabeçalho de 44 bytes. Os
  44 bytes do cabeçalho não passam pelo delta. Cada amostra é montada a partir
  de dois bytes em little-endian (`byte baixo | byte alto << 8`). A conta é feita
  em `unsigned short`, que dá a volta em 16 bits: a diferença −2 é gravada como
  `0xFFFE` e a soma na volta restaura o valor exato. No estéreo as amostras dos
  dois canais vêm intercaladas e o delta misturaria os canais, por isso o
  programa recusa esse caso.

## Os dados

- `municipios.txt`: 5.571 municípios da API de localidades do IBGE, um por
  linha, no formato `nome;UF;código` (134.384 bytes).
- `sic.wav`: 10 segundos (de 0:30 a 0:40) de "(Sic)", do Slipknot, convertidos
  de FLAC para WAV PCM mono, 44,1 kHz e 16 bits (882.044 bytes). O áudio não é
  versionado, por causa dos direitos autorais. Para gerar a partir do FLAC:

  ```
  ffmpeg -ss 30 -t 10 -i "02. (Sic).flac" -vn -ac 1 -ar 44100 -c:a pcm_s16le \
    -map_metadata -1 -fflags +bitexact -flags:a +bitexact sic.wav
  ```

  As opções `-map_metadata -1` e `bitexact` impedem o ffmpeg de acrescentar
  metadados que aumentariam o cabeçalho além dos 44 bytes.

## Resultados

Taxa de compressão = `1 − comprimido / original`, ou seja, quanto do espaço
original foi economizado.

```
$ ./huffman c municipios.txt saida.huf
  original:   134384 bytes
  comprimido: 91382 bytes
  taxa de compressao: 32.00%

$ ./huffman c sic.wav saida.huf
  original:   882044 bytes
  comprimido: 859986 bytes
  taxa de compressao: 2.50%

$ ./huffman cd sic.wav saida.huf
  original:   882044 bytes
  comprimido: 785776 bytes
  taxa de compressao: 10.91%
```

| Teste | Original | Comprimido | Taxa |
| --- | --- | --- | --- |
| Texto, Huffman | 134.384 | 91.382 | 32,00% |
| Áudio, Huffman | 882.044 | 859.986 | 2,50% |
| Áudio, delta + Huffman | 882.044 | 785.776 | 10,91% |

Em todos os casos, `./huffman d` seguido de `cmp original restaurado` não
apontou nenhuma diferença.

O texto comprime bem porque poucos bytes (letras minúsculas, `;`, dígitos,
quebra de linha) dominam o arquivo. No áudio, o Huffman sozinho quase não ganha
nada: os bytes das amostras se espalham pelos 256 valores. O delta mais que
quadruplica a taxa, mas ela continua baixa, porque o trecho é de metal com
guitarras distorcidas, um sinal próximo de ruído, em que amostras vizinhas
variam bastante. O FLAC original da faixa inteira ocupa cerca de 72% do WAV
equivalente (35,3 MB). Ele usa a mesma ideia de guardar a diferença para uma
previsão, mas prevê a partir de várias amostras anteriores e codifica o erro
com um código próprio para números pequenos.

Casos de borda testados, todos com a volta idêntica ao original: arquivo
vazio, arquivo de 1 byte, um único byte repetido 5.000 vezes (58,86%) e
100 KB de bytes aleatórios (−2,06%, porque sem padrão o custo da tabela
supera o ganho).

## Fontes

- Huffman: Programiz, "Huffman Coding",
  <https://www.programiz.com/dsa/huffman-coding>. A construção da árvore, os
  códigos (0 à esquerda, 1 à direita) e a decodificação percorrendo a árvore
  são os mesmos; lá os dois menores saem de um heap, aqui de uma busca linear.
- Delta no áudio: inspirado no preditor fixo de ordem 1 do FLAC, que prevê
  cada amostra como igual à anterior e guarda a diferença. RFC 9639, seção
  9.2.5, <https://www.rfc-editor.org/rfc/rfc9639.html>.
- Implementação: escrita para este trabalho.

## Uso de IA

Usei o Claude Opus 5.5 da Anthropic como apoio de implementação ao longo do
trabalho, principalmente nestas partes:

- Discussão do método e da codificação delta para o áudio;
- Ajuda na leitura do cabeçalho WAV e das amostras de 16 bits;
- Escrita de partes do código C;
- Preparação dos dados de teste e dos casos de borda;
- Escrita do `Makefile`.

Toda decisão de método, formato do arquivo e critério de correção foi minha; a
IA funcionou como par de programação para escrever, revisar e testar trechos de
código C a partir dessas decisões.
