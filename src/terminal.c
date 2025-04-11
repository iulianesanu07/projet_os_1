#include "terminal.h"
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Command commands[] = {
  {"ls", cmd_ls}, 
  {"size", cmd_size},   
  {"touch", cmd_touch},
  {"chmod", cmd_chmod}, 
  {"mkdir", cmd_mkdir}, 
  {NULL, NULL} // Fin du tableau
};

// Fonction pour exécuter la commande correspondante
void execute_command(char *cmd, char *arg) {
  for (int i = 0; commands[i].name != NULL; i++) {
    if (strcmp(cmd, commands[i].name) == 0) {
      commands[i].func(arg);
      return;
    }
  }
  printf("Commande inconnue : %s\n", cmd);
}

// Implémentation des commandes
void cmd_ls(char *arg) {
  printf("Affichage du contenu du disque... (À implémenter)\n");
}

void cmd_touch(char *arg) {
  // Verification validite argument a ajouter
  if (!arg) {
    printf("Usage : touch <nom_fichier>\n");
    return;
  }

  int inode_file = new_file(selected_disk, TYPE_INODE_FILE, 0b11111010, arg);

  add_file_to_dir(inode_file, current_dir_inode, disk);

  fclose(disk);
  disk = fopen(selected_disk, "r+b");
  if (!disk) {
    perror("Erreur lors de l'ouverture du fichier disque");
    return;
  }
  
  return;
}

void cmd_size(char *arg) {
  unsigned int disk_size;

  if (disk == NULL) {
    printf("Erreur : Aucun disque ouvert\n");
    return;
  }

  if (fseek(disk, 0, SEEK_SET) != 0) {
    printf("Erreur : Impossible de se positionner au debut du fichier\n");
    perror("fseek");
    return;
  }

  size_t bytes_read = fread(&disk_size, sizeof(unsigned int), 1, disk);
  if (bytes_read != 1) {
    printf("Erreur : Impossible de lire la taille du disque\n");
    perror("fread");
    return;
  }

  printf("Taille du disque : %u MB\n", disk_size / (1024 * 1024));
}

void cmd_chmod(char *arg) {
  printf("A implementer\n");

  return;
}

void cmd_mkdir(char *arg) { return; }

char* dir_name_getter(int current_dir_inode) {
  static char dir_name[MAX_FILENAME_LENGTH];

  fseek(disk, current_dir_inode + INODE_NAME_OFFSET, SEEK_SET);
  size_t bytes_read = fread(dir_name, sizeof(char), MAX_FILENAME_LENGTH, disk);

  if (bytes_read != MAX_FILENAME_LENGTH) {
    printf("Avertissement : %zu octets lus au lieu de %d.\n", bytes_read, MAX_FILENAME_LENGTH);
  }

  dir_name[MAX_FILENAME_LENGTH - 1] = '\0';
  return dir_name;
}

/**
 * @brief Fonction d'initialisation et de gestion du terminal
 * Cette fonction est en deux parties, la premiere etant l'initialisation du terminal 
 * et des differentes variables necessaires a son fonctionnement. La deuxieme etant le 
 * "coeur" du terminal qui gere les differentes commandes.
 */
void enter_terminal_mode() {

  disk = fopen(selected_disk, "r+b");
  if (!disk) {
    printf("Erreur : impossible d'ouvrir %s\n", selected_disk);
    return;
  }

  printf("Mode terminal actif. Tapez 'exit' pour quitter.\n");

  char command[200];
  memset(current_path, 0, sizeof(current_path));

  current_dir_inode = INODE_TABLE_OFFSET; // premier inode c'est le root
  strcpy(current_path, dir_name_getter(current_dir_inode));

  while (1) {
    printf("%s > ", current_path);
    fflush(stdout);
    if (!fgets(command, sizeof(command), stdin)) {
      printf("Erreur de lecture.\n");
      continue;
    }

    // Supprimer le '\n' en fin de ligne
    command[strcspn(command, "\n")] = 0;

    // Parsing de la commande
    char *cmd = strtok(command, " ");
    char *arg = strtok(NULL, " ");

    if (!cmd)
      continue;

    if (strcmp(cmd, "exit") == 0) {
      fclose(disk);
      break;
    }

    execute_command(cmd, arg);
  }
}
