#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

void printMensajeError(int nHilos, FILE *fp,int numLns);
int numero_lineas(char *ruta, int *tam_lineas);
void * contarPalabras(void * datosNecesarios);
void * printEstado(void * arg);

struct parametros{
  FILE *fp;
  int startRead;//el caracter desde donde debe empezar a leer
  int lnIni;//la linea inicial, para a partir de aqui obtener los tamaños
  int *tam_lineas;//el arreglo con los tamaños de linea
  int lnFin;//La ultima linea que debe de leer

};


#define MAXnumLNS 10000000
#define numWordsToFind 100

int numeroCaracteres = 0;
int numeroPalabras = 0;

char** palabras;
int num_palabras[numWordsToFind];

pthread_mutex_t mutex;

int main(int argc, char** argv)
{
//EN CASO DE HABER MENOS DE 4 PARAMETROS EL PROGRAMA SE CIERRA
//Y MUESTRA UN MENSAJE DE ERROR	
  if( argc<4 )
  {
    printMensajeError(1,(void*)1,0);
    return -1;
//EN CASO DE HABER POR LO MENOS 4 PARAMETROS SIGUE CORRIENDO
  }else
  {

//COMIENZA A VALIDAR LOS PARAMETROS
    int nHilos = atoi(argv[2]);//numero de hilos
    char *ruta = argv[1];//ruta del archivo
//---->//PUNTERO AL ARCHIVO
    FILE *fp = fopen(ruta,"r");
    //FILE *fpSearch = fopen(ruta,"r");
    if(fp==NULL || nHilos<=0)
    {
  //envia el nHilos y el puntero para indicar que error cometio el usuario
      printMensajeError(nHilos,fp,0);
      return -1;
    }
//TERMINA LA VALIDACION


//LAS PALABRAS A BUSCAR SE GUARDAN EN UN ARREGLO
    numeroPalabras = argc - 3;//ejecutable + ruta + nHilos = 3
//---->//ARREGLO DE PALABRAS Y LOS CONTADORES RESPECTIVOS
    palabras=(char**)malloc(sizeof(char*)*numeroPalabras);//arreglo de las palabras
    for(int i = 0; i<numeroPalabras; i++){ 
      palabras[i]=argv[i+3];//las palabras se encuentran a partir de la posicion 3
      num_palabras[i]=0;    //del vector de argumentos  	
    }                       
      
//---->//ARREGLO CON LOS TAMAÑOS PARA CADA LINEA  
	int *tam_lineas = (int*)malloc(sizeof(int)*MAXnumLNS);//se define un numero de lineas inicial igual a MAXnumLNS
	int numeroDeLineas = numero_lineas(ruta,tam_lineas);//se llena el arreglo con los tamaños por linea y retorna el numero de lineas
	if(numeroDeLineas>MAXnumLNS){      //si el numero de lineas es mayor al definido, no leeria todo el archivo,
	  printMensajeError(1,(void*)1,1);//mostraria un mensaje antes de seguir ejecutandose, el conteo no seria correcto.
	  return -1;
	}

//DETERMINA NUMERO DE CARACTERES POR HILO
    int numCharDivisible = numeroCaracteres;
    while((numCharDivisible % nHilos)!=0)
      numCharDivisible -= 1;
    int charxHilo = numCharDivisible/nHilos;

//---->//ARREGLO LOS HILOS
    pthread_t * idHilos = (pthread_t *)malloc(sizeof(pthread_t) * nHilos);

//---->//INICIALIZAMOS EL MUTEX
    if (pthread_mutex_init(&mutex, NULL) != 0){
        printf("\nInicializacion de mutex fallida\n");
        return -1;
    }

//GENERAMOS LOS HILOS Y LE ENVIAMOS MEDIANTE UNA ESTRUCTURA LOS PARAMETROS
//QUE NECESITA PARA LEER Y CONTAR LAS OCURRENCIAS EN SU SECCION
    int lnIni = 0;
    int lnItr=0;//para moverme por las lineas
    int iniRead=0;//para determinar el setOff
  	int caracteres=0;//cantidad de caracteres que se van recorriendo
  	int hilosCreados=0;

    for (int xHilo=0; xHilo<nHilos && caracteres<numeroCaracteres ; xHilo++)
    {	
//--->//ESTRUCTURA PARA ENVIARLE AL HILO COMO PARAMETRO	
      struct parametros *params = (struct parametros*)malloc(sizeof(struct parametros));

      int charHiloActual = 0;       
  //SUMA LOS TAMAÑOS DE LAS LINEAS HASTA QUE SEA EL MAYOR TAMAÑO POSIBLE
  //QUE TODAVIA SEA MENOR AL NUMERO DE CARACTERES POR HILO
      do{
        charHiloActual += tam_lineas[lnItr];
        caracteres += tam_lineas[lnItr];  
        lnItr++;
      }while( (charHiloActual+tam_lineas[lnItr]) < charxHilo );

//--->//CARGAMOS LA ESTRUCTURA
	    params->fp=fp;
      params->startRead=iniRead;
      params->lnIni=lnIni;
      if ( xHilo == nHilos-1 )
        params->lnFin = numeroDeLineas-1;
      else
      	params->lnFin = lnItr-1;
      params->tam_lineas=tam_lineas;

//--->//CREAMOS LOS HILOS      
      if ((pthread_create(&(idHilos[xHilo]), NULL, contarPalabras, (void *)params)<0))
      {
        printf("Algo salio mal con la creacion del hilo\n");
        return -1;
      }

  //ACTUALIZAMOS CONTADORES
      lnIni=lnItr;
      iniRead=caracteres;
  //CONTROLAMOS LA INCERTIDUMBRE DEL NUMERO DE LOS HILOS
  //AL ASIGNAR LAS LINEAS-->Puede ser que sobren hilos
      hilosCreados++;
      if(lnItr == (numeroDeLineas -1) || lnIni == (numeroDeLineas-1))
      	break;
    }

    pthread_t idHiloImpresion;
    if (pthread_create(&idHiloImpresion, NULL, printEstado, NULL)<0)
    {
      printf("Algo salio mal con la creacion del hilo de impresion\n");
      return -1;
    }

  //ESPERAMOS A QUE TERMINEN LOS HILOS
    for(int i = 0; i<hilosCreados; i++){
      int status = pthread_join(idHilos[i], NULL);
      if(status < 0){
	      fprintf(stderr, "Error al esperar por el hilo %i\n",i);
		    exit(-1);
	    }
    }

//CERRAMOS EL PUNTERO AL ARCHIVO
    fclose(fp);

//MONITOREAR EL PROCESO
/*
    printf("\n");
	  printf("numeroDeLineas: %i\n",numeroDeLineas );
	  printf("numeroCaracteres: %i\n",numeroCaracteres);
*/

//RESULTADOS DEL CONTEO
  	for(int i = 0; i<numeroPalabras; i++)
    {
      printf("palabra%i: %s  aparece %i veces\n",i+1,palabras[i],num_palabras[i]);
    }

  }

  
  return 0;
}

