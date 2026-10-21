#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODO_BYTES 0

typedef struct Nodo {
  unsigned char byte;
  long freq;
  struct Nodo *esq, *dir;
} Nodo;

long ler_arquivo(const char *nome, unsigned char **dados) {
  FILE *f;
  long n;

  f = fopen(nome, "rb");
  if (!f) {
    perror(nome);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  n = ftell(f);
  rewind(f);
  *dados = malloc(n > 0 ? n : 1);
  if (!*dados || (long)fread(*dados, 1, n, f) != n) {
    perror(nome);
    free(*dados);
    fclose(f);
    return -1;
  }

  fclose(f);
  return n;
}

int gravar_arquivo(const char *nome, unsigned char *dados, long n) {
  FILE *f;

  f = fopen(nome, "wb");
  if (!f) {
    perror(nome);
    return -1;
  }

  if ((long)fwrite(dados, 1, n, f) != n) {
    perror(nome);
    fclose(f);
    return -1;
  }

  fclose(f);
  return 1;
}

void contar_frequencias(unsigned char *dados, long n, long freq[256]) {
  memset(freq, 0, 256 * sizeof(long));
  for (long i = 0; i < n; i++) freq[dados[i]]++;
}

Nodo *novo_nodo(unsigned char byte, long freq, Nodo *esq, Nodo *dir) {
  Nodo *nodo;

  nodo = malloc(sizeof(Nodo));
  nodo->byte = byte;
  nodo->freq = freq;
  nodo->esq = esq;
  nodo->dir = dir;
  return nodo;
}

Nodo *remover_menor(Nodo *nos[], int *n) {
  Nodo *nodo;
  int menor = 0;

  for (int i = 1; i < *n; i++)
    if (nos[i]->freq < nos[menor]->freq) menor = i;

  nodo = nos[menor];
  nos[menor] = nos[*n - 1];
  (*n)--;
  return nodo;
}

/* a descompressao remonta a arvore com esta mesma funcao, entao os empates
   precisam ser decididos sempre do mesmo jeito */
Nodo *montar_arvore(long freq[256]) {
  Nodo *nos[256];
  Nodo *a, *b;
  int n = 0;

  for (int i = 0; i < 256; i++)
    if (freq[i] > 0) nos[n++] = novo_nodo((unsigned char)i, freq[i], NULL, NULL);
  if (n == 0) return NULL;

  while (n > 1) {
    a = remover_menor(nos, &n);
    b = remover_menor(nos, &n);
    nos[n++] = novo_nodo(0, a->freq + b->freq, a, b);
  }

  return nos[0];
}

void liberar_arvore(Nodo *nodo) {
  if (!nodo) return;
  liberar_arvore(nodo->esq);
  liberar_arvore(nodo->dir);
  free(nodo);
}

void gerar_codigos(Nodo *nodo, char caminho[256], int prof, char codigos[256][256]) {
  if (!nodo->esq) {
    caminho[prof] = '\0';
    strcpy(codigos[nodo->byte], caminho);
    return;
  }

  caminho[prof] = '0';
  gerar_codigos(nodo->esq, caminho, prof + 1, codigos);
  caminho[prof] = '1';
  gerar_codigos(nodo->dir, caminho, prof + 1, codigos);
}

int comprimir(const char *entrada, const char *saida, int modo) {
  unsigned char *dados;
  unsigned char buffer = 0;
  char codigos[256][256], caminho[256];
  long n, freq[256], tamanho;
  int n_bits = 0;
  Nodo *raiz;
  FILE *f;

  n = ler_arquivo(entrada, &dados);
  if (n < 0) return -1;

  contar_frequencias(dados, n, freq);
  raiz = montar_arvore(freq);
  if (raiz) gerar_codigos(raiz, caminho, 0, codigos);

  f = fopen(saida, "wb");
  if (!f) {
    perror(saida);
    liberar_arvore(raiz);
    free(dados);
    return -1;
  }

  fputc(modo, f);
  fwrite(&n, sizeof(long), 1, f);
  fwrite(freq, sizeof(long), 256, f);

  for (long i = 0; i < n; i++) {
    for (char *c = codigos[dados[i]]; *c; c++) {
      buffer = (unsigned char)((buffer << 1) | (*c == '1'));
      n_bits++;
      if (n_bits == 8) {
        fputc(buffer, f);
        buffer = 0;
        n_bits = 0;
      }
    }
  }
  if (n_bits > 0) fputc(buffer << (8 - n_bits), f);

  tamanho = ftell(f);
  fclose(f);
  liberar_arvore(raiz);
  free(dados);

  printf("  original:   %ld bytes\n", n);
  printf("  comprimido: %ld bytes\n", tamanho);
  if (n > 0) printf("  taxa de compressao: %.2f%%\n", 100.0 * (1.0 - (double)tamanho / n));
  return 1;
}

int descomprimir(const char *entrada, const char *saida) {
  unsigned char *dados;
  long n, freq[256], i = 0;
  int modo, c;
  Nodo *raiz, *nodo;
  FILE *f;

  f = fopen(entrada, "rb");
  if (!f) {
    perror(entrada);
    return -1;
  }

  modo = fgetc(f);
  if (modo == EOF || fread(&n, sizeof(long), 1, f) != 1 || fread(freq, sizeof(long), 256, f) != 256) {
    printf("  %s: cabecalho invalido\n", entrada);
    fclose(f);
    return -1;
  }

  dados = malloc(n > 0 ? n : 1);
  raiz = montar_arvore(freq);
  nodo = raiz;

  /* com um simbolo so a raiz e folha e o codigo tem zero bits */
  if (raiz && !raiz->esq) {
    memset(dados, raiz->byte, n);
    i = n;
  }

  while (i < n && (c = fgetc(f)) != EOF) {
    for (int b = 7; b >= 0 && i < n; b--) {
      nodo = ((c >> b) & 1) ? nodo->dir : nodo->esq;
      if (!nodo->esq) {
        dados[i++] = nodo->byte;
        nodo = raiz;
      }
    }
  }

  fclose(f);
  liberar_arvore(raiz);

  if (i < n) {
    printf("  %s: arquivo truncado\n", entrada);
    free(dados);
    return -1;
  }

  if (gravar_arquivo(saida, dados, n) < 0) {
    free(dados);
    return -1;
  }

  printf("  %ld bytes restaurados em %s\n", n, saida);
  free(dados);
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("uso: %s c|d <entrada> <saida>\n", argv[0]);
    printf("  c  comprime com Huffman sobre bytes\n");
    printf("  d  descomprime\n");
    return 1;
  }

  if (strcmp(argv[1], "c") == 0) return comprimir(argv[2], argv[3], MODO_BYTES) == 1 ? 0 : 1;
  if (strcmp(argv[1], "d") == 0) return descomprimir(argv[2], argv[3]) == 1 ? 0 : 1;

  printf("modo desconhecido: %s\n", argv[1]);
  return 1;
}
