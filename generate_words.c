#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_RULES 100
#define MAX_SYMBOLS 100
#define MAX_WORD_LEN 256

typedef struct {
    char non_terminal[MAX_SYMBOLS];
    char productions[MAX_RULES][MAX_SYMBOLS];
    int production_count;
} Rule;

typedef struct {
    Rule rules[MAX_RULES];
    int rule_count;
    char axiome[MAX_SYMBOLS];
} Grammaire;

// Nettoyer les espaces dans une chaîne
void nettoyer_chaine(char *str) {
    char *write_ptr = str;
    for (char *read_ptr = str; *read_ptr != '\0'; read_ptr++) {
        if (!isspace((unsigned char)*read_ptr)) {
            *write_ptr++ = *read_ptr;
        }
    }
    *write_ptr = '\0';
}

// Charger une grammaire depuis un fichier
int lire_grammaire(Grammaire *grammaire, const char *filename) {
    grammaire->rule_count = 0;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Supprime le saut de ligne
        nettoyer_chaine(line);           // Nettoyer les espaces inutiles

        if (strlen(line) == 0) {
            continue;
        }

        Rule rule;
        rule.production_count = 0;

        char *token = strtok(line, ":");
        if (token == NULL) {
            fprintf(stderr, "Erreur : Format incorrect : %s\n", line);
            fclose(file);
            return -1;
        }
        strcpy(rule.non_terminal, token);

        token = strtok(NULL, "|");
        while (token != NULL) {
            nettoyer_chaine(token); // Nettoyer chaque production
            strcpy(rule.productions[rule.production_count++], token);
            token = strtok(NULL, "|");
        }

        grammaire->rules[grammaire->rule_count++] = rule;
    }

    fclose(file);
    return 0;
}

// Décompose un mot en symboles (terminaux et non-terminaux)
char **decomposer_mot(const char *mot, int *nombre_de_symboles) {
    char **symboles = malloc(strlen(mot) * sizeof(char *));
    *nombre_de_symboles = 0;

    for (int i = 0; mot[i] != '\0'; ) {
        if (isupper(mot[i]) && isdigit(mot[i + 1])) {
            symboles[(*nombre_de_symboles)++] = strndup(&mot[i], 2);
            i += 2;
        } else {
            symboles[(*nombre_de_symboles)++] = strndup(&mot[i], 1);
            i++;
        }
    }
    return symboles;
}

// Vérifie si un symbole est un terminal
int est_terminal(const char *symbole) {
    return strlen(symbole) == 1 && islower(symbole[0]);
}

// Recherche des productions pour un non-terminal donné
int trouver_productions(const char *non_terminal, Grammaire *grammaire, char productions[MAX_RULES][MAX_SYMBOLS]) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        if (strcmp(grammaire->rules[i].non_terminal, non_terminal) == 0) {
            for (int j = 0; j < grammaire->rules[i].production_count; j++) {
                strcpy(productions[j], grammaire->rules[i].productions[j]);
            }
            return grammaire->rules[i].production_count;
        }
    }
    return 0;
}

// Génère récursivement tous les mots possibles
void generer_mots_recursif(const char *forme_courante, int longueur_max, Grammaire *grammaire, int profondeur_max, char mots[MAX_RULES * MAX_SYMBOLS][MAX_WORD_LEN], int *mot_count) {
    if (profondeur_max < 0) return;

    int nombre_de_symboles = 0;
    char **symboles = decomposer_mot(forme_courante, &nombre_de_symboles);

    // Si le mot est entièrement terminal, l'ajouter
    int est_terminal_total = 1;
    for (int i = 0; i < nombre_de_symboles; i++) {
        if (!est_terminal(symboles[i])) {
            est_terminal_total = 0;
            break;
        }
    }

    if (est_terminal_total) {
        if (strlen(forme_courante) <= longueur_max) {
            strcpy(mots[(*mot_count)++], forme_courante);
        }
        for (int i = 0; i < nombre_de_symboles; i++) free(symboles[i]);
        free(symboles);
        return;
    }

    // Développer les règles pour les non-terminaux
    for (int i = 0; i < nombre_de_symboles; i++) {
        if (!est_terminal(symboles[i])) {
            char productions[MAX_RULES][MAX_SYMBOLS];
            int production_count = trouver_productions(symboles[i], grammaire, productions);

            for (int j = 0; j < production_count; j++) {
                char nouvelle_forme[MAX_WORD_LEN] = "";
                for (int k = 0; k < i; k++) strcat(nouvelle_forme, symboles[k]);
                strcat(nouvelle_forme, productions[j]);
                for (int k = i + 1; k < nombre_de_symboles; k++) strcat(nouvelle_forme, symboles[k]);

                generer_mots_recursif(nouvelle_forme, longueur_max, grammaire, profondeur_max - 1, mots, mot_count);
            }
            break; // Un seul non-terminal est traité à la fois
        }
    }

    for (int i = 0; i < nombre_de_symboles; i++) free(symboles[i]);
    free(symboles);
}

