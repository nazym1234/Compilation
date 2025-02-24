Le but de ce projet est de programmer différentes transformations entre des grammaires algébriques puis
d’utiliser ces grammaires pour générer des mots de leur langage.
1 Les formes de grammaires algébriques
Dans tout ce document, on note :
— par des lettres majuscules (éventuellement indicées) A, B, S, X, ... les symboles non-terminaux (variables) ;
— par des lettres minuscules a, b, ... les terminaux ;
— par des lettres grecques α, β, ... les mots constitués de terminaux et de non-terminaux. ;
— de plus S est toujours l’axiome.
Les différentes formes normales ne s’appliquent qu’aux grammaires algébriques.
1.1 Forme générale des grammaires algébriques
Un grammaire est algébrique si toutes les règles sont de la forme :
— X → α
Le membre gauche d’une règle est toujours un unique non-terminal et le membre droit un mot quelconque.
Les formes normales ci-dessous donnent des contraintes sur le membre droit.
1
1.2 Forme normale de Greibach
Une grammaire algébrique est sous forme normale de Greibach si toutes les règles sont de la forme :
— X → aA1A2...An avec n ≥ 1
— X → a
— S → ε seulement si ε appartient au langage.
1.3 Forme normale de Chomsky
Une grammaire algébrique est sous forme normale de Chomsky si toutes les règles sont de la forme :
— X → Y Z
— X → a
— S → ε seulement si ε appartient au langage.
2 But du projet
Le but est de lire une grammaire algébrique depuis un fichier et de transformer cette grammaire dans les
formes normales de Greibach et de Chomsky. Puis à partir de chacune de ces nouvelles grammaires donner tous
les mots dont la longueur est inférieure à une longueur donnée.
On a les conventions suivantes :
— L’ensemble des terminaux est l’ensemble des 26 lettres minuscules.
— La lettre E majuscule représente ε.
— Un non-terminal est une des 25 lettres majuscules (toutes sauf E), suivie d’un des 10 chiffres.
Par exemple T8. Il y a donc 250 non terminaux possibles.
— Dans le fichier lu en entrée chaque ligne comprend une règle de la grammaire sous la forme :
membre_gauche : membre_droit. Des espaces peuvent être présents mais doivent être ignorés lors de
l’analyse lexicale de la règle. De plus l’axiome est le membre_gauche de la première règle du fichier.
3 Travail à faire
 trouver (livres, internet) les algos pour transformer une grammaire algébrique quelconque dans les
formes normales. https://fr.wikipedia.org/wiki/Forme_normale_de_Chomsky https://fr.wikipedia.org/
wiki/Forme_normale_de_Greibach peut-être un bon point d’entrée.
4 Code à faire
le programme doit :
— Définir une structure de données pour stocker une grammaire.
— Avoir une fonction lire pour lire un fichier contenant une grammaire et la stocker dans votre structure
de données.
— Avoir des fonctions greibach, chomsky, qui transforment une grammaire algébrique dans les formes normales dont elle ont les noms. Ces fonctions doivent donc générer une structure de données à partir d’une
autre en aucun ne faire de l’affichage (sauf debugage).
— Avoir une fonction ecrire pour écrire une grammaire depuis la structure de donnée vers un fichier.
Le fichier aura comme extension un mot correspondant au nom de la forme normale, comme le nom des
fonctions.
— le programma exécutable devra s’appeler grammaire.
— On doit pouvoir lancer le programme avec la commande : grammaire xxx.general qui doit générer les
fichiers : xxx.greibach et xxx.chomski. xxx pouvant être n’importe quel nom.
— un second programme qui s’appele generer qui prend en argument un fichier contenant une
des formes normales et un entier n et donne en sortie tous les mots de longueur inférieure ou égale à n
générés par la grammaire contenue dans le fichier et triés en ordre lexicographique, un par ligne et sans
espace.
on a aussi  d'autres fonctions utiles à notre programme.
