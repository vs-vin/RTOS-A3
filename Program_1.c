/*********************************************************
   ----- 48450 -- Assignment 3 - Program 1 ------
This program implements the Round Robin (RR) algorithm for CPU scheduling

 The input data of the cpu scheduling algorithm is:
--------------------------------------------------------
Process ID           Arrive time          Burst time
    1			               8	    	            10
    2                   10           	         3
    3                   14            	       7
    4                    9         	  	       5
    5                   16         	 	         4
    6                   21          	         6
    7                   26             		     2
--------------------------------------------------------

To compile:
	gcc -Wall -pthread -lrt Program_1.c -o prog1


*********************************************************/

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdint.h>
#include<errno.h>
#include<string.h>

/*---------------------------------- Defines -------------------------------*/
#define BUFFER_SIZE 30 + 1
#define MAX_PATH 100 + 1

/*---------------------------------- Variables -------------------------------*/
//a struct storing the information of each process
typedef struct
{
     int pid;//process id
     int arrive_t, wait_t, burst_t, turnaround_t, remain_t;//process time
}process_info;

//Max number of processes
#define PROCESSNUM 7
//Array of processes with 1 extra for placeholder remain_t
process_info process[9];
//Queue variable
int Len = 0;
//Time quantum for RR
int Time_quantum = 0;
//Output file
char Output_file[MAX_PATH];
//Averages calculated
float avg_wait_t = 0.0, avg_turnaround_t = 0.0;
//Semaphore
sem_t sem_SRTF, sem_read;
//Pthreads
pthread_t thread1, thread2;

//Named pipe
char * myfifo = "/tmp/myfifo";
int FD, fd;

/*---------------------------------- Functions -------------------------------*/
//Create process arrive times and burst times, taken from assignment details
void input_processes();
//Schedule processes according to SRTF rule
void process_RR();
//Simple calculate average wait time and turnaround time function
void calculate_average();
//Print results, taken from sample
void print_results(float wait, float turnaround);
//Thread 1 of assignment
void thread1_routine();
//Thread 2 of assignment
void thread2_routine();
//Add process to FIFO queue
int queue_add(int fd, uint8_t w);
//Take a process from queue
uint8_t queue_take(int fd);
//Used when processes finish to calculate wait times and turn around times
void calculate_process_times(int time, int current);

void instructions(void)
{
	printf("\n\tSecond argument is the time quantum in milliseconds\n with range 1-999\n");
	printf("\tThird argument is the output file name, including\n");
	printf("\tfile name extension (e.g. output.txt)\n");
}

/*---------------------------------- Implementation -------------------------------*/
//Check arguments entered, create semaphores, run threads then end threads and 
// semaphores when finished
int main(int argc, char* argv[]) 
{
	int len;
	int quantum; // holds value until confirmed within range

	/* ------------- Verify Arguments ------------- */

	// Check number of arguements entered on execution
	if (argc > 3)
	{
		printf("Too many arguments provided.\n");
		instructions();
		return (-7);
	}
	else if (argc < 3)
	{
		printf("Too few arguments provided.\n");
		instructions();
		return(-8);
	}

	// convert second arg to integer and store
	quantum = atoi(argv[1]);

	// Check entered time quantum within range
	if ((quantum > 999) || (quantum < 1))
	{
		printf("Time quantum out of range.\n");
		instructions();
		return(-9);
	}

	// measure file name length from argument 3
	len = strlen(argv[2]);

	// Check if length within range
	if (len < 5)
	{
		printf("Output file name too small, make sure file extension is included\n");
		instructions();
		return(-12);
	}
	else if (len > MAX_PATH)
	{
		printf("Output file name too big!\n");
		instructions();
		return(-13);
	}

	printf("\nArgument2: %d Argument3: %s of length %d", quantum, argv[2], len);

	// Save arguments to global variables
	Time_quantum = quantum;
	strcpy(Output_file, argv[2]);

	printf("\nOutput file: %s\n", Output_file);

	/* ------------- Make Named Pipe ------------- */
	mkfifo(myfifo, 0666);

	/* ------------- Initialise semaphores ------------- */
	if(sem_init(&sem_SRTF, 0, 0)!=0)
	{
	    printf("semaphore initialize error \n");
	    return(-10);
	}

	if(sem_init(&sem_read, 0, 0)!=0)
	{
	    printf("semaphore initialize error \n");
	    return(-11);
	}

	/* ------------- Create Threads ------------- */

	if(pthread_create(&thread1, NULL, (void *)thread1_routine, NULL)!=0)
 	{
	    printf("Thread 1 created error\n");
	    return -1;
	}
	if(pthread_create(&thread2, NULL, (void *)thread2_routine, NULL)!=0)
	{
	    printf("Thread 2 created error\n");
	    return -2;
	}

	/* ------------- Wait for Threads to finish ------------- */
	if(pthread_join(thread1, NULL)!=0)
	{
	    printf("join thread 1 error\n");
	    return -3;
	}
	if(pthread_join(thread2, NULL)!=0)
	{
	    printf("join thread 2 error\n");
	    return -4;
	}

	/* ------------- Destroy Semaphores ------------- */

	if(sem_destroy(&sem_SRTF)!=0)
	{
	    printf("Semaphore destroy error\n");
	    return -5;
	}

	if(sem_destroy(&sem_read)!=0)
	{
	    printf("Semaphore destroy error\n");
	    return -6;
	}

	return 0;
}

