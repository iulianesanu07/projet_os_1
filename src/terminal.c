#include "terminal.h"
#include "fs.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Command commands[] = {
    {"ls", cmd_ls},       {"size", cmd_size},   {"touch", cmd_touch},
    {"chmod", cmd_chmod}, {"mkdir", cmd_mkdir}, {"rm", cmd_rm},
    {NULL, NULL} // Fin du tableau
};

// Sert au calcul du nombre de blocks necessaires.
int ceil_div(int a, int b) {
    return (a + b - 1) / b;
}

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

void print_binary(char c) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (c >> i) & 1);
  }
  printf("\n");
}

void cmd_rm(char *arg) {
  // Resolution mauvais usage
  // (fin resolution au pied de biche mais qui a dit que la violance ne
  // resolevait jamais les problemes)
  if (!arg) {
    printf("Usage : rm <nom_fichier>\n");
    return;
  }

  // structure temporaire pour stocker le numero de l'inode d'un fichier
  // et son nom
  struct inode_file_name {
    int inode;
    char file_name[MAX_FILENAME_LENGTH];
  };

  // Recupere l'inode de chaque fichier/dossier du dossier
  // courent et l'enregistre dans "inode_file_list"
  open(current_dir_inode);
  char inode_file_list[BLOCK_SIZE];
  read(inode_file_list, 512);


  /* pour dev
  puts("debut affichage inode_file_list:");
  for (int i = 0; i < MAX_DIRECTORY_ENTRIES * BLOCKS_PER_INODE_MAX; i++) {
    int *entry_ptr = (int*)(opened_file + i * sizeof(int));
    if(*entry_ptr != 0) {
      printf("- %d\n", *entry_ptr);
    }
  }
  puts("fin affichage inode_file_list");
  */

  // Initialisation de la liste inode_file_name
  struct inode_file_name list_file[MAX_DIRECTORY_ENTRIES];
  memset(list_file, 0, sizeof(list_file));
  int nbr_elements_liste = 0;

  // Pour chaque element de "list_file", on recupere le nom du fichier 
  // et stock dans "inode_file_name" le nom du fichier l'inode correspondante
  for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
    int *entry_ptr = (int*)(inode_file_list + i * sizeof(int));
    if (*entry_ptr != 0) {
      char temp_file_name[MAX_FILENAME_LENGTH];
      strcpy(temp_file_name, dir_name_getter(*entry_ptr));
      //printf("%s\n", temp_file_name);
      list_file[i].inode = *entry_ptr;
      strcpy(list_file[i].file_name, dir_name_getter(*entry_ptr));
      nbr_elements_liste++;
    }
  }


  // Permet d'afficher le tableau "list_file" (pour dev)
  /*
  for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
    if (list_file[i].inode != 0) {
      printf("%d inode %d : %s\n", i, list_file[i].inode, list_file[i].file_name);
    }
  }
  */
  
  // On regarde dans la liste "list_file" si une des valeurs correspond
  // a la valeur entree par l'utilisateur (arg) et si on en trouve une, on la supprime
  // et on sort de la fonction (travail termine) sinon on affiche la liste des 
  // fichiers presents dans le dossier
  
  for (int i = 0; i < nbr_elements_liste; i++) {
    if (!strcmp(list_file[i].file_name, arg)) { // fichier trouve
      supr(list_file[i].inode, disk);
      // Suppression numero d'inode du fichier a supprimer dans le dossier 
      // courrent

      

      printf("Fichier \"%s\" supprime avec succes !\n", list_file[i].file_name);
      return;
    }
  }

  // Si on arrive la c'est que le fichier n'est pas existant.
  printf("Fichier \"%s\" non existant, voici la liste des fichiers disponibles : \n", arg);
  for (int i = 0; i < nbr_elements_liste; i++) {
    printf("%s, inode -> %d\n", list_file[i].file_name, list_file[i].inode);
  }

  return;
}

/**
 * %brief Fonction equivalante a open()
 * Elle prend en argument le numero d'inode d'un fichier et initialise sa
 * lecture %param int inode numero inode du fichier a "open()"
 */
void open(int inode) {
  //printf("inode  open : %d\n", inode);

  // Recuperation adresses blocks de donnees associes a l'inode
  int blocks[BLOCKS_PER_INODE_MAX];
  fseek(disk, INODE_TABLE_OFFSET + (inode * INODE_SIZE) + INODE_BLOCKS_OFFSET, SEEK_SET);
  fread(blocks, sizeof(int), BLOCKS_PER_INODE_MAX, disk);
 
  // affichage, pour dev
  /*
  for (int i = 0; i < BLOCKS_PER_INODE_MAX; i++) {
    printf("b %d\n", blocks[i]);
  }
  */

  // copie du contenu de chaque block de donnees dans la variable globale
  // de fichier ouvert
  for (int i = 0; i < BLOCKS_PER_INODE_MAX; i++) {
    fseek(disk, DATA_BLOCKS_OFFSET + (blocks[i] * BLOCK_SIZE), SEEK_SET);
    fread(opened_file + (BLOCK_SIZE * i), BLOCK_SIZE, 1, disk);
  }

  // lecture int par int de toute la variable opened_file (pour dev)
  /*
  for (int i = 0; i < MAX_DIRECTORY_ENTRIES * BLOCKS_PER_INODE_MAX; i++) {
    int *entry_ptr = (int *)(opened_file + i * sizeof(int));
    if (*entry_ptr != 0) {
      printf("c %d\n", *entry_ptr);
    }
  }
  */

  cursor = 0;

  return;
}

