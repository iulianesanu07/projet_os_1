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
    fseek(disk, INODE_TABLE_OFFSET + (i * INODE_SIZE) + INODE_CREATED_AT_OFFSET, SEEK_SET);

    time_t verif = -1;
    fread(&verif, sizeof(time_t), 1, disk);
//    printf("verif lu : %d\n", (int)verif);
    if (verif == 0) {
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
    
  inode_t new_inode;
  new_inode.id = inode;
  new_inode.size = 0;
  new_inode.type = file_type;
  new_inode.permissions = permissions;
  new_inode.blocks[0] = get_block_slot(disk, block_id);
  new_inode.block_count = 1;
  new_inode.created_at = time(NULL);
  new_inode.modified_at = time(NULL);
  strncpy(new_inode.name, file_name, MAX_FILENAME_LENGTH); 


  fseek(disk, INODE_TABLE_OFFSET + inode * INODE_SIZE, SEEK_SET);
  fwrite(&new_inode, sizeof(inode_t), 1, disk);
  fclose(disk);

  return inode;
}

void init_root_dir(const char*disk_name) {
  new_file(disk_name, TYPE_INODE_ROOT_DIR, 0b11110101, "root");

  return;
}

// Ajoute le num de l'inode du file (qui peut etre aussi dossier) au
// block de donnees du dir
void add_file_to_dir(int inode_file, int inode_dir, FILE *disk) {
  // a ajouter la verif des permissions (ici ou dans le touch directement)

  // Trouver l'adresse du block de donnees du dir dans lequel ajouter le fichier
  fseek(disk, INODE_TABLE_OFFSET + (inode_dir * INODE_SIZE) + INODE_BLOCKS_OFFSET, SEEK_SET);
  int adresse_datablock_dir;
  fread(&adresse_datablock_dir, sizeof(int), 1, disk);

  char data_block[BLOCK_SIZE];
  fseek(disk, DATA_BLOCKS_OFFSET + (adresse_datablock_dir * BLOCK_SIZE), SEEK_SET);
  fread(data_block, BLOCK_SIZE, 1, disk);

  // cette boucle permet de lire le data_block du dossier comme un tableau de int, de trouver l'emplacement
  // libre et d'y ecrire l'inode_file
  bool inserted = false;
  for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
    int *entry_ptr = (int*)(data_block + i * sizeof(int));
    if (*entry_ptr == 0) {  // l'emplacement est libre
      *entry_ptr = inode_file;
      
      // Ecriture dans le block de donnees du dossier
      fseek(disk, DATA_BLOCKS_OFFSET + (adresse_datablock_dir * BLOCK_SIZE), SEEK_SET);
      fwrite(data_block, BLOCK_SIZE, 1, disk);
      inserted = true;
      break;
    }
  }

  if (!inserted) {
    printf("Erreur : le dossier inode %d est plein. Il est impossible de depasser 128 elements par dossier, impossible donc de creer ce fichier.\n", inode_dir);
  }

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
