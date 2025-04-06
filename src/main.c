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
  
  /* pour disque de taille variable, a ajouter si suffisement de temps.
  while (!(*disk_size > 0 && *disk_size <= 100)) {
    printf("\nRappel des regles : \n");
    printf("\t-Taille disque entre 1 et 100MB\n");
    printf("Entrer la taille voulue du disque (en MB) : ");
    scanf("%d", disk_size);
  }

  while (getchar() != '\n')
    ;
  */ 

  *disk_size = DISK_SIZE_MB;
  return;
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
  
/*
  case 3:
    printf("\nVous avez choisi de supprimer un disque.\n");
    // Ajoute ici la logique pour supprimer un disque
    break;
*/

  case 3:
    printf("\nFin programme.\n");
    exit(0); // Terminer le programme
  default:
    printf("\nChoix invalide. Veuillez sélectionner une option valide.\n");
  }

  printf("\n\n\n");
}

/* outil debug/dev
void calculate_fs_structure(size_t disk_size) {
    // Nombre total de blocs
    size_t total_blocks = disk_size / BLOCK_SIZE;
    
    // Nombre d'inodes avec un ratio 1 inode pour 10 blocs
    size_t total_inodes = total_blocks / 10;

    // Espace occupé par la table des inodes
    size_t inode_table_size = total_inodes * INODE_SIZE;
    size_t inode_table_blocks = (inode_table_size + BLOCK_SIZE - 1) / BLOCK_SIZE; // Arrondi supérieur

    // Taille de la bitmap des blocs (1 bit par bloc)
    size_t bitmap_size = total_blocks / 8;
    size_t bitmap_blocks = (bitmap_size + BLOCK_SIZE - 1) / BLOCK_SIZE; // Arrondi supérieur
    
    // Nombre de blocs de données disponibles après allocation des structures
    size_t data_blocks = total_blocks - (1 + bitmap_blocks + inode_table_blocks); // 1 pour superbloc

    // Affichage des résultats
    printf("Disque : %lu MB (%lu octets)\n", disk_size / (1024 * 1024), disk_size);
    printf("- Nombre total de blocs : %lu\n", total_blocks);
    printf("- Nombre total d'inodes : %lu\n", total_inodes);
    printf("- Taille de la table des inodes : %lu octets (%lu blocs)\n", inode_table_size, inode_table_blocks);
    printf("- Taille de la bitmap des blocs : %lu octets (%lu blocs)\n", bitmap_size, bitmap_blocks);
    printf("- Blocs disponibles pour les données : %lu\n", data_blocks);
}
*/

int main() {
  int choix = 0;

  while (1) {
    printf("Bienvenue dans cette simulation de disque\n");
    printf("Veuillez sélectionner une action parmi les suivantes : \n");
    printf("\t-Créer un disque [1]\n");
    printf("\t-Afficher les disques [2]\n");
    //printf("\t-Supprimer un disque [3]\n");
    printf("\t-Fin programme [3]\n");

    // Lire l'input de l'utilisateur
    printf("> ");
    scanf("%d", &choix);

    // flush buffer scanf
    while (getchar() != '\n')
      ;

    // Gérer l'action en fonction du choix
    gestion_action(choix);
  }

  return 0;
}
