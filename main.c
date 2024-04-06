#include <stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include<time.h>
#include <fcntl.h> 
#include <libgen.h>

typedef struct{

  char name[50];
  off_t size;
  time_t modified; /* time of last modification */
  ino_t inode;
  mode_t mode;        /* file type and mode */
  char modifiedChar[100]; //the modified field in human-readable format
  char permissions[50];
}MetaData;

void printMetaData(MetaData metadata, FILE *file)
{
  FILE *output; // makes the output stdout if i don't give it a file
  if (file != NULL) { 
    output = file;
  } else {
    output = stdout;
  }
  
  fprintf(output, "Name: %s\n", metadata.name);
  fprintf(output, "Size: %lld bytes\n", metadata.size);
  fprintf(output, "Last Modified: %s\n", metadata.modifiedChar);
  fprintf(output, "Inode: %llu\n", metadata.inode);
  fprintf(output, "Permissions: %s\n", metadata.permissions);

  if (file != NULL && file != stdout) {
    fclose(output); // Close the file if it's not standard output
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
  
  return metadata;
}

void makeSnapshot(char *path, MetaData metadata, char *name)
{
  char *directory = dirname(strdup(path)); //i get the name of the dir
  // here i modify the path sa that i can use it as a name for the file
  //so that there will be no duplicates when i give multiple direcotires
  for (int i = 0; path[i] != '\0'; i++) { 
    if (path[i] == '/') {
      path[i] = '-';
    }
  }
  char snapshot_char[100] = "/";
  strcat(snapshot_char, path);
  strcat(snapshot_char, "_snapshot.txt");
  char output_file_path[strlen(directory) + strlen(snapshot_char) + 1];
  sprintf(output_file_path, "%s%s", directory, snapshot_char);
  
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

void readDir(char *path)
{
  DIR *dir = opendir(path);
  if(dir == NULL)
    {
      perror("File open error");
      exit(-1);
    }
  
  struct dirent *dirData;

  while((dirData = readdir(dir)) != NULL)
    {
      if(dirData->d_name[0] == '.')
	continue;
      
      char pathCurrent[257];
      sprintf(pathCurrent, "%s/%s", path, dirData->d_name);
      
      if(dirData->d_type==DT_DIR)
	{
	  readDir(pathCurrent);
	}
      if(dirData->d_type==DT_REG)
	{
	  MetaData metadata = makeMetaData(pathCurrent, dirData);
	  //printMetaData(metadata, NULL);
	  makeSnapshot(pathCurrent, metadata, dirData->d_name);
	}


    }
    closedir(dir);
}

int main(int argc, char** argv)
{
  
  readDir(argv[1]);

  return 0;
}
