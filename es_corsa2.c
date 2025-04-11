#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 5 // numero di corridori
//#define SEM
#define COND

#ifdef SEM
struct corsa_t {
    sem_t mutex;
  
    sem_t corr;     //semaforo privato corridori
    sem_t arb;   //semaforo privato arbitro
    
    int block;
    int act;
    int arb_block;
    int arb_act;
    int primo;
    int ultimo;
  }corsa;
  
void init_corsa(struct corsa_t *c) {
sem_init(&c->mutex, 0, 1);
sem_init(&c->corr, 0, 0);
sem_init(&c->arb, 0, 0);
c->block=0;
c->act=0;
c->arb_block=0;
c->arb_act=0;
c->primo = -1;
c->ultimo = -1;
}
  

void corridore_attendivia(struct corsa_t *corsa, int numerocorridore){
    sem_wait(&corsa->mutex);
    //l'ultimo ad essere pronto sblocca arbitro_attendicorridori
    if (corsa->block < N)
        corsa->block++;
    printf("[corridore %d] arrivato alla partenza\n", numerocorridore);
    if(corsa->block==N&&corsa->arb_block){
        corsa->arb_block--;
        printf("[corridore %d] sveglio arbitro\n", numerocorridore);
        sem_post(&corsa->arb);
    }

    sem_post(&corsa->mutex);
  
    sem_wait(&corsa->corr);
}

void corridore_arrivo(struct corsa_t *corsa, int numerocorridore){
    sem_wait(&corsa->mutex);
    corsa->act--;
    if(corsa->primo==-1)
        corsa->primo=numerocorridore;
    printf("[corridore %d] arrivato al traguardo\n", numerocorridore);

    //se sono l'ultimo ad arrivare sblocco arbitro_risultato
    if(corsa->act==0){ 
        printf("[corridore %d] sono l'ultimo, sveglio l'arbitro\n", numerocorridore);
        corsa->ultimo=numerocorridore;
        sem_post(&corsa->arb); 
    }
    sem_post(&corsa->mutex);
}


void arbitro_attendicorridori(struct corsa_t *corsa){
    sem_wait(&corsa->mutex);
    printf("[arbitro]: arrivato\n");
    if(corsa->block==N){
        printf("[arbitro]: tutti i corridori in attesa\n");
        sem_post(&corsa->arb);
    }
    else{
        printf("[arbitro]: attendo corridori\n");
        corsa->arb_block++;
    }
    sem_post(&corsa->mutex);
    sem_wait(&corsa->arb);
}

void arbitro_via(struct corsa_t *corsa){
    sem_wait(&corsa->mutex);
    printf("[arbitro]: sveglio tutti\n");

    while(corsa->block>0){
        //sblocco tutti i corridori in attesa in partenza
        corsa->block--;
        corsa->act++;
        sem_post(&corsa->corr);
    }
    sem_post(&corsa->mutex);
}

void arbitro_risultato(struct corsa_t *corsa, int *primo, int *ultimo){
    sem_wait(&corsa->mutex);
    printf("[arbitro] attendo corridori all'arrivo\n");

    if(corsa->act==0){
        sem_post(&corsa->arb);
        printf("[arbitro] tutti i corridori arrivati al traguardo\n");

    }
    else{
        corsa->arb_block++;
        printf("[arbitro] rimango bloccato in attesa che tutti arrivino...\n");

    }
    
    sem_post(&corsa->mutex);
    sem_wait(&corsa->arb);

    *primo=corsa->primo;
    *ultimo=corsa->ultimo;
}
#elif defined COND

struct corsa_t {
    pthread_mutex_t mutex;

    pthread_cond_t corr,arb;

    int block;
    int act;
    int arb_block;
    int arb_act;
    int primo;
    int ultimo;
  }corsa;

  void init_corsa(struct corsa_t *corsa){
    pthread_mutexattr_t m;
    pthread_condattr_t c;

    pthread_mutexattr_init(&m);
    pthread_mutex_init(&corsa->mutex, &m);
    pthread_mutexattr_destroy(&m);

    pthread_condattr_init(&c);
    pthread_cond_init(&corsa->arb, &c);
    pthread_cond_init(&corsa->corr, &c);
    pthread_condattr_destroy(&c);

    corsa->block = corsa->act =  corsa-> arb_block = corsa->arb_act = 0;
    corsa->primo = corsa->ultimo = -1;

    //corsa->via = false;
}

