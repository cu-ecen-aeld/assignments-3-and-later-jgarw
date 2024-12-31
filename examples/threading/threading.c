#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // if there is a wait before to lock the thres, sleep for duration
    if(thread_func_args->wait_before > 0){
        DEBUG_LOG("Thread %lu: Waiting for %d seconds before locking...\n", pthread_self(), thread_func_args->wait_before);
        printf("Thread %lu: Waiting for %d seconds vefore locking...\n", pthread_self(), thread_func_args->wait_before);
        sleep(thread_func_args->wait_before);
    }

    // lock the thread and provide logging messages
    DEBUG_LOG("Locking thread: %lu.\n", pthread_self());
    printf("Locking thread: %lu.\n", pthread_self());
    
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex in thread %lu.\n", pthread_self());
        return NULL; // Exit early on mutex lock failure
    }


    DEBUG_LOG("Thread: %lu locked.\n", pthread_self());
    printf("Thread: %lu locked.\n", pthread_self());

    // if there is a wait to release the lock, sleep until wait is over
    if(thread_func_args->wait_after > 0){
        DEBUG_LOG("Thread %lu: Waiting for %d seconds before releasing...\n", pthread_self(), thread_func_args->wait_after);
        printf("Thread %lu: Waiting for %d seconds before releasing...\n", pthread_self(), thread_func_args->wait_after);
        sleep(thread_func_args->wait_after);
    }

    // release the lock on thread and provide logging messages
    DEBUG_LOG("Releasing thread: %lu.\n", pthread_self());
    printf("Releasing thread: %lu.\n", pthread_self());
    
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex in thread %lu.\n", pthread_self());
    }

    DEBUG_LOG("Thread: %lu released.\n", pthread_self());
    printf("Thread: %lu released.\n", pthread_self());

    thread_func_args->thread_complete_success = true; 

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    // allocate memory for thread_data
    struct thread_data* thread_func_args = malloc(sizeof(struct thread_data));

    if(thread_func_args == NULL){
        printf("Error allocating memory for thread_data.");
        return false;
    }

    thread_func_args->mutex = mutex;
    thread_func_args->wait_before = wait_to_obtain_ms / 1000;
    thread_func_args->wait_after = wait_to_release_ms / 1000;
    thread_func_args->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, (void *)thread_func_args);

    if(rc != 0){
        DEBUG_LOG("Error creating thread %lu. Error: %d", *thread, rc);
        printf("Error creating thread %lu. Error: %d", *thread, rc);
        free(thread_func_args);
        return false;
    }
    
    return true;
}

