#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Pour isupper()
#include <stdbool.h>


#define MAX_RULES 100
#define MAX_SYMBOLS 100 // Augmentation de MAX_SYMBOLS si nécessaire
#define MAX_NON_TERMINAUX 250

typedef struct {
    char non_terminal[MAX_SYMBOLS]; // Stocke une chaîne pour le non-terminal
    char productions[MAX_RULES][MAX_SYMBOLS]; // Productions associées
    int production_count; // Nombre de productions
} Rule;

typedef struct {
    Rule rules[MAX_RULES]; // Ensemble des règles
    int rule_count; // Nombre de règles
} Grammaire;

// Fonction pour nettoyer une chaîne de caractères (supprimer les espaces)
void nettoyer_chaine(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src != ' ') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}
// Fonction pour vérifier si un symbole est un non-terminal
int isNonTerminal(const char *symbol) {
    // Vérifie que la chaîne est de longueur 2, le premier caractère est une majuscule
    // et le second est un chiffre.
    return strlen(symbol) == 2 && isupper(symbol[0]) && isdigit(symbol[1]);
}
// Vérifie si un non-terminal existe déjà dans la grammaire
int non_terminal_exists(const Grammaire *grammaire, const char *non_terminal) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        if (strcmp(grammaire->rules[i].non_terminal, non_terminal) == 0) {
            return 1; // Le non-terminal existe
        }
    }
    return 0; // Le non-terminal n'existe pas
}

// Fonction pour générer un nouveau non-terminal unique
void generate_non_terminal(char *result, const Grammaire *grammaire) {
    static int letter_index = 25; // Commencer par 'Z'
    static int number_index = 9;  // Commencer par 9
    int attempts = 0;

    printf("Début : letter_index=%d, number_index=%d\n", letter_index, number_index);

    do {
        snprintf(result, MAX_SYMBOLS, "%c%d", 'A' + letter_index, number_index);

        if (--number_index < 0) {
            number_index = 9;
            if (--letter_index < 0) {
                fprintf(stderr, "Erreur : Limite de non-terminaux atteinte (A0 à Z9 épuisés).\n");
                exit(EXIT_FAILURE);
            }
        }

        attempts++;
        if (attempts > MAX_NON_TERMINAUX) {
            fprintf(stderr, "Erreur : Trop de tentatives pour générer un nouveau non-terminal.\n");
            exit(EXIT_FAILURE);
        }

        printf("Généré : %s\n", result); // Affiche le non-terminal généré
    } while (non_terminal_exists(grammaire, result));

    printf("Fin : letter_index=%d, number_index=%d\n", letter_index, number_index);
}
// Fonction pour lire une grammaire depuis un fichier
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


int prefix_common_length(const char *str1, const char *str2) {
    int len = 0;
    while (str1[len] != '\0' && str2[len] != '\0' && str1[len] == str2[len]) {
        len++;
    }
    return len;
}

// Factoriser deux productions ayant un préfixe commun
int factoriser_productions(char *prod1, char *prod2, Grammaire *grammaire) {
    int prefix_len = prefix_common_length(prod1, prod2);

    if (prefix_len > 0) {
        char new_non_terminal[MAX_SYMBOLS];
        generate_non_terminal(new_non_terminal, grammaire); // Appel mis à jour

        Rule new_rule;
        strcpy(new_rule.non_terminal, new_non_terminal);
        new_rule.production_count = 0;

        // Ajouter les suffixes après le préfixe à la nouvelle règle
        if (strlen(prod1) > prefix_len) {
            strcpy(new_rule.productions[new_rule.production_count++], prod1 + prefix_len);
        } else {
            strcpy(new_rule.productions[new_rule.production_count++], "E");
        }

        if (strlen(prod2) > prefix_len) {
            strcpy(new_rule.productions[new_rule.production_count++], prod2 + prefix_len);
        } else {
            strcpy(new_rule.productions[new_rule.production_count++], "E");
        }

        // Ajouter la nouvelle règle dans la grammaire
        grammaire->rules[grammaire->rule_count++] = new_rule;

        // Mettre à jour prod1 pour inclure uniquement le préfixe + nouveau non-terminal
        snprintf(prod1, MAX_SYMBOLS, "%.*s%s", prefix_len, prod1, new_non_terminal);

        // Effacer prod2 car elle a été intégrée dans la nouvelle règle
        strcpy(prod2, prod1);

        return 1;
    }

    return 0;
}

// Appliquer la factorisation sur une règle
void factoriser_rule(Rule *rule, Grammaire *grammaire) {
    for (int i = 0; i < rule->production_count; i++) {
        for (int j = i + 1; j < rule->production_count; j++) {
            // Tenter de factoriser les deux productions
            if (factoriser_productions(rule->productions[i], rule->productions[j], grammaire)) {
                // Supprimer la production à l'indice `j` car elle a été absorbée
                for (int k = j; k < rule->production_count - 1; k++) {
                    strcpy(rule->productions[k], rule->productions[k + 1]);
                }
                rule->production_count--;
                j--; // Réexaminer la position actuelle
            }
        }
    }
}



// Appliquer la factorisation à toute la grammaire
void factoriser(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        factoriser_rule(rule, grammaire);
    }
}





