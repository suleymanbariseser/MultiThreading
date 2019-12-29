#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#define MAXCHAR 100
#define READERCOUNT 7
#define UPPERCOUNT 5
#define REPLACECOUNT 5
#define WRITERCOUNT 5

typedef struct read_thread
{
    pthread_t tid;
    int order;
    int index;
    int read_at;
    int write_at;
} r_t;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t rw_queue , upper_queue , replace_queue;

int reader_tail,
        upper_tail,
            replace_tail, 
                writer_tail, 
                    linenumber,
                        write_at,
                            read_at,
                                finished,
                                    read_order,
                                        write_order;

char *lines[1000];
FILE *file, *file2;


void* read(void* param){
    char str[MAXCHAR];
    int success = 0;
    pthread_mutex_lock(&mutex);
    r_t* temp = (r_t*) param;
    temp->tid = pthread_self();
    temp->index = read_order;
    temp->order = read_order;
    temp->read_at = 0;
    read_order++;
    pthread_mutex_unlock(&mutex);
    
    while (success == 0)
    { 
        if (temp->index != reader_tail)
        {
            continue;
        }   
        if(temp->index == reader_tail)
        {
            pthread_mutex_lock(&mutex);
            fseek(file, read_at ,SEEK_SET);
            if(fgets(str, MAXCHAR, file) != NULL){
                printf("Reader %d read the line %d which is %s", temp->order, reader_tail,str);
                lines[reader_tail] = malloc(100*sizeof(char));
                strncpy(lines[reader_tail], str,(strlen(str)-1));
                read_at = read_at + strlen(str);
                linenumber++;
            }
            else
            {
                if(finished == 0){
                    linenumber = reader_tail;
                    //printf("\n\n\n\n\n\n\nlinenumber ====== %d\n\n\n\n\n",linenumber);
                }
                finished = 1;
                sem_destroy(&rw_queue);
                success = 1;
            }
            pthread_mutex_unlock(&mutex);
            temp->index = temp->index + READERCOUNT;
        }
        reader_tail++;
    }
}

void* upper(void* param){
    int success = 0,i;
    r_t* temp = (r_t*) param;
    temp->tid = pthread_self();
    while (success == 0 && upper_tail != linenumber)
    {
        sem_wait(&upper_queue);
        
        
        if(temp->index != upper_tail  )
        {
            sem_post(&upper_queue);
            continue;
        }
        
        else
        {
            if(lines[upper_tail] != NULL){
                printf("\n");
                for (i = 0; i < strlen(lines[upper_tail]); i++)
                {
                    if(lines[upper_tail][i] <= 122 && 97 <= lines[upper_tail][i]){
                        lines[upper_tail][i] -= 32;
                    }
                }
                printf("Upper %d read index %d and converted '%s'\n", temp->order, upper_tail,lines[upper_tail]);
                temp->index = temp->index + UPPERCOUNT;
                upper_tail++;
            }
            else if (finished == 1)
            {
                success = 1;
            }
        }
        sem_post(&upper_queue);
    }
    
}

void* replace(void* param){
    int success = 0,i;
    r_t* temp = (r_t*) param;
    temp->tid = pthread_self();
    while (success == 0 && replace_tail != linenumber)
    {
        sem_wait(&replace_queue);
        if ((temp->index != replace_tail) || (upper_tail == replace_tail))
        {
            if (replace_tail == linenumber)
            {
                success = 1;
            }
            
            sem_post(&replace_queue);
            continue;
        }
        else
        {
            if(lines[replace_tail] != NULL){
                for (i = 0; i < strlen(lines[replace_tail]); i++)
                {
                    if(lines[replace_tail][i] == ' '){
                        lines[replace_tail][i] = '_';
                    }
                }
                printf("Replace %d read index %d and converted '%s'\n", temp->order, replace_tail,lines[replace_tail]);
                temp->index = temp->index + REPLACECOUNT;
                replace_tail++;
            }
            else if (finished == 1)
            {
                success = 1;
                continue;
            }
        }
        sem_post(&replace_queue);
    }
}

