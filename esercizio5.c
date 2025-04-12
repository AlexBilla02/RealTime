#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>


#define N 5
//#define COND
#define SEM
#ifdef COND
    struct urna_g{
        pthread_mutex_t mutex;
        pthread_cond_t cond;

        int voti_0, voti_1,vincitore;
        bool sbloccato;
        int bloccati,attivi;

    }urna;

    void init_urna(struct urna_g *urna){
        pthread_mutexattr_t m;
        pthread_condattr_t c;

        pthread_mutexattr_init(&m);
        pthread_mutex_init(&urna->mutex, &m);
        pthread_mutexattr_destroy(&m);

        pthread_condattr_init(&c);
        pthread_cond_init(&urna->cond, &c);

        pthread_condattr_destroy(&c);

        urna->voti_0=urna->voti_1=urna->bloccati=urna->attivi=0;
        urna->sbloccato=false;
        urna->vincitore=-1;
    }

    void vota(struct urna_g *urna, int voto){
        pthread_mutex_lock(&urna->mutex);
        if(voto==0)
            urna->voti_0++;
        else
            urna->voti_1++;
        printf("voti a 0: %d \t voti a 1: %d \n",urna->voti_0,urna->voti_1);
        if(urna->voti_0>=(N/2+1)|| urna->voti_1>=(N/2+1) && urna->sbloccato==false){
            urna->sbloccato=true;
            printf("Sblocco risultato\n");
            pthread_cond_broadcast(&urna->cond);
        }
        pthread_mutex_unlock(&urna->mutex);
    }


    int risultato(struct urna_g *urna){
        pthread_mutex_lock(&urna->mutex);
        while(urna->sbloccato==false){
            printf("Mi blocco...\n");
            urna->bloccati++;
            pthread_cond_wait(&urna->cond,&urna->mutex);
            urna->bloccati--;
        }

        urna->attivi++;
        if (urna->voti_0>urna->voti_1)
            urna->vincitore=0;
        else 
            urna ->vincitore=1;
        pthread_mutex_unlock(&urna->mutex);
        return urna->vincitore;
    }

#elif defined SEM
    struct urna_g{
        sem_t mutex;
        sem_t sem;
        int voti_0, voti_1,vincitore;
        bool sbloccato;
        int bloccati,attivi;

    }urna;

    void init_urna(struct urna_g *urna){
        sem_init(&urna->mutex,0,1);
        sem_init(&urna->sem,0,0);
        urna->voti_0=urna->voti_1=urna->bloccati=urna->attivi=0;
        urna->sbloccato=false;
        urna->vincitore=-1;
    }

    void vota(struct urna_g *urna,int voto){
        sem_wait(&urna->mutex);
        if(voto==0)
            urna->voti_0++;
        else
            urna->voti_1++;
        printf("voti a 0: %d \t voti a 1: %d \n",urna->voti_0,urna->voti_1);
        if(urna->voti_0 >= (N/2+1) || urna->voti_1 >=(N/2+1) && urna->sbloccato==false){
            urna->sbloccato=true;
            printf("Sblocco i thread\n");
            while(urna->bloccati){
                sem_post(&urna->sem);
                urna->bloccati--;
                urna->attivi++;
            }

        }

        sem_post(&urna->mutex);
    }

    int risultato(struct urna_g *urna){
        sem_wait(&urna->mutex);
        if(urna->sbloccato==true){
            urna->bloccati--;
            sem_post(&urna->sem);
        }
        else
            urna->bloccati++;
        sem_post(&urna->mutex);
        sem_wait(&urna->sem);
        if(urna->voti_0>urna->voti_1)
            urna->vincitore=0;
        else
            urna->vincitore=1;
        return urna->vincitore;
    }




#endif

void reset_voti(struct urna_g *urna){
    urna->attivi=0;
    urna->bloccati=0;
    urna->sbloccato=false;
    urna->vincitore=-1;
    urna->voti_0=0;
    urna->voti_1=0;
}
void pausetta(void)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand()%10+1)*1000000;
  nanosleep(&t,NULL);
}



void *thread(void *arg) {

    int voto=rand()%2;
    vota(&urna,voto);
    //pausetta();
    if(voto==risultato(&urna))
        printf("Ho vinto! Ho votato %d \n",voto);
    else
        printf("Ho perso! Ho votato %d \n",voto);
    pthread_exit(0);
    return NULL;
}

int main()
{
  pthread_attr_t att;
  pthread_t p;
  
  /* inizializzo il mio sistema */
  init_urna(&urna);

  /* inizializzo i numeri casuali, usati nella funzione pausetta */
  srand(0);

  pthread_attr_init(&att);

  /* non ho voglia di scrivere 10000 volte join! */
  pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);

  for(int i =0;i<N;i++){
    pthread_create(&p, &att, thread,NULL);

  }
  pthread_attr_destroy(&att);

  sleep(5);

  return 0;
}