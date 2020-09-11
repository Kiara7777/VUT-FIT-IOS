/*
 * Soubor:  rivercrossing.c
 * Datum:   2014/1/5
 * Autor:   Sara Skutova, xskuto00@stud.fit.vutbr.cz
 * Projekt: Synchronizace procesu, projekt c. 2 pro predmet IOS
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <semaphore.h>
 #include <sys/ipc.h>
 #include <fcntl.h>
 #include <sys/shm.h>
 #include <stdbool.h>
 #include <time.h>
 #include <errno.h>

 #define EXIT_SYS_ERROR 2



 /**Napoveda*/
const char *HELPMSG =
  "Program: Synchronizace procesu, rivercrossing\n"
  "Spusteni: ./rivercrossing P H S R\n"
  "P - pocet osob generovanych v kazde kategorii\n"
  "H - maximalni hodnota doby po ktere je generovan novy proces HACKER\n"
  "S - maximalni hodnota doby po ktere je generovan novy proces SERF\n"
  "R - maximalni hodnota doby plavby\n";

/** Kody chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby */
  ECLWRONG,    /**< Chybny prikazový radek*/
  ECLNUM,      /**< Chybne hodnoty prikayoveho radku*/
  EFILE,       /**< Chyba pri otevirani, nebo uzavirani souboru*/
  ESHKEY,      /**< Chyba pri generovani klice po sdilenou pamet*/
  ESHALOK,     /**< Chyba pri alokaci sdilene pameti*/
  ESHMAT,       /**< Chyba pri pripojovani ke sdilene pameti*/
  EFORK,        /**< Chyba pri vytvareni potomka*/
  EUNKNOWN,    /**< Neznama chyba */
};

/** Chybova hlaseni odpovidajici chybovym kodum. */
const char *ECODEMSG[] =
{
  [EOK] = "Vse v poradku\n",
  [ECLWRONG] = "Chybne parametry prikazoveho radku!\n",
  [ECLNUM] = "Chyvne hodnoty prikazoveho radku!\n",
  [EFILE] = "Soubor pro zapis se nepodarilo otevrit nebo uzavrit!\n",
  [ESHKEY] = "Chyba pri generovani klice pro sdilenou pamet\n",
  [ESHALOK] = "Chyba pri alokaci sdilene pameti\n",
  [ESHMAT] = "Chyba pri pripojovani ke sdilene pameti\n",
  [EFORK] = "Chyba pri vytvareni potomka\n",
  [EUNKNOWN] = "Neznama chyba\n",
};

 /**
 * Struktura obsahujici hodnoty parametru prikazove radky.
 */
typedef struct params
{
  int ecode;       /**< Chybový kod programu, odpovida vyctu tecodes. */
  int p;           /**< Pocet osob v obou kategoriich*/
  int time_hacker; /**< Maximalni hodnota doby po ktere je generovan novy proces HACKER*/
  int time_serf;   /**< Maximalni hodnota doby po ktere je generovan novy proces SERF*/
  int river;       /**< Maximalni hodnota doby plavby*/
} PARAMS;

/**
 * Struktura obsahujici hodnoty zdilenych promennych
 **/
typedef struct sh_memory
{
  int action;			//citac akci
  int N_heckers;        //pocet heckeru na molu
  int N_serfs;          //pocet serfsu na molu
  int na_lodi;          //pocet osob na lodi
  int heckers;          //pocet vztvorenych hackeru
  int serfs;            //pocet vytvorenych serfu
  int lidi_hodnost;     //pocet lidi kteri jim maji hodnost
  int vsichni;          //celkovy pocet osob 2*P
}SH_MEMORY;

/**
* Struktura obsahujici sdilene semafory
*/
typedef struct sh_semafor
{
    sem_t *zapis;
    sem_t *pristup_molo;
    sem_t *nalodovani_heckers;
    sem_t *nalodovani_serfs;
    sem_t *vypis_clenu;
    sem_t *plavba;
    sem_t *vylodovani;
    sem_t *captail_sleep;
    sem_t *konec;
}SEMAFOR;

