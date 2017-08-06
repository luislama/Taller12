#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


void printMensajeError(int nHilos, FILE *fp);
int numero_lineas(char *ruta, int *tam_lineas);
void * contarPalabras(void * datosStruct);

struct estructura{
  FILE *fp;//descriptor del archivo que debe de leer
  int startRead;//el caracter desde donde debe empezar a leer
  int lnIni;//la linea inicial, para a partir de aqui obtener los tamaños
  int *tam_lineas;//el arreglo con los tamaños de linea
  int lnFin;//La ultima linea que debe de leer
} datosStruct;

#define MAX 1000000
int numeroPalabras=0;
int numCoincidencias=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    //inicializamos el mutex estaticamente
char** palabras;
int num_palabras[100];

int main(int argc, char** argv)
{
  if( argc<4 )
  {
    printf("\nerror!: numero incorrecto de los parametros\n");
    printMensajeError(1,0);
    return -1;
  }
  else
  {
  //COMIENZA A VALIDAR LOS PARAMETROS
    int nHilos = atoi(argv[2]);//numero de hilos
    char *ruta = argv[1];//ruta del archivo
  //-->DESCRIPTOR DE ARCHIVO
    FILE *fp = fopen(ruta,"r");
    if(fp==NULL || nHilos<=0)
    {
      printf("\nerror!: orden y formato de los parametros\n");
      printMensajeError(nHilos,fp);
      return -1;
    }
  //TERMINA LA VALIDACION
    
  //-->ARREGLO CON PALABRAS A BUSCAR
    numeroPalabras = argc - 3;//numero de palabras a buscar
    palabras=(char**)malloc(sizeof(char*)*numeroPalabras);//arreglo de las palabras
    //int num_words[numeroPalabras];//arreglo que guardara el numero de coincidencias de cada palabra dentro del texto
    //num_palabras=num_words;
    for(int i = 0; i<numeroPalabras; i++)
      num_palabras[i]=0;
    //char **palabras = (char**)malloc(numeroPalabras*sizeof(char*));//Se asigna el espacio
    for(int i = 0; i<numeroPalabras; i++)
    { 
      palabras[i]=argv[i+3];
      //*(palabras+i)=argv[i+3];
    }
    int *tam_lineas = (int*)malloc(sizeof(int)*MAX);
    int numeroDeLineas = numero_lineas(ruta,tam_lineas);//incluye la ultima que no tiene nada
//    
    for(int i=0;i<numeroDeLineas;i++)
      printf("linea %i, tamaño %i\n",i+1, tam_lineas[i]);
    printf("numero de lineas: %i\n",numeroDeLineas);
//
  //DETERMINA NUMERO DE LINEAS POR HILO
    int numDivisible=numeroDeLineas;
    while((numDivisible % nHilos)!=0)
      numDivisible -= 1;
    int lnsxHilo = numDivisible/nHilos;

    pthread_t * idHilos = (pthread_t *)malloc(sizeof(pthread_t) * nHilos);

    int num=0;//Para moverme por los numeros de lineas
    int iniRead=0;
  //GENERAMOS EL NUMERO DE HILOS
    for (int x=0; x<nHilos; x++)
    {
      pthread_t h=0;
      struct estructura *r = (struct estructura *)malloc(sizeof(struct estructura));
    //DETERMINA que lineas debe de leer el hilo actual  
      int lnIni = num;
      int lnFin = 0;
      if ((x==nHilos-1)&&(numeroDeLineas!=numDivisible))
        lnFin = numeroDeLineas-1;
      else
        lnFin = num + lnsxHilo - 1;
//
      printf("\ni: %d --- f: %d\n",lnIni,lnFin);
//      
    //OBTIENE el total de caracteres que debe de leer el hilo actual
      int caracteres = 0;
      for(int i = lnIni; i<=lnFin; i++)
        caracteres += tam_lineas[i];
    //ASIGNA los valores a la estructura
      r->fp = fp;
      r->startRead = iniRead;
      r->lnIni = lnIni;
      r->tam_lineas = tam_lineas;
      r->lnFin = lnFin;
    //CREA el hilo
      if ((pthread_create(&h, NULL, contarPalabras, (void *)r)<0))
      {
        printf("Algo salio mal con la creacion del hilo\n");
        return -1;
      }
      idHilos[x] = h;
      num += lnsxHilo;
//      
      printf("hilo %i debe procesar %i caracteres",x,caracteres);
      printf(" y debe de comenzar a leer desde el caracter %i\n",iniRead);
//      
      iniRead+=caracteres;
    }

    for (int x=0; x<nHilos; x++)
    {
      void* status=0;
      pthread_join(idHilos[x], status);
    }

    //IMPRESION para verifica que:
    //  -Obtenga bien los parámetros
    //  -El calculo de lineas por hilo sea el correcto
//    
    printf("ruta: %s\n",ruta);
    printf("hilos: %i\n",nHilos);
    printf("lineas por hilo: %i\n",lnsxHilo);
    if(numeroDeLineas%nHilos != 0)
      printf("  excepto el ultimo hilo que tiene %i lineas\n", lnsxHilo + (numeroDeLineas - numDivisible) );
    for(int i = 0; i<numeroPalabras; i++)
    {
      printf("palabra%i: %s  aparece %i veces\n",i+1,palabras[i],num_palabras[i]);
    }
    printf("numero de coincidencias: %i\n",numCoincidencias );
//    
    return 0;
  }

}
void printMensajeError(int nHilos, FILE *fp)
{
  
  if(nHilos<=0)
    printf("Numero de Hilos debe de ser un entero mayor a cero\n");
  if(fp==NULL)
    printf("No se encuentra el archivo especificado en la ruta\n");
  printf("\nModo de Uso: ./buscar <ruta> <hilos> <palabra1> <palabra2> ... <palabraN>\n");
  printf("Puede ingresar hasta 100 palabras, pero el minimo es una palabra.\n\n");
}
int numero_lineas(char *ruta, int *tam_lineas)
{
  if(ruta != NULL)
    {
      FILE* ar = fopen(ruta,"r");
      int lineas = 0;
      int tam_linea=0;
      while( !feof(ar) && (lineas<MAX) )
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
      return lineas;
    }
    return -1;
}
void * contarPalabras(void * datosStruct)
{
  struct estructura *r = (struct estructura*)datosStruct;
  char *buff;
  char *token;
  
//  pthread_mutex_lock(&mutex);
//  pthread_mutex_unlock(&mutex);
  
  int s = fseek(r->fp,r->startRead,SEEK_SET);
   

  if( s==0 )
  {
    for(int i = r->lnIni; i <= r->lnFin; i++)
    { 
      int tam = r->tam_lineas[i];
    //  pthread_mutex_lock(&mutex);
      buff = (char *)malloc(sizeof(char)*tam);
      
      pthread_mutex_lock(&mutex);
      
      fgets(buff, r->tam_lineas[i],r->fp);
      token = strtok(buff," ,.!?;:");
      while( token != NULL )
      {
        
        for(int j=0; j<numeroPalabras;j++)
        {       
          int comp = strcmp(token,palabras[j]);

          if(  comp == 0 )
          {
            printf("%s\n",token); 
            (num_palabras[j])++;
            
          } 
        }
        token = strtok(NULL," ,.!?;:");
      }
      free(buff);

      pthread_mutex_unlock(&mutex);
      
    }
    
  }
  

  return (void *)0;
}