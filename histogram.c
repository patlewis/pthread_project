/* File:      histogram.c
 * Purpose:   Build a histogram from some random data
 * 
 * Compile:   gcc -g -pthread -lm -Wall -o histogram histogram.c
 * Run:       ./histogram <bin_count> <min_meas> <max_meas> <data_count> <thread_count>
 *
 * Input:     None
 * Output:    A histogram with X's showing the number of measurements
 *            in each bin
 *
 * Notes:
 * 1.  Actual measurements y are in the range min_meas <= y < max_meas
 * 2.  bin_counts[i] stores the number of measurements x in the range
 * 3.  bin_maxes[i-1] <= x < bin_maxes[i] (bin_maxes[-1] = min_meas)
 * 4.  DEBUG compile flag gives verbose output
 * 5.  The program will terminate if either the number of command line
 *     arguments is incorrect or if the search for a bin for a 
 *     measurement fails.
 *
 * IPP:  Section 2.7.1 (pp. 66 and ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <math.h>

void Usage(char prog_name[]);

void Get_args(
      char*    argv[]        /* in  */,
      int*     bin_count_p   /* out */,
      float*   min_meas_p    /* out */,
      float*   max_meas_p    /* out */,
      int*     data_count_p  /* out */,
      int*     thread_count_p/* out */);

void Gen_data(
      float   min_meas    /* in  */, 
      float   max_meas    /* in  */, 
      float   data[]      /* out */,
      int     data_count  /* in  */);

void Gen_bins(
      float min_meas      /* in  */, 
      float max_meas      /* in  */, 
      float bin_maxes[]   /* out */, 
      int   bin_counts[]  /* out */, 
      int   bin_count     /* in  */);

int Which_bin(
      float    data         /* in */, 
      float    bin_maxes[]  /* in */, 
      int      bin_count    /* in */, 
      float    min_meas     /* in */);

void Print_histo(
      float    bin_maxes[]   /* in */, 
      int      bin_counts[]  /* in */, 
      int      bin_count     /* in */, 
      float    min_meas      /* in */);

void* thread_do_work(void*);

/* Global variables.  The threads will need to access these. Somehow. */
int* local_bins;           //the "multiple", "local" bins
float* data;               //the full array of data
int* bin_counts;           //the final set of bins
float min_meas, max_meas;  //for finding out the data boundaries
float* bin_maxes;          //set of upper limits for all the bins
int bin_count;             //number of bins we'll use
int thread_count;          //number of threads we'll use
int data_count;            //number of data elements to sort
pthread_t* threads;        //memory for holding all the threads

/* These globals will be used as part of the barrier*/
int counter = 0;
pthread_mutex_t mutex;
pthread_cond_t cond_var;

int main(int argc, char* argv[]) 
{  
   int i;

   /* Check and get command line args */
   if (argc != 6) Usage(argv[0]); 
   Get_args(argv, &bin_count, &min_meas, &max_meas, &data_count, &thread_count);

   /* Allocate arrays needed */
   bin_maxes = (float *)malloc(bin_count * sizeof(float));
   bin_counts = (int *)malloc(bin_count * sizeof(int));
   data = (float *)malloc(data_count * sizeof(float));
   threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
   local_bins = (int *)malloc(bin_count * thread_count * sizeof(int));

   /* Initialize thread stuff */
   pthread_mutex_init(&mutex, NULL);
   pthread_cond_init(&cond_var, NULL);
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   /* Generate the data */
   Gen_data(min_meas, max_meas, data, data_count);

   /* Create bins for storing counts */
   Gen_bins(min_meas, max_meas, bin_maxes, bin_counts, bin_count);

   /* Count number of values in each bin.
    * This was used in the single-threaded version.
    * It is hereby deprecated.
   for (i = 0; i < data_count; i++) {
      bin = Which_bin(data[i], bin_maxes, bin_count, min_meas);
      bin_counts[bin]++;
   }
   */

   /* Create the threads and have them do work */
   for (i = 0;i < thread_count; i++)
   {
      pthread_create(&threads[i], &attr, thread_do_work, (void *) (intptr_t) i);
   }

   /* When they're all done, join them up */
   for (i = 0; i < thread_count; i++)
   {
      pthread_join(threads[i], NULL);
   }

#  ifdef DEBUG
   printf("bin_counts = ");
   for (i = 0; i < bin_count; i++)
      printf("%d ", bin_counts[i]);
   printf("\n");
#  endif

   /* Print the histogram */
   Print_histo(bin_maxes, bin_counts, bin_count, min_meas);

   free(data);
   free(bin_maxes);
   free(bin_counts);
   free(threads);
   free(local_bins);
   return 0;

}  /* main */