/**
 * %brief Fonction equivalante a close()
 * Prend en argument un numero d'inode correspondant au fichier a fermer, 
 * verifie la taille, divise "opened_file" en blocks de 512, (calcule une taille de fichier)
 * et pour chaque blocks de 512, on ecrit la chaine dans le block de donnees grace au tableau
 * d'inode. Si ne nombre de blocks de alloue au fichier n'est pas suffisant, on demande un numero
 * de block de donnees et on continue jusqu'a avoir copie l'entrierte du fichier. (dans la limite de 
 * 10 blocks par inode).
 * Enfin on fclose() et fopen() le disque pour actualiser les donnees.
 * (Note pour andrea: c'est aussi cette partie en duo avec ma fonction de recherche de blocks de donnees
 * qui formeront la fragmentation.)
 */
void close(int inode) {
  
  // Recherche taille fichier
  int size_file = 0;
  for (int i = 0; i < BLOCK_SIZE * BLOCKS_PER_INODE_MAX; i++) {
    if (opened_file[i] != 0) { size_file++;}
  }

  int blocks_necessaires = ceil_div(size_file, BLOCK_SIZE);

  if (blocks_necessaires > BLOCKS_PER_INODE_MAX) {
    printf("Fichier trop grand, pour rappel, la taille maximale d'un fichier est %d octets.\n", (BLOCK_SIZE * BLOCKS_PER_INODE_MAX));
    return;
  }

  int blocks[BLOCKS_PER_INODE_MAX];
  fseek(disk, INODE_TABLE_OFFSET + (inode * INODE_SIZE) + INODE_BLOCKS_OFFSET, SEEK_SET);
  fread(blocks, sizeof(int), BLOCKS_PER_INODE_MAX, disk);

  for (int i = 0; i < blocks_necessaires; i++) {
    if (blocks[i] == -1) {  // implique qu'il faut plus de blocks de donnees
      int temp = first_empty_block_slot(disk);

      // on marque la bitmap
      bool used = 1;
      fseek(disk, BLOCK_BITMAP_OFFSET + temp, SEEK_SET);
      fwrite(&used, sizeof(bool), 1, disk);

      blocks[i] = temp;
    }
  }

  // Ecriture dans le disque
  for (int i = 0; i < blocks_necessaires; i++) {
    char temp_block[BLOCK_SIZE];
    memcpy(temp_block, opened_file + (i * BLOCK_SIZE), BLOCK_SIZE);
    fseek(disk, DATA_BLOCKS_OFFSET + (blocks[i] * BLOCK_SIZE), SEEK_SET);
    fwrite(temp_block, BLOCK_SIZE, 1, disk);
  }

  fclose(disk);
  disk = fopen(selected_disk, "r+b");


  return;
}

/**
 * %brief Fonction equivalante a read()
 * Cette fonction prend en argument une certaine taille d'elements a copier
 * et renvois la chaine de caractere a partir du cursor jusqu'a cursor + size.
 */
void read(char *readed, int size) { 

  memcpy(readed, opened_file + cursor, size);

  return;
}

/**
 * %brien Fonction equivalante a write()
 * Cette fonction prend en argument une chaine a ecrire et un argument correspondant
 * a la taille de la chaine transmise. Grace au cursor, elle ecrit directement dans 
 * le fichier ouvert.
 */
void write(char *to_write, int size) {

  // il faut ajouter les verifs pour pas que ca depasse la taille de 
  // BLOCK_SIZE * BLOCK_PER_INODE_MAX, on peut utiliser le cursor + 
  // size.

  for (int i = cursor; i < size; i++) {
    opened_file[i] = to_write[i - cursor];
  }

  return;
}

/**
 * %brief Fonction equivalante de lseek()
 * Cette fonction permet de manipuler le curseur de lecture/ecriture du 
 * fichier ouvert (simule).
*/
void lseek(int i) {
  cursor = i;
  return;
}

