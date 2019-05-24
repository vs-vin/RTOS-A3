/*********************************************************
   ----- 48450 -- week 8 lab handout 2 ------
This is a program to practice CPU Scheduling algorithm about shortest remaining time first

 The input data of the cpu scheduling algorithm is:
--------------------------------------------------------
Process ID           Arrive time          Burst time
    1			              0	    	            10
    2                   7                   31
    3                   12                  12
    4                   3                   8
    5                   16                  5
    6                   30                  19
    7                   15                  6
    8                   30                  13
--------------------------------------------------------

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
// #include <limits.h>
// #include<stdin.h>

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
//Time quantum
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
void process_SRTF();
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
//Create semaphores, run threads then end threads and semaphores when finished
int main(int argc, char* argv[]) 
{
	int len;
	// int arg_num = argc;
	int quantum;

	// len[0] = strlen(argv[0]);
	// len[1] = strlen(argv[1]);

	// printf("Argument1: %s of length %d\n Argument2: %s of length %d", argv[0], len[0], argv[1], len[1]);

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

	// for (int i = 0; i < arg_num + 1; i++)
	// {
	// 	len[i] = strlen(argv[i]);
	// }

	quantum = atoi(argv[1]);

	// save time quantum
	if ((quantum > 999) || (quantum < 1))
	{
		printf("Time quantum out of range.\n");
		instructions();
		return(-9);
	}

	len = strlen(argv[2]);
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

	Time_quantum = quantum;

	strcpy(Output_file, argv[2]);

	printf("\nOutput file: %s\n", Output_file);


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

	int j = mkfifo(myfifo, 0666);

	if (j != 0)
	{
		printf("\nError creating FIFO\n");
		perror("ERROR mkfifo()");
	}

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

//Thread 1 of assignment
void thread1_routine() 
{
	 // Open FIFO for read and write 
  fd = open(myfifo, O_RDWR); // difference 2

	input_processes();
	process_SRTF();
	calculate_average();

	char buffer[BUFFER_SIZE] = " ";

	sprintf(buffer, "%f %f", avg_wait_t, avg_turnaround_t);

	printf("\nString: %s\n", buffer);

	if (fd < 0)
		{
			printf("\nfd ERROR is %d\n", fd);
			perror("ERROR open()");
		}

	// printf("\nhere?\n");
	int k = write(fd, buffer, BUFFER_SIZE);
	printf("\tadded %d byte(s) to fifo\n", k);

	sem_post(&sem_SRTF);
	sem_wait(&sem_read);
	// close file pointers
	close(fd);
}

//Thread 2 of assignment
void thread2_routine() 
{
	float wait, turnaround;

	char buffer[BUFFER_SIZE];

	sem_wait(&sem_SRTF);

	printf("\nhere?\n");

	// mkfifo(AvgFifo, 0666);
	// fd = open(myfifo, O_RDONLY);

	read(fd, buffer, BUFFER_SIZE);

	sscanf(buffer, "%f %f", &wait, &turnaround);

	printf("\nRead from FIFO: %f, %f\n", wait, turnaround);

	sem_post(&sem_read);

	print_results(wait, turnaround);

	// Print to output file

	// initialise file variables
	FILE *fp;
	//char* filename = Output_file;

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

		printf("Average Wait Time:\t %fms\n", wait);
		printf("Average Turnaround Time:\t %fms\n", turnaround);

		fclose(fp);
	}

}

/* The input data of the cpu scheduling algorithm is:
--------------------------------------------------------
Process ID           Arrive time          Burst time
    1										0		  						  10
    2                   7                   31
    3                   12                  12
    4                   3                   8
    5                   16                  5
    6                   30                  19
    7                   15                  6
    8                   30                  13
--------------------------------------------------------
*/
//Create process arrive times and burst times, taken from assignment details
void input_processes() {
	process[0].pid = 1; process[0].arrive_t = 8;	process[0].burst_t = 10;
	process[1].pid = 2; process[1].arrive_t = 10;	process[1].burst_t = 3;
	process[2].pid = 3; process[2].arrive_t = 14;	process[2].burst_t = 7;
	process[3].pid = 4; process[3].arrive_t = 9;	process[3].burst_t = 5;
	process[4].pid = 5; process[4].arrive_t = 16; process[4].burst_t = 4;
	process[5].pid = 6; process[5].arrive_t = 21; process[5].burst_t = 6;
	process[6].pid = 7; process[6].arrive_t = 26; process[6].burst_t = 2;
	//process[7].pid = 8; process[7].arrive_t = 30; process[7].burst_t = 13;

	//Initialise remaining time to be same as burst time
	for (int i = 0; i < PROCESSNUM; i++) {
		process[i].remain_t = process[i].burst_t;
	}
}

