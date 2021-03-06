#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "../include/mt.h"

extern char progname[128];

/*** my own array of file names for myscandir() and getsacfiles()  ***/

typedef struct { char fn[256]; } FileNameList;

/*************************************************************************************/
/*** this subroutine has been retooled to use readdir() instead of scandir()       ***/
/*** differences between standard libc and BSD Unix is too great between platforms ***/
/*************************************************************************************/

void getrespfile( char *pathname, char *sta, char *net, char *cmp, 
                  char *khole, int verbose, char respfile[256] )
{
	int i, count=0;
	FileNameList *filen;
	FileNameList *myscandirRESP( char *, int *, FileNameList * );
	char tmp[1024], *ptr, tok[2] = { '_', '\0' } ;
	int my_file_name_cmp( char *, char * );
	void fatal_error( char *, char * );

	if(verbose)
	  printf("respdir=%s sta=%s net=%s cmp=%s khole=%s\n",
		pathname, sta, net, cmp, khole );
	
/*************************************************************************************/
/*** allocate space for filename list                                              ***/
/*************************************************************************************/
	filen = (FileNameList *)malloc( sizeof(FileNameList) );

/*************************************************************************************/
/*** get only sac file name list from remote directory                             ***/
/*************************************************************************************/
	filen = (FileNameList *)myscandirRESP( pathname, &count, filen );

/*************************************************************************************/
/*** sort the file names                                                           ***/
/*************************************************************************************/
	/* qsort( filen, count, sizeof(FileNameList), my_file_name_cmp ); */

	if( verbose )
	  printf("%s: number of files = %d in directory %s\n", 
		progname, count, pathname );
	
	if( count <= 0 )
	  fatal_error( "No SAC_PZs Response files in this directory", pathname );

	for( i=0; i<count; i++ ) 
	{
		if(verbose)
		  printf("i=%d filen=%s\n", i, filen[i].fn );
		
		strcpy( tmp, filen[i].fn );

		ptr = strtok( tmp, tok ); /*** SAC ***/
		ptr = strtok( NULL, tok ); /*** PZs ***/

		/*** network ***/
		while( ( ptr = strtok( NULL, tok )) != NULL )
		{
			if( strcmp( ptr, net ) == 0 )
			{
				if( verbose ) printf("net=%s\n", ptr );

				ptr = strtok( NULL, tok ); /*** station ***/
				if( strcmp( ptr, sta ) == 0 )
				{
					if(verbose)printf("sta=%s\n", ptr );

					/*** component ***/
					ptr = strtok( NULL, tok );

					if( strcmp( ptr, cmp ) == 0 )
					{
						if(verbose)
						{
						  printf(
					     "i=%d ptr=%s cmp=%s filen=%s (next) khole=%s\n",
						  	i, ptr, cmp, filen[i].fn, khole );
						}

						/**********************************************/
						/*** first check to see if khole is defined ***/
						/**********************************************/
						if( khole[0] == '\0' || 
							strncmp(khole, "-12345", 6 ) == 0 )
						{
							sprintf( respfile, "%s/%s", 
								pathname, filen[i].fn );

							if( verbose )
							{
							  printf("%s: Found respfile=%s\n",
								progname, respfile );
							}
							return;
						}

						/*********************************************/
						/*** else check for non null khole values ****/
						/*********************************************/
					
						/*** this was not the right khole, skip to
							next file ***/

						if( (ptr = strtok( NULL, tok )) == NULL )
						{
							break;
						}

						/*** this is the right khole, file found ***/

						if( strcmp( ptr, khole ) == 0 )
						{
						  sprintf( respfile, "%s/%s", 
							pathname, filen[i].fn );

						  if( verbose )
						  {
						    printf("%s: Found respfile=%s\n",
							progname, respfile );
						  }
						  return;
						}
						break;

					} /*** if cmp ***/
	      				break;

				} /*** if sta ***/
				break;

			} /*** if net ***/
			break;

		} /** while loop over words ***/

	} /*** for loop over files ***/

/*** looped over all SAC_PZs_* files and did not return a vaild 
	SAC Pole Zero response file for sta,net,cmp,khole set ***/

	fprintf(stderr, "%s: getrespfile.c: ERROR Respfile not found %s.%s.%s.%s\n", 
		progname, sta,net,cmp,khole );
	fprintf(stdout, "%s: getrespfile.c: ERROR Respfile not found %s.%s.%s.%s\n", 
		progname, sta,net,cmp,khole );

	exit(-1);
}

FileNameList *myscandirRESP( char *pathname, int *count, FileNameList *filen )
{
	struct stat f;
	DIR *dip;
	struct dirent *dit;
	int i=0;
	char *eptr;

	if( ( dip = opendir(pathname) ) == NULL )
	{
		perror("opendir");
		fprintf(stderr, "\n\n Directory %s does not exit\n", pathname );
		exit(2);
	}

	while( (dit = readdir(dip)) != NULL )
	{
		if( (strcmp( dit->d_name, "." ) == 0) ||
			(strcmp(dit->d_name, ".." ) == 0 ) ) continue;

		stat( dit->d_name, &f );

	/*** does not work on suns/solaries and linux ***/
		/*** if( S_ISDIR( f.st_mode ) ) continue; ***/

		eptr = index( dit->d_name, '_' );

		if( eptr == NULL ) continue;

		if( strncmp(eptr, "_PZs_", 5) == 0 )
		{
			strcpy( filen[i].fn, dit->d_name );
			i++;
			filen = (FileNameList *) 
			  realloc( filen, (i+1)*sizeof(FileNameList) );
		}
	}
	if( closedir(dip) == -1 )
	{
		perror("closedir");
		exit(3);
	}
	*count = i;
	return filen;
}