// Supprimer epsilon
 void supprimer_epsilon(Grammaire *grammaire, const char *axiome) {
    int epsilon_non_terminals[MAX_RULES] = {0};
    int changes;

    // Étape 1 : Identifier les non-terminaux produisant epsilon directement ou indirectement
    do {
        changes = 0;
        for (int i = 0; i < grammaire->rule_count; i++) {
            if (epsilon_non_terminals[i]) continue; // Déjà marqué comme epsilon
            Rule *rule = &grammaire->rules[i];
            for (int j = 0; j < rule->production_count; j++) {
                if (strcmp(rule->productions[j], "E") == 0) {
                    epsilon_non_terminals[i] = 1;
                    changes = 1;
                    break;
                }
                // Vérifier si toutes les parties de la production peuvent produire epsilon
                int all_epsilon = 1;
                for (int k = 0; k < strlen(rule->productions[j]); k += 2) {
                    char non_terminal[3] = {rule->productions[j][k], rule->productions[j][k + 1], '\0'};
                    int found = 0;
                    for (int l = 0; l < grammaire->rule_count; l++) {
                        if (strcmp(grammaire->rules[l].non_terminal, non_terminal) == 0) {
                            if (epsilon_non_terminals[l]) {
                                found = 1;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        all_epsilon = 0;
                        break;
                    }
                }
                if (all_epsilon) {
                    epsilon_non_terminals[i] = 1;
                    changes = 1;
                    break;
                }
            }
        }
    } while (changes);

    // Étape 2 : Ajouter des variantes en remplaçant les epsilon-productions
    do {
        changes = 0; // Réinitialiser l'indicateur de modifications
        for (int i = 0; i < grammaire->rule_count; i++) {
            Rule *rule = &grammaire->rules[i];
            int original_count = rule->production_count;

            for (int j = 0; j < original_count; j++) {
                char *prod = rule->productions[j];

                // Générer toutes les combinaisons en remplaçant les non-terminaux epsilon
                for (int k = 0; k < grammaire->rule_count; k++) {
                    if (epsilon_non_terminals[k]) {
                        char *non_terminal = grammaire->rules[k].non_terminal;
                        char *found = strstr(prod, non_terminal);
                        while (found) {
                            char new_production[MAX_SYMBOLS] = "";

                            // Partie avant le non-terminal
                            strncpy(new_production, prod, found - prod);
                            new_production[found - prod] = '\0';

                            // Partie après le non-terminal
                            strcat(new_production, found + strlen(non_terminal));

                            // Ajouter la nouvelle production si elle n'existe pas déjà
                            int exists = 0;
                            for (int l = 0; l < rule->production_count; l++) {
                                if (strcmp(rule->productions[l], new_production) == 0) {
                                    exists = 1;
                                    break;
                                }
                            }
                            if (!exists && strlen(new_production) > 0) {
                                strcpy(rule->productions[rule->production_count++], new_production);
                                changes = 1; // Une modification a été effectuée
                            }

                            // Chercher la prochaine occurrence
                            found = strstr(found + 1, non_terminal);
                        }
                    }
                }
            }
        }
    } while (changes);

    // Étape 3 : Supprimer explicitement les productions contenant uniquement epsilon
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        if (strcmp(rule->non_terminal, axiome) != 0) {
            for (int j = 0; j < rule->production_count;) {
                if (strcmp(rule->productions[j], "E") == 0) {
                    for (int k = j; k < rule->production_count - 1; k++) {
                        strcpy(rule->productions[k], rule->productions[k + 1]);
                    }
                    rule->production_count--;
                } else {
                    j++;
                }
            }
        }
    }

    // Étape 4 : Supprimer les règles inutiles
    for (int i = 0; i < grammaire->rule_count;) {
        Rule *rule = &grammaire->rules[i];
        if (rule->production_count == 0 && strcmp(rule->non_terminal, axiome) != 0) {
            char non_terminal_to_remove[MAX_SYMBOLS];
            strcpy(non_terminal_to_remove, rule->non_terminal);

            // Supprimer cette règle
            for (int j = i; j < grammaire->rule_count - 1; j++) {
                grammaire->rules[j] = grammaire->rules[j + 1];
            }
            grammaire->rule_count--;

            // Supprimer les références dans les autres règles
            for (int j = 0; j < grammaire->rule_count; j++) {
                Rule *other_rule = &grammaire->rules[j];
                for (int k = 0; k < other_rule->production_count;) {
                    if (strstr(other_rule->productions[k], non_terminal_to_remove)) {
                        for (int l = k; l < other_rule->production_count - 1; l++) {
                            strcpy(other_rule->productions[l], other_rule->productions[l + 1]);
                        }
                        other_rule->production_count--;
                    } else {
                        k++;
                    }
                }
            }
        } else {
            i++;
        }
    }

    // Étape supplémentaire : Ajouter E à l'axiome s'il peut produire epsilon
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        if (strcmp(rule->non_terminal, axiome) == 0) {
            if (epsilon_non_terminals[i]) {
                int already_has_epsilon = 0;
                for (int j = 0; j < rule->production_count; j++) {
                    if (strcmp(rule->productions[j], "E") == 0) {
                        already_has_epsilon = 1;
                        break;
                    }
                }
                if (!already_has_epsilon) {
                    strcpy(rule->productions[rule->production_count++], "E");
                }
            }
        }
    }
}
void nettoyer_grammaire(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        int is_used = 0;
        for (int j = 0; j < grammaire->rule_count; j++) {
            if (i != j) {
                for (int k = 0; k < grammaire->rules[j].production_count; k++) {
                    if (strstr(grammaire->rules[j].productions[k], rule->non_terminal)) {
                        is_used = 1;
                        break;
                    }
                }
            }
            if (is_used) break;
        }
        if (!is_used && rule->production_count == 0) {
            for (int j = i; j < grammaire->rule_count - 1; j++) {
                grammaire->rules[j] = grammaire->rules[j + 1];
            }
            grammaire->rule_count--;
            i--;
        }
    }
}
void supprimer_unite(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        int index = 0;

        // Parcourir les productions
        while (index < rule->production_count) {
            char *prod = rule->productions[index];

            // Vérifier si c'est une règle unité (une lettre majuscule suivie d'un chiffre)
            if (strlen(prod) > 1 && isupper(prod[0]) && isdigit(prod[1])) {
                char target_non_terminal[MAX_SYMBOLS];
                strcpy(target_non_terminal, prod);

                // Trouver la règle associée
                int found = 0;
                for (int j = 0; j < grammaire->rule_count; j++) {
                    if (strcmp(grammaire->rules[j].non_terminal, target_non_terminal) == 0) {
                        Rule *target_rule = &grammaire->rules[j];
                        found = 1;

                        // Ajouter les productions de la règle cible à la règle courante
                        for (int k = 0; k < target_rule->production_count; k++) {
                            char *new_prod = target_rule->productions[k];

                            // Vérifier si la production existe déjà
                            int exists = 0;
                            for (int l = 0; l < rule->production_count; l++) {
                                if (strcmp(rule->productions[l], new_prod) == 0) {
                                    exists = 1;
                                    break;
                                }
                            }

                            // Ajouter la production si elle n'existe pas encore
                            if (!exists && rule->production_count < MAX_RULES) {
                                strcpy(rule->productions[rule->production_count++], new_prod);
                            }
                        }
                        break;
                    }
                }

                // Si la règle associée est trouvée, supprimer la règle unité
                if (found) {
                    for (int k = index; k < rule->production_count - 1; k++) {
                        strcpy(rule->productions[k], rule->productions[k + 1]);
                    }
                    rule->production_count--;
                } else {
                    index++; // Passer à la production suivante si aucune règle associée
                }
            } else {
                index++; // Passer à la production suivante si ce n'est pas une règle unité
            }
        }
    }
}
void supprimer_non_terminaux_en_tete(Grammaire *grammaire) {
    int changes;

    do {
        changes = 0; // Indicateur de modifications

        for (int i = 0; i < grammaire->rule_count; i++) {
            Rule *rule = &grammaire->rules[i];

            for (int j = 0; j < rule->production_count; j++) {
                char *prod = rule->productions[j];

                // Identifier si le premier symbole est un non-terminal valide (Majuscule + Chiffre uniquement)
                if (strlen(prod) > 1 && isupper(prod[0]) && isdigit(prod[1])) {
                    char non_terminal_tete[MAX_SYMBOLS] = {0};

                    // Extraire le non-terminal (ex: "A0")
                    snprintf(non_terminal_tete, 3, "%c%c", prod[0], prod[1]);

                    // Vérifier si ce non-terminal existe dans les règles
                    int found = 0;
                    for (int l = 0; l < grammaire->rule_count; l++) {
                        if (strcmp(grammaire->rules[l].non_terminal, non_terminal_tete) == 0) {
                            found = 1;
                            Rule *target_rule = &grammaire->rules[l];

                            // Remplacer le non-terminal en tête par ses productions
                            for (int m = 0; m < target_rule->production_count; m++) {
                                char nouvelle_production[MAX_SYMBOLS];

                                // Construire la nouvelle production
                                snprintf(nouvelle_production, sizeof(nouvelle_production), "%s%s",
                                         target_rule->productions[m], prod + 2);

                                // Vérifier si cette nouvelle production existe déjà
                                int existe = 0;
                                for (int n = 0; n < rule->production_count; n++) {
                                    if (strcmp(rule->productions[n], nouvelle_production) == 0) {
                                        existe = 1;
                                        break;
                                    }
                                }

                                // Ajouter la nouvelle production si elle n'existe pas
                                if (!existe && rule->production_count < MAX_RULES) {
                                    strcpy(rule->productions[rule->production_count++], nouvelle_production);
                                }
                            }

                            // Supprimer l'ancienne production
                            for (int m = j; m < rule->production_count - 1; m++) {
                                strcpy(rule->productions[m], rule->productions[m + 1]);
                            }
                            rule->production_count--;
                            j--; // Réexaminer la position actuelle après suppression

                            changes = 1; // Indiquer qu'une modification a été effectuée
                            break;
                        }
                    }

                    // Si le non-terminal n'existe pas dans les règles, ce n'est pas une erreur ici
                    if (!found) {
                        continue;
                    }
                }
            }
        }
    } while (changes); // Répéter jusqu'à ce qu'il n'y ait plus de modifications
}
void supprimer_terminaux_non_en_tete(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];

        for (int j = 0; j < rule->production_count; j++) {
            char *prod = rule->productions[j];
            char new_production[MAX_SYMBOLS] = "";
            int changed = 0;

            // Vérifier chaque caractère dans la production
            for (int k = 0; prod[k] != '\0'; k++) {
                if (islower(prod[k]) && k != 0) { // Si un terminal n'est pas en tête
                    // Créer un nouveau non-terminal pour ce terminal
                    char new_non_terminal[MAX_SYMBOLS];
                    generate_non_terminal(new_non_terminal, grammaire);

                    // Ajouter une nouvelle règle pour ce terminal
                    Rule new_rule;
                    strcpy(new_rule.non_terminal, new_non_terminal);
                    new_rule.production_count = 1;
                    snprintf(new_rule.productions[0], MAX_SYMBOLS, "%c", prod[k]);
                    grammaire->rules[grammaire->rule_count++] = new_rule;

                    // Remplacer le terminal par le nouveau non-terminal
                    snprintf(new_production + strlen(new_production), MAX_SYMBOLS - strlen(new_production), "%s", new_non_terminal);
                    changed = 1;
                } else {
                    // Ajouter le caractère original s'il n'est pas modifié
                    snprintf(new_production + strlen(new_production), MAX_SYMBOLS - strlen(new_production), "%c", prod[k]);
                }
            }

            // Mettre à jour la production si des changements ont été effectués
            if (changed) {
                strcpy(rule->productions[j], new_production);
            }
        }
    }
}

