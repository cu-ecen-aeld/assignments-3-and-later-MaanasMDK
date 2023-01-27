/***********************************************************************
* Filename : writer.c
* Author : Maanas Makam Dileep Kumar
* Description : C file to write a given string of text into a file.
* Date : 01/26/2023
* References : Man pages and Week2 lectures
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
	if( argc != 3)
	{
		printf("Error : Invalid number of arguments\n\r");
		printf("Usage : ./writer <filename> <string>\n\r");
		return 1;
	}
	
	// extract the filepath and string
	char *filename = argv[1];
	char *text = argv[2];
	
	// open log for syslog
	openlog(NULL, 0, LOG_USER);
	
	int fd;
	
	// opening an existing file in write mode or creating a file with permission modes of user=rw ,group=rw ,others=rw
	fd = open( filename, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH | S_IWOTH );
	
	// check for error with file operation
	if(fd == -1) 
	{
		syslog(LOG_ERR, "Error opening file with error: %d", errno);
		return 1;
	}
	
	// debug message before the write operation
	syslog(LOG_DEBUG, "Writing %s to file %s", text, filename);
	
	// writing text to the file
	int bytecount = write( fd, text, strlen(text));
	
	// check for partial or writing errors to the file
	if(bytecount != strlen(text))
	{
		// check for error while writing
		if(bytecount == -1)
		{
			syslog(LOG_ERR, "Failed to write %s to file %s", text, filename);
		}
		// incomplete write error
		else
		{
			syslog(LOG_ERR, "Incomplete write of %s to file %s", text, filename);
		}
		return 1;
	}
	
	close(fd);
	closelog();
	
	return 0;

}
