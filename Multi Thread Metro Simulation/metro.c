#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "queue.c"
#include <unistd.h>  
#include <string.h>


int speed = 100;
int tunnel_length =100;
time_t sim_start_time = 0;
int train_id = 0;

int sim_time = 0; 
float p = 0.0;   

void *A_SectionThread(void *arg);
void *B_SectionThread(void *arg);
void *E_SectionThread(void *arg);
void *F_SectionThread(void *arg);

void *MetroControlThread(void *arg);

void WriteLog(const char *Event, const char *Event_time, const char *Train_ID, const char *Trains_Waiting);
void WriteTrainLog(const char *Train_ID, const char *StartingPoint, const char *DestinationPoint, const char *Length, const char *ArrivalTime, const char *DepartureTime );



Queue *TunnelQueue;

//creating mutexes for the queue and the log file
pthread_mutex_t TunnelQueueMutex;
pthread_mutex_t logFileMutex;



//I used this function to sleep the threads safely
int pthread_sleep(int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }
    struct timeval tp;
    
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds;
    timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    
    return res;
}


int main(int argc, char *argv[]){

    
    for (int i = 1; i < argc; i++) {

        //simulation time as input -s
        if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) { 
                sim_time = atoi(argv[++i]);
            } 
            else {
                
                return 1;
            }

            //probability as input -p
        } 
        else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                p = atof(argv[++i]);
            } 
            else {
                return 1;
            }
        }
    }

    //printf("probability %f\n", p);
    //printf("sim time %d\n", sim_time);

    //initiate simulation start time
    sim_start_time = time(NULL);

    //headers for control center log file
    FILE *logFile = fopen("control_center_log.txt", "w");
    fprintf(logFile, "Event, Event Time, Train ID, Trains Waiting Passage\n");
    fclose(logFile);

    //headers for train log file
    FILE *TrainlogFile = fopen("Train_log.txt", "w");
    fprintf(TrainlogFile, "Train ID, Starting Point, Destination Point, Length(m), Departure Time, Arrival Time \n");
    fclose(TrainlogFile);

    //build the queue
    TunnelQueue = ConstructQueue(1000);

    //mutex initilaze
    pthread_mutex_init(&TunnelQueueMutex, NULL);
    pthread_mutex_init(&logFileMutex, NULL);

    //declaring threads
    pthread_t A_Thread;
    pthread_t B_Thread;
    pthread_t E_Thread;
    pthread_t F_Thread;
    pthread_t Metro_Control_Thread;

    //create threads
    pthread_create(&A_Thread, NULL, A_SectionThread, NULL);
    pthread_create(&B_Thread, NULL, B_SectionThread, NULL);
    pthread_create(&E_Thread, NULL, E_SectionThread, NULL);
    pthread_create(&F_Thread, NULL, F_SectionThread, NULL);    
    pthread_create(&Metro_Control_Thread, NULL, MetroControlThread, NULL); 


    //joining threads
    pthread_join(A_Thread, NULL);
    pthread_join(B_Thread, NULL);
    pthread_join(E_Thread, NULL);
    pthread_join(F_Thread, NULL);
    pthread_join(Metro_Control_Thread, NULL);


    return 0;
}

//Generating trains from point A
void *A_SectionThread(void *arg){

        float rand_train = (float)rand() / RAND_MAX;
        float rand_length = (float)rand() / RAND_MAX;
        float rand_line = (float)rand() / RAND_MAX;
        int train_length = 0; 
        

        while(time(NULL) < sim_start_time + sim_time){
            
            rand_train = (float)rand() / RAND_MAX;

            if(rand_train <= p){
                pthread_sleep(1);
                //LOCK
                pthread_mutex_lock(&TunnelQueueMutex);
                Train t;
                t.ID = train_id;
                train_id = train_id +1;
                t.starting_point ='A';
                t.arrivalTime = time(NULL);
                

                rand_length = (float)rand() / RAND_MAX;
                if(rand_length <= 0.7){
                    t.length = 100;
                }
                else{
                    t.length = 200 ;
                }

                //deciding destination
                rand_line = (float)rand() / RAND_MAX;
                if (rand_line <= 0.5){
                    t.destination_point ='E';

                }
                else{
                    t.destination_point ='F';
                }
                
                //add train to the queue after defining it's parameters
                Enqueue(TunnelQueue, t);

                pthread_mutex_unlock(&TunnelQueueMutex);

                //////UNLOCK            
                
                
            }
            
            //if train is not generated try 1 second later
            else{
            
            pthread_sleep(1);
        }
    }

 
}