/**
 * Vytiskn e hlaseni odpovidajici chybovemu kodu.
 * @param ecode kod chyby programu
 */
void printECode(int ecode)
{
  if (ecode < EOK || ecode > EUNKNOWN)
  { ecode = EUNKNOWN; }

  fprintf(stderr, "%s", ECODEMSG[ecode]);
  if (ecode == ECLWRONG)
    fprintf(stderr, "%s", HELPMSG);
}

/**
 * Zpracuje argumenty prikazoveho radku a vrati je ve strukture PARAMS.
 * Pokud je formát argumentu chybny, ukonci program s chybovym kodem.
 * @param argc Pocet argumentu.
 * @param argv Pole textových retezcu s argumenty.
 * @return Vraci analyzovane argumenty prikazoveho radku.
 */

 PARAMS getParams(int argc, char *argv[])
 {
    // inicializace struktury
    PARAMS result =
  {
    .ecode = EOK,
    .p = 0,
    .time_hacker = 0,
    .time_serf = 0,
    .river = 0,
  };

  if (argc == 5)
  {
    char *chyba;

    result.p = strtol(argv[1], &chyba, 10);
    if (result.p > 0 && (result.p % 2 == 0) && *chyba == '\0')
        result.ecode = EOK;
    else
    {
        result.ecode = ECLNUM;
        return result;
    }

    result.time_hacker = strtol(argv[2], &chyba, 10);
    if (result.time_hacker >= 0 && result.time_hacker < 5001 && *chyba == '\0')
        result.ecode = EOK;
    else
    {
        result.ecode = ECLNUM;
        return result;
    }

    result.time_serf = strtol(argv[3], &chyba, 10);
    if (result.time_serf >= 0 && result.time_serf < 5001 && *chyba == '\0')
        result.ecode = EOK;
    else
    {
        result.ecode = ECLNUM;
        return result;
    }

    result.river = strtol(argv[4], &chyba, 10);
    if (result.time_serf >= 0 && result.time_serf < 5001 && *chyba == '\0')
        result.ecode = EOK;
    else
    {
        result.ecode = ECLNUM;
        return result;
    }
  }
  else
    result.ecode = ECLWRONG;

    return result;
}

void semafore_close(SEMAFOR *p_sem)
{
    sem_close(p_sem->konec);
    sem_close(p_sem->captail_sleep);
    sem_close(p_sem->nalodovani_heckers);
    sem_close(p_sem->nalodovani_serfs);
    sem_close(p_sem->plavba);
    sem_close(p_sem->pristup_molo);
    sem_close(p_sem->vylodovani);
    sem_close(p_sem->vypis_clenu);
    sem_close(p_sem->zapis);
}