//Schedule processes according to SRTF rule
void process_SRTF() 
{

  int current = 0, time = 0, finished = 0, arrived = 0, idle = 1;

  // int fd;

  int q = 0;

  uint8_t r, w;

  // uint8_t integer;

  // FIFO file path 
  	// char * myfifo = "/tmp/myfifo"; 

  	// mkfifo(myfifo, 0666);  //difference 1

  // O_RDWR read and write mode

  // // Open FIFO for read and write 
  	// fd = open(myfifo, O_RDWR); // difference 2

  //Run function until remain is equal to number of processes
  for (time = 0; finished < PROCESSNUM; time++) 
  {
  	/* when decrementing remaining time then un comment below */
  	// if (arrived == finished)
  	// {
  	// 	idle = 1;
  	// }

		//Check if a process has arrived and add to the queue
	  for (int i = 0;i < PROCESSNUM;i++) 
	  {
	  	if (process[i].arrive_t == time)
	  	{
	  		arrived++;
	  		w = i + 1;
	  		printf("\nP%d has arrived at time %d\n", w, time);
	  		if (Len > 0 || !(idle))
	  		{
	  			queue_add(fd, w);
	  		}
	  		else
	  		{
	  			current = w - 1;
	  			printf("\t Current process set to process[%d]\n", current);
	  			idle = 0;
	  		}
	  		
	  	}

	  }

	  // Check if processes have finished
	  if (!idle)
	  {
	  	process[current].remain_t--;

	  	if (process[current].remain_t == 0)
	  	{
	  		printf("\n<<<<<<<< Process %d finished at time %d >>>>>>>>\n", (current +1), time);
	  		finished++;
	  		q = 0;

	  		calculate_process_times(time, current);

	  		// get next process from queue
	  		if (Len > 0)
	  		{
					r = queue_take(fd);
		    	current = r - 1;
		    	printf("\t Current process set to process[%d]\n", current);
		    	idle = 0;
		    }
		    else
		    	idle = 1;

	  	}
	  	
	  }

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

  			// get next process from queue
  			r = queue_take(fd);
	    	current = r - 1;
	    	printf("\t Current process set to process[%d]\n", current);
	    	idle = 0;
	    	
  		}
  		
  	}
  	// getchar(); // step through
  }

  printf("\nqueue in FIFO is:\n");
  while (Len > 0)
  {
		queue_take(fd);
  }

  printf("\n\tROUND ROBIN FINISHED\n");

  // close(fd);
  // unlink(myfifo);
/*****************************************************************
        // if (process[i].arrive_t <= time && process[i].remain_t < process[smallest].remain_t && 
        // 		process[i].remain_t > 0) 
        // {
        //   smallest = i;
        // }
      }

//Decrease remaining time as time increases
      process[current].remain_t--;

//If process is finished, save time information, add to average totals and increase remain
    if (process[current].remain_t == 0) 
    {

      finished++;

      endTime=time+1;

	    process[current].turnaround_t = endTime - process[current].arrive_t;

	    process[current].wait_t = process{current].turnaround_t - process[current].burst_t;

	    avg_wait_t += process[current].wait_t;

	    avg_turnaround_t += process[current].turnaround_t;
    }
  }

***************************************************************/
}

//Simple calculate average wait time and turnaround time function
void calculate_average() 
{
	avg_wait_t /= PROCESSNUM;
	avg_turnaround_t /= PROCESSNUM;
}

//Print results, taken from sample
void print_results(float wait, float turnaround) 
{

	printf("Process Schedule Table: \n");

	printf("\tProcess ID\tArrival Time\tBurst Time\tWait Time\tTurnaround Time\n");

	for (int i = 0; i<PROCESSNUM; i++) {
	  	printf("\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", process[i].pid,process[i].arrive_t, process[i].burst_t, process[i].wait_t, process[i].turnaround_t);
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
	int endTime = time + 1;

  process[current].turnaround_t = endTime - process[current].arrive_t;

  process[current].wait_t = process[current].turnaround_t - process[current].burst_t;

  avg_wait_t += process[current].wait_t;

  avg_turnaround_t += process[current].turnaround_t;
}