//Generating trains from point B
void *B_SectionThread(void *arg){
        float rand_train = (float)rand() / RAND_MAX;
        float rand_length = (float)rand() / RAND_MAX;
        float rand_line = (float)rand() / RAND_MAX;
        int train_length = 0; 
      

        while(time(NULL) < sim_start_time + sim_time){
        rand_train = (float)rand() / RAND_MAX;
        if(rand_train <= (1-p)){
            pthread_sleep(1);
            //LOCK
            pthread_mutex_lock(&TunnelQueueMutex);
            Train t;
            t.ID = train_id;
            train_id = train_id +1;
            t.starting_point ='B';
            t.arrivalTime = time(NULL);
            

            rand_length = (float)rand() / RAND_MAX;
            if(rand_length <= 0.7){
                t.length = 100;
            }
            else{
                t.length = 200 ;
            }
            rand_line = (float)rand() / RAND_MAX;
            if (rand_line <= 0.5){
                t.destination_point ='E';

            }
            else{
                t.destination_point ='F';
            }


            Enqueue(TunnelQueue, t);

            pthread_mutex_unlock(&TunnelQueueMutex);

            //////UNLOCK          
            
        }
        

        else{
            
            pthread_sleep(1);
        }
    }

}

//Generating trains from point E
void *E_SectionThread(void *arg){
        float rand_train = (float)rand() / RAND_MAX;
        float rand_length = (float)rand() / RAND_MAX;
        float rand_line = (float)rand() / RAND_MAX;
        int train_length = 0; 
       



        while(time(NULL) < sim_start_time + sim_time){
        rand_train = (float)rand() / RAND_MAX;

        
        if(rand_train <= p){
            pthread_sleep(1);
            pthread_mutex_lock(&TunnelQueueMutex);
            Train t;
            t.ID = train_id;
            train_id = train_id +1;
            t.starting_point ='E';
            
            t.arrivalTime = time(NULL);
            

            rand_length = (float)rand() / RAND_MAX;
            if(rand_length <= 0.7){
                t.length = 100;
            }
            else{
                t.length = 200 ;
            }
            rand_line = (float)rand() / RAND_MAX;
            if (rand_line <= 0.5){
                t.destination_point ='A';

            }
            else{
                t.destination_point ='B';
            }
            

            Enqueue(TunnelQueue, t);

            pthread_mutex_unlock(&TunnelQueueMutex);

            //////UNLOCK
            
            
            
        }
        

         else{

            pthread_sleep(1);
        }
    }


}

//Generating trains from point F
void *F_SectionThread(void *arg){
        float rand_train = (float)rand() / RAND_MAX;
        float rand_length = (float)rand() / RAND_MAX;
        float rand_line = (float)rand() / RAND_MAX;
        int train_length = 0; 



        while(time(NULL) < sim_start_time + sim_time){
        rand_train = (float)rand() / RAND_MAX;
        if(rand_train <= p){
            pthread_sleep(1);
            //LOCK
            pthread_mutex_lock(&TunnelQueueMutex);
            Train t;
            t.ID = train_id;
            train_id = train_id +1;
            t.starting_point ='F';
            t.arrivalTime = time(NULL);
            

            rand_length = (float)rand() / RAND_MAX;
            if(rand_length <= 0.7){
                t.length = 100;
            }
            else{
                t.length = 200 ;
            }
            rand_line = (float)rand() / RAND_MAX;

             if (rand_line <= 0.5){
                t.destination_point ='A';

            }
            else{
                t.destination_point ='B';
            }          


            

            Enqueue(TunnelQueue, t);

            pthread_mutex_unlock(&TunnelQueueMutex);

            //////UNLOCK        
            
                

        }
        else{
            pthread_sleep(1);
        }
        
    }

    
}

