/*************************************************************************************************
* Filename : aesdsocket.c
* Author : Maanas Makam Dileep Kumar
* Description : Server implementation using Sockets.
* Date : 02/20/2023
* Reference : https://beej.us/guide/bgnet/html/ & Lecture Videos
*************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

/************************************
* Defines
*************************************/
#define RET_ERROR (-1)
#define RET_SUCCESS (0)
#define SERVER_PORT ("9000")
#define BACKLOG (10)
#define BUFFER_SIZE (3)
#define USE_AESD_CHAR_DEVICE 

/************************************
* Global Variables
************************************/
// socket file descriptor
int sockfd;
// temperory file desciptor for the output file
int tmp_fd;
// varibale to track file size
int file_size;
// mutex to for writing to the common file
#ifndef USE_AESD_CHAR_DEVICE
pthread_mutex_t mtx;
#endif
// execution status tracking
int execution_status = 0;
#ifndef USE_AESD_CHAR_DEVICE
// flag to track timeout
bool timeout = false;
// timer object
timer_t timer;
#endif
/**********************************
* Linked list data structures
**********************************/
// structure for storing thread data
typedef struct 
{
    pthread_t tid;
    bool complete_flag;
    int fd;
    struct sockaddr_storage t_addr;
} tdata_t;

// structure for a node of the linked list
typedef struct node
{
    tdata_t data;
    struct node *next;
} node_t;

/**********************************
* Function Definitions
**********************************/
#ifndef USE_AESD_CHAR_DEVICE
// function to print timestamp
void log_timestamp();
// function to create timer
int create_timer();
#endif
// function to insert an element into head of the singly linked list
int sll_insert( node_t **head, node_t *new_node)
{
    // check if new_node is empty
    if( new_node == NULL )
    {
        return RET_ERROR;
    }
    new_node->next = *head;
    *head =  new_node;
    return RET_SUCCESS;
}

// Handler for SIGINT, SIGTERM, and SIGALRM
void signal_handler(int signum)
{
    if (signum == SIGINT) 
    {
        syslog(LOG_DEBUG, "Caught signal SIGINT, exiting!!");
        execution_status = 1;
    }
    else if( signum == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting!!");
        execution_status = 1;
    }
    #ifndef USE_AESD_CHAR_DEVICE
    else if(signum == SIGALRM)
    {
        syslog(LOG_DEBUG, "Caught signal SIGALRM!!");
        // set timeout flag
        timeout = true;
    }
    #endif
}

// Cleanup function
void cleanup()
{
    int ret = close(sockfd);
    if(ret == -1)
    {
        syslog(LOG_ERR, "Close socket error with errno : %d", errno);
    }

    ret = close(tmp_fd);
    if(ret == -1)
    {
        syslog(LOG_ERR, "Close storage file error with errno : %d", errno);
    }
    #ifndef USE_AESD_CHAR_DEVICE
    ret = unlink("/var/tmp/aesdsocketdata");
    if(ret == -1)
    {
        syslog(LOG_ERR, "Unlink error with errno : %d", errno);
    }
    // destroy mutex
    pthread_mutex_destroy(&mtx);
    #endif
    // delete timer
    #ifndef USE_AESD_CHAR_DEVICE
    timer_delete(timer);
    #endif
    // close syslog
    closelog();

    exit(RET_SUCCESS);
}

// Reference : Linux System Programming Chapter 5
int deamonize()
{
    pid_t child_pid = fork();

    if( child_pid == -1 )
    {
        syslog(LOG_ERR, "Failed to fork deamon process");
        return RET_ERROR;
    }

    if( child_pid != RET_SUCCESS )
    {
        //parent process
        exit(RET_SUCCESS);
    }

    //create new session
    if( setsid() == -1)
    {
        return RET_ERROR;
    }

    // change working dir to root
    if( chdir("/") == -1 )
    {
        return RET_ERROR;
    }

    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
    syslog(LOG_DEBUG, "daemon");

    return RET_SUCCESS;
}