/*---------------------------------------------------------------------
 * Function:  Usage 
 * Purpose:   Print a message showing how to run program and quit
 * In arg:    prog_name:  the name of the program from the command line
 */
void Usage(char prog_name[] /* in */) {
   fprintf(stderr, "usage: %s ", prog_name); 
   fprintf(stderr, "<bin_count> <min_meas> <max_meas> <data_count> <thread_count>\n");
   exit(0);
}  /* Usage */


/*---------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get the command line arguments
 * In arg:    argv:  strings from command line
 * Out args:  bin_count_p:   number of bins
 *            min_meas_p:    minimum measurement
 *            max_meas_p:    maximum measurement
 *            data_count_p:  number of measurements
 *            thread_count_p:number of threads
 */
void Get_args(
      char*    argv[]        /* in  */,
      int*     bin_count_p   /* out */,
      float*   min_meas_p    /* out */,
      float*   max_meas_p    /* out */,
      int*     data_count_p  /* out */,
      int*     thread_count_p/* out */) {

   *bin_count_p = strtol(argv[1], NULL, 10);
   *min_meas_p = strtof(argv[2], NULL);
   *max_meas_p = strtof(argv[3], NULL);
   *data_count_p = strtol(argv[4], NULL, 10);
   *thread_count_p = strtol(argv[5], NULL, 10);

#  ifdef DEBUG
   printf("bin_count = %d\n", *bin_count_p);
   printf("min_meas = %f, max_meas = %f\n", *min_meas_p, *max_meas_p);
   printf("data_count = %d\n", *data_count_p);
   printf("thread_count = %d\n", *thread_count_p);
#  endif
}  /* Get_args */


/*---------------------------------------------------------------------
 * Function:  Gen_data
 * Purpose:   Generate random floats in the range min_meas <= x < max_meas
 * In args:   min_meas:    the minimum possible value for the data
 *            max_meas:    the maximum possible value for the data
 *            data_count:  the number of measurements
 * Out arg:   data:        the actual measurements
 */
void Gen_data(
        float   min_meas    /* in  */, 
        float   max_meas    /* in  */, 
        float   data[]      /* out */,
        int     data_count  /* in  */) {
   int i;

   srandom(0);
   for (i = 0; i < data_count; i++)
      data[i] = min_meas + (max_meas - min_meas)*random()/((double) RAND_MAX);

#  ifdef DEBUG
   printf("data = ");
   for (i = 0; i < data_count; i++)
      printf("%4.3f ", data[i]);
   printf("\n");
#  endif
}  /* Gen_data */


/*---------------------------------------------------------------------
 * Function:  Gen_bins
 * Purpose:   Compute max value for each bin, and store 0 as the
 *            number of values in each bin
 * In args:   min_meas:   the minimum possible measurement
 *            max_meas:   the maximum possible measurement
 *            bin_count:  the number of bins
 * Out args:  bin_maxes:  the maximum possible value for each bin
 *            bin_counts: the number of data values in each bin
 */
void Gen_bins(
      float min_meas      /* in  */, 
      float max_meas      /* in  */, 
      float bin_maxes[]   /* out */, 
      int   bin_counts[]  /* out */, 
      int   bin_count     /* in  */) {
   float bin_width;
   int   i;

   bin_width = (max_meas - min_meas)/bin_count;

   for (i = 0; i < bin_count; i++) {
      bin_maxes[i] = min_meas + (i+1)*bin_width;
      bin_counts[i] = 0;
   }

#  ifdef DEBUG
   printf("bin_maxes = ");
   for (i = 0; i < bin_count; i++)
      printf("%4.3f ", bin_maxes[i]);
   printf("\n");
#  endif
}  /* Gen_bins */


/*---------------------------------------------------------------------
 * Function:  Which_bin
 * Purpose:   Use binary search to determine which bin a measurement 
 *            belongs to
 * In args:   data:       the current measurement
 *            bin_maxes:  list of max bin values
 *            bin_count:  number of bins
 *            min_meas:   the minimum possible measurement
 * Return:    the number of the bin to which data belongs
 * Notes:      
 * 1.  The bin to which data belongs satisfies
 *
 *            bin_maxes[i-1] <= data < bin_maxes[i] 
 *
 *     where, bin_maxes[-1] = min_meas
 * 2.  If the search fails, the function prints a message and exits
 */
