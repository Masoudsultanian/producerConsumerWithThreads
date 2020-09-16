/***********************************************************************************************************
Write a C program taking as input a text file (as command line parameter) containing several lines, 
each one describing the production of an Item in terms of time and amount: 
<product_name> <type> <number_of_items_ to_build> <building time_per_item> 
Type identifies the thread able to produce the item and it must be a number from 1 to N/where N. is the 
number of different thread "builders" that the program accepts (provided as command line parameter too). 
Each type and/or product name can appear in several lines. The program must have one main thread àccessing 
the file, reading one line each 2 seconds and dispatching the request to build the items to the selected 
thread(by the type number). A second thread (named collectors deputed to collect all items built by each 
single thread and, once all productions ended, it writes the production result into a new file (provided 
as command line parameter), where each line represents a single <type> with its total amount of items built. 
Eventually, all builders must behave as following: a. The builder waits for the main thread to notify the 
product to be built, together with the number of items and the building time; b. It waits for a time equal
to <number_of_items_to_build>*<building_time_per_item> to simulate the building process. C. It sends to the 
collector the end building notification, then it goes back to a. Once all builders end, and the collector 
saves the results, the main thread must terminate every remaining thread.
************************************************************************************************************/
/*Author: Masoud Soltanian. This program works fine without any particular problem :)
************************************************************************************************************/
#include <pthread.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#define BUFFERSIZE 5


pthread_mutex_t mutex;
pthread_mutex_t mutexCol;
sem_t *sem_builders; 
sem_t sem_collector;
sem_t full;
sem_t empty;
int position =0 ;

int inIndex=0;  //it computes at every step how many lines we read from file, so finaly its size of file
int  collector_type;
int  collector_time;

int N; //number of builders
int finish=-1;

//Structure of items in input file
typedef struct {
	char product_name[20];
	int type;
	int num_item;
	int time_item;

}info;

int bufferItems[BUFFERSIZE][3]; //to save item type, numbre and time on it

void cleaner(void* arg){

	pthread_mutex_unlock(&mutex);
	pthread_mutex_unlock(&mutexCol);
}


void* builders_thread(void* arg) {

	int type= *(int*) arg;
	int waitTime;
	int out=0;
	int i;
	

	pthread_cleanup_push(cleaner,NULL);
	while (1) {        //.....................................steps must be reapeted in builders
	
	sem_wait(&sem_builders[type-1]);
 
	sem_wait(&full);
	pthread_mutex_lock(&mutex);
	if (out<position){
		for (i=out;i<position;i++){
			if (bufferItems[i][0]==type) { //it means information is mine
				//take item
				waitTime=bufferItems[i][1] * bufferItems[i][2]; //num_item_build * time_item_build;
				//update pointer to read from here next time
				out=i;
				out=(out+1) % BUFFERSIZE;
				break;
			}
		}
	}else
	{
	
		for (i=out;i<BUFFERSIZE;i++){
			if (bufferItems[i][0]==type) { //it means information is mine
				//take item
				waitTime=bufferItems[i][1] * bufferItems[i][2]; //num_item_build * time_item_build;
				//update pointer to read from here next time
				out=i;
				out=(out+1) % BUFFERSIZE;
				break;
			}
		}
		for (i=0;i<out;i++){
			if (bufferItems[i][0]==type) { //it means information is mine
				//take item
				waitTime=bufferItems[i][1] * bufferItems[i][2]; //num_item_build * time_item_build;
				//update pointer to read from here next time
				out=i;
				out=(out+1) % BUFFERSIZE;
				break;
			}
		}


	}
	pthread_mutex_unlock(&mutex);
	sem_post(&empty);
	//consume item
	printf("(Builder=%d) BUILDING item=%d in %d seconds.\n",type,type,waitTime);
	sleep(waitTime); //...........................................................wait for computed time to build item
	
	//This information is for collector
	pthread_mutex_lock(&mutexCol);
	collector_time=waitTime;
	collector_type=type;
	printf("(Builder=%d) FINISHES its job after=%d second and send signal to collector.\n",type,waitTime);
	sem_post(&sem_collector);
	pthread_mutex_unlock(&mutexCol);

	}
	//printf("builder((%d)) exited,\n", type);
	pthread_cleanup_pop(0);
	pthread_exit(NULL);
	return NULL;
}