void cmd_ls(char *arg) {
  // recuperation adresse datablock du dossier
  fseek(disk,
        INODE_TABLE_OFFSET + (current_dir_inode * INODE_SIZE) +
            INODE_BLOCKS_OFFSET,
        SEEK_SET);
  int data_block_adr;
  fread(&data_block_adr, sizeof(int), 1, disk);
  printf("adresse cmd_ls %d\n", data_block_adr);

  // recuperation du datablock en lui meme pour traitement
  fseek(disk, DATA_BLOCKS_OFFSET + (data_block_adr * BLOCK_SIZE), SEEK_SET);
  char data_block[BLOCK_SIZE];
  fread(data_block, BLOCK_SIZE, 1, disk);

  for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
    int *entry_ptr = (int *)(data_block + i * sizeof(int));
    if (*entry_ptr != 0) {

      // recuperation infos du fichier(ou dossier)
      // (ca aurait peut etre ete plus propre de recuperer l'inode entier et de
      // le traiter comme ca x) (au lieu de spam les fseek() et fread())
      fseek(disk,
            INODE_TABLE_OFFSET + (*entry_ptr * INODE_SIZE) + INODE_NAME_OFFSET,
            SEEK_SET);
      char file_name[MAX_FILENAME_LENGTH];
      fread(file_name, sizeof(char) * MAX_FILENAME_LENGTH, 1, disk);

      fseek(disk,
            INODE_TABLE_OFFSET + (*entry_ptr * INODE_SIZE) + INODE_SIZE_OFFSET,
            SEEK_SET);
      int size;
      fread(&size, sizeof(int), 1, disk);

      fseek(disk,
            INODE_TABLE_OFFSET + (*entry_ptr * INODE_SIZE) +
                INODE_PERMISSIONS_OFFSET,
            SEEK_SET);
      char perm;
      fread(&perm, sizeof(char), 1, disk);

      fseek(disk,
            INODE_TABLE_OFFSET + (*entry_ptr * INODE_SIZE) +
                INODE_CREATED_AT_OFFSET,
            SEEK_SET);
      time_t crea_time;
      fread(&crea_time, sizeof(time_t), 1, disk);
      struct tm *crea_time_tm = localtime(&crea_time);
      // traitement de valeur time_t pour la rendre lisible
      char buff_crea_time[80];
      strftime(buff_crea_time, sizeof(buff_crea_time), "%Y-%m-%d %H:%M:%S",
               crea_time_tm);

      fseek(disk,
            INODE_TABLE_OFFSET + (*entry_ptr * INODE_SIZE) +
                INODE_MODIFIED_AT_OFFSET,
            SEEK_SET);
      time_t modif_time;
      fread(&modif_time, sizeof(time_t), 1, disk);
      struct tm *modif_time_tm = localtime(&modif_time);
      char buff_modif_time[80];
      strftime(buff_modif_time, sizeof(buff_modif_time), "%Y-%m-%d %H:%M:%S",
               modif_time_tm);

      printf("\t%s, Inode %d, %d Bytes\n\tCree a %s, Modifie a "
             "%s\n\tPermissions : ",
             file_name, *entry_ptr, size, buff_crea_time, buff_modif_time);
      print_binary(perm);
      printf("\n");
    }
  }

  return;
}

void cmd_touch(char *arg) {
  // Verification validite argument a ajouter
  if (!arg) {
    printf("Usage : touch <nom_fichier>\n");
    return;
  }

  int inode_file = new_file(selected_disk, TYPE_INODE_FILE, 0b11111010, arg);

  add_file_to_dir(inode_file, current_dir_inode, disk);

  // Petit code pas tres "propre", sert a actualiser le disque pour que les
  // modifications soient appliques
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

// L'erreur vient du fait que tu additionnes deux fois ta taille 
// de l'offset pour le tableau d'inode
// la variable current_dir_inode est deja a minimum 1700 et quelques
// (inode_table_offset en fait, et tu peux le voir des l'initialisation)

void cmd_chmod(char *arg) {
  for (int i = 0; i < BLOCKS_PER_INODE_MAX; i++) {
    fseek(disk, INODE_TABLE_OFFSET + INODE_BLOCKS_OFFSET + sizeof(int) * i, SEEK_SET);
    int temp;
    fread(&temp, sizeof(int), 1, disk);
    printf("inode lu : %d\n", temp);
    printf("current_dir : %d\n", current_dir_inode);
  }


  return;
}

void cmd_mkdir(char *arg) { return; }

char *dir_name_getter(int current_dir_inode) {
  static char dir_name[MAX_FILENAME_LENGTH];

  fseek(disk,INODE_TABLE_OFFSET + (current_dir_inode * INODE_SIZE) + INODE_NAME_OFFSET, SEEK_SET);
  size_t bytes_read = fread(dir_name, sizeof(char), MAX_FILENAME_LENGTH, disk);

  if (bytes_read != MAX_FILENAME_LENGTH) {
    printf("Avertissement : %zu octets lus au lieu de %d.\n", bytes_read,
           MAX_FILENAME_LENGTH);
  }

  dir_name[MAX_FILENAME_LENGTH - 1] = '\0';
  return dir_name;
}

/**
 * @brief Fonction d'initialisation et de gestion du terminal
 * Cette fonction est en deux parties, la premiere etant l'initialisation du
 * terminal et des differentes variables necessaires a son fonctionnement. La
 * deuxieme etant le "coeur" du terminal qui gere les differentes commandes.
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

  current_dir_inode = 0; // premier inode c'est le root
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