void printMensajeError(int nHilos, FILE *fp, int numLns)
{
  if(numLns==0){
    printf("\nerror!: Verifique el numero, el formato y el orden de los parametros.\n");
    if(nHilos<=0)
      printf("  Numero de Hilos debe de ser un entero mayor a cero\n");
    if(fp==NULL)
      printf("  No se encuentra el archivo especificado en la ruta\n");
    printf("\nModo de Uso: ./buscar <ruta> <hilos> <palabra1> <palabra2> ... <palabraN>\n");
    printf("              Puede ingresar hasta %i palabras, pero el minimo es una \n",numWordsToFind);
    printf("              palabra.\n\n");
  }
  else
  	printf("\nADVERTENCIA!!  El archivo de la ruta tiene mas lineas de las que maneja\n");
  	printf("                el programa.\n");
}

int numero_lineas(char *ruta, int *tam_lineas)
{
  if(ruta != NULL) {
	FILE* ar = fopen(ruta,"r");
	int lineas = 0;
	int tam_linea_actual = 0;
	while( !feof(ar) && (lineas<MAXnumLNS) ){
	  tam_linea_actual++;
//OBTENEMOS EL NUMERO DE CARACTERES
	  numeroCaracteres++;
	  char c = getc(ar);
	  if(c == '\n'){
	    if(tam_lineas != NULL){
	      tam_lineas[lineas] = tam_linea_actual;
	    }
	    lineas++;
	    tam_linea_actual=0;
	  }
	}
    fclose(ar);
    return lineas;
  }
  return -1;
}

void * printEstado(void * arg)
{
  while(1)
  {
    pthread_mutex_lock(&mutex);
    for(int i = 0; i<numeroPalabras; i++)
    {
        printf("palabra%i: %s  aparece %i veces\n",i+1,palabras[i],num_palabras[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&mutex);
    sleep(1);
  }
}

void * contarPalabras(void * datosStruct)
{
  struct parametros *r = (struct parametros*)datosStruct;

  for(int i = r->lnIni; i <= r->lnFin; i++)
  { 
    int tam = (r->tam_lineas[i]);
    char *buff;
    buff = (char *)malloc(sizeof(char)*tam);
    
    pthread_mutex_lock(&mutex);
    fseek(r->fp,r->startRead,SEEK_SET);
    fgets(buff,tam,r->fp);
    pthread_mutex_unlock(&mutex);    

    char *token;
    while((token = strtok_r(buff," \n\e\t\\\?\f\'\"\v\b\r!^~><·$%&/,()=º¡¢£¤¥¦§¨©ª«®¯|@#~½¬-_ç`+*[]{}ḉç¿,.!?:;",&buff)))	
    {
      for(int j=0; j<numeroPalabras;j++)
      {     
        int comp = strcmp(token,palabras[j]);
        if(  comp == 0 )
        {
          pthread_mutex_lock(&mutex);  
            (num_palabras[j])++;
          pthread_mutex_unlock(&mutex); 
        }
      }
    }
    (r->startRead) += tam;
  }
  return (void *)0;
}