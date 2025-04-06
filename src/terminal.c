#include "terminal.h"
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Prototypes internes
void cmd_ls(char *arg);
void cmd_touch(char *arg);
void cmd_size(char *arg);

// Ptr vers disque
FILE *disk;

// Structure pour associer une commande à une fonction
typedef struct {
  char *name;
  void (*func)(char *arg);
} Command;

// Liste des commandes disponibles
Command commands[] = {
    {"ls", cmd_ls},
    {"size", cmd_size},
    {"touch", cmd_touch},
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
  if (!arg) {
    printf("Usage : touch <nom_fichier>\n");
    return;
  }
  printf("Création du fichier '%s'... (À implémenter)\n", arg);
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

char *current_dir(const char *selected_disk,
                  char dir_name[MAX_FILENAME_LENGTH]) {

  // Affiche l'offset actuel avant fseek
  long current_offset = ftell(disk);
  printf("Offset avant fseek: %ld\n", current_offset);

  // Déplace le curseur à l'offset souhaité
  fseek(disk, INODE_TABLE_OFFSET + INODE_NAME_OFFSET, SEEK_SET);

  // Affiche l'offset après fseek pour vérifier où il mène
  current_offset = ftell(disk);
  printf("Offset après fseek: %ld\n", current_offset);

  // Lit le nom du répertoire
  size_t bytes_read =
      fread(dir_name, sizeof(char), MAX_FILENAME_LENGTH, disk);

  // Vérifie si la lecture a réussi
  if (bytes_read != MAX_FILENAME_LENGTH) {
    printf("Avertissement : %zu octets lus au lieu de %d.\n", bytes_read,
           MAX_FILENAME_LENGTH);
  }

  // S'assure que le nom est bien une chaîne C valide
  dir_name[MAX_FILENAME_LENGTH - 1] = '\0';
  
  printf("Nom du répertoire : '%s'\n", dir_name);

  return dir_name;
}

// Fonction principale du terminal
void enter_terminal_mode() {

  disk = fopen(selected_disk, "r+b");
  if (!disk) {
    printf("Erreur : impossible d'ouvrir %s\n", selected_disk);
    return;
  }

  printf("Mode terminal actif. Tapez 'exit' pour quitter.\n");

  char command[200];

  char dir_name[MAX_FILENAME_LENGTH];
  current_dir(selected_disk, dir_name);

  while (1) {
    printf("%s >", dir_name);
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
