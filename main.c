#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h> 
#include <libgen.h>

#define MAX_NUMBER_OF_DIRECTORIES 10
#define MAX_DIRECTORY_NAME 50
#define MAX_FILE_NAME 100
#define MAX_MODIFIED_LENGTH 100
#define MAX_PERMISSIONS_LENGTH 50
#define MAX_PATH_LENGTH 100
#define BUFFER_SIZE 512

typedef struct{

  char name[MAX_FILE_NAME];
  off_t size;
  time_t modified; /* time of last modification */
  ino_t inode;
  mode_t mode;        /* file type and mode */
  char modifiedChar[MAX_MODIFIED_LENGTH]; //the modified field in human-readable format
  char permissions[MAX_PERMISSIONS_LENGTH];
  char path[MAX_PATH_LENGTH];
}MetaData;

int checkDirectories(char **argv, int argc, int start, char dirNames [MAX_NUMBER_OF_DIRECTORIES][MAX_DIRECTORY_NAME]); // check if the directories given resepect the requirements, puts their names in dirNames and returns the number of directories
void readDir(char *path, char *output_dir); // opens the directory and reads all the files
MetaData makeMetaData(char *path, struct dirent* dirData); // returns a MetaData from the file's path given
void makeSnapshot(char *path, MetaData metadata, char *output_dir); // makes the snapshot file or updates the already existing one if there are changes
MetaData parseMetaDataFromFile(const char *file_path); // returns a MetaData from a given file
int compareMetaData(MetaData new, MetaData old); // compares two metadatas
void printMetaData(MetaData metadata, FILE *file); // prints the metadata in the given file, if there is none given it will print to stdout
char *permissionToString(mode_t mode); // returns a human-readable string with the file permissions

void printMetaData(MetaData metadata, FILE *file)
{
    int fd;  // makes the output stdout if i don't give it a file
    if (file != NULL) {
        fd = fileno(file);
    } else {
      fd = STDOUT_FILENO;
    }

    char buffer[BUFFER_SIZE]; // Buffer to hold formatted strings
    int n; // Variable to store number of bytes written

    n = snprintf(buffer, sizeof(buffer), "Path: %s\n", metadata.path);
    if(write(fd, buffer, n) == -1)
      {
	perror("Write failed\n");
	exit(-6);
      }

    n = snprintf(buffer, sizeof(buffer), "Name: %s\n", metadata.name);
    if(write(fd, buffer, n) == -1)
      {
	perror("Write failed\n");
	exit(-6);
      }

    n = snprintf(buffer, sizeof(buffer), "Size: %lld bytes\n", metadata.size);
    if(write(fd, buffer, n) == -1)
      {
	perror("Write failed\n");
	exit(-6);
      }

    n = snprintf(buffer, sizeof(buffer), "Last Modified: %s\n", metadata.modifiedChar);
    if(write(fd, buffer, n) == -1)
      {
	perror("Write failed\n");
	exit(-6);
      }

    n = snprintf(buffer, sizeof(buffer), "Inode: %llu\n", metadata.inode);
    if(write(fd, buffer, n) == -1)
      {
	perror("Write failed\n");
	exit(-6);
      }

    n = snprintf(buffer, sizeof(buffer), "Permissions: %s\n", metadata.permissions);
    if(write(fd, buffer, n) == -1)
      {
	perror("Write failed\n");
	exit(-6);
      }

    if (file != NULL && file != stdout) {
        fclose(file); // Close the file if it's not standard output
    }
}

char *permissionToString(mode_t mode) {
    static char perm[10];
    perm[0] = (mode & S_IRUSR) ? 'r' : '-';
    perm[1] = (mode & S_IWUSR) ? 'w' : '-';
    perm[2] = (mode & S_IXUSR) ? 'x' : '-';
    perm[3] = (mode & S_IRGRP) ? 'r' : '-';
    perm[4] = (mode & S_IWGRP) ? 'w' : '-';
    perm[5] = (mode & S_IXGRP) ? 'x' : '-';
    perm[6] = (mode & S_IROTH) ? 'r' : '-';
    perm[7] = (mode & S_IWOTH) ? 'w' : '-';
    perm[8] = (mode & S_IXOTH) ? 'x' : '-';
    perm[9] = '\0';
    return perm;
}