// Thread 1
void thread1_routine() 
{
	// Open FIFO for read and write 
  fd = open(myfifo, O_RDWR); 

  // Check file opened correctly
  if (fd < 0)
		{
			printf("\nfd ERROR is %d\n", fd);
			perror("ERROR open()");
			exit(-1);
		}

	input_processes();
	process_RR();
	calculate_average();

	// Write calculated global floats to formatted string
	char buffer[BUFFER_SIZE] = " ";

	sprintf(buffer, "%f %f", avg_wait_t, avg_turnaround_t);

	printf("\nString: %s\n", buffer);

	// Send formatted string to named pipe
	int k = write(fd, buffer, BUFFER_SIZE);
	printf("\tadded %d byte(s) to fifo\n", k);

	// signal Thread 2 to continue running
	sem_post(&sem_SRTF);
	// wait for Thread 2 to finish reading from named pipe
	sem_wait(&sem_read);
	// close file pointers
	close(fd);
}

// Thread 2
void thread2_routine() 
{
	float wait, turnaround;

	char buffer[BUFFER_SIZE];

	// Wait for Thread 1 to write values to named pipe
	sem_wait(&sem_SRTF);

	read(fd, buffer, BUFFER_SIZE);

	// Obtaing float values from formatted string from named pipe
	sscanf(buffer, "%f %f", &wait, &turnaround);

	printf("\nRead from FIFO: %f, %f\n", wait, turnaround);

	// Signal Thread 1 that named pipe has been read from
	sem_post(&sem_read);

	// Print obtained float values
	print_results(wait, turnaround);

	/* ------------- Generate Output File ------------- */
	FILE *fp;

	// Open data file
	fp = fopen(Output_file, "w");
	if (fp == NULL)
	{
	    printf("Could not open file %s",Output_file);
	    return;
	}
	else
	{
		fprintf(fp, "Average Wait Time:\t %fms\t", wait);
		fprintf(fp, "Average Turnaround Time:\t %fms", turnaround);
		// Mirror in terminal
		printf("Average Wait Time:\t %fms\n", wait);
		printf("Average Turnaround Time:\t %fms\n", turnaround);

		fclose(fp);
	}

}

/*  The input data of the cpu scheduling algorithm is:
--------------------------------------------------------
Process ID           Arrive time          Burst time
    1			               8	    	            10
    2                   10           	         3
    3                   14            	       7
    4                    9         	  	       5
    5                   16         	 	         4
    6                   21          	         6
    7                   26             		     2
--------------------------------------------------------
*/
// Create process arrive times and burst times, taken from assignment details
void input_processes() {
	process[0].pid = 1; process[0].arrive_t = 8;	process[0].burst_t = 10;
	process[1].pid = 2; process[1].arrive_t = 10;	process[1].burst_t = 3;
	process[2].pid = 3; process[2].arrive_t = 14;	process[2].burst_t = 7;
	process[3].pid = 4; process[3].arrive_t = 9;	process[3].burst_t = 5;
	process[4].pid = 5; process[4].arrive_t = 16; process[4].burst_t = 4;
	process[5].pid = 6; process[5].arrive_t = 21; process[5].burst_t = 6;
	process[6].pid = 7; process[6].arrive_t = 26; process[6].burst_t = 2;

	// Initialise remaining time to be same as burst time
	for (int i = 0; i < PROCESSNUM; i++) 
	{
		process[i].remain_t = process[i].burst_t;
	}
}