//controling the tunnel
void *MetroControlThread(void *arg){


    while(time(NULL) < sim_start_time + sim_time){ 


            pthread_mutex_lock(&TunnelQueueMutex);
            int queueSize = QueueSize(TunnelQueue);
            pthread_mutex_unlock(&TunnelQueueMutex);

            //part2 if there is more than 10 train in the queue don't accept newcomers finish the queue
            if(queueSize >= 10){
                
                char event[50], event_time[20], c[20];
                time_t now;
                time(&now);
                struct tm *local = localtime(&now);
                snprintf(event_time, sizeof(event_time), "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);
                strcpy(event, "System Overload");
                strcpy(c, "#");
                char* queue_id = QueueIDsToString(TunnelQueue);

                pthread_mutex_lock(&logFileMutex);
                WriteLog(event,event_time,c,queue_id);
                
                pthread_mutex_unlock(&logFileMutex);


                pthread_mutex_lock(&TunnelQueueMutex);

                //don't accept any new train until tunnel is cleared
                while(queueSize != 0){
                    //printf("Queue Size: %d\n", QueueSize(TunnelQueue));
                    char event[50], event_time[20], train_id[20], start_point[20], destination_pointt[20], train_len[20],departure_time_str[40] ,arrival_time_str[40];
                    time_t now;
                    time_t arrival_time;
                    time_t departure_time;
                    time(&now);

                    //defining event time and event
                    struct tm *local = localtime(&now);
                    snprintf(event_time, sizeof(event_time), "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);
                    strcpy(event, "Tunnel Passing");

                    Train train;
                    //finding which starting point is more occured in the queue
                    char delete_item = MostCommonStartingPoint(TunnelQueue);

                    //dequeue the element which has most priority (first element of most occured starting point)
                    train = DequeueByStartingPoint(TunnelQueue, delete_item);
                    queueSize = QueueSize(TunnelQueue);

                    //sleep time is the time that train takes to pass the tunnel
                    int sleep_time = ((train.length)/ 100) +1 ;

                    // after pass the tunnel it takes 1 second to arrive
                    arrival_time = now + (sleep_time +1);
                    struct tm *arrival_local = localtime(&arrival_time);
                    snprintf(arrival_time_str, sizeof(arrival_time_str), "%02d:%02d:%02d", arrival_local->tm_hour, arrival_local->tm_min, arrival_local->tm_sec);

                    //train.arrivalTime is arrival time to the tunnel queue that means it generated 1 second before that time
                    departure_time = train.arrivalTime -1;
                    struct tm *departure_local = localtime(&departure_time);
                    snprintf(departure_time_str, sizeof(departure_time_str), "%02d:%02d:%02d", departure_local->tm_hour, departure_local->tm_min, departure_local->tm_sec);

                    snprintf(train_id, sizeof(train_id), "%d", train.ID);
                    char* queue_id = QueueIDsToString(TunnelQueue);

                    snprintf(start_point, sizeof(start_point), "%c", train.starting_point);
                    snprintf(destination_pointt, sizeof(destination_pointt), "%c", train.destination_point);
                    snprintf(train_len, sizeof(train_len), "%d", train.length);


                    // write to the log
                    pthread_mutex_lock(&logFileMutex);
                    WriteLog(event,event_time,train_id,queue_id);
                    WriteTrainLog(train_id, start_point, destination_pointt, train_len, departure_time_str, arrival_time_str);
                    pthread_mutex_unlock(&logFileMutex);
                    //slep until tunnel is available
                    pthread_sleep(sleep_time); 

                    float breakdown = (float)rand() / RAND_MAX;
                // part 3 breakdown probability by 0.1
                if(breakdown <=0.1){

                    char event[50], event_time[20];
                    strcpy(event, "Breakdown");
                    time_t now;
                    time(&now);

                    
                    struct tm *local = localtime(&now);
                    snprintf(event_time, sizeof(event_time), "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);


                    pthread_mutex_lock(&logFileMutex);
                    WriteLog(event,event_time,train_id,queue_id);
                    
                    pthread_mutex_unlock(&logFileMutex);

                    pthread_sleep(4);
  
                
                }
                    

                //After tunnel cleared
                }
                 pthread_mutex_lock(&logFileMutex);
                char event2[50], event_time2[20], g[20];
                
                time_t now2;
                time(&now2);
                struct tm *local2 = localtime(&now2);
                snprintf(event_time2, sizeof(event_time2), "%02d:%02d:%02d", local2->tm_hour, local2->tm_min, local2->tm_sec);
                strcpy(event2, "Tunnel Cleared");
                strcpy(g, "#");
                char differenceString[50];
                int differenceInSeconds = difftime(now2, now);
                snprintf(differenceString, sizeof(differenceString), "#Time to clear: %d secs", differenceInSeconds);

               
                WriteLog(event2,event_time2,g,differenceString);
                
                pthread_mutex_unlock(&logFileMutex);
                pthread_mutex_unlock(&TunnelQueueMutex);
            }

            //if not system overload
            else{
     
            //if queue is empty sleep for 1 seconds
            if(isEmpty(TunnelQueue) == 1){

                pthread_sleep(1); 

            }
            
            else{

            int sleep_time = 1;
            char event[50], event_time[20], train_id[20], strat_point[20], destination_point[20], train_len[20], departure_time_str[40], arrival_time_str[40];
            time_t now;
            time_t arrival_time;
            time_t departure_time;
            time(&now);

            
            struct tm *local = localtime(&now);
            snprintf(event_time, sizeof(event_time), "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);
            strcpy(event, "Tunnel Passing");
            
            pthread_mutex_lock(&TunnelQueueMutex);

            Train train;
            char delete_item = MostCommonStartingPoint(TunnelQueue);
            
            train = DequeueByStartingPoint(TunnelQueue, delete_item);
            sleep_time = ((train.length)/ 100) +1;

            arrival_time = now + (sleep_time +1);
            struct tm *arrival_local = localtime(&arrival_time);
            snprintf(arrival_time_str, sizeof(arrival_time_str), "%02d:%02d:%02d", arrival_local->tm_hour, arrival_local->tm_min, arrival_local->tm_sec);

            departure_time = train.arrivalTime -1;
            struct tm *departure_local = localtime(&departure_time);
            snprintf(departure_time_str, sizeof(departure_time_str), "%02d:%02d:%02d", departure_local->tm_hour, departure_local->tm_min, departure_local->tm_sec);

  

            snprintf(train_id, sizeof(train_id), "%d", train.ID);
            char* d = QueueIDsToString(TunnelQueue);
            

            snprintf(strat_point, sizeof(strat_point), "%c", train.starting_point);
            snprintf(destination_point, sizeof(destination_point), "%c", train.destination_point);
            snprintf(train_len, sizeof(train_len), "%d", train.length);
             
            
            pthread_mutex_lock(&logFileMutex);
            WriteLog(event,event_time,train_id,d);
            WriteTrainLog(train_id, strat_point, destination_point, train_len, departure_time_str, arrival_time_str);
            pthread_mutex_unlock(&logFileMutex);
            

            pthread_mutex_unlock(&TunnelQueueMutex);

            
            //printf("Sleep time %d\n", sleep_time);

            pthread_sleep(sleep_time);

            float breakdown = (float)rand() / RAND_MAX;

            //part 3 if breakdown happens sleep for 4 seconds
            if(breakdown <=0.1){

                char event2[50], time2[20];
                strcpy(event2, "Breakdown");
                time_t now;
                time(&now);
                
                struct tm *local = localtime(&now);
                snprintf(time2, sizeof(time2), "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);


                pthread_mutex_lock(&logFileMutex);
                WriteLog(event2,time2,train_id,d);
           
                pthread_mutex_unlock(&logFileMutex);

                pthread_sleep(4);
  
            
            }
        }
    }


    }
    

    return 0;
}

//write function for control center log
void WriteLog(const char *Event, const char *Event_time, const char *Train_ID, const char *Trains_Waiting) {
    FILE *file = fopen("control_center_log.txt", "a"); 
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "%s, %s, %s, (%s)\n", Event, Event_time, Train_ID, Trains_Waiting);

    
    fclose(file);
}

//write function for train log
void WriteTrainLog(const char *Train_ID, const char *StartingPoint, const char *DestinationPoint, const char *Length, const char *ArrivalTime, const char *DepartureTime ) {
    FILE *file = fopen("Train_log.txt", "a"); 
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "%s, %s, %s, %s, %s, %s\n", Train_ID, StartingPoint, DestinationPoint, Length, ArrivalTime, DepartureTime);

    fclose(file);
}