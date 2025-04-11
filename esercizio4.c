#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define CARTA 0
#define SASSO 1
#define FORBICE 2

char *nomi_mosse[3] = {"carta", "sasso", "forbice"};

struct morra_g{
    pthread_mutex_t mutex;

    pthread_cond_t g,arb;

    int g_blocked;
    int g_act;
    int arb_block;
    int arb_act;
    bool via;
    int g1_scelta,g2_scelta;
    int vincitore;
    }morra;

void init_morra(struct morra_g *morra){
    pthread_mutexattr_t m;
    pthread_condattr_t c;

    pthread_mutexattr_init(&m);
    pthread_mutex_init(&morra->mutex, &m);
    pthread_mutexattr_destroy(&m);

    pthread_condattr_init(&c);
    pthread_cond_init(&morra->arb, &c);
    pthread_cond_init(&morra->g, &c);

    pthread_condattr_destroy(&c);

    morra->g_blocked = morra->g_act = morra->arb_block = morra->arb_act = 0;
    morra->via=false;
    morra->vincitore= morra->g1_scelta=morra->g2_scelta=-1;
    //corsa->via = false;
}

void giocatore_attendivia(struct morra_g *morra){
    pthread_mutex_lock(&morra->mutex);
    printf("[GIOCATORE] pronto \n");

    if (morra->g_blocked<2)
        morra->g_blocked++;

    if(morra->g_blocked==2&&morra->arb_block){
        pthread_cond_signal(&morra->arb);
        morra->arb_block--;
        printf("[GIOCATORE] sveglio arbitro\n");
    }
        
    pthread_cond_wait(&morra->g,&morra->mutex);
    morra->g_blocked--;
    pthread_mutex_unlock(&morra->mutex);

}

void giocatore_comunicamossa(struct morra_g *morra, int scelta, int id){
    pthread_mutex_lock(&morra->mutex);
    morra->g_act--;


    if (id == 1)
        morra->g1_scelta = scelta;
    else
        morra->g2_scelta = scelta;

    //printf("Giocatore %d ha scelto: %s\n", id, nomi_mosse[scelta]);
    //printf("g act= %d \n",morra->g_act);
    //se sono l'ultimo giocatore a fare la mossa allora sblocco arbitro

    if(morra->g_act==0){
        pthread_cond_signal(&morra->arb);
        printf("sblocco arbitro \n");
    }

    pthread_mutex_unlock(&morra->mutex);

}

void arbitro_giocatoriinattesa(struct morra_g *morra){
    pthread_mutex_lock(&morra->mutex);
    printf("[ARBITRO]: arrivato\n");
    while(morra->g_blocked<2){
        morra->arb_block++;
        printf("[ARBITRO]: in attesa dei giocatori...\n");
        pthread_cond_wait(&morra->arb,&morra->mutex);
        morra->arb_block--;
    }

    printf("[ARBITRO]: tutti i giocatori in attesa\n");


    pthread_mutex_unlock(&morra->mutex);
}

void arbitro_sbloccagiocatori(struct morra_g *morra){
    pthread_mutex_lock(&morra->mutex);
    printf("[ARBITRO] sblocco i giocatori\n");

    morra->via=true;
    pthread_cond_broadcast(&morra->g);
    morra->g_act=2;

    pthread_mutex_unlock(&morra->mutex);
}


void arbitro_attendigiocatori(struct morra_g *morra,int *vincitore){
    pthread_mutex_lock(&morra->mutex);
    while(morra->g_act>0){
        morra->arb_block++;
        printf("[ARBITRO] attendo che i giocatori facciano la scelta\n");
        pthread_cond_wait(&morra->arb,&morra->mutex);
        morra->arb_block--;
    }
    morra->arb_act++;
    if(morra->g1_scelta == morra->g2_scelta) {
        printf("Pareggio\n");
        morra->vincitore = -1;
    } else if((morra->g1_scelta == CARTA && morra->g2_scelta == SASSO) ||
              (morra->g1_scelta == SASSO && morra->g2_scelta == FORBICE) ||
              (morra->g1_scelta == FORBICE && morra->g2_scelta == CARTA)) {
        printf("Giocatore 1 vince\n");
        morra->vincitore = 1;
    } else {
        printf("Giocatore 2 vince\n");
        morra->vincitore = 2;
    }
    printf("g1 = %d \n",morra->g1_scelta);
    printf("g2 = %d \n",morra->g2_scelta);
    pthread_mutex_unlock(&morra->mutex);
    *vincitore=morra->vincitore;
}

void reset_partita(struct morra_g *morra) {
    pthread_mutex_lock(&morra->mutex);
    morra->via = false;
    morra->g1_scelta = -1;
    morra->g2_scelta = -1;
    pthread_mutex_unlock(&morra->mutex);
}

void pausetta(void)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand()%10+1)*1000000;
  nanosleep(&t,NULL);
}

void *giocatore_thread(void *arg) {
    int id = (intptr_t)arg;

    for(;;){
        giocatore_attendivia(&morra);

        int mossa = rand() % 3;
        pausetta();
        printf("Giocatore %d ha scelto: %s\n", id, nomi_mosse[mossa]);

        giocatore_comunicamossa(&morra, mossa, id);
    }

    return NULL;
}

void *arbitro_thread(void *arg) {
    int vincitore;

    for(;;){
        arbitro_giocatoriinattesa(&morra);

        arbitro_sbloccagiocatori(&morra);

        arbitro_attendigiocatori(&morra, &vincitore);

        printf("PARTITA FINITA \n");
        printf("Premi INVIO per continuare...\n");
        getchar();
        reset_partita(&morra);
    }
    return NULL;
}

int main()
{
  pthread_attr_t att;
  pthread_t p;
  
  /* inizializzo il mio sistema */
  init_morra(&morra);

  /* inizializzo i numeri casuali, usati nella funzione pausetta */
  srand(555);

  pthread_attr_init(&att);

  /* non ho voglia di scrivere 10000 volte join! */
  pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);

  pthread_create(&p, &att, giocatore_thread, (void *)(intptr_t)0);
  pthread_create(&p, &att, giocatore_thread, (void *)(intptr_t)1);

  pthread_create(&p, &att, arbitro_thread, NULL);

  pthread_attr_destroy(&att);

  sleep(55);

  return 0;
}