MetaData makeMetaData(char *path, struct dirent* dirData)
{
  struct stat statData;
  int data = lstat(path, &statData);
	  
  if(data == -1)
    {
      perror("Stat failed\n");
      exit(-3);
    }

  MetaData metadata;

  strcpy(metadata.name, dirData->d_name);
  metadata.size = statData.st_size;
  metadata.modified = statData.st_mtime;
  metadata.inode = statData.st_ino;
  metadata.mode = statData.st_mode;

  struct tm *tm_info;
  tm_info = localtime(&statData.st_mtime);
  char modified_time_str[26];
  strftime(modified_time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

  strcpy(metadata.modifiedChar, modified_time_str);

  mode_t permissions = statData.st_mode & 0777;
  char *permission_str = permissionToString(permissions);
  strcpy(metadata.permissions, permission_str);

  strcpy(metadata.path, path);
  
  return metadata;
}

MetaData parseMetaDataFromFile(const char *file_path)
{
  MetaData metadata;
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    perror("fopen");
    exit(-5);
  }

  char line[50];
  while (fgets(line, 100, file) != NULL) {
    // Check each line for the relevant data
    if (strncmp(line, "Name:", 5) == 0) {
      sscanf(line, "Name: %[^\n]", metadata.name);
    } else if (strncmp(line, "Size:", 5) == 0) {
      sscanf(line, "Size: %lld bytes", &metadata.size);
    } else if (strncmp(line, "Inode:", 6) == 0) {
      sscanf(line, "Inode: %llu", &metadata.inode);
    } else if (strncmp(line, "Permissions:", 12) == 0) {
      sscanf(line, "Permissions: %[^\n]", metadata.permissions);
    }
  }
  strcpy(metadata.modifiedChar, "x"); // i don't care for this when i compare the two metadas
  strcpy(metadata.path, file_path);
  
  fclose(file);
  return metadata;
}

int compareMetaData(MetaData new, MetaData old)
{
  if(strcmp(new.name, old.name))  return 1;
  
  if(new.size != old.size) return 1;
  if(new.inode != old.inode) return 1;
  if(strcmp(new.permissions, old.permissions)) return 1;

  return 0;
}

void makeSnapshot(char *path, MetaData metadata, char *output_dir)
{
  char directory[MAX_DIRECTORY_NAME];
  if(output_dir[0] == '\0') // if there is not output_dir
    {
      strcpy(directory, dirname(strdup(path))); //i get the name of the dir
    }
  else
    {
      strcpy(directory, output_dir);
    }
  
   // here i modify the path sa that i can use it as a name for the file so that there will be no duplicates when i give multiple direcotires
  for (int i = 0; path[i] != '\0'; i++) { 
    if (path[i] == '/') {
      path[i] = '-';
    }
  }
  
  char snapshot_char[MAX_FILE_NAME] = "/";
  strcat(snapshot_char, path);
  strcat(snapshot_char, "_snapshot.txt");
  char output_file_path[strlen(directory) + strlen(snapshot_char) + 1];
  sprintf(output_file_path, "%s%s", directory, snapshot_char); // the output_file_path has the following format path/path-name_of_file_snapshot.txt
  
  // Check if the snapshot exists
  struct stat file_stat;
  if (stat(output_file_path, &file_stat) == 0) {
    // Snapshot exists 
    MetaData newMetaData = parseMetaDataFromFile(output_file_path);
    if(compareMetaData(newMetaData, metadata) != 0) // there are changes
      {
	int file_descriptor = open(output_file_path, O_WRONLY);
	if (file_descriptor == -1) {
	  perror("open snapshot");
	  exit(-4); 
	}

	FILE *file = fdopen(file_descriptor, "w");
	if (file == NULL) {
	  perror("fdopen");
	  close(file_descriptor); // Close the snapshot file
	  exit(-5);
	}
	
	printMetaData(metadata, file);
	fclose(file);
      }
    
    return;
  }
  // Snapshot does not exist
  
  int snapshot_file = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(snapshot_file == -1)
    {
      perror("Snapshot create error\n");
      exit(-4);
    }

  FILE *file = fdopen(snapshot_file, "w");
  if (file == NULL) {
    perror("fdopen");
    close(snapshot_file); // Close the snapshot file
    exit(-5);
  }

  printMetaData(metadata, file);
  
  fclose(file);
}


