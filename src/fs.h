#ifndef __FS__H__
#define __FS__H__

#define SUCCESS_CREA_DISK 0
#define FAILURE_CREA_DISK 1

#define BLOCK_SIZE 512  // Taille d'un bloc en octets
#define MAX_FILENAME_LENGTH 100

// Offsets fixes dans le fichier .img
#define SUPERBLOCK_OFFSET 0
#define INODE_BITMAP_OFFSET BLOCK_SIZE
#define BLOCK_BITMAP_OFFSET (INODE_BITMAP_OFFSET + X * BLOCK_SIZE)
#define INODE_TABLE_OFFSET (BLOCK_BITMAP_OFFSET + Y * BLOCK_SIZE)
#define DATA_BLOCKS_OFFSET (INODE_TABLE_OFFSET + Z * BLOCK_SIZE)

typedef struct {
    unsigned int disk_size;   // Taille totale du disque en octets
    unsigned int block_size;  // Taille d'un bloc (ex: 512 octets)
    unsigned int inode_count; // Nombre total d'inodes
    unsigned int block_count; // Nombre total de blocs
    unsigned int free_inodes; // Nombre d'inodes libres
    unsigned int free_blocks; // Nombre de blocs libres
} Superblock;


typedef struct {
    int id;               // Numéro unique de l'inode
    int size;             // Taille du fichier en octets
    int is_dir;           // 1 = dossier, 0 = fichier
    int permissions;      // Droits d'accès (ex: 755)
    int blocks[10];       // Blocs de données du fichier
    int created_at;       // Timestamp de création
    int modified_at;      // Timestamp de modification
} inode_t;

// Variable globale pour stocker le disque select

int new_disk(const char * file_name, int size_mb);
void create_superblock(const char *file_name, int disk_size);

#endif  //__FS__H__
