#include "fs.h"

#include <stdio.h>
#include <stdlib.h>

int new_disk(const char *file_name, int size_mb) {
  // Ouvrir le fichier en mode écriture binaire
  FILE *fichier = fopen(file_name, "wb");
  if (fichier == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
    return FAILURE_CREA_DISK; // Erreur à l'ouverture du fichier
  }

  // Calcul de la taille en octets
  long taille_octets = size_mb * 1024 * 1024;

  // Remplir le fichier avec des zéros (ou un autre pattern si nécessaire)
  char *buffer = (char *)calloc(taille_octets, sizeof(char));
  if (buffer == NULL) {
    perror("Erreur d'allocation mémoire");
    fclose(fichier);
    return FAILURE_CREA_DISK; // Erreur d'allocation mémoire
  }

  // Écrire les données dans le fichier
  size_t ecrits = fwrite(buffer, sizeof(char), taille_octets, fichier);
  if (ecrits != taille_octets) {
    perror("Erreur lors de l'écriture dans le fichier");
    free(buffer);
    fclose(fichier);
    return FAILURE_CREA_DISK; // Erreur d'écriture dans le fichier
  }

  // Libérer la mémoire et fermer le fichier
  free(buffer);
  fclose(fichier);

  create_superblock(file_name, size_mb);

  printf("Le disque '%s' a été créé avec succès, taille: %d Mo\n", file_name,
         size_mb);
  return SUCCESS_CREA_DISK;
}

void create_superblock(const char *file_name, int disk_size) {
  Superblock sb;
  sb.disk_size = disk_size * 1024 * 1024;
  sb.block_size = BLOCK_SIZE;
  sb.block_count = sb.disk_size / sb.block_size;
  sb.inode_count = sb.block_count / 10; 
  sb.free_blocks = sb.block_count - sb.inode_count;
  sb.free_inodes = sb.inode_count;

  FILE *file =
      fopen(file_name, "r+b"); // ouvre en mode lecture/écriture binaire
  if (!file) {
    printf("Erreur lors de l'ouverture du disque.\n");
    return;
  }

  fwrite(&sb, sizeof(Superblock), 1, file);
  fclose(file);
}