int non_terminal_in_rule(const Grammaire *grammaire, const char *non_terminal, const char *rule_production) {
    int len = strlen(rule_production);

    for (int i = 0; i < len; i += 2) {
        if (isupper(rule_production[i]) && isdigit(rule_production[i + 1])) {
            char current_non_terminal[MAX_SYMBOLS];
            snprintf(current_non_terminal, 3, "%c%c", rule_production[i], rule_production[i + 1]);
            if (strcmp(current_non_terminal, non_terminal) == 0) {
                return 1; // Le non-terminal est trouvé dans la production
            }
        }
    }
    return 0; // Non-terminal absent de la production
}
void supprimer_regles_avec_plus_de_deux_non_terminaux(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];

        for (int j = 0; j < rule->production_count; j++) {
            char *prod = rule->productions[j];
            int len = strlen(prod);

            // Compter les non-terminaux dans la production
            int non_terminal_count = 0;
            for (int k = 0; k < len; k += 2) {
                if (isupper(prod[k]) && isdigit(prod[k + 1])) {
                    non_terminal_count++;
                }
            }

            // Si plus de deux non-terminaux, procéder à la décomposition
            if (non_terminal_count > 2) {
                char current_prod[MAX_SYMBOLS];
                strcpy(current_prod, prod); // Copie la production actuelle

                char new_non_terminal[MAX_SYMBOLS];
                char remaining_prod[MAX_SYMBOLS];

                // Initialisation : traiter le premier non-terminal
                strncpy(remaining_prod, current_prod + 2, MAX_SYMBOLS - 2);
                remaining_prod[MAX_SYMBOLS - 2] = '\0';

                do {
                    generate_non_terminal(new_non_terminal, grammaire);
                } while (non_terminal_exists(grammaire, new_non_terminal) ||
                         non_terminal_in_rule(grammaire, new_non_terminal, prod));

                snprintf(rule->productions[j], MAX_SYMBOLS, "%.*s%s", 2, current_prod, new_non_terminal);

                // Créer de nouvelles règles pour gérer le reste
                while (strlen(remaining_prod) > 2) {
                    char first_non_terminal[MAX_SYMBOLS];
                    strncpy(first_non_terminal, remaining_prod, 2);
                    first_non_terminal[2] = '\0';

                    // Générer un nouveau non-terminal
                    char temp_non_terminal[MAX_SYMBOLS];
                    do {
                        generate_non_terminal(temp_non_terminal, grammaire);
                    } while (non_terminal_exists(grammaire, temp_non_terminal) ||
                             non_terminal_in_rule(grammaire, temp_non_terminal, prod));

                    Rule new_rule;
                    strcpy(new_rule.non_terminal, new_non_terminal);
                    snprintf(new_rule.productions[0], MAX_SYMBOLS, "%s%s", first_non_terminal, temp_non_terminal);
                    new_rule.production_count = 1;
                    grammaire->rules[grammaire->rule_count++] = new_rule;

                    strcpy(new_non_terminal, temp_non_terminal);
                    memmove(remaining_prod, remaining_prod + 2, strlen(remaining_prod) - 1);
                }

                // Cas où il reste exactement deux non-terminaux
                if (strlen(remaining_prod) == 2) {
                    Rule final_rule;
                    strcpy(final_rule.non_terminal, new_non_terminal);
                    snprintf(final_rule.productions[0], MAX_SYMBOLS, "%s", remaining_prod);
                    final_rule.production_count = 1;
                    grammaire->rules[grammaire->rule_count++] = final_rule;
                }
            }
        }
    }
}
void supprimer_recursivite_gauche(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule_i = &grammaire->rules[i];

        // Identifier les productions récursives et non récursives
        char recursive_productions[MAX_RULES][MAX_SYMBOLS] = {0};
        char non_recursive_productions[MAX_RULES][MAX_SYMBOLS] = {0};
        int recursive_count = 0, non_recursive_count = 0;

        for (int j = 0; j < rule_i->production_count; j++) {
            if (strncmp(rule_i->productions[j], rule_i->non_terminal, strlen(rule_i->non_terminal)) == 0) {
                // Production récursive
                strcpy(recursive_productions[recursive_count++], rule_i->productions[j] + strlen(rule_i->non_terminal));
            } else {
                // Production non récursive
                strcpy(non_recursive_productions[non_recursive_count++], rule_i->productions[j]);
            }
        }

        // Si aucune récursivité gauche, passer à la règle suivante
        if (recursive_count == 0) {
            continue;
        }

        // Générer un nouveau non-terminal pour gérer la récursivité
        char new_non_terminal[MAX_SYMBOLS];
        generate_non_terminal(new_non_terminal, grammaire);

        // Remplacer les règles de rule_i avec les productions non récursives suivies du nouveau non-terminal
        rule_i->production_count = 0;
        for (int j = 0; j < non_recursive_count; j++) {
            snprintf(rule_i->productions[rule_i->production_count++], MAX_SYMBOLS, "%s%s",
                     non_recursive_productions[j], new_non_terminal);
        }

        // Ajouter les règles pour le nouveau non-terminal
        Rule new_rule;
        strcpy(new_rule.non_terminal, new_non_terminal);
        new_rule.production_count = 0;
        for (int j = 0; j < recursive_count; j++) {
            snprintf(new_rule.productions[new_rule.production_count++], MAX_SYMBOLS, "%s%s",
                     recursive_productions[j], new_non_terminal);
        }
        strcpy(new_rule.productions[new_rule.production_count++], "E"); // Ajout de epsilon

        // Ajouter le nouveau non-terminal à la grammaire
        grammaire->rules[grammaire->rule_count++] = new_rule;
        
    }
    
}


