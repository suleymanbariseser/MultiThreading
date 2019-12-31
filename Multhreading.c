#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#define MAXCHAR 100

//Thread numbers
int readThread_n;
int upperThread_n;
int replaceThread_n;
int writeThread_n;

typedef struct read_thread
{
    pthread_t tid;
    int order;
    int index;
} r_t;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t rw_queue , upper_queue , replace_queue;

int reader_tail,    //keeps last readed index
        upper_tail,    //keeps last uppercase conversion index
            replace_tail, //keeps last replace conversion index
                writer_tail, //keeps last writed index
                    linenumber, //keeps the number of lines
                        write_at,   //keeps the current letter count, will be used for fseek in writer
                            read_at,    //keeps the current letter count, will be used for fseek in reader
                                finished,   //if reader finished == 1
                                    read_order, //order of read threads
                                        write_order; //order of write threads

char *lines[1000];
FILE *file, *file2;


void* read(void* param){
    char str[MAXCHAR];  //will keeps last readed line
    int success = 0;    //if success = 1 then while loop will break
    pthread_mutex_lock(&mutex);
    /*critical section*/
    r_t* temp = (r_t*) param; //the current thread
    temp->tid = pthread_self(); //thread id
    temp->index = read_order; //thread first index that will read
    temp->order = read_order; //thread order
    read_order++;
    /*End of critical section*/
    pthread_mutex_unlock(&mutex);
    
    while (success == 0) //run until the last readed
    {
        if (temp->index != reader_tail) //if the order nat
        {
            continue;
        }   
        if(temp->index == reader_tail)
        {
            pthread_mutex_lock(&mutex);
            /*critical section*/
            fseek(file, read_at ,SEEK_SET); //while write change position of pointer by read_at variable bring it back
            if(fgets(str, MAXCHAR, file) != NULL){  //read the line
                printf("Reader %d read the line %d which is %s", temp->order, reader_tail,str); //printf the information
                lines[reader_tail] = malloc(100*sizeof(char));  //malloc for the readed line
                strncpy(lines[reader_tail], str,(strlen(str)-1));   //the last index is \n and delete it
                read_at = read_at + strlen(str);    /*next readed line is calculated with this formula
                                                    number of chars of the last readed line + read_at*/
                linenumber++;   //keeps number of line
            }
            else
            {
                if(finished == 0){  /*example there is 5 thread and 3 thread reads the last line and there are still 2 running thread
                                    so by this function if the first time finished variable changed. that's mean the last line is reader_tail*/
                    linenumber = reader_tail;
                }
                finished = 1;   //finished
                success = 1;    //read operation is done
            }
            /*end of critical section*/
            pthread_mutex_unlock(&mutex);
            temp->index = temp->index + readThread_n;    //the next duty of thread
        }
        reader_tail++;  //increase reader_tail
    }
}

void* upper(void* param){
    int success = 0,i;
    r_t* temp = (r_t*) param;
    temp->tid = pthread_self();
    while (success == 0 && upper_tail != linenumber)
    {
        sem_wait(&upper_queue);
        
        
        if(temp->index != upper_tail)//If this is not its line it will wait 
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
                temp->index = temp->index + upperThread_n;
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
                temp->index = temp->index + replaceThread_n;
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
                temp->index = temp->index + writeThread_n;
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

int main(int argc, char **argv, char **envp){
    
	 if(argc != 8){
	 	printf("Invalid arguments!\n");
	 	printf("Must be like : ./a.out -d deneme.txt -n 15 5 8 6"); 
		return 0;
	 }
	 
	 readThread_n = atoi(argv[4]);
	 upperThread_n = atoi(argv[5]);
	 replaceThread_n = atoi(argv[6]);
	 writeThread_n = atoi(argv[7]);     
    
    FILE *fp;
    int i = 0;
    linenumber = 200000;
    finished = 0;
    write_at = 0;
    read_at = 0;

    char* filename;
    strcpy(filename , args[2]);
    file = fopen(filename, "r+");
    
    
    sem_init(&rw_queue,0,1);
    sem_init(&upper_queue,0,1);
	 sem_init(&replace_queue,0,1);
    
    //tails (for queue))
    reader_tail = 0;
    upper_tail = 0;
    replace_tail = 0;
    writer_tail = 0;
    
    
    

    //Thread arrays START
    pthread_t read_thread[readThread_n];
    pthread_t replace_thread[replaceThread_n];
    pthread_t upper_thread[upperThread_n];
    pthread_t writer_thread[writeThread_n];
   
    r_t reader_args[readThread_n];
    r_t replace_args[replaceThread_n];
    r_t upper_args[upperThread_n];
    r_t writer_args[writeThread_n];

    if (file == NULL){
        printf("Could not open file %s",filename);
        return 1;
    }

    
    for (i = 0; i < readThread_n; i++){//Read threads initialization
        pthread_create(&read_thread[i], NULL, read, &reader_args[i]); //create readers
    }
        /*Replace args Start*/
    for (i = 0; i < replaceThread_n; i++){//Repalce threads initialization
        replace_args[i].order = i;
        replace_args[i].index = i;
        pthread_create(&replace_thread[i], NULL, replace, &replace_args[i]); //create replacers
    }
    for (i = 0; i < upperThread_n; i++){//Upper threads initialization
    	  //pthread_join(read_thread[i],NULL);
    	  
        upper_args[i].order = i;
        upper_args[i].index = i;
        pthread_create(&upper_thread[i], NULL, upper, &upper_args[i]);  //create uppers
    }


    /*Writer args Start*/
    for (i = 0; i < writeThread_n; i++){//Write threads initialization
        writer_args[i].order = i;
        writer_args[i].index = i;
        pthread_create(&writer_thread[i], NULL, write, &writer_args[i]);    //create writers
    }

    /*wait for readers*/
    for (i = 0; i < readThread_n; i++)
    {
        pthread_join(read_thread[i],NULL);
    }
    /*wait for replacers*/
     for (i = 0; i < replaceThread_n; i++)
    {
        pthread_join(replace_thread[i],NULL);
    }
    /*wait for upper*/
    for (i = 0; i < upperThread_n; i++)
    {
        pthread_join(upper_thread[i],NULL);
    }
   /*wait for writers*/
    for (i = 0; i < writeThread_n; i++)
    {
        pthread_join(writer_thread[i],NULL);
    }
    i = 0;
    
    //Pritns the converted lines
    printf("\n\nARRAY\n");
    printf("--------------\n");
    while (lines[i] != NULL)
    {
        printf("%s\n", lines[i]);   //write the array
        i++;
    }
    
    sem_destroy(&rw_queue); //destroy the queue
    pthread_exit(NULL);
    fclose(file);
    return 0;
}