int Which_bin(
      float   data          /* in */, 
      float   bin_maxes[]   /* in */, 
      int     bin_count     /* in */, 
      float   min_meas      /* in */) {
   int bottom = 0, top =  bin_count-1;
   int mid;
   float bin_max, bin_min;

   while (bottom <= top) {
      mid = (bottom + top)/2;
      bin_max = bin_maxes[mid];
      bin_min = (mid == 0) ? min_meas: bin_maxes[mid-1];
      if (data >= bin_max) 
         bottom = mid+1;
      else if (data < bin_min)
         top = mid-1;
      else
         return mid;
   }

   /* Whoops! */
   fprintf(stderr, "Data = %f doesn't belong to a bin!\n", data);
   fprintf(stderr, "Quitting\n");
   exit(-1);
}  /* Which_bin */


/*---------------------------------------------------------------------
 * Function:  Print_histo
 * Purpose:   Print a histogram.  The number of elements in each
 *            bin is shown by an array of X's.
 * In args:   bin_maxes:   the max value for each bin
 *            bin_counts:  the number of elements in each bin
 *            bin_count:   the number of bins
 *            min_meas:    the minimum possible measurment
 */
void Print_histo(
        float  bin_maxes[]   /* in */, 
        int    bin_counts[]  /* in */, 
        int    bin_count     /* in */, 
        float  min_meas      /* in */) {
   int i, j;
   float bin_max, bin_min;

   for (i = 0; i < bin_count; i++) {
      bin_max = bin_maxes[i];
      bin_min = (i == 0) ? min_meas: bin_maxes[i-1];
      printf("%.3f-%.3f:\t", bin_min, bin_max);
      for (j = 0; j < bin_counts[i]; j++)
         printf("X");
      printf("\n");
   }
}  /* Print_histo */

/**
 * When running a multithreaded implementation
 * of histogram.c, each thread has to have its
 * own routine is uses to do work.  This is that
 * routine.  There are two basic parts to it--
 * the first part takes a chunk of the data and
 * categorizes it into (local versions of) bins.
 * The second part takes the local bins and 
 * consolidates them into global bins.
 *
 * Overall, the complexity of this function
 * is O(n/t + b) where n is the size of the 
 * data, t is the number of threads, and b
 * is the number of bins.
 */
void* thread_do_work(void* thread_data)
{
   //set up data
   int id = (int)(intptr_t) thread_data;
   int num_elements = floor(data_count / thread_count);
   if(id == thread_count-1)//if this is the "last" thread
   {
      num_elements = floor(data_count/thread_count)+(data_count%thread_count);
      //have it take the remainder of the elements too
      //a sloppy solution but it works
   }

   //find out start and stop points for each section of the overall data array
   int start_n, stop_n;
   start_n = floor(data_count/thread_count)*id;
   stop_n = start_n + num_elements; //stop just short of next partition

   //find out what bins they go in and put them there
   int k, bin;
   for(k = start_n; k < stop_n; k++)
   {
       bin = Which_bin(data[k], bin_maxes, bin_count, min_meas);
       //the offset to the "individual" bin should be (id*bin_count)
       //further offset in the bin should just be the bin number
       local_bins[(id*bin_count)+bin]++; //make sure it writes to correct bin
   }

   /* Barrier, wait for other threads to catch up */
   pthread_mutex_lock(&mutex);
   counter++;
   if(counter == thread_count) //if this is the last thread to join the wait
   {
      counter = 0;
      pthread_cond_broadcast(&cond_var); //wake everyone up
   }
   else 
   {
      while(pthread_cond_wait(&cond_var, &mutex) != 0); //wait until condition
   }
   pthread_mutex_unlock(&mutex); //release the lock after waking and acquiring


   //When consolidating bins, we may run into a problem.  What if there are
   //more threads than bins? bins than threads?
   int sum;
   int m;
   if(bin_count > thread_count) //more bins than threads
   {
      int bins_per_thread = floor(bin_count/thread_count);
      if (id < (bin_count % thread_count)) 
      {
         //if there is a bin left over, the "early" threads take one
         bins_per_thread++;
      }
      //outer loop is to make sure it gets through all of 
      //its bin responsibilities
      // quantity "m" is the new "moving" thread id
      for(m = id; m < bin_count; m = m+thread_count)
      {
         sum = 0;
         //now loop through the bins
         //there are always <thread_count> number of bins
         //bin_count is the "size" of a set of bins
         for(k = 0; k < thread_count; k++)
         {
            sum+=local_bins[m+(k*bin_count)];
         }
         //give the final count
         bin_counts[m] = sum;
      }
   }
   else
   {
      //only need to do work if there's work that needs to be done
      if(id < bin_count)
      {
         sum = 0;
         for(k = 0; k < thread_count; k++)
         {
            sum+=local_bins[id+(k*bin_count)];
         }
         //give the final count
         bin_counts[id] = sum;
      }
      //else, nothing to work on cause all the other threads
      //are already on it
   }
   return NULL;
}