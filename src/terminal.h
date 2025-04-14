/**
 * @file terminal.h
 * @brief Ce fichier contient la declaration des fonctions et variables du
 * terminal.
 *
 * Ce fichier contient les declarations des fonctions et differentes variables
 * globales servant a la simulation du terminal. Il necessite un "disk" sur
 * lequel travailler. Il est lance depuis le menu dans main.c et il suffit de
 * faire "exit" pour en sortir.
 */
#ifndef __TERMINAL__H__
#define __TERMINAL__H__

#include "fs.h"

// ressources externes
extern char selected_disk[MAX_FILENAME_LENGTH];
FILE *disk;
char current_path[MAX_FILENAME_LENGTH * 32];
int current_dir_inode;    ///> Position dans le disque de l'inode

// Variables/ressources de manipulation de fichiers
int cursor;
char opened_file[BLOCK_SIZE * BLOCKS_PER_INODE_MAX];


/**
 * @brief Scructure de commande
 * Elle associe le nom d'une commande a sa fonction.
 */
typedef struct {
  char *name;              ///< Nom de la commande
  void (*func)(char *arg); ///< Pointeur vers la fonction associee
} Command;

extern Command commands[];

// prototypes
void enter_terminal_mode();
void cmd_ls(char *arg);
void cmd_touch(char *arg);
void cmd_size(char *arg);
void cmd_chmod(char *arg);
void cmd_mkdir(char *arg);
void cmd_rm(char *arg);
void execute_command(char *cmd, char *arg);
char* dir_name_getter(int current_dir_inode);
void print_binary(char c);

// fonctions excel
void open(int inode);
void close(int inode);
void read(char *readed, int size);
void write(char *to_write, int size);
void lseek(int i);

#endif // __TERMINAL__H__
