#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "terminal.h"

void enter_terminal_mode();

char selected_disk[MAX_FILENAME_LENGTH];

// Fonction pour lister les fichiers .img dans le répertoire courant
void list_disks() {
  DIR *dir;
  struct dirent *entry;

  dir = opendir(".");
  if (dir == NULL) {
    perror("Erreur lors de l'ouverture du répertoire");
    return;
  }

  printf("Disques disponibles :\n");
  printf("[0] Retour\n");
  int count = 0;
  char disk_names[100][MAX_FILENAME_LENGTH]; // Stocker temporairement les noms

  while ((entry = readdir(dir)) != NULL) {
    if (strstr(entry->d_name, ".img") != NULL) {
      printf("[%d] %s\n", count + 1, entry->d_name);
      strcpy(disk_names[count], entry->d_name);
      count++;
    }
  }
  closedir(dir);

  if (count == 0) {
    printf("Aucun disque trouvé.\n");
    return;
  }

  // Demander à l'utilisateur de sélectionner un disque
  int choice;
  printf("Sélectionnez un disque (1-%d) : ", count);
  scanf("%d", &choice);
  getchar(); // Consommer le \n laissé par scanf

  if (choice < 1 || choice > count) {
    printf("Choix invalide.\n");
    return;
  }

  strcpy(selected_disk, disk_names[choice - 1]);
  printf("Vous avez sélectionné : %s\n", selected_disk);
  enter_terminal_mode();
}

void crea_disk(char disk_name[], int *disk_size) {
  printf("Rappel des règles : \n\t- Pas d'espaces\n\t- Pas de caractères "
         "spéciaux\n\t- Pas plus de 100 caractères\n");
  printf("Entrez le nom du disque : ");

  fgets(disk_name, 100, stdin);

  disk_name[strcspn(disk_name, "\n")] = 0;

  while (!(*disk_size > 0 && *disk_size <= 100)) {
    printf("\nRappel des regles : \n");
    printf("\t-Taille disque entre 1 et 100MB\n");
    printf("Entrer la taille voulue du disque (en MB) : ");
    scanf("%d", disk_size);
  }

  while (getchar() != '\n')
    ;
}

void gestion_action(int choix) {
  switch (choix) {
  case 1:
    printf("\nVous avez choisi de créer un disque.\n");

    char disk_name[100];
    int disk_size;
    
    crea_disk(disk_name, &disk_size);

    new_disk(disk_name, disk_size);

    break;
  case 2:
    printf("\nVous avez choisi d'afficher les disques.\n");
    list_disks();
    break;
  case 3:
    printf("\nVous avez choisi de supprimer un disque.\n");
    // Ajoute ici la logique pour supprimer un disque
    break;
  case 4:
    printf("\nFin programme.\n");
    exit(0); // Terminer le programme
  default:
    printf("\nChoix invalide. Veuillez sélectionner une option valide.\n");
  }

  printf("\n\n\n");
}

int main() {
  int choix = 0;

  while (1) {
    printf("Bienvenue dans cette simulation de disque\n");
    printf("Veuillez sélectionner une action parmi les suivantes : \n");
    printf("\t-Créer un disque [1]\n");
    printf("\t-Afficher les disques [2]\n");
    printf("\t-Supprimer un disque [3]\n");
    printf("\t-Fin programme [4]\n");

    // Lire l'input de l'utilisateur
    printf("> ");
    scanf("%d", &choix);

    // flush buffer scanf
    while (getchar() != '\n');

    // Gérer l'action en fonction du choix
    gestion_action(choix);
  }

  return 0;
}
