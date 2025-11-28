#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h> // For stat
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h> // For O_CREAT, O_EXCL

// Shared memory for rubric
char* shared_rubric;
// Shared memory for exam
char* shared_exam;

int fd; // Shared memory file descriptor
struct stat sb;

// Semaphore for accessing the exam queue
sem_t *exam_queue_sem;

// Function for TA process
void ta_process(int ta_id) {


   printf("TA %d started.\n", ta_id);

    while (1) {
       // Acquire semaphore to access exam queue
       //sem_wait(exam_queue_sem);
       int id_length = 4;
    
       char student_number[20];

       for(int i = 0; i < id_length; i++){
            student_number[i] = shared_exam[i];
       }

       student_number[id_length] = '\0';

       // Simulate getting an exam (replace with actual exam fetching)
       printf("TA %d is grading exam %s.\n", ta_id, student_number);

       // Access shared_rubric to assess the exam
       // (No lock needed on rubric if it's read-only after initialization)

       for(int i = id_length; i < sb.stat_size; i+= id_length){
            if((int)shared_exam[i+2] == (int)'N'){
                shared_exam[i+2] = 'Y';
                break
            }
       }
       
       printf("TA %d graded exam %s, question %d.\n", ta_id, student_number);

       // Release semaphore (if needed for other shared resources, not for the queue here)
       // For a simple queue, only one TA takes an exam at a time.
       // If the queue itself is a shared data structure, it would need its own mutex.

       sleep(1); // Simulate grading time
       //sem_post(exam_queue_sem);
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
   int num_tas = 2;

   ////
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

   // Now, shared_memory_ptr points to the file's content in shared memory.
   // You can access and modify it like a regular char array.
   printf("Content loaded into shared memory:\n%s\n", shared_exam);
   printf("\n");

   for(int i = 0; i < sb.st_size; i++){
    printf("Character %d:%c, %d\n", i, shared_exam[i], (int) shared_exam[i]);
   }

   for(int i = 0; i < sb.st_size; i++){
    printf("Character %d:%c, %d\n", i, shared_rubric[i], (int) shared_rubric[i]);
   }

   // Example modification (optional)
   // shared_memory_ptr[0] = 'N';

   // Create and initialize semaphore for exam queue
   exam_queue_sem = sem_open("/exam_queue_sem", O_CREAT | O_EXCL, 0666, 1); // Initial value 1

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

   // Parent process waits for children 
   for (int i = 0; i < num_tas; i++) {
       wait(NULL);
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
