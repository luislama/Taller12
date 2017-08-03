#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX 1000000

int main(int argc, char** argv)
{
  if(argc < 4)
  {
    printf("Modo de Uso: ./buscar [ruta] [hilos] [palabra1] [palabra2] [palabra3] ... [palabraN]\n");
    printf("Minimo una palabra");
  }
  else
  {
    char *ruta = argv[1];
    int nHilos = atoi(argv[2]);
    int numeroPalabras = argc - 2;
    char **palabras = (char*)malloc(numeroPalabras*sizeof(char*));
    for(int i = 0; i<numeroPalabras; i++)
    {
      *(char+i)=argv[i+numeroPalabras];
    }
  }
}

int numero_lineas(char *ruta, int *tam_lineas)
{
  if(ruta != NULL)
    {
      FILE* ar = fopen(ruta,"r");
      int lineas = 0;
      int tam_lineas;
      while(!feof(ar))
      {
        tam_linea++;
        char c = getc(ar);
        if(c == '\n')
        {
          if(tam_lineas != NULL)
          {
            tam_lineas[lineas] = tam_linea;
          }
          lineas++;
          tam_linea=0;
        }
      }
      fclose(ar);
      return -1;
    }
}
