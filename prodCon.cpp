/**
 * Program does the classic Sleeping TA example. Altered to meet project
 * expectations to include specific reports when students visit the TA.
 *
 * @author Jacob McFadden
 *
 * Some research notes I learned while working on this for future me:
 * - cout is not thread safe, printf is
 * - When possible try to keep information thread local unless necessary
 * - Make sure to initialize your semaphores, otherwise they don't work
 * - srand() is thread local - meaning calling it from main doesn't help,
 * had to call it at top of thread function for actual random numbers to
 * generate
 * - rand() is denoted to not be thread-safe
 * - a new line character is necessary for a thread to actual print the output
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
/**
 * help with pthread / semaphores:
 * https://www.bogotobogo.com/cplusplus/multithreading_pthread.php
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

using namespace std;

//Input data storage
struct student {
    int id;
    int firstTimeArrival;
    int timeNeeded;
};

//Constants
//Changing this will speed up the program - report still reflects system time, stats should be accurate though as if it wasn't sped up
#define MINUTE_AS_SECONDS 60
#define NUM_WR_CHAIRS 3
#define WAIT_FACTOR 10

//Global Variables
pthread_mutex_t accessLock;
sem_t taSemaphore, studentSemaphore, waitingRoomSemaphore;

vector<student> students; //Will hold student data
int numStudents;
int waitingRoomStudents[NUM_WR_CHAIRS]; //ID's which student is currently in the waiting room.
int waitroomChairsOccupied = 0;
/**
 * Works kind of like a list where we increment these by 1
 * as they are used, then modulate back to 0 based on number of
 * chairs in the waiting room.
 */
int nextChairAvailable = 0;
int nextStudentToHelp = 0;

//Statistic Data
/**
 * Wait time is defined by the moment a student sits in the wait room chair
 * to the moment the TA helps them. Coming back at later time does not count as wait
 * time, however if it did, some minor adjustments can be made to include a student
 * leaving and coming back later in addition to waiting in a chair.
 */
double totalStudentWaitTime = 0.0;
double taBusyPercent = 0.0;
int totalArrivals = 0;
int totalNoChairLeaves = 0;

//Functions
void parseInput(vector<student>& inputList, std::istream& originalInput);
void* studentBehavior(void* threadID);
void* taBehavior(void* arg);

int main(int argc, char** argv)
{
    //Input Validation
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " input_file_name\n";
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile) {
        cout << "ERROR: " << argv[1] << " could not be opened" << "\n";
        return 2;
    }
    //End of validation

    //Setup
    parseInput(students, inputFile);
    numStudents = (int)students.size();
    pthread_t studentThreads[numStudents];
    pthread_t taThread;
    int rc, i; //Error check / loop

    //Initialization
    rc = pthread_mutex_init(&accessLock, NULL); //Provide default attributes
    if(rc) {
        cerr << "ERROR: return code from pthread_mutex_init() is " << rc << endl;
        exit(-1);
    }
    rc = sem_init(&taSemaphore, 0, 0);
    if(rc) {
        cerr << "ERROR: return code from sem_init(taSemaphore) is " << rc << endl;
        exit(-1);
    }
	rc = sem_init(&studentSemaphore, 0, 0);
    if(rc) {
        cerr << "ERROR: return code from sem_init(studentSemaphore) is " << rc << endl;
        exit(-1);
    }
    rc = sem_init(&waitingRoomSemaphore, 0, 0);
    if(rc) {
        cerr << "ERROR: return code from sem_init(studentSemaphore) is " << rc << endl;
        exit(-1);
    }

    //Thread creation
    rc = pthread_create(&taThread, NULL, taBehavior, NULL);
    if(rc) {
        cerr << "ERROR: return code from pthread_create() is " << rc << endl;
        exit(-1);
    }
    for(i = 0; i < numStudents; i++) {
        rc = pthread_create(&studentThreads[i], NULL, studentBehavior, (void*)&i);
        if(rc) {
            cerr << "ERROR: return code from pthread_create() is " << rc << endl;
            exit(-1);
        }
    }
    //Join threads - ensures parallel run
    pthread_join(taThread, NULL);
    for(i=0; i< numStudents; i++) {
        pthread_join(studentThreads[i],NULL);
    }
    //Determine statistics - if necessary
    double avgStudentWait = totalStudentWaitTime / (1.0 * numStudents);
    double avgComeback = (1.0 * totalNoChairLeaves) / (1.0 * totalArrivals);
    //Print final statistics
    printf("Statistics:\n");
    printf("%% time the TA was busy helping students: %f%%\n", taBusyPercent);
    printf("Average time a student had to wait to get help: %f minutes\n", (avgStudentWait/MINUTE_AS_SECONDS));
    printf("Average number of times a student had to come back for help (due to lack of chairs): %f times\n", avgComeback);

    return 0;
}

/**
 * Parse arguments when input is done by line in the following order:
 * Student ID, First Arrival Time, Time needed from TA
 *
 * @param inputVector is the where the process data is stored
 * @param originalInput is the data to be parsed through and stored
 *
 * @pre assumes that times are in minutes and are integers
 */
void parseInput(vector<student>& inputList, std::istream& originalInput) {
    std::string line;
    while(getline(originalInput,line)) {
        std::istringstream input(line);
        student temp;
        std::string num;

        input >> num;
        temp.id = std::stoi(num);

        input >> num;
        temp.firstTimeArrival = std::stoi(num);

        input >> num;
        temp.timeNeeded = std::stoi(num);

        inputList.push_back(temp);
    }
}

/**
 * Behavior for the student: sleeps for start time, goes to TA,
 * waits for TA or comes back later if waiting room is full. Leaves thread
 * when finished at TA.
 *
 * @param threadID is studentID being passed in to associate this thread with a certain student
 *
 * @return a null when the thread exits when done
 */