//cerna magie a temna strana Sily
void create_hackers(PARAMS *p_params, SEMAFOR *p_sem, SH_MEMORY *p_memory, FILE *file)
{
    pid_t hacker;
    srand(time(NULL));
    for(int i = 0; i < p_params->p; i++)
    {
        usleep(rand() % (p_params->time_hacker * 1000 + 1));
        hacker = fork();
        if(hacker == 0)
        {
            bool captain = false;
            int id_hecker;

            /**  START  */
            sem_wait(p_sem->zapis); // semafor pro zapis uzavren
            p_memory->action = p_memory->action + 1;
            p_memory->heckers = p_memory->heckers + 1;
            id_hecker = p_memory->heckers;
            fprintf(file, "%d: hacker: %d: started\n", p_memory->action, id_hecker);
            fflush(NULL);
            sem_post(p_sem->zapis);

            /**  WAITING FOR BOARDING  */
            sem_wait(p_sem->pristup_molo); // pristup na molo je uzavren, otevre se jenom kdyz skupinka neni jeste vytvorena, nebo jakmile lod pripluje
            sem_wait(p_sem->zapis);
            p_memory->N_heckers = p_memory->N_heckers + 1;
            if (p_memory->N_heckers == 4)
            {
                p_memory->action = p_memory->action + 1;
                fprintf(file, "%d: hacker: %d: waiting for boarding: %d: %d\n", p_memory->action,
                    id_hecker, p_memory->N_heckers, p_memory->N_serfs);
                fflush(NULL);
                p_memory->N_heckers = 0;
                captain = true;
                sem_post(p_sem->nalodovani_heckers); //1
                sem_post(p_sem->nalodovani_heckers); //2
                sem_post(p_sem->nalodovani_heckers); //3
                sem_post(p_sem->nalodovani_heckers); //4, povoleni se naplodit pro 4 heckers
                sem_post(p_sem->zapis);

            }
            else if (p_memory->N_heckers == 2 && p_memory->N_serfs >= 2)
            {
                p_memory->action = p_memory->action + 1;
                fprintf(file, "%d: hacker: %d: waiting for boarding: %d: %d\n", p_memory->action,
                    id_hecker, p_memory->N_heckers, p_memory->N_serfs);
                fflush(NULL);
                p_memory->N_heckers = 0;
                p_memory->N_serfs = p_memory->N_serfs - 2;
                captain = true;
                sem_post(p_sem->nalodovani_heckers); //1
                sem_post(p_sem->nalodovani_heckers); //2, povoleni se nalodid 2 hackers
                sem_post(p_sem->nalodovani_serfs); //1
                sem_post(p_sem->nalodovani_serfs); //2, povoleni se nalodit 2 serfs
                sem_post(p_sem->zapis);
            }
            else
            {
                p_memory->action = p_memory->action + 1;
                fprintf(file, "%d: hacker: %d: waiting for boarding: %d: %d\n", p_memory->action,
                    id_hecker, p_memory->N_heckers, p_memory->N_serfs);
                fflush(NULL);
                sem_post(p_sem->pristup_molo);
                sem_post(p_sem->zapis);
            }

            /**  NALODOVANI  */
            sem_wait(p_sem->nalodovani_heckers);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            fprintf(file, "%d: hacker: %d: boarding: %d: %d\n", p_memory->action, id_hecker,
                    p_memory->N_heckers, p_memory->N_serfs);
            fflush(NULL);
            p_memory->na_lodi = p_memory->na_lodi + 1;
            if (p_memory->na_lodi == 4)
            {
                sem_post(p_sem->vypis_clenu); //1
                sem_post(p_sem->vypis_clenu); //2
                sem_post(p_sem->vypis_clenu); //3
                sem_post(p_sem->vypis_clenu); //4 povoleni na vypis clenu posadky
            }
            sem_post(p_sem->zapis);

            /**  VYPIS CLENU POSADKY  */
            sem_wait(p_sem->vypis_clenu);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            if (captain)
                fprintf(file, "%d: hacker: %d: captain\n", p_memory->action, id_hecker);
            else
                fprintf(file, "%d: hacker: %d: member\n", p_memory->action, id_hecker);
            fflush(NULL);
            p_memory->lidi_hodnost = p_memory->lidi_hodnost + 1;
            if (p_memory->lidi_hodnost == 4)
            {
                sem_post(p_sem->plavba); // abych mohla uspat capitana
            }
            sem_post(p_sem->zapis);

            /**  PLAVBA  */
            sem_wait(p_sem->plavba);
            if (captain)
            {

                usleep(rand() % (p_params->river * 1000 + 1));
                sem_post(p_sem->vylodovani); //1
                sem_post(p_sem->vylodovani); //2
                sem_post(p_sem->vylodovani); //3
                sem_post(p_sem->vylodovani); //4 kapitan dospal, plavba probehla, mozno se vylodovat
                sem_post(p_sem->plavba);
            }
            else
                sem_post(p_sem->plavba);

            /**  VYLODOVANI  */

            sem_wait(p_sem->vylodovani);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            fprintf(file, "%d: hacker: %d: landing: %d: %d\n", p_memory->action, id_hecker,
                    p_memory->N_heckers, p_memory->N_serfs);
            fflush(NULL);
            p_memory->lidi_hodnost = p_memory->lidi_hodnost - 1;
            p_memory->na_lodi = p_memory->na_lodi - 1;
            p_memory->vsichni = p_memory->vsichni - 1;
            captain = false;
            if(p_memory->na_lodi == 0)
                sem_post(p_sem->pristup_molo);

            if(p_memory->vsichni == 0)
                sem_post(p_sem->konec);
            sem_post(p_sem->zapis);


            /**  FINISHED   */
            sem_wait(p_sem->konec);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            fprintf(file, "%d: hacker: %d: finished\n", p_memory->action, id_hecker);
            fflush(NULL);
            sem_post(p_sem->zapis);
            sem_post(p_sem->konec);

            semafore_close(p_sem);

            exit(0);
        }
        else if (hacker < 0)
            printf("AJAJAJAJAJA");
    }
    wait(NULL);
}