//remplacer axiome du membre droit par un non terminal

void ajouter_regle_pour_axe(const char *axiome, Grammaire *grammaire) {
    // Vérifier si l'axiome est présent dans les membres droits
    int axiome_present = 0;

    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        for (int j = 0; j < rule->production_count; j++) {
            if (strstr(rule->productions[j], axiome)) {
                axiome_present = 1;
                break;
            }
        }
        if (axiome_present) break; // Sortir dès que l'axiome est trouvé
    }

    // Si l'axiome n'est pas présent dans les membres droits, ne rien modifier
    if (!axiome_present) {
        printf("Aucune règle ne contient l'axiome '%s' dans ses membres droits. Pas de modification nécessaire.\n", axiome);
        return;
    }

    // Générer un nouveau non-terminal
    char nouveau_non_terminal[MAX_SYMBOLS];
    generate_non_terminal(nouveau_non_terminal, grammaire);

    // Ajouter une règle qui lie l'axiome au nouveau non-terminal
    Rule nouvelle_regle;
    strcpy(nouvelle_regle.non_terminal, axiome);
    strcpy(nouvelle_regle.productions[0], nouveau_non_terminal);
    nouvelle_regle.production_count = 1;

    if (grammaire->rule_count < MAX_RULES) {
        // Déplacer toutes les règles existantes vers la droite pour insérer la nouvelle règle en première position
        for (int i = grammaire->rule_count; i > 0; i--) {
            grammaire->rules[i] = grammaire->rules[i - 1];
        }
        // Ajouter la nouvelle règle en première position
        grammaire->rules[0] = nouvelle_regle;
        grammaire->rule_count++;
    } else {
        fprintf(stderr, "Erreur : Limite de règles atteinte, impossible d'ajouter la nouvelle règle.\n");
        return;
    }

    // Parcourir toutes les règles pour remplacer les occurrences de l'axiome par le nouveau non-terminal
    for (int i = 1; i < grammaire->rule_count; i++) { // Commence à 1 pour ignorer la règle ajoutée
        Rule *rule = &grammaire->rules[i];

        // Si le non-terminal de la règle est l'axiome, le remplacer par le nouveau non-terminal
        if (strcmp(rule->non_terminal, axiome) == 0) {
            strcpy(rule->non_terminal, nouveau_non_terminal);
        }

        // Parcourir les productions et remplacer chaque occurrence de l'axiome
        for (int j = 0; j < rule->production_count; j++) {
            char *production = rule->productions[j];
            char temp[MAX_SYMBOLS] = "";
            char *pos = production;

            // Remplacer toutes les occurrences de l'axiome par le nouveau non-terminal
            while ((pos = strstr(pos, axiome)) != NULL) {
                // Copier la partie avant l'axiome
                strncat(temp, production, pos - production);
                strcat(temp, nouveau_non_terminal);
                production = pos + strlen(axiome);
                pos = production;
            }

            // Ajouter la partie restante
            strcat(temp, production);
            strcpy(rule->productions[j], temp);
        }
    }
}
int isTerminal(char c) {
    return islower(c);
}
// retirer les terminaux dans le membre droit si la taille du membre droit>=2
// ( donc terminaux non isoles)
void transformRule(Rule *rule, Grammaire *grammaire) {
    for (int i = 0; i < rule->production_count; i++) {
        char *production = rule->productions[i];
        char nouvelle_production[MAX_SYMBOLS] = "";
        int len = strlen(production);

        for (int j = 0; j < len; j++) {
            if (isTerminal(production[j]) && len > 1) {
                // Remplacer tous les terminaux dans une production de taille > 1
                char nouveau_non_terminal[MAX_SYMBOLS];
                generate_non_terminal(nouveau_non_terminal, grammaire);

                // Créer une nouvelle règle associant le terminal au non-terminal
                Rule nouvelle_regle;
                strcpy(nouvelle_regle.non_terminal, nouveau_non_terminal);
                nouvelle_regle.production_count = 1;
                snprintf(nouvelle_regle.productions[0], MAX_SYMBOLS, "%c", production[j]);

                // Ajouter la règle à la grammaire
                if (grammaire->rule_count < MAX_RULES) {
                    grammaire->rules[grammaire->rule_count++] = nouvelle_regle;
                } else {
                    fprintf(stderr, "Erreur : Limite de règles atteinte, impossible d'ajouter une nouvelle règle.\n");
                    return;
                }

                // Remplacer le terminal par le nouveau non-terminal dans la production
                strcat(nouvelle_production, nouveau_non_terminal);
            } else {
                // Garder les non-terminaux ou les terminaux isolés
                char temp[2] = {production[j], '\0'};
                strcat(nouvelle_production, temp);
            }
        }

        // Remplacer l'ancienne production par la nouvelle
        strcpy(rule->productions[i], nouvelle_production);
    }
}

