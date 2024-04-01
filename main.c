#include <stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
       #include <sys/types.h>
   #include <sys/stat.h>
    #include <unistd.h>


void readDirNames(DIR* dir,struct dirent* dirData, int *i, char dirNames[100][500])
{

  while((dirData = readdir(dir)) != NULL && *i < 100){

    if(strcmp(dirData->d_name,".") == 0 || strcmp(dirData->d_name,"..") == 0) continue;


    if(dirData != NULL)
      {
        strcpy(dirNames[(*i)++],dirData->d_name);

        struct stat statData;

        int data = stat(dirData->d_name, &statData);

        if(data == -1)
	  {
            perror("Stat failed\n");
            exit(-3);
	  }

        if(S_ISDIR(statData.st_mode))
	  {
	    printf("director\n");
	  }
        else
	  {
	    printf("fisier\n");
	  }

 
      }
      
    
  }
}

int main(int argc, char** argv)
{
  DIR *dir = NULL;
  if((dir = opendir(argv[1])) == NULL)
    {
      perror("File open error");
      exit(-1);
    }

  int i = 0;
  char dirNames[100][500];
  struct dirent *dirData = (struct dirent*)calloc(1, sizeof(struct dirent));

  if(!dirData)
    {
      perror("Allocation error\n");
      exit(-2);
    }
  
  readDirNames(dir, dirData, &i, dirNames);

  closedir(dir);
  return 0;
}