void create_serfs(PARAMS *p_params, SEMAFOR *p_sem, SH_MEMORY *p_memory, FILE *file)
{
    srand(time(NULL));
    pid_t serf;
    for(int i = 0; i < p_params->p; i++)
    {
        usleep(rand() % (p_params->time_serf * 1000 + 1));
        serf = fork();
        if(serf == 0)
        {
           bool captain = false;
            int id_serf;

            /**  START  */
            sem_wait(p_sem->zapis); // semafor pro zapis uzavren
            p_memory->action = p_memory->action + 1;
            p_memory->serfs = p_memory->serfs + 1;
            id_serf = p_memory->serfs;
            fprintf(file, "%d: serf: %d: started\n", p_memory->action, id_serf);
            fflush(NULL);
            sem_post(p_sem->zapis);

            /**  WAITING FOR BOARDING  */
            sem_wait(p_sem->pristup_molo);
            sem_wait(p_sem->zapis);
            p_memory->N_serfs= p_memory->N_serfs + 1;
            if (p_memory->N_serfs == 4)
            {
                p_memory->action = p_memory->action + 1;
                fprintf(file, "%d: serf: %d: waiting for boarding: %d: %d\n", p_memory->action,
                    id_serf, p_memory->N_heckers, p_memory->N_serfs);
                fflush(NULL);
                p_memory->N_serfs = 0;
                captain = true;
                sem_post(p_sem->nalodovani_serfs); //1
                sem_post(p_sem->nalodovani_serfs); //2
                sem_post(p_sem->nalodovani_serfs); //3
                sem_post(p_sem->nalodovani_serfs); //4, povoleni na nalodeni 4 serfs
                sem_post(p_sem->zapis);
            }
            else if (p_memory->N_serfs == 2 && p_memory->N_heckers >= 2)
            {
                p_memory->action = p_memory->action + 1;
                fprintf(file, "%d: serf: %d: waiting for boarding: %d: %d\n", p_memory->action,
                    id_serf, p_memory->N_heckers, p_memory->N_serfs);
                fflush(NULL);
                p_memory->N_serfs = 0;
                p_memory->N_heckers = p_memory->N_heckers - 2;
                captain = true;
                sem_post(p_sem->nalodovani_serfs); //1
                sem_post(p_sem->nalodovani_serfs); //2, povoleni na nalodeni 2 serfs
                sem_post(p_sem->nalodovani_heckers); //1
                sem_post(p_sem->nalodovani_heckers); //2, povoleni se nalodid 2 hackers
                sem_post(p_sem->zapis);
            }
            else
            {
                p_memory->action = p_memory->action + 1;
                fprintf(file, "%d: serf: %d: waiting for boarding: %d: %d\n", p_memory->action,
                    id_serf, p_memory->N_heckers, p_memory->N_serfs);
                fflush(NULL);
                sem_post(p_sem->pristup_molo);
                sem_post(p_sem->zapis);
            }

            /**  NALODOVANI  */
            sem_wait(p_sem->nalodovani_serfs);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            fprintf(file, "%d: serf: %d: boarding: %d: %d\n", p_memory->action, id_serf,
                    p_memory->N_heckers, p_memory->N_serfs);
            fflush(NULL);
             p_memory->na_lodi = p_memory->na_lodi + 1;
            if (p_memory->na_lodi == 4)
            {
                sem_post(p_sem->vypis_clenu); //1
                sem_post(p_sem->vypis_clenu); //2
                sem_post(p_sem->vypis_clenu); //3
                sem_post(p_sem->vypis_clenu); //4 povoleni na vypis clenu posadky
            }
            sem_post(p_sem->zapis);

            /**  VYPIS CLENU POSADKY  */
            sem_wait(p_sem->vypis_clenu);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            if (captain)
                fprintf(file, "%d: serf: %d: captain\n", p_memory->action, id_serf);
            else
                fprintf(file, "%d: serf: %d: member\n", p_memory->action, id_serf);
            fflush(NULL);
            p_memory->lidi_hodnost = p_memory->lidi_hodnost + 1;
            if (p_memory->lidi_hodnost == 4)
            {
                sem_post(p_sem->plavba); // abych mohla uspat capitana
                p_memory->lidi_hodnost = 0;
            }
            sem_post(p_sem->zapis);

            /**  PLAVBA  */
            sem_wait(p_sem->plavba);
            if (captain)
            {
                usleep(rand() % (p_params->river * 1000 + 1));
                sem_post(p_sem->vylodovani); //1
                sem_post(p_sem->vylodovani); //2
                sem_post(p_sem->vylodovani); //3
                sem_post(p_sem->vylodovani); //4 kapitan dospal, plavba probehla, mozno se vylodovat
                sem_post(p_sem->plavba);
            }
            else
                sem_post(p_sem->plavba);

            /**  VYLODOVANI  */

           sem_wait(p_sem->vylodovani);
           sem_wait(p_sem->zapis);
           p_memory->action = p_memory->action + 1;
           fprintf(file, "%d: serf: %d: landing: %d: %d\n", p_memory->action, id_serf,
                    p_memory->N_heckers, p_memory->N_serfs);
           fflush(NULL);
           p_memory->lidi_hodnost = p_memory->lidi_hodnost - 1;
           p_memory->na_lodi = p_memory->na_lodi - 1;
           p_memory->vsichni = p_memory->vsichni - 1;
           captain = false;
           if(p_memory->na_lodi == 0)
                sem_post(p_sem->pristup_molo);

            if(p_memory->vsichni == 0)
                sem_post(p_sem->konec);
           sem_post(p_sem->zapis);

            /**  FINISHED   */
            sem_wait(p_sem->konec);
            sem_wait(p_sem->zapis);
            p_memory->action = p_memory->action + 1;
            fprintf(file, "%d: serf: %d: finished\n", p_memory->action, id_serf);
            fflush(NULL);
            sem_post(p_sem->zapis);
            sem_post(p_sem->konec);

            semafore_close(p_sem);

            exit(0);
        }
        else if (serf < 0)
            printf("AJAJAJAJAJA");
    }
    wait(NULL);
}