void transform(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        transformRule(&grammaire->rules[i], grammaire);
    }
}
// Afficher la grammaire
void afficher_grammaire(Grammaire *grammaire) {
    printf("Grammaire:\n");
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        printf("%s -> ", rule->non_terminal);
        for (int j = 0; j < rule->production_count; j++) {
            printf("%s", rule->productions[j]);
            if (j < rule->production_count - 1) printf(" | ");
        }
        printf("\n");
    }
}
void regrouper_terminaux(Grammaire *grammaire) {
    char terminal_to_non_terminal[128][MAX_SYMBOLS] = {{0}}; // Associer chaque terminal à un unique non-terminal
    char non_terminals_to_replace[MAX_RULES][MAX_SYMBOLS];   // Liste des anciens non-terminaux à remplacer
    char terminal_for_non_terminal[MAX_RULES][2] = {{0}};    // Terminal associé à chaque ancien non-terminal
    int replace_count = 0;                                  // Compteur des non-terminaux à remplacer
    Grammaire updated_grammaire = *grammaire;               // Copie pour modification

    // Étape 1 : Identifier les terminaux similaires et créer un unique non-terminal pour chaque terminal
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        if (rule->production_count == 1 && strlen(rule->productions[0]) == 1 && islower(rule->productions[0][0])) {
            char terminal = rule->productions[0][0];

            // Vérifier si un non-terminal existe déjà pour ce terminal
            if (strlen(terminal_to_non_terminal[(int)terminal]) == 0) {
                // Générer un nouveau non-terminal
                char new_non_terminal[MAX_SYMBOLS];
                generate_non_terminal(new_non_terminal, &updated_grammaire);

                // Associer ce non-terminal au terminal
                strcpy(terminal_to_non_terminal[(int)terminal], new_non_terminal);

                // Ajouter une règle pour ce terminal
                Rule new_rule;
                strcpy(new_rule.non_terminal, new_non_terminal);
                snprintf(new_rule.productions[0], MAX_SYMBOLS, "%c", terminal);
                new_rule.production_count = 1;
                updated_grammaire.rules[updated_grammaire.rule_count++] = new_rule;
            }

            // Enregistrer l'ancien non-terminal à remplacer
            strcpy(non_terminals_to_replace[replace_count], rule->non_terminal);
            terminal_for_non_terminal[replace_count][0] = terminal;
            replace_count++;
        }
    }

    // Étape 2 : Supprimer les anciennes règles redondantes
    Grammaire temp_grammaire = {0};
    for (int i = 0; i < updated_grammaire.rule_count; i++) {
        Rule *rule = &updated_grammaire.rules[i];

        // Vérifier si c'est une règle redondante à supprimer
        int skip = 0;
        for (int j = 0; j < replace_count; j++) {
            if (strcmp(rule->non_terminal, non_terminals_to_replace[j]) == 0) {
                skip = 1;
                break;
            }
        }

        if (!skip) {
            temp_grammaire.rules[temp_grammaire.rule_count++] = *rule;
        }
    }

    updated_grammaire = temp_grammaire;

    // Étape 3 : Mettre à jour toutes les règles avec les nouveaux non-terminaux
    for (int i = 0; i < updated_grammaire.rule_count; i++) {
        Rule *rule = &updated_grammaire.rules[i];
        for (int j = 0; j < rule->production_count; j++) {
            char *prod = rule->productions[j];
            for (int k = 0; k < replace_count; k++) {
                if (strcmp(prod, non_terminals_to_replace[k]) == 0) {
                    strcpy(prod, terminal_to_non_terminal[(int)terminal_for_non_terminal[k][0]]);
                }
            }
        }
    }

    // Étape 4 : Remplacer les anciens non-terminaux dans toutes les productions où ils apparaissent
    for (int i = 0; i < updated_grammaire.rule_count; i++) {
        Rule *rule = &updated_grammaire.rules[i];
        for (int j = 0; j < rule->production_count; j++) {
            char *prod = rule->productions[j];
            for (int k = 0; k < replace_count; k++) {
                // Si le non-terminal à remplacer est dans une production
                char *found = strstr(prod, non_terminals_to_replace[k]);
                if (found) {
                    // Remplacer par le nouveau non-terminal
                    strncpy(found, terminal_to_non_terminal[(int)terminal_for_non_terminal[k][0]], strlen(non_terminals_to_replace[k]));
                }
            }
        }
    }

    // Mise à jour finale de la grammaire
    *grammaire = updated_grammaire;
}
void sauvegarder_grammaire(const Grammaire *grammaire, const char *nom_base, char c) {
    // Construire le nom du fichier en fonction du caractère c
    char nom_fichier[MAX_SYMBOLS];
    if (c == 'c') {
        snprintf(nom_fichier, sizeof(nom_fichier), "%s.chomsky", nom_base);
    } else if (c == 'g') {
        snprintf(nom_fichier, sizeof(nom_fichier), "%s.greibach", nom_base);
    } else {
        printf("Erreur : caractère non valide. Utilisez 'c' ou 'g'.\n");
        return;
    }

    // Ouvrir le fichier en mode écriture
    FILE *fichier = fopen(nom_fichier, "w");
    if (!fichier) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    // Parcourir les règles de la grammaire
    for (int i = 0; i < grammaire->rule_count; i++) {
        const Rule *rule = &grammaire->rules[i];

        // Écrire le non-terminal
        fprintf(fichier, "%s : ", rule->non_terminal);

        // Écrire les productions séparées par " | "
        for (int j = 0; j < rule->production_count; j++) {
            fprintf(fichier, "%s", rule->productions[j]);
            if (j < rule->production_count - 1) {
                fprintf(fichier, " | ");
            }
        }

        // Fin de ligne pour la règle
        fprintf(fichier, "\n");
    }

    // Fermer le fichier
    fclose(fichier);
    printf("Grammaire sauvegardée dans le fichier '%s'.\n", nom_fichier);
}
bool est_majuscule_ou_minuscule(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

void supprimer_E_non_isole(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];
        
        // Parcours de toutes les productions de chaque règle
        for (int j = 0; j < rule->production_count; j++) {
            char *production = rule->productions[j];
            int len = strlen(production);
            
            for (int i = 0; i < len; i++) {
                // On cherche les 'E' dans la production
                if (production[i] == 'E') {
                    // Vérifier si E est précédé ou suivi d'une majuscule ou minuscule
                    if ((i > 0 && est_majuscule_ou_minuscule(production[i - 1])) || 
                        (i < len - 1 && est_majuscule_ou_minuscule(production[i + 1]))) {
                        
                        // Supprimer le 'E' de la production
                        printf("Avant suppression de E : %s\n", production);
                        // Décaler le reste de la chaîne après le 'E'
                        memmove(production + i, production + i + 1, len - i);
                        printf("Après suppression de E : %s\n", production);
                        break; // Passer à la production suivante après modification
                    }
                }
            }
        }
    }
}
void transformer_en_chomsky(Grammaire *grammaire, const char *axiome) {
    printf("Début de la transformation en forme normale de Chomsky\n");

    printf("\nÉtape 1 : Supprimer la récursivité gauche\n");
    supprimer_recursivite_gauche(grammaire);
    afficher_grammaire(grammaire);
    printf("\n==== Suppression de E-non-isolé si il existe\n");
    supprimer_E_non_isole(grammaire);
    afficher_grammaire(grammaire);
    
    printf("\nÉtape 2 : factoriser\n");
    factoriser(grammaire);
    afficher_grammaire(grammaire);
    printf("Étape 3 : Retirer l'axiome des membres droits\n");
    ajouter_regle_pour_axe(axiome, grammaire);
    afficher_grammaire(grammaire); 

    printf("\nÉtape 4 : Supprimer les terminaux dans le membre droit des règles de longueur au moins deux\n");
    transform(grammaire);
    afficher_grammaire(grammaire);

   

    printf("\nÉtape 5 : Supprimer les règles avec plus de deux non-terminaux\n");
    supprimer_regles_avec_plus_de_deux_non_terminaux(grammaire);
    afficher_grammaire(grammaire);

    printf("\nÉtape 6 : Supprimer les règles X → ε sauf si X est l'axiome\n");
    supprimer_epsilon(grammaire, axiome);
    afficher_grammaire(grammaire);

    printf("\nÉtape 7 : Supprimer les règles unité X → Y\n");
    supprimer_unite(grammaire);
    afficher_grammaire(grammaire);

    printf("\nÉtape 8 : nettoyer la grammaire\n");
     regrouper_terminaux(grammaire);
    afficher_grammaire(grammaire);

    printf("Fin de la transformation en forme normale de Chomsky\n");
    afficher_grammaire(grammaire);
}