// Fonction de tri lexicographique
int comparer_mots(const void *a, const void *b) {
    const char *mot_a = (const char *)a;
    const char *mot_b = (const char *)b;

    int len_diff = strlen(mot_a) - strlen(mot_b);
    if (len_diff != 0) return len_diff;

    return strcmp(mot_a, mot_b);
}
// Générer tous les mots
// Générer tous les mots
void generer_mots(Grammaire *grammaire, int longueur_max, const char *nom_fichier_sortie) {
    char mots[MAX_RULES * MAX_SYMBOLS][MAX_WORD_LEN] = {{0}}; // Initialisation du tableau à zéro
    int mot_count = 0;

    // Vérifier si l'axiome a epsilon (E) comme production
    for (int i = 0; i < grammaire->rule_count; i++) {
        if (strcmp(grammaire->rules[i].non_terminal, grammaire->axiome) == 0) {
            for (int j = 0; j < grammaire->rules[i].production_count; j++) {
                if (strcmp(grammaire->rules[i].productions[j], "E") == 0) {
                    // Ajouter explicitement "E" dans les mots générés
                    strcpy(mots[mot_count++], "E");
                }
            }
            break;
        }
    }

    // Générer les autres mots récursivement
    generer_mots_recursif(grammaire->axiome, longueur_max, grammaire, longueur_max * 2, mots, &mot_count);

    // Trier les mots
    qsort(mots, mot_count, MAX_WORD_LEN, comparer_mots);

    // Sauvegarder dans le fichier
    FILE *output = fopen(nom_fichier_sortie, "w");
    if (!output) {
        perror("Erreur d'ouverture du fichier de sortie");
        return;
    }

    for (int i = 0; i < mot_count; i++) {
        fprintf(output, "%s\n", mots[i]);
    }

    fclose(output);
    printf("Mots générés sauvegardés dans %s\n", nom_fichier_sortie);
}
// Fonction principale
int main() {
    Grammaire grammaire_chomsky;
    Grammaire grammaire_greibach;

    // Charger la grammaire en forme normale de Chomsky
    if (lire_grammaire(&grammaire_chomsky, "exemple.Transforme.chomsky") == -1) {
        fprintf(stderr, "Erreur : Impossible de lire la grammaire en forme normale de Chomsky.\n");
        return -1;
    }
    // Définir automatiquement l'axiome comme le premier non-terminal pour Chomsky
    strcpy(grammaire_chomsky.axiome, grammaire_chomsky.rules[0].non_terminal);

    // Générer des mots pour la grammaire en forme normale de Chomsky
    printf("\n==== Génération de mots (Chomsky) ====");
    generer_mots(&grammaire_chomsky, 4, "mots_chomsky_generes.txt");

    // Charger la grammaire en forme normale de Greibach
    if (lire_grammaire(&grammaire_greibach, "exemple.Transforme.greibach") == -1) {
        fprintf(stderr, "Erreur : Impossible de lire la grammaire en forme normale de Greibach.\n");
        return -1;
    }
    // Définir automatiquement l'axiome comme le premier non-terminal pour Greibach
    strcpy(grammaire_greibach.axiome, grammaire_greibach.rules[0].non_terminal);

    // Générer des mots pour la grammaire en forme normale de Greibach
    printf("\n==== Génération de mots (Greibach) ====");
    generer_mots(&grammaire_greibach,4, "mots_greibach_generes.txt");

    return 0;
}
