#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h> 

// Shared memory for rubric
char* shared_rubric;

// Shared memory for exam
char* shared_exam;

// Shared exam index
int* shared_index;

//all student exams
char exams[20][10] = {"exam1.txt","exam2.txt","exam3.txt","exam4.txt","exam5.txt","exam6.txt","exam7.txt","exam8.txt","exam9.txt","exam10.txt",
"exam11.txt","exam12.txt","exam13.txt","exam14.txt","exam15.txt","exam16.txt","exam17.txt","exam18.txt","exam19.txt","exam20.txt"};

// Shared memory file descriptor
int fd; 

struct stat sb;

sem_t *sem_exam;
sem_t *sem_rubric;

bool exam_marked = false;

// Semaphore for accessing the exam queue
sem_t *exam_queue_sem;

// Function for TA process
void ta_process(int ta_id) {


   printf("TA %d started.\n", ta_id);

    while (1) {
       // Acquire semaphore to access exam 
        sem_wait(sem_exam);

       int id_length = 4;
    
       char student_number[20];

       for(int i = 0; i < id_length; i++){
            student_number[i] = shared_exam[i];
       }

       student_number[id_length] = '\0';

       // Simulate getting an exam 
       printf("TA %d is grading exam %s.\n", ta_id, student_number);

       // Access shared_rubric to assess the exam

       int question;

       for(int i = id_length+1; i < sb.st_size; i+=2){
            if((int)shared_exam[i] < 0){
                shared_exam[i] = '0';
                question = i;
                exam_marked = false;
                break;
            }
            
            exam_marked = true;
            
       }
       
       if(exam_marked){
            printf("All questions in exam are graded. Accessing next exam.\n");
            int index = *shared_index;
            *shared_index = index + 1; 
       }
       else{
            printf("TA %d graded exam %s, question %c.\n", ta_id, student_number, shared_exam[question]);
       }

       // Release semaphore 
       sem_post(sem_exam);

       // Random chance to modify rubric
       if (rand() % 5 == 0) {
            sem_wait(sem_rubric);
            int change = (rand() % 5); // 1, 2, 3, 4, 5
            int count = 1;
            for(int i = 3; i < sb.st_size; i+=5){
                if(count == change){
                    if(shared_rubric[i] == 52){
                        shared_rubric[i] = 48;
                    }
                    else{
                        shared_rubric[i] = (int)shared_rubric[i] + 1;
                    }
                    break;
                }
                
                count++;
            }
            
            printf("TA %d modified rubric → # %d\n", ta_id, change);
            sem_post(sem_rubric);
        }

       sleep(1); // Simulate grading time
   }
}

char* shared_memory(const char* filename){

    // Open the file
   fd = open(filename, O_RDWR);
   if (fd == -1) {
       perror("Error opening file");
       exit(EXIT_FAILURE);
   }

   // Get file size
   if (fstat(fd, &sb) == -1) {
       perror("Error getting file size");
       close(fd);
       exit(EXIT_FAILURE);
   }

   // Map the file into memory
   return mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

int main() {

    // Generate a unique key for the shared memory segment
    key_t key = ftok("shmfile", 65); 

    // Create a shared memory segment for an integer (sizeof(int) bytes)
    // IPC_CREAT creates if it doesn't exist, 0666 sets permissions
    int shmid = shmget(key, sizeof(int), IPC_CREAT | 0666); 
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    shared_index = (int *)shmat(shmid, NULL, 0); 
    if (shared_index == (int *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    *shared_index = 0;

   int num_tas = 2;

   const char* filename_rubric = "rubric.txt";
   const char* filename_exam = "exam1.txt";

   // Map the file into memory
   shared_rubric = shared_memory(filename_rubric);

   if (shared_rubric == MAP_FAILED) {
       perror("Error mapping file to memory");
       close(fd);
       exit(EXIT_FAILURE);
   }

   printf("Content loaded into shared memory:\n%s\n", shared_rubric);
   printf("\n");

   shared_exam = shared_memory(filename_exam);

   if (shared_exam == MAP_FAILED) {
       perror("Error mapping file to memory");
       close(fd);
       exit(EXIT_FAILURE);
   }

   printf("Content loaded into shared memory:\n%s\n", shared_exam);
   printf("\n");

   // Create and initialize semaphores

    sem_exam   = sem_open("/sem_exam",   O_CREAT, 0666, 1);
    sem_rubric = sem_open("/sem_rubric", O_CREAT, 0666, 1);
   
   // Fork TA processes
   for (int i = 0; i < num_tas; i++) {
       pid_t pid = fork();
       if (pid == 0) { // Child process
           ta_process(i + 1);
           exit(0);
       } else if (pid < 0) {
           perror("fork failed");
           exit(1);
       }
   }

   for (int i = 0; i < num_tas; i++) {
       // Parent loads student data
        sem_wait(sem_exam);
        int index = *shared_index;
        shared_exam = shared_memory(exams[index]);
        if (shared_exam == MAP_FAILED) {
            perror("Error mapping file to memory");
            close(fd);
            exit(EXIT_FAILURE);
        }
        printf("Content loaded into shared memory:\n%s\n", shared_exam);
        printf("\n");
        sem_post(sem_exam);
   }

   // Unmap the memory and close the file
   if (munmap(shared_rubric, sb.st_size) == -1) {
       perror("Error unmapping memory");
   }

   if (munmap(shared_exam, sb.st_size) == -1) {
       perror("Error unmapping memory");
   }
   close(fd);

   return 0;
   
}