void readDir(char *path, char *output_dir)
{
  DIR *dir = opendir(path);
  if(dir == NULL)
    {
      perror("Dir open error");
      exit(-1);
    }
  
  struct dirent *dirData;

  while((dirData = readdir(dir)) != NULL)
    {
      if(dirData->d_name[0] == '.')
	continue;

      if(strstr(dirData->d_name, "_snapshot.txt")) // skip the snapshots
	continue;
      
      char pathCurrent[257];
      sprintf(pathCurrent, "%s/%s", path, dirData->d_name);
     
      if(dirData->d_type==DT_DIR)
	{
	  readDir(pathCurrent, output_dir);
	}
      if(dirData->d_type==DT_REG)
	{
	  MetaData metadata = makeMetaData(pathCurrent, dirData);

	  makeSnapshot(pathCurrent, metadata, output_dir); // there is no output file  
	}

    }
    closedir(dir);
}

int checkDirectories(char **argv, int argc, int start, char dirNames [MAX_NUMBER_OF_DIRECTORIES][MAX_DIRECTORY_NAME])
{
  int number_directories = 0;
  
    for(int i = start; i < argc; i++)
    {
      struct stat argv_stat;
      if(lstat(argv[i], &argv_stat) == -1)
	{
	  perror("Stat failed\n");
	  exit(-3);
	}

      if (S_ISDIR(argv_stat.st_mode))
	{
	  if(number_directories == MAX_NUMBER_OF_DIRECTORIES) // check if there are more directories than maximum number
	    {
	      perror("Too many directories");
	      exit(-1);
	    }

	  strcpy(dirNames[number_directories], argv[i]);
	  number_directories++;
	  
	  for(int j = 0; j < number_directories - 1; j++) // check if there are no duplicates
	    {
	      if(strcmp(dirNames[j], dirNames[number_directories - 1]) == 0)
		{
		  perror("Same directory was given mutiple times");
		  exit(-2);
		}
	    }
	}
    }

    return number_directories;
}

int main(int argc, char** argv)
{
  int number_directories = 0;
  char output_dir[MAX_DIRECTORY_NAME] = "";
  char dirNames [MAX_NUMBER_OF_DIRECTORIES][MAX_DIRECTORY_NAME];

  if(strcmp(argv[1], "-o") == 0)
    {
      strcpy(output_dir, argv[2]);
      number_directories = checkDirectories(argv, argc, 3, dirNames); //if there is an ouput file, than the directories start from argv[3]
      
      DIR *dir = opendir(argv[2]);
      if (dir == NULL)
	{
	  perror("Open dir error");
	  exit(-1);
	}
      closedir(dir);
    }
  else
    {
      number_directories = checkDirectories(argv, argc, 1, dirNames);
    }

  for(int i = 0; i < number_directories; i++)
    {
      pid_t pid = fork();
      if (pid == -1) {
	perror("fork");
	exit(-1);
      } else if (pid == 0) {
	// Child process
	readDir(dirNames[i], output_dir); // arguments are well given
	printf("\nSnapshot for %s created successfully.\n", dirNames[i]);
	exit(0);
      }
    }
  
  // Parent process: Wait for all child processes to complete
  for (int i = 0; i < number_directories; i++)
    {
      int status;
      pid_t child_pid = wait(&status);
      
      if (WIFEXITED(status))
	{
	  printf("\nChild Process %d terminated with PID %d and exit code %d.\n", i + 1, child_pid, WEXITSTATUS(status));
	}
      else
	{
	  printf("\nChild Process %d with PID %d did not exit normally.\n", i + 1, child_pid);
	}
    }
  
  
  return 0;
}
