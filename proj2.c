#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>


// Custom function for getting random numbers in the needed range.
int get_rand(int min, int max, int sran_val)
{
    // Updating random generator every time using time now.
	srand(sran_val);
	
    int rand_num = 0;
    rand_num = rand() % (max - min + 1);
    rand_num += min;
    return rand_num;
}


int main(int argc, char** argv)
{



    // Checking the number of passed arguments.
    if(argc != 6)
    {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }
    
    
    int NZ = atoi(argv[1]), // Customers.
        NU = atoi(argv[2]), // Workers.
        TZ = atoi(argv[3]), // Max time for customer to wait before entering.
        TU = atoi(argv[4]), // Break time.
        F  = atoi(argv[5]); // Max time before closing for new customers.
        
        
    // Checking if each argument is in the right range.
    if((0>TZ||TZ>10000)||(0>TU||TU>100)||(0>F||F>10000))
    {
        fprintf(stderr, "Some arguments are in the wrong range of values!\n");
        exit(1);
    }


    // Setting up proj2.out file for output.
    
    int shmidFILE;
    FILE *fp; 
    key_t keyFILE = 1123; 
    shmidFILE = shmget(keyFILE, sizeof(FILE), IPC_CREAT | 0666);
    fp = (FILE *) shmat(shmidFILE, NULL, 0);
    fp = fopen("proj2.out", "w");

    
    
    
    // Setting up shared memory variables.
    
    
    // AN stands for Action Number.
    int shmidAN;
    int *AN; 
    key_t keyAN = 1234; 
    shmidAN = shmget(keyAN, sizeof(int), IPC_CREAT | 0666);
    AN = (int *) shmat(shmidAN, NULL, 0);
    *AN = 0;
    
    // Open or Closed bool variable. 
    int shmidOpen;
    bool *isOpen;
    key_t keyOp = 1235;
    shmidOpen = shmget(keyOp, sizeof(bool), IPC_CREAT | 0666);
    isOpen = (bool *) shmat(shmidOpen, NULL, 0);
    *isOpen = true;
    
    // Queue 1 array + Call variable, where the id of customer will be sent to.
    // We will use the number of customers as a size.
    int shmidQ1; 
    int *shm_Q1;
    key_t keyQ1 = 1236;
    shmidQ1 = shmget(keyQ1, NZ * sizeof(int), IPC_CREAT | 0666);
    shm_Q1 = (int *) shmat(shmidQ1, NULL, 0);
    
    int shmidCallQ1;
    int *shm_CallQ1; 
    key_t keyCallQ1 = 1000; 
    shmidCallQ1 = shmget(keyCallQ1, sizeof(int), IPC_CREAT | 0666);
    shm_CallQ1 = (int *) shmat(shmidCallQ1, NULL, 0);
    *shm_CallQ1 = 0;
    
    
    // Queue 2 array.
    int shmidQ2; 
    int *shm_Q2;
    key_t keyQ2 = 1237;
    shmidQ2 = shmget(keyQ2, NZ * sizeof(int), IPC_CREAT | 0666);
    shm_Q2 = (int *) shmat(shmidQ2, NULL, 0);
    
    int shmidCallQ2;
    int *shm_CallQ2; 
    key_t keyCallQ2 = 1001; 
    shmidCallQ2 = shmget(keyCallQ2, sizeof(int), IPC_CREAT | 0666);
    shm_CallQ2 = (int *) shmat(shmidCallQ2, NULL, 0);
    *shm_CallQ2 = 0;
    
    // Queue 3 array.
    int shmidQ3; 
    int *shm_Q3;
    key_t keyQ3 = 1238;
    shmidQ3 = shmget(keyQ3, NZ * sizeof(int), IPC_CREAT | 0666);
    shm_Q3 = (int *) shmat(shmidQ3, NULL, 0);
    
    int shmidCallQ3;
    int *shm_CallQ3; 
    key_t keyCallQ3 = 1002; 
    shmidCallQ3 = shmget(keyCallQ3, sizeof(int), IPC_CREAT | 0666);
    shm_CallQ3 = (int *) shmat(shmidCallQ3, NULL, 0);
    *shm_CallQ3 = 0;
    
    // Setting the default value for all the arrays.
    for(int i = 0; i<NZ; i++)
    {
        shm_Q1[i]=-1;
        shm_Q2[i]=-1;
        shm_Q3[i]=-1;
    }

    

    // Setting up the semaphore for Action Number and moving it to shared memory.
    // Now we can use it in every child process.
    int shared_memory_fd = shm_open("SHM_AN", O_CREAT | O_RDWR, 0666);
    if (shared_memory_fd == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    if (ftruncate(shared_memory_fd, sizeof(sem_t)) == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_t *sem_an = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
    if (sem_an == MAP_FAILED) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_init(sem_an, 1, 1);
    
    
    // Setting up the semaphore for Queue 1.
    // 1 for actually 2 vars in shared memory.
    int shared_memory_q1 = shm_open("SHM_Q1", O_CREAT | O_RDWR, 0666);
    if (shared_memory_q1 == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    if (ftruncate(shared_memory_q1, sizeof(sem_t)) == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_t *sem_q1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_q1, 0);
    if (sem_q1 == MAP_FAILED) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_init(sem_q1, 1, 1);
    
    
    // Setting up the semaphore for Queue 2.
    int shared_memory_q2 = shm_open("SHM_Q2", O_CREAT | O_RDWR, 0666);
    if (shared_memory_q2 == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    if (ftruncate(shared_memory_q2, sizeof(sem_t)) == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_t *sem_q2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_q2, 0);
    if (sem_q2 == MAP_FAILED) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_init(sem_q2, 1, 1);
    
    
    // Setting up the semaphore for Queue 3.
    int shared_memory_q3 = shm_open("SHM_Q3", O_CREAT | O_RDWR, 0666);
    if (shared_memory_q3 == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    if (ftruncate(shared_memory_q3, sizeof(sem_t)) == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_t *sem_q3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_q3, 0);
    if (sem_q3 == MAP_FAILED) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_init(sem_q3, 1, 1);
    
    
    // Setting up the semaphore for workers.
    int shared_memory_wrk = shm_open("SHM_WRK", O_CREAT | O_RDWR, 0666);
    if (shared_memory_wrk == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    if (ftruncate(shared_memory_wrk, sizeof(sem_t)) == -1) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_t *sem_wrk = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_wrk, 0);
    if (sem_wrk == MAP_FAILED) 
    {
        fprintf(stderr, "Shared memory error!\n");
        exit(1);
    }
    sem_init(sem_wrk, 1, 1);
    
  

    // Starting customers` processes.
    for (int i = 0; i <NZ; i++)
    {
        pid_t pid = fork();
        if (pid == -1) 
        {
            perror("Fork error!\n");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) 
        {
            sem_wait(sem_an);
            *AN+=1;
            fprintf(fp, "%d: Z %d: started\n", *AN, i+1);  
            fflush(fp);
            sem_post(sem_an);
	
	        // Choosing random activity after the random usleep().
            usleep(get_rand(0, TZ, getpid())*1000);
            int activity = get_rand(1, 3, getpid());
                
                
                    sem_wait(sem_an);
                    *AN+=1;
                    if(*isOpen)
                    {
                        fprintf(fp, "%d: Z %d: entering office for a service %d\n", *AN, i+1, activity);
                        fflush(fp);
                        sem_post(sem_an);
                        
                        // Adding the customer to the right Queue, depending on the activity.
                        switch(activity)
                        {
                            case 1:
                                sem_wait(sem_q1);
                                for (int y = 0; y < NZ; y++) 
                                {
                                    if (shm_Q1[y] == -1) 
                                    {
                                        shm_Q1[y] = i+1; 
                                        break;
                                    }
                                }
                                sem_post(sem_q1);
                                break;
                            case 2:
                                sem_wait(sem_q2);
                                for (int y = 0; y < NZ; y++) 
                                {
                                    if (shm_Q2[y] == -1) 
                                    {
                                        shm_Q2[y] = i+1; 
                                        break;
                                    }
                                }
                               
                                sem_post(sem_q2);
                                break;
                            case 3:
                                sem_wait(sem_q3);
                                for (int y = 0; y < NZ; y++) 
                                {
                                    if (shm_Q3[y] == -1) 
                                    {
                                        shm_Q3[y] = i+1; 
                                        break;
                                    }
                                }
                                
                                sem_post(sem_q3);
                                break;
                        }
                        
                    
                        //Waiting for call. 
                        while(true)
                        {
                            sem_wait(sem_q1);
                            sem_wait(sem_q2);
                            sem_wait(sem_q3);
                            if(*shm_CallQ1==i+1)
                            {
                                sem_wait(sem_an);
                                *AN+=1;
                                fprintf(fp, "%d: Z %d: called by office worker\n", *AN, i+1);  
                                fflush(fp);
                                sem_post(sem_an);
                                
                                sem_post(sem_q1);
                                sem_post(sem_q2);
                                sem_post(sem_q3);
                                break;
                            }
                            
                            if(*shm_CallQ2==i+1)
                            {
                                sem_wait(sem_an);
                                *AN+=1;
                                fprintf(fp, "%d: Z %d: called by office worker\n", *AN, i+1);  
                                fflush(fp);
                                sem_post(sem_an);
                                
                                sem_post(sem_q1);
                                sem_post(sem_q2);
                                sem_post(sem_q3);
                                break;
                            }
                            
                            
                            if(*shm_CallQ3==i+1)
                            {
                                sem_wait(sem_an);
                                *AN+=1;
                                fprintf(fp, "%d: Z %d: called by office worker\n", *AN, i+1);  
                                fflush(fp);
                                sem_post(sem_an);
                                
                                sem_post(sem_q1);
                                sem_post(sem_q2);
                                sem_post(sem_q3);
                                break;
                            }
                            
                                sem_post(sem_q1);
                                sem_post(sem_q2);
                                sem_post(sem_q3);
                        }
                        
                        // Leaving after everything is done.
                        usleep(get_rand(0, 10, getpid())*1000);
                        
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: Z %d: going home\n", *AN, i+1);  
                        fflush(fp);
                        sem_post(sem_an);
                        
                        exit(EXIT_SUCCESS);
                        
                    }
                    else
                    {
                        
                        // Closed
                        fprintf(fp, "%d: Z %d: going home\n", *AN, i+1);
                        fflush(fp);
                        sem_post(sem_an);
                    }
                    exit(EXIT_SUCCESS);
        }
    }
    
    
    // Starting workers` processes.
    for (int j = 0; j < NU; j++)
    {
        pid_t pid = fork();
        if (pid == -1) 
        {
            perror("Fork error!\n");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) 
        {
            sem_wait(sem_an);
            *AN+=1;
            fprintf(fp, "%d: U %d: started\n", *AN, j+1);  
            fflush(fp);
            sem_post(sem_an);
            
            usleep(get_rand(0, TZ, getpid())*1000);
            
            int activity;
            while(true)
            {
                activity=0;
                // Choosing random not-empty Queue to service
                sem_wait(sem_wrk);
                sem_wait(sem_q1);
                sem_wait(sem_q2);
                sem_wait(sem_q3);
                if(shm_Q1[0]>0 && shm_Q2[0]>0 && shm_Q3[0]>0)
                {
                    activity=get_rand(1, 3, getpid());
                }
                else if(shm_Q1[0]>0)
                {
                    activity=1;
                }
                else if(shm_Q2[0]>0)
                {
                    activity=2;
                }
                else if(shm_Q3[0]>0)
                {
                    activity=3;
                }
                
                
                // Sending a call to the right customer, updating.
                switch(activity)
                {
                    case 0:
                        break;
                    case 1: 
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: U %d: serving a service of type 1\n", *AN, j+1);  
                        fflush(fp);
                        sem_post(sem_an);
                        *shm_CallQ1 = shm_Q1[0];
                        for(int y=0; y<NZ; y++) 
                        {
                            shm_Q1[y] = shm_Q1[y+1]; //Clearing the queue.
                        }
                        break;
                    case 2:
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: U %d: serving a service of type 2\n", *AN, j+1);  
                        fflush(fp);
                        sem_post(sem_an);
                        *shm_CallQ2 = shm_Q2[0];
                        for(int y=0; y<NZ; y++) 
                        {
                            shm_Q2[y] = shm_Q2[y+1]; 
                        }
                        break;
                    case 3:
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: U %d: serving a service of type 3\n", *AN, j+1);  
                        fflush(fp);
                        sem_post(sem_an);
                        *shm_CallQ3 = shm_Q3[0];
                        for(int y=0; y<NZ; y++) 
                        {
                            shm_Q3[y] = shm_Q3[y+1];
                        }
                        break;
                }
                sem_post(sem_wrk);
                sem_post(sem_q1);
                sem_post(sem_q2);
                sem_post(sem_q3);
                
                if(activity>0)
                {
                    usleep(get_rand(0, 10, getpid())*1000);
                    sem_wait(sem_an);
                    *AN+=1;
                    fprintf(fp, "%d: U %d: service finished\n", *AN, j+1);  
                    fflush(fp);
                    sem_post(sem_an);
                }
                
                
                if(activity==0)
                {
                    if(*isOpen)
                    {
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: U %d: taking break\n", *AN, j+1);  
                        fflush(fp);
                        sem_post(sem_an);
                        
                        usleep(get_rand(0, TU, getpid())*1000);
                        
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: U %d: break finished\n", *AN, j+1);  
                        fflush(fp);
                        sem_post(sem_an);
                    }
                    else
                    {
                        sem_wait(sem_an);
                        *AN+=1;
                        fprintf(fp, "%d: U %d: going home\n", *AN, j+1);  
                        fflush(fp);
                        sem_post(sem_an);
                        exit(EXIT_SUCCESS);
                    }
                }
                
                
                
                
            }
            exit(EXIT_SUCCESS);
        }
    }
    
    
    
    
    pid_t pid = fork();
    
    
    if (pid == -1) 
    {
        perror("Fork error!");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) 
    {
        usleep(get_rand((int)(F/2), F, getpid())*1000);
        sem_wait(sem_an);
        fprintf(fp, "%d: closing\n", ++*AN);
        fflush(fp);
        sem_post(sem_an);
        *isOpen=false;
        exit(EXIT_SUCCESS);
    }
    
    
    
    // Waiting for the end of all processes.
    for (int i = 0; i < NZ+NU+1; i++) 
    {
        wait(NULL);
    }
    
    // Closing semaphores and shared memory.
    sem_close(sem_an);
    sem_close(sem_q1);
    sem_close(sem_q2);
    sem_close(sem_q3);
    sem_close(sem_wrk);
    
    fclose(fp);

    shmctl(shmidAN, IPC_RMID, NULL);
    shmctl(shmidOpen, IPC_RMID, NULL);
    shmctl(shmidQ1, IPC_RMID, NULL);
    shmctl(shmidQ2, IPC_RMID, NULL);
    shmctl(shmidQ3, IPC_RMID, NULL);
    shmctl(shmidCallQ1, IPC_RMID, NULL);
    shmctl(shmidCallQ2, IPC_RMID, NULL);
    shmctl(shmidCallQ3, IPC_RMID, NULL);
    shmctl(shmidFILE, IPC_RMID, NULL);

    
    return 0;
}









