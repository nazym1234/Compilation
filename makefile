# Programme principal 'grammaire' 
EXEC = grammaire
SRC = grammaire.c

# Programme secondaire 'generates_words'
P2_EXEC = generate_words
P2_SRC = generate_words.c

# Compilateur
CC = gcc
CFLAGS =

# Compilation par défaut pour 'grammaire'
all: $(EXEC)

# Règle pour générer l'exécutable 'grammaire'
$(EXEC): $(SRC)
	$(CC) $(SRC) -o $(EXEC)

# Commande pour exécuter le programme 'grammaire' avec un fichier par défaut
run: $(EXEC)
	@echo "Exécution avec le fichier exemple.general.txt"
	./$(EXEC) exemple.general.txt

# Règle pour générer l'exécutable 'generate_words'
make2: $(P2_EXEC)
	$(CC) $(P2_SRC) -o $(P2_EXEC)

# Commande pour exécuter le programme 'generate_words'
run2: $(P2_EXEC)
	./$(P2_EXEC)

# Nettoyage des fichiers générés
clean:
	rm -f $(EXEC) $(P2_EXEC)