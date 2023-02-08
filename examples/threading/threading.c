#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define MILLI_TO_MICRO_SCALE (1000)

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data *thread_func_args = (struct thread_data *) thread_param;

    // wait before obtaining mutex
    int ret = usleep(thread_func_args->wait_to_obtain_ms * MILLI_TO_MICRO_SCALE);
    if( ret == -1 )
    {
        ERROR_LOG("Error usleep at obtain failed with error number %d", errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Obtain mutex sleep success!!");

    // acquire mutex
    ret = pthread_mutex_lock(thread_func_args->mtx);
    if( ret == -1 )
    {
        ERROR_LOG("Error mutex lock failed with error number %d", errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Obtain mutex success!!");

    // wait before release of mutex
    ret = usleep(thread_func_args->wait_to_release_ms * MILLI_TO_MICRO_SCALE);
    if( ret == -1 )
    {
        ERROR_LOG("Error usleep at release failed with error number %d", errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Release mutex sleep success!!");

    // release mutex
    ret = pthread_mutex_unlock(thread_func_args->mtx);
    if( ret == -1 )
    {
        ERROR_LOG("Error mutex lock failed with error number %d", errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Release mutex  success!!");

    thread_func_args->thread_complete_success = true;
    DEBUG_LOG("Thread function success!!");
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
    // malloc a new thread_data 
    struct thread_data *thread_parameters = (struct thread_data *)malloc(sizeof(struct thread_data));
    if( thread_parameters == NULL )
    {
        ERROR_LOG("Error in malloc of new thread with error number : %d", errno);
        return false;
    }
    else
    {
        DEBUG_LOG("malloc for new thread success!!");
    }

    thread_parameters->thread = thread;
    thread_parameters->mtx = mutex;
    thread_parameters->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_parameters->wait_to_release_ms = wait_to_release_ms;

    // create a new thread
    int ret =  pthread_create(thread, NULL, threadfunc, thread_parameters);
    if( ret == 0 )
    {
        DEBUG_LOG("Thread creation success!!");
        return true;
    }
    ERROR_LOG("Error in pthread_create with error number : %d", errno);
    return false;
}

