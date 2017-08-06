#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>


void printMensajeError(int numero, int nHilos, int ruta);
int numero_lineas(char *ruta, int *tam_lineas);
void * contarPalabras(void * datosStruct);

struct estructura{
  char* palabras;//palabras a buscar
  int* num_palabras;//para actualizar el conteo de las palabras
  FILE* fd;//descriptor del archivo que debe de leer
  int startRead;
  int caracteres;
} datosStruct;

#define MAX 1000000
pthread_mutex_t lock;

int main(int argc, char** argv)
{
  if(argc < 4)
  {
    printMensajeError(1,0,0);
    return -1;
  }
  else
  {
  //COMIENZA A VALIDAR LOS PARAMETROS
    int nHilos = atoi(argv[2]);//numero de hilos
    if(nHilos <= 0)
    {
      printMensajeError(1,1,0);
      return -1;
    }
    char *ruta = argv[1];//ruta del archivo
//-->DESCRIPTOR DE ARCHIVO
    FILE* fd = fopen(ruta,O_RDONLY);
    if(fd < 0)
    {
      printMensajeError(1,0,1);
      return -1;
    }
//-->ARREGLO CON PALABRAS A BUSCAR
    int numeroPalabras = argc - 3;//numero de palabras a buscar
    char* palabras[numeroPalabras];//arreglo de las palabras
    int num_palabras[numeroPalabras];//arreglo que guardara el numero de coincidencias de cada palabra dentro del texto
    for(int i = 0; i<numeroPalabras; i++)
      num_palabras[i]=0;
    //char **palabras = (char**)malloc(numeroPalabras*sizeof(char*));//Se asigna el espacio
    for(int i = 0; i<numeroPalabras; i++)
    { 
      palabras[i]=argv[i+3];
      //*(palabras+i)=argv[i+3];
    }
  //TERMINA LA VALIDACION DE LOS PARAMETROS
    int *tam_lineas = (int*)malloc(sizeof(int)*MAX);
    int numeroDeLineas = numero_lineas(ruta,tam_lineas);//incluye la ultima que no tiene nada
    for(int i=0;i<numeroDeLineas;i++)
      printf("linea %i, tamaño %i\n",i+1, tam_lineas[i]);
    printf("numero de lineas: %i\n",numeroDeLineas);

  //DETERMINA NUMERO DE LINEAS POR HILO
    int numDivisible=numeroDeLineas;
    while((numDivisible % nHilos)!=0)
      numDivisible -= 1;
    int lnsxHilo = numDivisible/nHilos;

    pthread_t * idHilos = (pthread_t *)malloc(sizeof(pthread_t) * nHilos);

    int num = 0;//Para moverme por los numeros de lineas
    int iniRead = 0;
  
  //INICIALIZAMOS EL MUTEX
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\nInicializacion de mutex fallida\n");
        return -1;
    }
  
  //GENERAMOS EL NUMERO DE HILOS
    for (int x = 0; x < nHilos; x++)
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
      printf("\ni: %d --- f: %d\n",lnIni,lnFin);
    //OBTIENE el total de caracteres que debe de leer el hilo actual
      int caracteres = 0;
      for(int i = lnIni; i<=lnFin; i++)
        caracteres += tam_lineas[i];
    //ASIGNA los valores a la estructura
      r->palabras = *palabras;
      r->num_palabras = num_palabras;
      r->fd = fd;//descriptor del archivo que debe de leer
      r->startRead = iniRead;
      r->caracteres = caracteres;

    //CREA el hilo
      if ((pthread_create(&h, NULL, contarPalabras, (void *)r)<0))
      {
        printf("Algo salio mal con la creacion del hilo\n");
        return -1;
      }
      idHilos[x] = h;
      num += lnsxHilo;
      
      printf("hilo %i debe procesar %i caracteres",x,caracteres);
      printf(" y debe de comenzar a leer desde el caracter %i\n",iniRead);
      iniRead+=caracteres;
    }

    //Verifica que:
    //  -Obtenga bien los parámetros
    //  -El calculo de lineas por hilo sea el correcto
    
    printf("ruta: %s\n",ruta);
    printf("hilos: %i\n",nHilos);
    printf("lineas por hilo: %i\n",lnsxHilo);
    if(numeroDeLineas % nHilos != 0)
      printf("  excepto el ultimo hilo que tiene %i lineas\n", lnsxHilo + (numeroDeLineas - numDivisible) );
    for(int i = 0; i < numeroPalabras; i++)
    {
      printf("palabra%i: %s\n",i+1,palabras[i]);
    }

  //ESPERAMOS QUE TERMINEN TODOS LOS HILOS
    for (int x = 0; x < nHilos; x++)
      pthread_join(idHilos[x], NULL);

  //TERMINAMOS EL MUTEX
    pthread_mutex_destroy(&lock);
    
    return 0;
  }

}
void printMensajeError(int numero, int nHilos, int ruta)
{
  if(numero==1)
    printf("Modo de Uso: ./buscar <ruta> <hilos> <palabra1> <palabra2> ...\n");
  if(nHilos==1)
    printf("Numero de Hilos debe de ser un entero mayor a cero\n");
  if(ruta==1)
    printf("No se encuentra el archivo especificado en la ruta\n");
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
void * contarPalabras(void * arg)
{
  struct estructura *param = (struct estructura *)arg;
  pthread_mutex_lock(&lock);
  //REGION CRITICA
  if (fseek(param->fd, param->startRead, SEEK_SET)!=0){
    printf("Algo salio mal moviendo el cursos en el archivo");
    return (void *)-1;
  }
  for (int i = 1; i< param->lineas; i++){
    char *linea;
    fgets(linea, MAX, param->fd);
    char *palabra;
    palabra = strtok(linea," ,.-");
    while (palabra != NULL)
    {
      printf ("%s\n",palabra);
      palabra = strtok(NULL, " ,.-");
    }
  }
  pthread_mutex_unlock(&lock);
  return (void *)0;
}