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

  init_disk(file_name, size_mb);

  return SUCCESS_CREA_DISK;
}

void create_root_directory(FILE *disk) {
  Superblock sb;
  fseek(disk, 0, SEEK_SET);
  fread(&sb, sizeof(Superblock), 1, disk);

  int root_inode = 0;

  // Lire la table des inodes
  inode_t inode_table[sb.inode_count];
  fseek(disk, sb.inode_table_start, SEEK_SET);
  fread(inode_table, sizeof(inode_t), sb.inode_count, disk);

  // Initialiser l'inode de la racine
  inode_table[root_inode].type = TYPE_DIRECTORY;
  inode_table[root_inode].size = sizeof(Directory);
  inode_table[root_inode].blocks[0] = sb.first_data_block;
  inode_table[root_inode].block_count = 1;

  fseek(disk, sb.inode_table_start * BLOCK_SIZE, SEEK_SET);
  fwrite(inode_table, sizeof(inode_t), sb.inode_count, disk);

  // Initialiser le contenu du répertoire root
  Directory root_dir;
  root_dir.entry_count = 2;
  root_dir.entries[0] = (DirectoryEntry){".", root_inode};
  root_dir.entries[1] = (DirectoryEntry){"..", root_inode};

  fseek(disk, sb.first_data_block * BLOCK_SIZE, SEEK_SET);
  fwrite(&root_dir, sizeof(Directory), 1, disk);

  printf("✅ Répertoire root '/' créé avec succès !\n");
}

void init_disk(const char *file_name, int disk_size) {
  Superblock sb;
  sb.disk_size = disk_size * 1024 * 1024;
  sb.block_size = BLOCK_SIZE;

  puts("mais on arrive la ?");

  size_t total_blocks = sb.disk_size / BLOCK_SIZE;
  size_t total_inodes = total_blocks / 10;
  size_t inode_table_size = total_blocks * INODE_SIZE;
  size_t inode_table_blocks = (inode_table_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  size_t bitmap_size = total_blocks / 32;
  size_t bitmap_blocks = (bitmap_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  size_t data_blocks = total_blocks - (1 + bitmap_blocks + inode_table_blocks);

  sb.block_count = data_blocks;
  sb.inode_count = total_inodes;
  sb.free_blocks = data_blocks;
  sb.free_inodes = total_inodes;
  sb.bitmap_start = sizeof(Superblock);
  sb.inode_table_start = sb.bitmap_start + bitmap_blocks * BLOCK_SIZE;
  sb.first_data_block = sb.inode_table_start + sb.inode_count * INODE_SIZE;

  FILE *file =
      fopen(file_name, "r+b"); // ouvre en mode lecture/écriture binaire
  if (!file) {
    printf("Erreur lors de l'ouverture du disque.\n");
    fclose(file);
    return;
  }

  printf("Disque : %d MB\n", disk_size);
  printf("- Nombre total de blocs : %lu\n", total_blocks);
  printf("- Nombre total d'inodes : %lu\n", total_inodes);
  printf("- Taille de la table des inodes : %lu octets (%lu blocs)\n",
         inode_table_size, inode_table_blocks);
  printf("- Taille de la bitmap des blocs : %lu octets (%lu blocs)\n",
         bitmap_size, bitmap_blocks);
  printf("- Blocs disponibles pour les données : %lu\n", data_blocks);

  printf("Le disque '%s' a été créé avec succès, taille: %d Mo\n", file_name,
         disk_size);

  create_root_directory(file);

  fwrite(&sb, sizeof(Superblock), 1, file);
  fclose(file);
}
