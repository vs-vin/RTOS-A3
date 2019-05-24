/* 
* RTOS Autumn 2019
* Assignment 3 Program_2 template
*
* To compile:
* 	gcc -Wall -pthread -lrt Program_2.c -o prog2
*/

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

//Number of pagefaults in the program
int PageFaults = 0;

//Function declaration
void SignalHandler(int signal);

/**
 Main routine for the program. In charge of setting up threads and the FIFO.

 @param argc Number of arguments passed to the program.
 @param argv array of values passed to the program.
 @return returns 0 upon completion.
 */
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("\nERROR: Too few arguments. "
						 "Make sure to provide the frame size as second argument.\n");
		return(-1);
	}
	else if (argc > 2)
	{
		printf("\nERROR: Too many arguments. "
						 "Make sure to provide the frame size as second argument.\n");
		return(-2);
	}
	
	//Register Ctrl+c(SIGINT) signal and call the signal handler for the function.
  signal(SIGINT, SignalHandler);

	// reference number
	int REFERENCESTRINGLENGTH = 24;
	//Argument from the user on the frame size, such as 4 frames in the document
	int frameSize = atoi(argv[1]);
	//Frame where we will be storing the references. -1 is equivalent to an empty value
	uint frame[REFERENCESTRINGLENGTH];
	//Reference string from the assignment outline
	int referenceString[24] = {7,0,1,2,0,3,0,4,2,3,0,3,0,3,2,1,2,0,1,7,0,1,7,5};
	//Next position to write a new value to.
	int nextWritePos = 0;
	//Boolean value for whether there is a match or not.
	bool match = false;
	//Current value of the reference string.
	int currentValue;

	printf("\nFrame size entered is %d", frameSize);
	printf("\n\tPF: Page Fault");
	printf("\n\tPH: Page Hit\n\n");

	//Initialise the empty frame with -1 to simulate empty values.
	for(int i = 0; i < frameSize; i++)
	{
		frame[i] = -1;
	}

	//Loop through the reference string values.
	for(int i = 0; i < REFERENCESTRINGLENGTH; i++)
	{
		//add your code here
		currentValue = referenceString[i];
		printf("%d ", currentValue);

		match = 0;
		for(int j = 0; j < frameSize; j++)
		{
			if (currentValue == frame[j])
			{
				match = true;
				break;
			}
		}

		if (match)
		{
			printf(" PH");
		}
		else
		{
			printf(" PF\t");
			PageFaults++;
			// FIFO algorithm
			frame[nextWritePos] = currentValue;
			nextWritePos++;
			if (nextWritePos >= frameSize)
			{
				nextWritePos = 0;
			}

			for(int j = 0; j < frameSize; j++)
			{
				printf("%d ", frame[j]);
			}
		}
		printf("\n");
	}

	printf("\nPress Ctrl+C to exit>\n");

	//Sit here until the ctrl+c signal is given by the user.
	while(1)
	{
		sleep(1);
	}

	return 0;
}

/**
 Performs the final print when the signal is received by the program.

 @param signal An integer values for the signal passed to the function.
 */
void SignalHandler(int signal)
{
	printf("\nTotal page faults: %d\n", PageFaults);
	exit(0);
}