// Schedule processes according to RR Algorithm
void process_RR() 
{

  int current = 0, time = 0, finished = 0, arrived = 0, idle = 1;

  int q = 0;

  uint8_t r, w;

  //Run function until remain is equal to number of processes
  for (time = 0; finished < PROCESSNUM; time++) 
  {
  	// Check if processes need to run on CPU
  	if (arrived == finished)
  	{
  		idle = 1;
  	}

		// Check if a process has arrived and add to the queue
	  for (int i = 0;i < PROCESSNUM;i++) 
	  {
	  	if (process[i].arrive_t == time)
	  	{
	  		arrived++;
	  		w = i + 1;
	  		printf("\nP%d has arrived at time %d\n", w, time);

	  		// Check if processes in queue or CPU not idle
	  		if (Len > 0 || !(idle))
	  		{
	  			queue_add(fd, w);
	  		}
	  		else
	  		{
	  			// If no queue or CPU idle, set arrived process to run immediately
	  			current = w - 1;
	  			printf("\n--------- Current process set to P%d ---------\n", (current+1));
	  			printf("\t  Remaining time %d\n", process[current].remain_t);
	  			idle = 0; // CPU now active
	  			q = 0; // reset quantum
	  		}
	  		
	  	}

	  }

	  // Check if processes have finished, only if CPU not idle
	  if (!idle)
	  {
	  	if (process[current].remain_t <= 0)
	  	{
	  		printf("\n<<<<< Process %d finished at time %d >>>>>\n", 
	  																				(current+1), time);
	  		finished++;
	  		q = 0;

	  		calculate_process_times(time, current);

	  		// get next process from queue
	  		if (Len > 0)
	  		{
					r = queue_take(fd);
		    	current = r - 1;
		    	printf("\n--------- Current process set to P%d ---------\n", (current+1));
		    	printf("\t  Remaining time %d\n", process[current].remain_t);
		    	idle = 0;
		    }
		    else
		    {
		    	// if no queue, set CPU as idle
		    	idle = 1;
		    }
	  	}
	  }

	  // Check time quantum
    q++;
  	if (q > Time_quantum)
  	{
  		printf("\n--------- Time quantum elapsed at time %d ---------\n", time);
  		q = 1;

  		if (Len > 0)
  		{
  			// add curent to queue
  			w = current + 1;
  			queue_add(fd, w);
  			printf("\tRemaining time in P%d is %d\n", w, process[current].remain_t);

  			// get next process from queue
  			r = queue_take(fd);
	    	current = r - 1;
	    	printf("\n--------- Current process set to P%d ---------\n", (current+1));
	    	printf("\t  Remaining time %d\n", process[current].remain_t);
	    	idle = 0;
	    	
  		}
  		
  	}
  	// if CPU active, decrement currently running process' remaining time
  	if (!idle)
	  {
  		process[current].remain_t--;
  		printf("\tRemaining time in P%d is %d *****\n", (current+1), process[current].remain_t);
  	}
  }

  printf("\n\tROUND ROBIN FINISHED\n");

}

//Simple calculate average wait time and turnaround time function
void calculate_average() 
{
	avg_wait_t /= PROCESSNUM;
	avg_turnaround_t /= PROCESSNUM;
}

//Print results of scheduling algorithm
void print_results(float wait, float turnaround) 
{

	printf("Process Schedule Table: \n");

	printf("\tProcess ID\tArrival Time\tBurst Time\tWait Time\tTurnaround Time\n");

	for (int i = 0; i<PROCESSNUM; i++) {
	  	printf("\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", 
	  						process[i].pid,process[i].arrive_t, process[i].burst_t, 
	  						process[i].wait_t, process[i].turnaround_t);
	}

	printf("\nAverage wait time: %fms\n", avg_wait_t);

	printf("\nAverage turnaround time: %fms\n", avg_turnaround_t);
}


int queue_add(int fd, uint8_t w)
{
	printf("\tadding P%u to queue...\n", w);
	int k = write(fd, &w, sizeof(w));
	Len++;
	printf("\tadded %d byte(s) to fifo\n", k);
	return 0;  		
}

uint8_t queue_take(int fd)
{
	uint8_t r;
	int k = read(fd, &r, sizeof(r));
  Len--;
  printf("\tread %d bytes from fifo as %d..\n", k, r);
  return r;
}

void calculate_process_times(int time, int current)
{
	int endTime = time;

  process[current].turnaround_t = endTime - process[current].arrive_t;

  process[current].wait_t = process[current].turnaround_t - process[current].burst_t;

  avg_wait_t += process[current].wait_t;

  avg_turnaround_t += process[current].turnaround_t;
}