void* write(void* param){
    int success = 0;
    char str[MAXCHAR];
    r_t* temp = (r_t*) param;
    temp->tid = pthread_self();
    while (success == 0 && writer_tail != linenumber)
    {
        sem_wait(&rw_queue);
        if(temp->index != writer_tail){
            sem_post(&rw_queue);
            continue;
        }
        else if ((writer_tail < upper_tail && writer_tail < replace_tail) || (writer_tail == (linenumber-1)))
        {
            if(lines[writer_tail] != NULL)
            {
                fseek(file, write_at, SEEK_SET);
                strcpy(str, lines[writer_tail]);
                strcat(str, "\n");
                fprintf(file, str, writer_tail);
                printf("Writer %d writes %s", temp->order, str);
                temp->index = temp->index + WRITERCOUNT;
                write_at = write_at + strlen(str);
                writer_tail++;
            }
            else if(finished == 1){
                success = 1;
            }
        }
        sem_post(&rw_queue);
    }
    
}

int main() {
    FILE *fp;
    int i = 0;
    linenumber = 200000;
    finished = 0;
    write_at = 0;
    read_at = 0;

    char* filename = "test.txt";
    file = fopen(filename, "r+");
    
    
    sem_init(&rw_queue,0,1);
    sem_init(&upper_queue,0,1);
	sem_init(&replace_queue,0,1);
    
    /*tails START*/
    reader_tail = 0;
    upper_tail = 0;
    replace_tail = 0;
    writer_tail = 0;
    /*tails END*/

    /*Thread arrays START*/
    pthread_t read_thread[READERCOUNT];
    pthread_t replace_thread[REPLACECOUNT];
    pthread_t upper_thread[UPPERCOUNT];
    pthread_t writer_thread[WRITERCOUNT];
    /*Thread arrays END*/
    r_t reader_args[READERCOUNT];
    r_t replace_args[REPLACECOUNT];
    r_t upper_args[UPPERCOUNT];
    r_t writer_args[WRITERCOUNT];

    if (file == NULL){
        printf("Could not open file %s",filename);
        return 1;
    }

    for (i = 0; i < READERCOUNT; i++){
        pthread_create(&read_thread[i], NULL, read, &reader_args[i]);
    }
    for (i = 0; i < UPPERCOUNT; i++){
    	  //pthread_join(read_thread[i],NULL);
    	  
        upper_args[i].order = i;
        upper_args[i].index = i;
        pthread_create(&upper_thread[i], NULL, upper, &upper_args[i]);
    }

    /*Replace args Start*/
    for (i = 0; i < REPLACECOUNT; i++){
        replace_args[i].order = i;
        replace_args[i].index = i;
        pthread_create(&replace_thread[i], NULL, replace, &replace_args[i]);
    }
    /*Writer args Start*/
    for (i = 0; i < WRITERCOUNT; i++){
        writer_args[i].order = i;
        writer_args[i].index = i;
        pthread_create(&writer_thread[i], NULL, write, &writer_args[i]);
    }

    
    for (i = 0; i < READERCOUNT; i++)
    {
        pthread_join(read_thread[i],NULL);
    }
    
    
    for (i = 0; i < UPPERCOUNT; i++)
    {
        pthread_join(upper_thread[i],NULL);
    }
    for (i = 0; i < REPLACECOUNT; i++)
    {
        pthread_join(replace_thread[i],NULL);
    }
    for (i = 0; i < WRITERCOUNT; i++)
    {
        pthread_join(writer_thread[i],NULL);
    }
    i = 0;
    printf("\n\nARRAY\n");
    printf("--------------\n");
    while (lines[i] != NULL)
    {
        printf("%s\n", lines[i]);
        i++;
    }
    
    sem_destroy(&rw_queue);
    pthread_exit(NULL);
    fclose(file);
    return 0;
}