void uklid_pamet(int sh_id, SH_MEMORY *p_memory)
{
    shmdt (p_memory);
    shmctl (sh_id, IPC_RMID, NULL);
}

void semafore_un()
{
    sem_unlink("/xskuto00_zapis1");
    sem_unlink("/xskuto00_pristup_molo2");
    sem_unlink("/xskuto00_nalodovani_heckers3");
    sem_unlink("/xskuto00_nalodovani_serfs4");
    sem_unlink("/xskuto00_plavba5");
    sem_unlink("/xskuto00_konec6");
    sem_unlink("/xskuto00_vypis_clenu7");
    sem_unlink("/xskuto00_vylodovani8");
    sem_unlink("/xskuto00_captain_sleep9");
}


/** Hlavni program */ // dopsat kontrolovani chyb
int main(int argc, char *argv[])
{
    FILE *fw;
    PARAMS params = getParams(argc, argv);


    if ((fw = fopen("rivercrossing.out", "w")) == NULL)
        params.ecode = EFILE;

      if (params.ecode != EOK)
    {
        printECode(params.ecode);
        fclose(fw);
        return EXIT_FAILURE;
    }
    //inicializace a otevreni semaforu
    SEMAFOR sem;
    sem.zapis = sem_open("/xskuto00_zapis1", O_CREAT | O_EXCL, 0666, 1);
    sem.pristup_molo = sem_open("/xskuto00_pristup_molo2", O_CREAT | O_EXCL, 0666, 1);
    sem.nalodovani_heckers = sem_open("/xskuto00_nalodovani_heckers3", O_CREAT | O_EXCL, 0666, 0);
    sem.nalodovani_serfs = sem_open("/xskuto00_nalodovani_serfs4", O_CREAT | O_EXCL, 0666, 0);
    sem.plavba = sem_open("/xskuto00_plavba5", O_CREAT | O_EXCL, 0666, 0);
    sem.konec = sem_open("/xskuto00_konec6", O_CREAT | O_EXCL, 0666, 0);
    sem.vypis_clenu = sem_open("/xskuto00_vypis_clenu7", O_CREAT | O_EXCL, 0666, 0);
    sem.vylodovani = sem_open("/xskuto00_vylodovani8", O_CREAT | O_EXCL, 0666, 0);
    sem.captail_sleep = sem_open("/xskuto00_captain_sleep9", O_CREAT | O_EXCL, 0666, 0);


    //sdilena pamet

    SH_MEMORY *memory;
    // 1. klic pro sdilenou pamet
    key_t sh_key = ftok("rivercrossing", 4);
    if (errno != 0)
    {
        semafore_close(&sem);
        semafore_un(&sem);
        printECode(ESHKEY);
        fclose(fw);
        return EXIT_SYS_ERROR;
    }
    //2. alokace sdilene pameti
    int sh_id = shmget(sh_key, sizeof(SH_MEMORY), IPC_CREAT | 0666);
    if (sh_id < 0)
    {
        semafore_close(&sem);
        semafore_un(&sem);
        printECode(ESHALOK);
        fclose(fw);
        return EXIT_SYS_ERROR;

    }

    //3. pripojeni ke sdilene pameti, rodic pripojen == decka pripojena
    memory = (SH_MEMORY *)shmat(sh_id, NULL, 0);
    if (memory == (void *) -1)
    {
        semafore_close(&sem);
        semafore_un(&sem);
        uklid_pamet(sh_id, NULL);
        printECode(ESHMAT);
        fclose(fw);
        return EXIT_SYS_ERROR;
    }

    //inicializace zdilene pameti
    memory->action = 0;
    memory->heckers = 0;
    memory->serfs = 0;
    memory->N_heckers = 0;
    memory->N_serfs = 0;
    memory->na_lodi = 0;
    memory->lidi_hodnost = 0;
    memory->vsichni = 2 * params.p;



    //forkovani
     pid_t child_pid_hecker;
     pid_t child_pid_serf;

    child_pid_hecker = fork(); // vztvoreni generatoru hacker

    if (child_pid_hecker == 0)
    {
       create_hackers(&params, &sem, memory, fw);
       semafore_close(&sem);
       exit(0);
    }
    else if (child_pid_hecker < 0)
        {
            printf("AJAJAJAJA\n");
        }
    else
    {
        child_pid_serf = fork(); // vztvoreni generatoru serf

        if (child_pid_serf == 0)
        {
            create_serfs(&params, &sem, memory, fw);
            semafore_close(&sem);
            exit(0);
        }
        else if (child_pid_hecker < 0)
            printf("AJAJAJA\n");
        else
            wait(NULL);

        wait(NULL);
    }

    uklid_pamet(sh_id, memory);
    semafore_close(&sem);
    semafore_un();
    fclose(fw);
return EXIT_SUCCESS;
}
