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
// #include<stdint.h>

/*---------------------------------- Variables -------------------------------*/
//a struct storing the information of each process
typedef struct
{
     int pid;//process id
     int arrive_t, wait_t, burst_t, turnaround_t, remain_t;//process time
}process;

//Max number of processes
#define PROCESSNUM 7
//Array of processes with 1 extra for placeholder remain_t
process processes[9];
//Index variable
int i;
//Averages calculated
float avg_wait_t = 0.0, avg_turnaround_t = 0.0;
//Semaphore
sem_t sem_SRTF;
//Pthreads
pthread_t thread1, thread2;

/*---------------------------------- Functions -------------------------------*/
//Create process arrive times and burst times, taken from assignment details
void input_processes();
//Schedule processes according to SRTF rule
void process_SRTF();
//Simple calculate average wait time and turnaround time function
void calculate_average();
//Print results, taken from sample
void print_results();
//Thread 1 of assignment
void thread1_routine();
//Thread 2 of assignment
void thread2_routine();

/*---------------------------------- Implementation -------------------------------*/
//Create semaphores, run threads then end threads and semaphores when finished
int main() {

	if(sem_init(&sem_SRTF, 0, 0)!=0)
	{
	    printf("semaphore initialize erro \n");
	    return(-10);
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

	return 0;
}

//Thread 1 of assignment
void thread1_routine() {
	input_processes();
	process_SRTF();
	calculate_average();
	sem_post(&sem_SRTF);
}

//Thread 2 of assignment
void thread2_routine() {
	sem_wait(&sem_SRTF);
	print_results();
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
	processes[0].pid = 1; processes[0].arrive_t = 8;	processes[0].burst_t = 10;
	processes[1].pid = 2; processes[1].arrive_t = 10;	processes[1].burst_t = 3;
	processes[2].pid = 3; processes[2].arrive_t = 14;	processes[2].burst_t = 7;
	processes[3].pid = 4; processes[3].arrive_t = 9;	processes[3].burst_t = 5;
	processes[4].pid = 5; processes[4].arrive_t = 16; processes[4].burst_t = 4;
	processes[5].pid = 6; processes[5].arrive_t = 21; processes[5].burst_t = 6;
	processes[6].pid = 7; processes[6].arrive_t = 26; processes[6].burst_t = 2;
	//processes[7].pid = 8; processes[7].arrive_t = 30; processes[7].burst_t = 13;

	//Initialise remaining time to be same as burst time
	for (i = 0; i < PROCESSNUM; i++) {
		processes[i].remain_t = processes[i].burst_t;
	}
}

//Schedule processes according to SRTF rule
void process_SRTF() 
{

  int endTime, current, time, finished = 0;

  int fd, k, m = 0, q = 0, len = 0;

  char p, j;

  int r, w;

  // uint8_t integer;

  // FIFO file path 
  char * myfifo = "/tmp/myfifo"; 

  mkfifo(myfifo, 0666);  //difference 1

  // O_RDWR read and write mode

  // Open FIFO for read and write 
  fd = open(myfifo, O_RDWR); // difference 2

  // //Placeholder remaining time to be replaced
  // processes[8].remain_t=9999;

  //Run function until remain is equal to number of processes
  for (time = 0; finished < PROCESSNUM; time++) 
  {

		//Assign placeholder remaining time as smallest
    current = 8;

		//Check all processes that have arrived for lowest remain time then set the lowest to be smallest
	  for (i=0;i<PROCESSNUM;i++) 
	  {
	  	if (processes[i].arrive_t == time)
	  	{
	  		// fd = open(myfifo, O_RDWR);
	  		p = '1' + i;
	  		printf("\nP%c has arrived at time %d\n", p, time);
	  		
	  		k = write(fd, &p, sizeof(p));
	  		len++;
	  		
	  		printf("\tadded %d byte(s) to fifo..\n", k);

	  		  		
	  		finished++;
	  		// close(fd);
	  	}

	  }

    q++;
  	if (q > 4)
  	{
  		printf("\nTime quantum elapsed at time %d\n", time);
  		q = 1;
  		
  		if (len > 0)
  		{
  			k = read(fd, &j, sizeof(j));
  			len--;
    		printf("\tread %d bytes from fifo as %c..\n", k, j);
	    	current = j - '0';
	    	printf("\t Current process set to P%d\n", current);
  		}
  		


  	}

  }

  printf("\nqueue in FIFO is:\n");
  while (len > 0)// read(fd, &j, sizeof(j)) > 0)
  {
  	// fd = open(myfifo, O_RDWR);
  	k = read(fd, &j, sizeof(j));
  	len--;
    printf("\tread %d bytes from fifo as %c..\n", k, j);

    // m++;
  	//printf("P%d, ", j);
  	// close(fd);
  }

  close(fd);
/*****************************************************************
        // if (processes[i].arrive_t <= time && processes[i].remain_t < processes[smallest].remain_t && 
        // 		processes[i].remain_t > 0) 
        // {
        //   smallest = i;
        // }
      }

//Decrease remaining time as time increases
      processes[current].remain_t--;

//If process is finished, save time information, add to average totals and increase remain
    if (processes[current].remain_t == 0) 
    {

      finished++;

      endTime=time+1;

	    processes[current].turnaround_t = endTime - processes[current].arrive_t;

	    processes[current].wait_t = process{current].turnaround_t - processes[current].burst_t;

	    avg_wait_t += processes[current].wait_t;

	    avg_turnaround_t += processes[current].turnaround_t;
    }
  }

***************************************************************/
}

//Simple calculate average wait time and turnaround time function
void calculate_average() {
	avg_wait_t /= PROCESSNUM;
	avg_turnaround_t /= PROCESSNUM;
}

//Print results, taken from sample
void print_results() {

	printf("Process Schedule Table: \n");

	printf("\tProcess ID\tArrival Time\tBurst Time\tWait Time\tTurnaround Time\n");

	for (i = 0; i<PROCESSNUM; i++) {
	  	printf("\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", processes[i].pid,processes[i].arrive_t, processes[i].burst_t, processes[i].wait_t, processes[i].turnaround_t);
	}

	printf("\nAverage wait time: %fs\n", avg_wait_t);

	printf("\nAverage turnaround time: %fs\n", avg_turnaround_t);
}