void greibach(Grammaire *grammaire, const char *axiome) {
    // Étape 0 : Factoriser les règles (simplification préalable)
    printf("==== Application de la factorisation ====\n");
    factoriser(grammaire);
    afficher_grammaire(grammaire);

    // Étape 2 : Supprimer la récursivité gauche
    printf("\n==== Suppression de la récursivité gauche ====\n");
    supprimer_recursivite_gauche(grammaire);
    afficher_grammaire(grammaire);
    printf("\n==== Suppression de E-non-isolé si il existe\n");
    supprimer_E_non_isole(grammaire);
    afficher_grammaire(grammaire);
    // Étape 3 : Ajouter une règle pour l’axiome
    printf("\n==== Ajout de la règle pour l'axiome ====\n");
    ajouter_regle_pour_axe((char *)axiome, grammaire);
    afficher_grammaire(grammaire);

    // Étape 4 : Supprimer les règles epsilon
    printf("\n==== Suppression des règles epsilon ====\n");
    supprimer_epsilon(grammaire, axiome);
    afficher_grammaire(grammaire);

    // Étape 5 : Supprimer les règles unité
    printf("\n==== Suppression des règles unité ====\n");
    supprimer_unite(grammaire);
    afficher_grammaire(grammaire);


    // Étape 6 : Supprimer les non-terminaux en tête
    printf("\n==== Suppression des non-terminaux en tête des règles ====\n");
    supprimer_non_terminaux_en_tete(grammaire);
    afficher_grammaire(grammaire);

    // Étape 7 : Supprimer les terminaux qui ne sont pas en tête
    printf("\n==== Suppression des terminaux non en tête ====\n");
    supprimer_terminaux_non_en_tete(grammaire);
    afficher_grammaire(grammaire);

    // Étape 8 : nettoyer la grammaire 
    printf("\n==== nettoyer la grammaire ====\n");
    regrouper_terminaux(grammaire);
    printf("\nTransformation en forme normale de Greibach terminée.\n");
    afficher_grammaire(grammaire);
}

