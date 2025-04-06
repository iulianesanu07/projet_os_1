#include <stdio.h>
#include <string.h>

#define TARGET_STRING "root"  // La chaîne que nous cherchons
#define MAX_FILENAME_LENGTH 256  // La longueur maximale des noms de fichiers (à adapter si nécessaire)

char* find_string_in_disk(const char *selected_disk) {
    FILE *disk = fopen(selected_disk, "rb");
    if (!disk) {
        perror("Erreur lors de l'ouverture du fichier disque");
        return NULL;
    }

    // Taille de la chaîne cible
    size_t target_len = strlen(TARGET_STRING);

    // Buffer pour lire des données depuis le disque
    char buffer[1024];  // Lisons par blocs de 1024 bytes pour plus de performance
    size_t bytes_read;

    // Parcourir le fichier byte par byte
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), disk)) > 0) {
        // Chercher la chaîne dans le buffer
        for (size_t i = 0; i < bytes_read - target_len + 1; i++) {
            // Comparer la sous-chaîne à la chaîne recherchée
            if (strncmp(&buffer[i], TARGET_STRING, target_len) == 0) {
                // Si trouvé, retourner l'adresse correspondante
                // Calculer l'adresse dans le fichier
                long offset = ftell(disk) - bytes_read + i;
                fclose(disk);
                printf("Trouvé 'root' à l'offset %ld\n", offset);
                return TARGET_STRING;
            }
        }
    }

    // Si on n'a pas trouvé la chaîne, retourner NULL
    fclose(disk);
    printf("Chaîne '%s' non trouvée.\n", TARGET_STRING);
    return NULL;
}

int main() {
    const char *disk_image = "allez.img";  // Remplace par le nom de ton disque virtuel
    char *result = find_string_in_disk(disk_image);
    if (result != NULL) {
        printf("La chaîne '%s' a été trouvée.\n", result);
    }
    return 0;
}

