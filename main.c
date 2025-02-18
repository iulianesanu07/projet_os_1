#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_INODES 100
#define DISK_SIZE (10 * 1024 * 1024)

typedef struct {
  int inode_number;
  char *filename;
  size_t size;
  char permissions;
  time_t created_at;
  time_t modified_at;
  char *data;
} Inode;

Inode inode_table[MAX_INODES];

int main() {
  int fd = open("disk.img", O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    perror("Erreur de creation de fichier");
    return 1;
  }

  if (ftruncate(fd, DISK_SIZE) != 0) {
    perror("Erreur d'allocation");
    close(fd);
    return 1;
  }

  close(fd);
  printf("Fichier disque 'disk.img' cree avec succes\n");
  return 0;
}