// Fonction pour vérifier si la grammaire est sous la forme normale de Chomsky
int isChomsky(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];

        for (int j = 0; j < rule->production_count; j++) {
            const char *production = rule->productions[j];
            printf("Vérification de la production : %s\n", production);

            // Cas 1 : Un seul terminal
            if (strlen(production) == 1 && isTerminal(production[0])) {
                printf("Valide : terminal isolé\n");
                continue;
            }

            // Cas 2 : Deux non-terminaux
            if (strlen(production) == 4) { // Longueur 4, ex : "Y8Z0"
                char left[3] = {production[0], production[1], '\0'}; // Premier non-terminal : Y8
                char right[3] = {production[2], production[3], '\0'}; // Second non-terminal : Z0

                if (isNonTerminal(left) && isNonTerminal(right)) {
                    printf("Valide : deux non-terminaux (%s et %s)\n", left, right);
                    continue;
                } else {
                    if (!isNonTerminal(left)) {
                        printf("Non valide : %s n'est pas un non-terminal valide.\n", left);
                    }
                    if (!isNonTerminal(right)) {
                        printf("Non valide : %s n'est pas un non-terminal valide.\n", right);
                    }
                }
            }

            // Cas 3 : Axiome produisant epsilon
            if (strcmp(production, "E") == 0 &&
                strcmp(rule->non_terminal, grammaire->rules[0].non_terminal) == 0) {
                printf("Valide : epsilon pour l'axiome\n");
                continue;
            }

            printf("Non valide : La production %s ne respecte pas CNF.\n", production);
            return 0;
        }
    }
    return 1; // Toutes les règles sont valides
}
int isGreibach(Grammaire *grammaire) {
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];

        for (int j = 0; j < rule->production_count; j++) {
            const char *production = rule->productions[j];
            printf("Vérification de la production : %s\n", production);

            // Cas 1 : Axiome produisant epsilon
            if (strcmp(production, "E") == 0 &&
                strcmp(rule->non_terminal, grammaire->rules[0].non_terminal) == 0) {
                printf("Valide : epsilon pour l'axiome\n");
                continue;
            }

            // Cas 2 : La production commence par un terminal
            if (isTerminal(production[0])) {
                int valide = 1;

                // Vérifier que les symboles suivants sont des non-terminaux
                for (int k = 1; k < strlen(production); k += 2) {
                    char symbol[3] = {production[k], production[k + 1], '\0'};
                    if (!isNonTerminal(symbol)) {
                        valide = 0;
                        break;
                    }
                }

                if (valide) {
                    printf("Valide : commence par un terminal suivi de non-terminaux\n");
                    continue;
                }
            }

            // Si aucune condition n'est remplie, la production n'est pas valide
            printf("Non valide : La production %s ne respecte pas GNF.\n", production);
            return 0;
        }
    }
    return 1; // Toutes les productions respectent GNF
}

