#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define BARBIERI 3
#define DIVANO 4

#define CLIENTI 10
#define SHAVING_ITERATIONS 5
#define PAYING_ITERATIONS 3

struct negozio_g{
    sem_t barbiere,divano,cassiere;
    sem_t mutex;

    int b_att,b_block,divano_att,divano_block,cassiere_att,cassiere_block;
    int personeAllaCassa;
}negozio;

void init_negozio(struct negozio_g *negozio){
    sem_init(&negozio->mutex,0,1);
    sem_init(&negozio->barbiere,0,0);
    sem_init(&negozio->divano,0,0);
    sem_init(&negozio->cassiere,0,0);

    negozio->b_att=negozio->b_block=negozio->divano_att=negozio->divano_block=negozio->cassiere_att=negozio->cassiere_block=negozio->personeAllaCassa=0;
}

void attendiDivano(struct negozio_g *negozio){
    sem_wait(&negozio->mutex);
    if(negozio->divano_att<DIVANO){
        sem_post(&negozio->divano);
        negozio->divano_att++;
    }
    else
        negozio->divano_block++;

    sem_post(&negozio->mutex);
    sem_wait(&negozio->divano);
}

void alzatiDalDivano(struct negozio_g *negozio){
    sem_wait(&negozio->mutex);
    negozio->divano_att--;
    if(negozio->divano_block){
        negozio->divano_att++;
        negozio->divano_block--;
        sem_post(&negozio->divano);
    }
    sem_post(&negozio->mutex);
}


void attendiCliente(struct negozio_g *negozio){
    sem_wait(&negozio->mutex);
    //solo se il numero di barbieri attivi è uguale al numero massimo di barbieri dovrò bloccare il cliente
    if(negozio->b_att<BARBIERI){
        negozio->b_att++;
        sem_post(&negozio->barbiere);
    }
    else{
        negozio->b_block++;
    }
    sem_post(&negozio->mutex);
    sem_wait(&negozio->barbiere);
}

void concludiTaglio(struct negozio_g *negozio){
    sem_wait(&negozio->mutex);
    negozio->b_att--;
    if(negozio->b_block){
        negozio->b_att++;
        negozio->b_block--;
        sem_post(&negozio->barbiere);
    }
    negozio->personeAllaCassa++;
    sem_post(&negozio->mutex);
}

void attendiCassa(struct negozio_g *negozio){
    sem_wait(&negozio->mutex);
    if(negozio->personeAllaCassa){
        negozio->cassiere_att++;
        sem_post(&negozio->cassiere);
    }
    else{
        negozio->cassiere_block++;
    }

    sem_post(&negozio->mutex);
    sem_wait(&negozio->cassiere);
}

void liberaCliente(struct negozio_g *negozio){
    sem_wait(&negozio->mutex);
    negozio->personeAllaCassa--;
    negozio->cassiere_att--;
    if(negozio->cassiere_block){
        negozio->cassiere_att++;
        negozio->cassiere_block--;
        sem_post(&negozio->cassiere);
    }
    
    sem_post(&negozio->mutex);

}

void pausetta(void)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand()%10+1)*1000000;
  nanosleep(&t,NULL);
}


void* clienti(void* arg){
    int id = (int)(intptr_t)arg;  // cast inverso
    //while(1){
        attendiDivano(&negozio);
        attendiCliente(&negozio);
        alzatiDalDivano(&negozio);

       
        for(int i = 0; i < SHAVING_ITERATIONS; i++){
            printf("Cliente %ld si sta facendo tagliare la barba (%d)\n", id, i+1);
            pausetta();
        }

        concludiTaglio(&negozio);

        
        attendiCassa(&negozio);
        for(int i = 0; i < PAYING_ITERATIONS; i++){
            printf("Cliente %ld sta pagando (%d)\n", id, i+1);
            pausetta();
        }
        liberaCliente(&negozio);

        
       
        //sleep(1); 
    //}
}

int main()
{
  pthread_attr_t att;
  pthread_t p;
  
  /* inizializzo il mio sistema */
  init_negozio(&negozio);

  /* inizializzo i numeri casuali, usati nella funzione pausetta */
  srand(0);

  pthread_attr_init(&att);

  /* non ho voglia di scrivere 10000 volte join! */
  pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);

  for(int i=0;i<CLIENTI;i++){
    pthread_create(&p, &att, clienti,(void*)(intptr_t)i);
  }


  pthread_attr_destroy(&att);

  sleep(5);

  return 0;
}