#ifndef __FS__H__
#define __FS__H__

#include <time.h>
#include <stdio.h>
#define SUCCESS_CREA_DISK 0
#define FAILURE_CREA_DISK 1

#define BLOCK_SIZE 512 // Taille d'un bloc en octets
#define DISK_SIZE_MB 6
#define MAX_FILENAME_LENGTH 25
#define INODE_SIZE 128

#define INODE_COUNT 1024
#define BLOCK_COUNT 10240

// Offsets fixes dans le fichier .img
#define SUPERBLOCK_OFFSET 0
#define BLOCK_BITMAP_OFFSET BLOCK_SIZE
#define INODE_TABLE_OFFSET BLOCK_BITMAP_OFFSET + 1280
#define DATA_BLOCKS_OFFSET INODE_TABLE_OFFSET + 131072

// Offsets fixes pour infos inodes
#define INODE_ID_OFFSET             0
#define INODE_SIZE_OFFSET           INODE_ID_OFFSET + 4                       
#define INODE_TYPE_OFFSET           INODE_SIZE_OFFSET + 4                     
#define INODE_PERMISSIONS_OFFSET    INODE_TYPE_OFFSET + 2       
#define INODE_BLOCKS_OFFSET         INODE_PERMISSIONS_OFFSET + 2        
#define INODE_BLOCK_COUNT_OFFSET    INODE_BLOCKS_OFFSET + 40            
#define INODE_CREATED_AT_OFFSET     INODE_BLOCK_COUNT_OFFSET + 4      
#define INODE_MODIFIED_AT_OFFSET    INODE_CREATED_AT_OFFSET + 8       
#define INODE_NAME_OFFSET           INODE_MODIFIED_AT_OFFSET + 8      

#define MAX_FILENAME_LEN 32
#define MAX_DIRECTORY_ENTRIES 16

// errors/ info
#define INODE_TABLE_FULL -1
#define BLOCK_BITMAP_FULL -2
#define COULD_NOT_CREATE_NEW_FILE -3

#define TYPE_INODE_ROOT_DIR 0
#define TYPE_INODE_DIR 1
#define TYPE_INODE_FILE 2

// Structure pour une entrée de répertoire
typedef struct {
  char name[MAX_FILENAME_LEN];
  int inode;
} DirectoryEntry;

// Structure pour un répertoire
typedef struct {
  int entry_count;
  DirectoryEntry entries[MAX_DIRECTORY_ENTRIES];
} Directory;

typedef struct {
  unsigned int disk_size;   // Taille totale du disque en octets
  unsigned int block_size;  // Taille d'un bloc (ex: 512 octets)
  unsigned int inode_count; // Nombre total d'inodes
  unsigned int block_count; // Nombre total de blocs
  unsigned int free_inodes; // Nombre d'inodes libres
  unsigned int free_blocks; // Nombre de blocs libres
  unsigned int bitmap_start;
  unsigned int inode_table_start;
  unsigned int data_block_start;
} Superblock;

typedef struct {
  int id;           // Numéro unique de l'inode
  int size;         // Taille du fichier en octets
  char type;      
  char permissions; // Droits d'accès (ex: 755)
  int blocks[10];   // Blocs de données du fichier
  int block_count;
  time_t created_at;   // Timestamp de création
  time_t modified_at;  // Timestamp de modification
  char name[MAX_FILENAME_LENGTH];
} inode_t;

int new_disk(const char *disk_name, int size_mb);
void init_disk(const char *disk_name, int disk_size);
void init_root_dir(const char *disk_name);
int new_file(const char *disk_name, char file_type, char permissions,char file_name[MAX_FILENAME_LENGTH]);
int first_empty_inode_slot(FILE *disk);
int first_empty_block_slot(FILE *disk);
int get_block_slot(FILE* disk, int id_block_bitmap);


#endif //__FS__H__