void* collector_thread(void* arg) {
	FILE *out; //only collector can write into the file;
	char filename[30];
	int total_price[N];
	int i=0;
	int count=0;

	while(1){
		sem_wait(&sem_collector);
		pthread_mutex_lock(&mutex); 
		//inIndex is shared variable to have last update we read it through mutex
		if (count < inIndex){
		pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutexCol);
			count++;
			printf("(Collector) COLLECT item=%d with building time=%d, then it is computing total needed time.\n",collector_type,collector_time);
			pthread_mutex_lock(&mutex);
			printf("..............................................................(read items from main file) =%d\n",inIndex);
			pthread_mutex_unlock(&mutex);	
			printf("..............................................................(delivered items to collector)=%d\n",count);
			total_price[collector_type-1]=total_price[collector_type-1]+collector_time;
			pthread_mutex_unlock(&mutexCol);
		} 
		pthread_mutex_lock(&mutex);
		if ((count==inIndex) && (finish==1)) break;
		pthread_mutex_unlock(&mutex);
	}
	
	printf("Congratulations, all productions has been built correctly.\n");
	printf("All the productions ended and are SAVED in output file.\n");//..............................................all production ended
	//all productions ended
	strcpy(filename,(char *)arg);  //.............................................output file provided as command line
	if ((out=fopen(filename,"w"))== NULL)
		{
		fprintf(stderr,"error in creation file.\n");
		return NULL;
		}
	for (i=0;i<N;i++)
		fprintf(out,"Item=%d total amount of needed time to build=%d \n",i+1,total_price[i]); //..........................................saved in file
	fclose(out);
	pthread_exit(NULL);
	return NULL;
}




int main(int argc, char *argv[])
{    
	info item_info;  //to read info of each line
	pthread_t thCollector;	//collector thread
	pthread_t *thBuilders; //we have many builders
	int *buildersType;	//it specifies which builder we are
	int i; //loop counter
	FILE *in;  //only main thread read from inpute file


	
	//initialization 
	pthread_mutex_init(&mutex,NULL);
	pthread_mutex_init(&mutexCol, NULL);
	sem_init(&sem_collector,0,0);
	sem_init(&full,0,0);
	sem_init(&empty,0,BUFFERSIZE);
	
	//check number of parameters in command line
	if(argc!=4){
		fprintf(stderr,"Wrong number of arguments");
		return -1;
	}
	
	
	//assign and parsing N which is readed from command line
	sscanf(argv[3], "%d", &N); //............................,,.........................number of builders reading from commandline
	
	//create threads
	thBuilders=(pthread_t *)malloc(N*sizeof(pthread_t)); //.....................
	buildersType=(int *)malloc(N*sizeof(int));
	sem_builders=(sem_t *)malloc(N*sizeof(sem_t));
	for(i=0;i<N;i++){
		sem_init(&sem_builders[i],0,0); //we need one semaphore for each builder
		buildersType[i]=i+1;
		if (pthread_create(&thBuilders[i],NULL,builders_thread,&buildersType[i]))
		{
             		fprintf(stderr,"Error in creating builders \n");
            		 return -2;
        	}
	}
   	if (pthread_create(&thCollector,NULL,collector_thread,argv[2])){
             fprintf(stderr,"Error in creating collector thread \n");
             return -3;
        }
    
	//What we have to do in main thread
	if ((in=fopen(argv[1],"r"))== NULL)
		{
		fprintf(stderr,"error in creation file.\n");
		return -5;
		}
	else while (fscanf(in,"%s %d %d %d",item_info.product_name,&item_info.type,&item_info.num_item,&item_info.time_item) != EOF)//................read file,produce item
		{
		//size of buffer has not been mention so we do not need full and empty
		sem_wait(&empty);
		pthread_mutex_lock(&mutex); 
		//append info to buffer
		bufferItems[position][0]=item_info.type;
		bufferItems[position][1]=item_info.num_item;
		bufferItems[position][2]=item_info.time_item;
		printf("(Main) READS infos of item=%d with num=%d and needed time=%d,and delivered to builder=%d.\n",bufferItems[position][0],bufferItems[position][1],bufferItems[position][2],bufferItems[position][0]);
		position=(position+1)%BUFFERSIZE;
		inIndex++; //it is total number of item that has been read unti now
		sem_post(&sem_builders[(item_info.type)-1]);  //index start from zero
		pthread_mutex_unlock(&mutex);
		sem_post(&full);
		sleep(2); //..............................................................................................read one line every 2 seconds
	     }

	// it means we reached end of file
	pthread_mutex_lock(&mutex);
	finish=1;
	pthread_mutex_unlock(&mutex);
	

	
	
	

	//check to terminate all threads to avoid having Zambie
	pthread_join(thCollector,NULL);
	printf("Collector threaded ended.\n");
	for(i=0;i<N;i++)
	          if( (pthread_cancel(thBuilders[i]))!=0) 
			printf("Error in cancelation thread %dth\n",i+1); 
		 else printf("Main thread terminated builder=%d\n",i+1);
        //...............................................................................main thread terminates all remaining thread
	for (i=0;i<N;i++)
		pthread_join(thBuilders[i],NULL); //it waits to terminate all builders
	
	/* Clean up and exit */
	fclose(in);
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutexCol);
	sem_destroy(&full);
	sem_destroy(&empty);
	free(thBuilders);
	free(buildersType);
	printf("Clean up and exit.\n");
	printf("Hope for the best.\n");
	pthread_exit(NULL);
	return 0;
}