void corridore_attendivia(struct corsa_t *corsa, int numerocorridore){
    pthread_mutex_lock(&corsa->mutex);
    //l'ultimo ad essere pronto sblocca arbitro_attendicorridori
    if (corsa->block < N)
        corsa->block++;
    printf("[corridore %d] arrivato alla partenza\n", numerocorridore);
    if(corsa->block==N && corsa->arb_block){
        corsa->arb_block--;
        printf("[corridore %d] sveglio arbitro\n", numerocorridore);
        pthread_cond_signal(&corsa->arb);
    }

    pthread_cond_wait(&corsa->corr,&corsa->mutex);
    corsa->block--;
    pthread_mutex_unlock(&corsa->mutex);

}

void corridore_arrivo(struct corsa_t *corsa, int numerocorridore){
    pthread_mutex_lock(&corsa->mutex);    
    
    corsa->act--;
    if(corsa->primo==-1)
        corsa->primo=numerocorridore;
    printf("[corridore %d] arrivato al traguardo\n", numerocorridore);

    //se sono l'ultimo ad arrivare sblocco arbitro_risultato
    if(corsa->act==0){ 
        printf("[corridore %d] sono l'ultimo, sveglio l'arbitro\n", numerocorridore);
        corsa->ultimo=numerocorridore;
        pthread_cond_signal(&corsa->arb);
    }
    pthread_mutex_unlock(&corsa->mutex);
}


void arbitro_attendicorridori(struct corsa_t *corsa){
    pthread_mutex_lock(&corsa->mutex);    
    printf("[arbitro]: arrivato\n");
    
    while(corsa->block<N){
        corsa->arb_block++;
        printf("[arbitro]: attendo corridori\n");
        pthread_cond_wait(&corsa->arb,&corsa->mutex);
        corsa->arb_block--;
    }

    printf("[arbitro]: tutti i corridori in attesa\n");
    
    pthread_mutex_unlock(&corsa->mutex);
}

void arbitro_via(struct corsa_t *corsa){
    pthread_mutex_lock(&corsa->mutex);    
    printf("[arbitro]: sveglio tutti\n");

    pthread_cond_broadcast(&corsa->corr);
    corsa->act=N;
    //corsa->block=0;
    pthread_mutex_unlock(&corsa->mutex);
}

void arbitro_risultato(struct corsa_t *corsa, int *primo, int *ultimo){
    pthread_mutex_lock(&corsa->mutex);    
    printf("[arbitro] attendo corridori all'arrivo\n");

    while(corsa->act>0){
        corsa->arb_block++;
        printf("[arbitro] rimango bloccato in attesa che tutti arrivino...\n");
        pthread_cond_wait(&corsa->arb,&corsa->mutex);
        corsa->arb_block--;
    }
    printf("[arbitro] tutti i corridori arrivati al traguardo\n");
   
    
    pthread_mutex_unlock(&corsa->mutex);

    *primo=corsa->primo;
    *ultimo=corsa->ultimo;
}

#endif


void pausetta(void)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand()%10+1)*1000000;
  nanosleep(&t,NULL);
}


void *corridore(void* arg)
{
    //fprintf(stderr,"p");
    corridore_attendivia(&corsa, (int)(intptr_t)arg);
    //fprintf(stderr,"c");
    pausetta();
    corridore_arrivo(&corsa, (int)(intptr_t)arg);
    //fprintf(stderr,"a");

    return 0;
}


void *arbitro(void* arg)
{
    int primo, ultimo;
    //fprintf(stderr,"P");
    arbitro_attendicorridori(&corsa);
    //fprintf(stderr,"V");
    arbitro_via(&corsa);
    //fprintf(stderr,"F");
    arbitro_risultato(&corsa, &primo, &ultimo);
    //pausetta();

    fprintf(stderr,"%d",primo);
    fprintf(stderr,"%d",ultimo);
    return 0;
}

int main()
{
  pthread_attr_t a;
  pthread_t p;
  
  /* inizializzo il mio sistema */
  init_corsa(&corsa);

  /* inizializzo i numeri casuali, usati nella funzione pausetta */
  srand(555);

  pthread_attr_init(&a);

  /* non ho voglia di scrivere 10000 volte join! */
  pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

  for(int i = 0; i < N; i++)
    pthread_create(&p, &a, corridore, (void *)(intptr_t)i);


  pthread_create(&p, &a, arbitro, NULL);

  pthread_attr_destroy(&a);

  /* aspetto 10 secondi prima di terminare tutti quanti */
  sleep(2);

  fprintf(stderr,"%d",corsa.ultimo);

  return 0;
}