// Fonction pour ajouter une production à une règle, en évitant les doublons
void ajouter_production(Rule *rule, const char *production) {
    for (int i = 0; i < rule->production_count; i++) {
        if (strcmp(rule->productions[i], production) == 0) {
            return; // Production déjà présente, on ne l'ajoute pas
        }
    }
    strcpy(rule->productions[rule->production_count++], production);
}

// Fonction pour réécrire la grammaire sous la forme avec "|" entre les productions pour chaque non-terminal
void rewriter_grammaire(Grammaire *grammaire) {
    // Création d'un tableau pour stocker les nouvelles règles
    Grammaire nouvelle_grammaire;
    nouvelle_grammaire.rule_count = 0;

    // Parcours de chaque règle
    for (int i = 0; i < grammaire->rule_count; i++) {
        Rule *rule = &grammaire->rules[i];

        // Chercher si une règle avec le même non-terminal existe déjà dans la nouvelle grammaire
        int found = 0;
        for (int j = 0; j < nouvelle_grammaire.rule_count; j++) {
            if (strcmp(nouvelle_grammaire.rules[j].non_terminal, rule->non_terminal) == 0) {
                // Ajouter toutes les productions de cette règle à la règle correspondante
                for (int k = 0; k < rule->production_count; k++) {
                    ajouter_production(&nouvelle_grammaire.rules[j], rule->productions[k]);
                }
                found = 1;
                break;
            }
        }

        // Si la règle n'a pas encore été ajoutée, on l'ajoute à la nouvelle grammaire
        if (!found) {
            strcpy(nouvelle_grammaire.rules[nouvelle_grammaire.rule_count].non_terminal, rule->non_terminal);
            nouvelle_grammaire.rules[nouvelle_grammaire.rule_count].production_count = 0;

            for (int k = 0; k < rule->production_count; k++) {
                ajouter_production(&nouvelle_grammaire.rules[nouvelle_grammaire.rule_count], rule->productions[k]);
            }

            nouvelle_grammaire.rule_count++;
        }
    }

    // Copier la nouvelle grammaire dans la grammaire d'origine
    *grammaire = nouvelle_grammaire;
}

int main() {
    Grammaire grammaire_originale;

    // Lire la grammaire depuis un fichier
    if (lire_grammaire(&grammaire_originale, "exemple.general.txt") == -1) {
        fprintf(stderr, "Erreur : Impossible de lire la grammaire.\n");
        return -1;
    }

    const char *axiome = grammaire_originale.rules[0].non_terminal;

    // Affichage initial de la grammaire
    printf("Grammaire originale :\n");
    afficher_grammaire(&grammaire_originale);

    // Affichage avant réécriture
    printf("Avant réécriture:\n");
    afficher_grammaire(&grammaire_originale);

    // Réécrire la grammaire
    rewriter_grammaire(&grammaire_originale);

    // Affichage après réécriture
    printf("\nAprès réécriture:\n");
    afficher_grammaire(&grammaire_originale); 

    // Créer une copie de la grammaire originale pour chaque transformation
    Grammaire grammaire_greibach = grammaire_originale;
    Grammaire grammaire_chomsky = grammaire_originale;

    // Transformation en forme normale de Greibach
    printf("\n==== Transformation en forme normale de Greibach ====\n");
    greibach(&grammaire_greibach, axiome);
    sauvegarder_grammaire(&grammaire_greibach, "exemple.Transforme", 'g');
    if (isGreibach(&grammaire_greibach)) {
        printf("La grammaire est sous forme normale de Greibach.\n");
    } else {
        printf("La grammaire n'est PAS sous forme normale de Greibach.\n");
    }

    // Transformation en forme normale de Chomsky
    printf("\n==== Transformation en forme normale de Chomsky ====\n");
    transformer_en_chomsky(&grammaire_chomsky, axiome);
    sauvegarder_grammaire(&grammaire_chomsky, "exemple.Transforme", 'c');
    if (isChomsky(&grammaire_chomsky)) {
        printf("La grammaire est en forme de Chomsky.\n");
    } else {
        printf("La grammaire n'est PAS en forme de Chomsky.\n");
    }
    

    return 0;
    
}