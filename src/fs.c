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
      printf("first_empty_inode_slote : %d\n", i);
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
      printf("first_empty_block_slot : %d\n", i);
      return i;
    }
  }

  return BLOCK_BITMAP_FULL;
}

/*
 * %brief Permet de supprimer un fichier/dossier
 * Remet un "blank" a la place de l'inode du fichier (suppression)
 * et libere la "case" de la bitmap correspondante ainsi que le block
 * lui meme (on aurait pu s'en passer mais c'est un moyen de se proteger
 * contre d'eventuelles betises)
 */
void supr(int inode, FILE *disk) {
  
  // Pour chaque block de donnees du tableau de blocks de l'inode du fichier, 
  // on libere la "case" de la bitmap correspondante ainsi que le block lui meme.
  for (int i = 0; i < BLOCKS_PER_INODE_MAX; i++) {
    fseek(disk, INODE_TABLE_OFFSET + (inode * INODE_SIZE) + INODE_BLOCKS_OFFSET + i * sizeof(int), SEEK_SET);
    int temp_block_adr;
    fread(&temp_block_adr, sizeof(int), 1, disk);
    if (temp_block_adr != -1) {

      // bitmap liberee
      fseek(disk, BLOCK_BITMAP_OFFSET + temp_block_adr, SEEK_SET);
      bool unused = 0;
      fwrite(&unused, sizeof(bool), 1, disk);

      // liberation block de donnees
      char blank[BLOCK_SIZE];
      memset(blank, 0, BLOCK_SIZE);
      fseek(disk, DATA_BLOCKS_OFFSET + (temp_block_adr * BLOCK_SIZE), SEEK_SET);
      fwrite(blank, BLOCK_SIZE, 1, disk);
    }
  }

  // liberation inode
  char blank_inode[INODE_SIZE];
  memset(blank_inode, 0, INODE_SIZE);
  fseek(disk, INODE_TABLE_OFFSET + (inode * INODE_SIZE), SEEK_SET);
  fwrite(blank_inode, INODE_SIZE, 1, disk);

  return;
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
  new_inode.blocks[0] = block_id;

  for (int i = 1; i < BLOCKS_PER_INODE_MAX; i++) {
    new_inode.blocks[i] = -1;
  }

  new_inode.block_count = 1;
  new_inode.created_at = time(NULL);
  new_inode.modified_at = time(NULL);
  strncpy(new_inode.name, file_name, MAX_FILENAME_LENGTH); 

  /* (pour dev) permet d'afficher l'adresse de chaque block de donnees de l'inode
  for (int i = 0; i < BLOCKS_PER_INODE_MAX; i++) {
    printf("%d ", new_inode.blocks[i]);
  }
  printf("\n");
  */


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

      // Actualise la taille du dossier
      fseek(disk, INODE_TABLE_OFFSET + (inode_dir * INODE_SIZE) + INODE_SIZE_OFFSET, SEEK_SET);
      int size;
      fread(&size, sizeof(int), 1, disk);
      size += sizeof(int);
      fwrite(&size, sizeof(int), 1, disk);

      inserted = true;
      break;
    }
  }

  if (!inserted) {
    printf("Erreur : le dossier inode %d est plein. Il est impossible de depasser 128 elements par dossier, impossible donc de creer ce fichier.\n", inode_dir);
  }

  return;
}