// thread function
void *thread_func( void *thread_param )
{
    tdata_t *thread_data = (tdata_t *) thread_param;
    // Reference : https://man7.org/linux/man-pages/man3/inet_ntop.3.html
    char address_string[INET_ADDRSTRLEN];
    struct sockaddr_in *p = (struct sockaddr_in *)&thread_data->t_addr;
    syslog(LOG_DEBUG, "Accepted connection from %s", inet_ntop(AF_INET, &p->sin_addr, address_string, sizeof(address_string)));

    char buffer[BUFFER_SIZE];
    int byte_recv;
    int packet_len;

    bool data_packet = false;
    //clearing old values
    memset(buffer, '\0', BUFFER_SIZE);
    int cycle_count = 0;
    int total_len = 0;
    int old_size = BUFFER_SIZE;
    data_packet = false;
    char *recieve_buffer = (char *)malloc(BUFFER_SIZE);
    if(recieve_buffer == NULL)
    {
        syslog(LOG_ERR, "malloc");
    }

    while(!data_packet)
    {
        packet_len = 0;

        if( (byte_recv =  recv(thread_data->fd, &buffer, BUFFER_SIZE, 0)) == -1 )
        {
            syslog(LOG_ERR, "recv");
            goto recv_error;
        }
        for(int i=0; i< BUFFER_SIZE; i++)
        {
            packet_len++;
            if( buffer[i] == '\n' )
            {
                data_packet = true;
                break;
            }
        }

        if(cycle_count == 0)
        {
            recieve_buffer = (char *)realloc(recieve_buffer, packet_len);
            if(recieve_buffer == NULL)
            {
                syslog(LOG_ERR, "realloc");
            }
            // clearing buffer
            memset(recieve_buffer, '\0', packet_len);
            total_len = packet_len;
        }
        else
        {
            char *ptr = realloc(recieve_buffer, old_size+packet_len);
            if(ptr == NULL)
            {
                syslog(LOG_ERR, "realloc");
                
            }
            else
            {
                recieve_buffer = ptr;
                old_size += packet_len;
            }
            total_len = old_size;
        }
    memcpy(recieve_buffer + (cycle_count * BUFFER_SIZE), buffer, packet_len);
    cycle_count ++;
    }
    // move to end of file
    lseek(tmp_fd, 0, SEEK_END);
    // lock mutex before writing
    #ifndef USE_AESD_CHAR_DEVICE
    int ret = pthread_mutex_lock(&mtx);
    if( ret != RET_SUCCESS )
    {
        perror("mutex lock");
    }
    #endif
    #ifdef USE_AESD_CHAR_DEVICE
    char *filename = "/dev/aesdchar";
    #else
    char *filename = "/var/tmp/aesdsocketdata";
    #endif
	// opening an existing file in write mode or creating a file with permission modes of user=rw ,group=rw ,others=rw
	int new_tmp_fd = open( filename, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH | S_IWOTH );

    if( new_tmp_fd == -1 )
    {
        syslog(LOG_ERR, "Error opening file with error %d", errno);
    }
    int wc = write(new_tmp_fd, recieve_buffer, total_len);
    if( wc == -1)
    {
        syslog(LOG_ERR, "write");
    }
    file_size += wc;
    // resend data to client
    memset(buffer, '\0', BUFFER_SIZE);
    #ifndef USE_AESD_CHAR_DEVICE
    lseek(new_tmp_fd, 0, SEEK_SET);
    #endif
    int read_bytes;

    char *read_buffer = (char *)malloc(BUFFER_SIZE);

    if (read_buffer == NULL) {
        printf("Unable to allocate memory to read_buffer\n");
    }
    while( (read_bytes = read(new_tmp_fd, read_buffer, BUFFER_SIZE))>0)
    {
        // bytes_sent is return value from send function
        int bytes_sent = send(thread_data->fd, read_buffer, read_bytes, 0);

        if (bytes_sent == RET_ERROR) 
        {
            syslog(LOG_ERR, "send");
            goto send_error;
        }
    }
    close(new_tmp_fd);
    #ifndef USE_AESD_CHAR_DEVICE
    ret = pthread_mutex_unlock(&mtx);
    if( ret!=RET_SUCCESS )
    {
        perror("mutex unlock");
    }
    #endif
    if(read_bytes == RET_ERROR)
    {
        perror("read");
        syslog(LOG_ERR, "read");
    }
    send_error : free(read_buffer);

    // cleanup

    recv_error : free(recieve_buffer);
    close(thread_data->fd);
    thread_data->fd = -1;
    thread_data->complete_flag = true;
    syslog(LOG_DEBUG, "Closed connection from %s", inet_ntop(AF_INET, &p->sin_addr, address_string, sizeof(address_string)));
    return NULL;
}

