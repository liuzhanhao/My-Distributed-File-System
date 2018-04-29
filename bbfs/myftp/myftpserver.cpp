#include "myftpserver.hpp"

int main(int argc, char **argv)
{
	unsigned short port;

	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);

	DIR* dir = opendir("data");
	if (dir)
	{
	    /* Directory data/ exists. */
	    closedir(dir);
	}
	else if (ENOENT == errno)
	{
	    /* Directory data/ does not exist, create it. */
	    const int dir_err = mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)
		{
		    printf("Error creating directory data/\n");
		    exit(1);
		}
	}
	else
	{
	    printf("Error checking directory data/\n");
	    exit(1);
	}

	main_loop(port);

	return 0;
}
