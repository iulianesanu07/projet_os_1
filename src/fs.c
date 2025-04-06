#include "fs.h"

#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int new_disk(const char *disk_name, int size_mb) {
  // Ouvrir le fichier en mode écriture binaire
  FILE *fichier = fopen(disk_name, "wb");
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

  init_disk(disk_name, size_mb);

  return SUCCESS_CREA_DISK;
}

/*
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
*/

void init_disk(const char *disk_name, int disk_size) {
  Superblock sb;
  sb.disk_size = disk_size * 1024 * 1024;
  sb.block_size = BLOCK_SIZE;
  sb.inode_count = INODE_COUNT;
  sb.block_count = BLOCK_COUNT;
  sb.free_inodes = INODE_COUNT;
  sb.free_blocks = BLOCK_COUNT;
  sb.bitmap_start = BLOCK_BITMAP_OFFSET;
  sb.inode_table_start = INODE_TABLE_OFFSET;
  sb.data_block_start = DATA_BLOCKS_OFFSET;
  
  // Écriture du superblock dans le fichier disque
  FILE *disk = fopen(disk_name, "r+b"); // lecture/écriture binaire
  if (!disk) {
    perror("Erreur lors de l'ouverture du fichier disque");
    return;
  }

  fseek(disk, SUPERBLOCK_OFFSET, SEEK_SET);
  fwrite(&sb, sizeof(Superblock), 1, disk);
  fclose(disk);

  init_root_dir(disk_name);
  
  return;
}

int first_empty_inode_slot(FILE *disk) {
  for (int i = 0; i < INODE_COUNT; i++) {
    fseek(disk, INODE_TABLE_OFFSET + (i * INODE_SIZE), SEEK_SET);

    int id = -1;
    fread(&id, sizeof(int), 1, disk);
    if (id == 0) {
      return i;
    }
  }

  return INODE_TABLE_FULL;
}

int first_empty_block_slot(FILE *disk) {
  for (int i = 0; i < BLOCK_COUNT; i++) {
    fseek(disk, BLOCK_BITMAP_OFFSET + i, SEEK_SET);
    bool id = 1;
    fread(&id, sizeof(bool), 1, disk);
    if (id == 0) { // block not used
      return i;
    }
  }

  return BLOCK_BITMAP_FULL;
}

// globalement ca converti un block vide sur la bitmap en son emplacement reel dans le disque
int get_block_slot(FILE* disk, int id_block_bitmap) {
  
  
  return DATA_BLOCKS_OFFSET + (id_block_bitmap * BLOCK_SIZE);
}

// creation inode "pure"
int new_file(const char *disk_name, char file_type, char permissions, char file_name[MAX_FILENAME_LENGTH]) {  
  FILE *disk = fopen(disk_name, "r+b");
  if (!disk) {
    perror("Erreur lors de l'ouverture du fichier disque");
    return COULD_NOT_CREATE_NEW_FILE;
  }

  int inode = first_empty_inode_slot(disk);
  if (inode == INODE_TABLE_FULL) {
        perror("Disque plein, impossible de creer un nouveau fichier");
        return COULD_NOT_CREATE_NEW_FILE;
        }

  int block_id = first_empty_block_slot(disk);
  if (block_id == BLOCK_BITMAP_FULL) {
    perror("Plus de blocs disponibles");
    fclose(disk);
    return COULD_NOT_CREATE_NEW_FILE;
  }

  // Marquer le bloc comme utilisé
  bool used = 1;
  fseek(disk, BLOCK_BITMAP_OFFSET + block_id, SEEK_SET);
  fwrite(&used, sizeof(bool), 1, disk);
  puts("baaaaah ?");
    
  inode_t new_inode;
  new_inode.id = inode;
  new_inode.size = -1;
  new_inode.type = file_type;
  new_inode.permissions = permissions;
  new_inode.blocks[0] = get_block_slot(disk, block_id);
  new_inode.block_count = 0;
  new_inode.created_at = time(NULL);
  new_inode.modified_at = time(NULL);
  strncpy(new_inode.name, file_name, MAX_FILENAME_LENGTH); 


  fseek(disk, INODE_TABLE_OFFSET + inode * INODE_SIZE, SEEK_SET);
  fwrite(&new_inode, sizeof(inode_t), 1, disk);
  fclose(disk);

  return inode;
}

void init_root_dir(const char*disk_name) {
  puts("bah ?");
  new_file(disk_name, TYPE_INODE_ROOT_DIR, 0b11110101, "root");

  return;
}



/* Future code, peut etre si suffisement de temps. (il n'y aura pas suffisement de temps x) 
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

  //create_root_directory(file);

  fwrite(&sb, sizeof(Superblock), 1, file);
  fclose(file);
}
*/
