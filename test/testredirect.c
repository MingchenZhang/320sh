#include<stdio.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main (int argc, char**argv){
  ssize_t write_bytes=0;
 // assume fd =3 is already open, but for test purpose I am openning a file 
  int fd1=open("test1.txt",O_RDWR,0);
  int fd2=open("test2.txt",O_RDWR,0);
 // assume fd=4 file is already open, but for test purppse I am openning a file 
if(write_bytes=sendfile(3,0,NULL,10)==-1){
   perror("writing from stdout to file with filedescriptor3 failed\n");
 }
if(write_bytes=sendfile(4,0,NULL,10)==-1){
   perror("writing from stdout to file with filedescriptor4 failed\n");
 }
  return 0;

}