void* studentBehavior(void* threadID) {
    //Setup
    int threadNum = *(int*)threadID-1;

    pthread_mutex_lock(&accessLock);
    student threadStudent = students[threadNum];
    srand(time(NULL)); //Seed
    pthread_mutex_unlock(&accessLock);

    //Sleep thread until first arrival
    int sleepTime = threadStudent.firstTimeArrival * MINUTE_AS_SECONDS;
    int numArrivals = 0;
    int numNoChairLeave = 0;
    double studentWaitTime = 0.0;
    time_t startWait, endWait, arriveTime, leaveTime;
    string arriveTimeString, leaveTimeString;

    //Actual behavior
    while(true) {
        //Sleep until time to arrive at TA's office
        sleep(sleepTime);
        time(&arriveTime);
        arriveTimeString = asctime(localtime(&arriveTime));
        numArrivals++;

        //Student Checks TA
        pthread_mutex_lock(&accessLock);
        int chairsInUse = waitroomChairsOccupied;
        pthread_mutex_unlock(&accessLock);

        //Student tries to sit in TA's waiting room
        if(chairsInUse < NUM_WR_CHAIRS) {
            if(chairsInUse == 0) {
                //First student at office, so we wake up the TA
                sem_post(&taSemaphore);
            }
            //Wait in chair for TA to request us in
            pthread_mutex_lock(&accessLock);

            waitingRoomStudents[nextChairAvailable] = threadStudent.id;
            nextChairAvailable = (nextChairAvailable + 1) % NUM_WR_CHAIRS;
            waitroomChairsOccupied++;

            pthread_mutex_unlock(&accessLock);

            //Wait to get called up from chair (necessary for tracking wait time properly)
            time(&startWait);
            sem_wait(&waitingRoomSemaphore);
            time(&endWait);

            studentWaitTime = (double)(difftime(endWait,startWait));

            pthread_mutex_lock(&accessLock);
            totalStudentWaitTime += studentWaitTime;
            pthread_mutex_unlock(&accessLock);

            //Wait for teacher to finish helping
            sem_wait(&studentSemaphore);

            time(&leaveTime);
            leaveTimeString =  asctime(localtime(&leaveTime));
            printf("Student ID: %d | Student Arrive Time: %s | Student Leave Time: %s | Provided Service: Yes\n",
                   threadStudent.id, arriveTimeString.c_str(), leaveTimeString.c_str());
            //Once student is done, leave for good and note that
            break;
        }
        //Student didn't have a chair to sit at
        else {
            pthread_mutex_lock(&accessLock);
            sleepTime = ((rand() % WAIT_FACTOR) + WAIT_FACTOR) * MINUTE_AS_SECONDS;
            pthread_mutex_unlock(&accessLock);

            numNoChairLeave++;
            time(&leaveTime);
            leaveTimeString =  asctime(localtime(&leaveTime));
            printf("Student ID: %d | Student Arrive Time: %s | Student Leave Time: %s | Provided Service: No\n",
                   threadStudent.id, arriveTimeString.c_str(), leaveTimeString.c_str());
        }
    }
    //Stat report information
    pthread_mutex_lock(&accessLock);
    totalArrivals += numArrivals;
    totalNoChairLeaves += numNoChairLeave;
    pthread_mutex_unlock(&accessLock);

    pthread_exit(NULL);
}

/**
 * Behavior for the TA: sleeps for start time, wakes up when student wakes them,
 * helps that student and any students waiting, goes back to sleep and then closes when all students
 * have been serviced once.
 *
 * @param arg isn't used, needed for compiling
 *
 * @return a null when the thread exits when done
 */
void* taBehavior(void* arg) {
    //Note how many students have been helped today - no return visits after getting help in this version
    time_t taStartTime;
    time(&taStartTime);
    int numStudentsDone = 0;
    double timeSleeping = 0.0;

    printf("TA office is now open.\n");

    while(true) {

        //Check if TA is done for the day
        if(numStudentsDone >= numStudents) {
            break;
        }
        //TA is asleep until a student is there
        time_t taStartSleep;
        time(&taStartSleep);

        //Wait for a student to wake them
        sem_wait(&taSemaphore);

        //The TA woke up - add time to time sleeping
        time_t taEndSleep;
        time(&taEndSleep);
        timeSleeping += (double)(difftime(taEndSleep,taStartSleep));

        while(true) {

            pthread_mutex_lock(&accessLock);

            //If no students are waiting, go back to sleep
            if(waitroomChairsOccupied == 0) {
                pthread_mutex_unlock(&accessLock);
                break;
            }
            //Get a student that is waiting
            int currentStudentID = waitingRoomStudents[nextStudentToHelp];
            nextStudentToHelp = (nextStudentToHelp + 1) % NUM_WR_CHAIRS;
            waitroomChairsOccupied--;
            int helpTime = students[currentStudentID-1].timeNeeded;

            pthread_mutex_unlock(&accessLock);

            //Inform student they can come into TA office
            sem_post(&waitingRoomSemaphore);

            sleep(helpTime * MINUTE_AS_SECONDS);
            numStudentsDone++;

            //Inform student they are done getting help
            sem_post(&studentSemaphore);
        }
    }
    //Determine how long the TA was active
    time_t taEndTime;
    time(&taEndTime);

    double totalOfficeTime = (double)(difftime(taEndTime,taStartTime));

    //Lock isn't necessary here due to this being the only thread that edits the variable / reads it while threads are running
    taBusyPercent = (1.0-(timeSleeping/totalOfficeTime))*100;
    printf("TA office is now close.\n");

    pthread_exit(NULL);
}
