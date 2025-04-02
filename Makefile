# Nom du projet
PROJECT_NAME = disk_simulator

# Répertoires
SRC_DIR = src
BUILD_DIR = build
DOCS_DIR = docs
TEST_DIR = test

# Fichiers sources
SRC_FILES = $(SRC_DIR)/main.c $(SRC_DIR)/fs.c $(SRC_DIR)/utils.c $(SRC_DIR)/terminal.c
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Options de compilation
CC = clang
CFLAGS = -Wall -Werror -g

# Cibles par défaut
all: $(BUILD_DIR)/$(PROJECT_NAME)

# Création du binaire final
$(BUILD_DIR)/$(PROJECT_NAME): $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ_FILES) -o $@

# Compilation des fichiers sources en objets
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Cible pour les tests
# Si tu as des tests dans le dossier 'test', tu peux ajouter une cible comme celle-ci
# test: $(BUILD_DIR)/$(PROJECT_NAME)
#   @./$(BUILD_DIR)/$(PROJECT_NAME) <tes commandes pour lancer les tests ici>

# Nettoyage des fichiers intermédiaires et du binaire
clean:
	rm -rf $(BUILD_DIR)

full_clean:
	$(MAKE) clean
	rm *.img

# Cible pour générer la documentation (optionnel si tu utilises Doxygen)
docs:
	doxygen $(DOCS_DIR)/Doxyfile

.PHONY: all clean docs