// main function
int main(int argc, char *argv[])
{
    bool is_daemon = false;
	// check number of arguments
	if( argc > 2)
	{
		printf("Error : Invalid number of arguments\n");
		printf("Usage : ./aesdsocket [-d]\n");
		return RET_ERROR;
	}

    // check if executable must be a daemon
    if( argv[1] == NULL )
    {
        is_daemon =  false;
    }
    else if( strcmp(argv[1], "-d") == 0)
    {
        is_daemon = true;
    }
    else
    {
		printf("Error : Invalid option\n\r");
		printf("Usage : ./aesdsocket [-d]\n\r");
		return RET_ERROR;
    }

	// open log for syslog
	openlog(NULL, 0, LOG_USER);

    // initliaze signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGALRM, signal_handler);

    // socket setup
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // IPv4 address only
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int ret; int status = 0;
    ret = getaddrinfo(NULL, SERVER_PORT, &hints, &server_info);
    if( ret !=RET_SUCCESS )
    {
        syslog(LOG_ERR, "Error : getaddrinfo with error no : %d", errno);
        status = 1;
    }
    else
    {
        syslog(LOG_DEBUG, "getaddrinfo");
    }

    sockfd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if( sockfd == RET_ERROR )
    {
        syslog(LOG_ERR, "Error : socket with error no : %d", errno);
        status = 1;
    }
    else
    {
        syslog(LOG_DEBUG, "socket");
    }
    
    // to eliminate binding issue
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
    	printf("Error : setsockopt\n");
    }
    int flags = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    ret = bind(sockfd, server_info->ai_addr, server_info->ai_addrlen);
    if( ret != RET_SUCCESS )
    {
        syslog(LOG_ERR, "Error : bind with error no : %d", errno);
        status = 1;
    }
    else
    {
        syslog(LOG_DEBUG, "bind");
    }


    // calling deamon function
    if( is_daemon )
    {
        if(deamonize() == 0)
        {
            syslog(LOG_DEBUG, "Successfully created deamon");
        }
        else
        {
            syslog(LOG_ERR, "Failed to create deamon");
        }
    }
    else
    {
        syslog(LOG_DEBUG, "NO daemon");
    }

    ret = listen(sockfd, BACKLOG);
    if( ret != RET_SUCCESS )
    {
        syslog(LOG_ERR, "Error : listen with error no : %d", errno);
        status = 1;
    }
    else
    {
        syslog(LOG_DEBUG, "listen");
    }

    freeaddrinfo(server_info);
    addr_size = sizeof their_addr;

    if(status)
    {
        syslog(LOG_ERR, "Error connecting to client");
        closelog();
        return RET_ERROR;
    }

    #ifndef USE_AESD_CHAR_DEVICE
    ret = create_timer();
    if(ret == -1)
    {
        perror("timer creation");
    }
    #endif

    #ifndef USE_AESD_CHAR_DEVICE
    pthread_mutex_init(&mtx, NULL);
    #endif

    node_t *head =NULL;
    node_t *prev,*current;

    while(!execution_status)
    {
        #ifndef USE_AESD_CHAR_DEVICE
        if(timeout)
        {
            timeout = false;
            log_timestamp();
        }
        #endif
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if( new_fd == -1 )
        {
            if(errno == EWOULDBLOCK)
            {
                // make accept non blocking
                continue;
            }
            syslog(LOG_ERR, "Error : accept with error no : %d", errno);
            // accept new connection
            continue;
        }
        node_t *new_node = (node_t *)malloc(sizeof(node_t));
        new_node->data.complete_flag = false;
        new_node->data.fd = new_fd;
        new_node->data.t_addr = their_addr;
        // create a thread for the connection
        int ret = pthread_create( &(new_node->data.tid), NULL, &thread_func, &(new_node->data));
        if( ret != RET_SUCCESS )
        {
            syslog(LOG_ERR, "pthread_create");
        }
        else
        {
            syslog(LOG_DEBUG, "pthread_create");
        }

        sll_insert(&head, new_node);
    }
    current =  head;
    prev = head;
    while(current)
    {
        if((current->data.complete_flag == true) && (current == head))
        {
            // when current is the head node
            head = current->next;
            pthread_join(current->data.tid, NULL);
            syslog(LOG_DEBUG, "pthread_join");
            free(current);
            current = head;
        }
        else if ((current->data.complete_flag == true) && (current != head)) 
        { 
            // when current is not the head node
            prev->next = current->next;
            current->next = NULL;
            pthread_join(current->data.tid, NULL);
            syslog(LOG_DEBUG, "pthread_join");
            free(current);
            current = prev->next;
        } 
        else 
        {
            // traverse the list as current is not complete
            prev = current;
            current = current->next;
        }
    }
    printf("All threads exited!!\n");
    cleanup();
}

#ifndef USE_AESD_CHAR_DEVICE
// Function to log timestamp value to file
void log_timestamp()
{
    time_t timestamp;
    char time_buf[40];
    char buffer[100];

    struct tm* ts;

    time(&timestamp);
    ts = localtime(&timestamp);

    // Reference : https://man7.org/linux/man-pages/man3/strftime.3.html for RFC 2822 compliant
    strftime(time_buf, 40, "%a, %d %b %Y %T %z", ts);
    sprintf(buffer, "timestamp:%s\n", time_buf);

    lseek(tmp_fd, 0, SEEK_END);

    // atomically write timestamp
    #ifndef USE_AESD_CHAR_DEVICE
    pthread_mutex_lock(&mtx);
    #endif
    int wc = write(tmp_fd, buffer, strlen(buffer));
    if (wc == RET_ERROR) 
    {
        perror("write");
    }
    #ifndef USE_AESD_CHAR_DEVICE
    pthread_mutex_unlock(&mtx);
    #endif
}

// Function to create a timer
int create_timer()
{
    // CLOCK_REALTIME to access the wall time
    int ret = timer_create(CLOCK_REALTIME, NULL, &timer);
    if(ret == -1)
    {
        perror("timer_create");
        return RET_ERROR;
    }

    // struct itimerval delay;
    struct itimerspec delay;

    // first timeout after 10 seconds
    delay.it_value.tv_sec = 10;
    delay.it_value.tv_nsec = 0;
    // continuous timeouts every 10 seconds
    delay.it_interval.tv_sec = 10;
    delay.it_interval.tv_nsec = 0;

    ret = timer_settime(timer, 0, &delay, NULL);
    if(ret) 
    {
        perror ("timer_settime");
        return RET_ERROR;
    }

    return RET_SUCCESS;
}
#endif