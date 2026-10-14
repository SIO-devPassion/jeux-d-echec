/* ===== LES BIBLIOTHEQUES =====
   Une bibliotheque est une boite a outils qu'on ajoute a son programme. */
#include <stdio.h>    // pour afficher (printf) et lire au clavier (fgets)
#include <string.h>   // pour comparer des textes (strncmp)
#include <ctype.h>    // pour manipuler les lettres (tolower)


/* ===== Jeu d'echecs simplifie =====
   Majuscule = pieces blanches, minuscule = pieces noires, '.' = case vide.
   Lettres : p pion, r tour, n cavalier, b fou, q dame, k roi. */


/* ===== LES VARIABLES GLOBALES =====
   Une variable globale est declaree en dehors des fonctions :
   toutes les fonctions du programme peuvent la lire et la modifier. */

// Le plateau : un tableau a 2 dimensions de 8 lignes et 8 colonnes.
// Chaque case contient UN caractere (une lettre de piece ou un point).
// plateau[0][0] est la case en haut a gauche (a8), plateau[7][7] celle en bas a droite (h1).
char plateau[8][8];

// Le joueur dont c'est le tour : 1 = blancs, 2 = noirs
int joueur = 1;

// Le coup tape par le joueur, transforme en numeros de lignes et de colonnes :
// dep_l / dep_c = ligne et colonne de DEPART, arr_l / arr_c = ligne et colonne d'ARRIVEE
int dep_l, dep_c, arr_l, arr_c;


/* ---------- 1. Petites fonctions utiles ---------- */

// Renvoie 1 (vrai) si le caractere est une majuscule, donc une piece blanche.
// Les lettres majuscules vont de 'A' a 'Z' dans la table des caracteres.
// Le && veut dire "ET" : les deux conditions doivent etre vraies.
int est_blanc(char piece) { return piece >= 'A' && piece <= 'Z'; }

// Pareil mais pour une minuscule, donc une piece noire
int est_noir(char piece)  { return piece >= 'a' && piece <= 'z'; }

// Renvoie la valeur sans le signe moins : -3 devient 3, 5 reste 5.
// Utile pour mesurer une distance sans se soucier de la direction.
int valeur_absolue(int x) {
    if (x < 0) return -x;   // si le nombre est negatif, on change son signe
    return x;               // sinon on le renvoie tel quel
}


// Vrai si la piece est a nous (camp 1 = blancs, camp 2 = noirs)
int appartient_au_joueur(char piece, int camp) {
    if (camp == 1) return est_blanc(piece);   // le joueur 1 possede les majuscules
    return est_noir(piece);                   // sinon le joueur 2 possede les minuscules
}

// Vrai si les deux pieces sont de camps differents
// || veut dire "OU" : il suffit qu'une des deux situations soit vraie
int sont_ennemies(char a, char b) {
    return (est_blanc(a) && est_noir(b)) || (est_noir(a) && est_blanc(b));
}


/* ---------- 2. Preparer et afficher le plateau ---------- */

// Met toutes les pieces a leur place de depart
void initialiser_plateau() {
    // Chaque texte est une ligne du plateau (8 lettres + 1 case cachee pour la fin du texte).
    // La premiere ligne = rangee 8 (pieces noires), la derniere = rangee 1 (pieces blanches).
    char depart[8][9] = {
        "rnbqkbnr", "pppppppp", "........", "........",
        "........", "........", "PPPPPPPP", "RNBQKBNR"
    };
    // On recopie le modele "depart" dans le vrai plateau, case par case.
    // La boucle exterieure (l) parcourt les lignes, l'interieure (c) les colonnes.
    for (int l = 0; l < 8; l++)
        for (int c = 0; c < 8; c++)
            plateau[l][c] = depart[l][c];
}


// Dessine le plateau a l'ecran
void afficher_plateau() {
    printf("\n    a b c d e f g h\n\n");        // les lettres des colonnes en haut (\n = retour a la ligne)
    for (int l = 0; l < 8; l++) {              // pour chaque ligne du plateau
        printf(" %d  ", 8 - l);                // numero de rangee a gauche (la ligne 0 est la rangee 8)
        for (int c = 0; c < 8; c++)            // pour chaque colonne de cette ligne
            printf("%c ", plateau[l][c]);      // on affiche la piece suivie d'un espace
        printf(" %d\n", 8 - l);                // numero de rangee a droite, puis retour a la ligne
    }
    printf("\n    a b c d e f g h\n\n");        // les lettres des colonnes en bas
}


/* ---------- 3. Comment bougent les pieces ---------- */

// Toutes les cases entre le depart et l'arrivee sont-elles vides ?
// (l1,c1) = case de depart, (l2,c2) = case d'arrivee. Sert pour tour, fou et dame.
int chemin_libre(int l1, int c1, int l2, int c2) {
    // pas_l et pas_c disent dans quelle direction on avance a chaque etape :
    // 1 = vers le bas / la droite, -1 = vers le haut / la gauche, 0 = on ne bouge pas
    int pas_l = 0, pas_c = 0;
    if (l2 > l1) pas_l = 1;
    if (l2 < l1) pas_l = -1;
    if (c2 > c1) pas_c = 1;
    if (c2 < c1) pas_c = -1;


    // On commence a la case juste APRES le depart
    int l = l1 + pas_l, c = c1 + pas_c;
    // On avance case par case jusqu'a arriver a la case d'arrivee (non incluse)
    while (l != l2 || c != c2) {
        if (plateau[l][c] != '.') return 0;   // une piece bloque le passage : chemin non libre
        l = l + pas_l;                        // on avance d'une case
        c = c + pas_c;
    }
    return 1;   // on n'a rien rencontre : le chemin est libre
}


// La tour se deplace en ligne droite (horizontalement ou verticalement)
int mouvement_tour(int l1, int c1, int l2, int c2) {
    if (l1 != l2 && c1 != c2) return 0;       // si la ligne ET la colonne changent, ce n'est pas droit
    return chemin_libre(l1, c1, l2, c2);      // sinon il faut que rien ne bloque
}


// Le fou se deplace en diagonale
int mouvement_fou(int l1, int c1, int l2, int c2) {
    // En diagonale, on avance du meme nombre de cases en ligne et en colonne
    if (valeur_absolue(l2 - l1) != valeur_absolue(c2 - c1)) return 0;
    return chemin_libre(l1, c1, l2, c2);
}


// Le cavalier saute en "L" : 2 cases dans un sens et 1 case dans l'autre
int mouvement_cavalier(int l1, int c1, int l2, int c2) {
    int dl = valeur_absolue(l2 - l1);   // distance en lignes
    int dc = valeur_absolue(c2 - c1);   // distance en colonnes
    // Il saute par-dessus les pieces, donc pas besoin de verifier le chemin
    return (dl == 2 && dc == 1) || (dl == 1 && dc == 2);
}


// Le roi avance d'une seule case dans n'importe quelle direction
int mouvement_roi(int l1, int c1, int l2, int c2) {
    return valeur_absolue(l2 - l1) <= 1 && valeur_absolue(c2 - c1) <= 1;
}


// Le pion a des regles particulieres : il avance tout droit mais prend en diagonale
int mouvement_pion(int l1, int c1, int l2, int c2) {
    char piece = plateau[l1][c1];   // la piece qui bouge
    char cible = plateau[l2][c2];   // ce qu'il y a sur la case d'arrivee
    // "sens" : direction de marche. Les blancs montent (le numero de ligne diminue : -1).
    // "ligne_depart" : la ligne ou se trouvent les pions au debut de la partie.
    int sens = -1, ligne_depart = 6;          // les blancs montent
    if (est_noir(piece)) { sens = 1; ligne_depart = 1; }   // les noirs descendent


    // avancer d'une case : meme colonne, une ligne plus loin, case d'arrivee vide
    if (c1 == c2 && l2 == l1 + sens && cible == '.') return 1;
    // avancer de deux cases depuis la ligne de depart (les deux cases doivent etre vides)
    if (c1 == c2 && l1 == ligne_depart && l2 == l1 + 2 * sens
        && cible == '.' && plateau[l1 + sens][c1] == '.') return 1;
    // prendre en diagonale : une colonne a cote, une ligne plus loin, et il y a une piece a prendre
    if (valeur_absolue(c2 - c1) == 1 && l2 == l1 + sens && cible != '.') return 1;
    return 0;   // aucun des cas autorises : mouvement interdit
}


// La piece peut-elle aller de (l1,c1) a (l2,c2) ? (sans penser a l'echec)
int mouvement_possible(int l1, int c1, int l2, int c2) {
    char piece = plateau[l1][c1];   // la piece qu'on veut bouger
    char cible = plateau[l2][c2];   // le contenu de la case d'arrivee
    if (l1 == l2 && c1 == c2) return 0;                          // il faut bouger
    if (cible != '.' && !sont_ennemies(piece, cible)) return 0;  // pas sur sa propre piece


    // tolower met la lettre en minuscule : 'N' et 'n' deviennent 'n'.
    // Comme ca on identifie le type de piece sans se soucier de sa couleur.
    char type = tolower(piece);
    if (type == 'p') return mouvement_pion(l1, c1, l2, c2);
    if (type == 'r') return mouvement_tour(l1, c1, l2, c2);
    if (type == 'b') return mouvement_fou(l1, c1, l2, c2);
    if (type == 'n') return mouvement_cavalier(l1, c1, l2, c2);
    // la dame = une tour + un fou
    if (type == 'q') return mouvement_tour(l1, c1, l2, c2) || mouvement_fou(l1, c1, l2, c2);
    if (type == 'k') return mouvement_roi(l1, c1, l2, c2);
    return 0;   // case vide ou caractere inconnu : pas de mouvement
}


/* ---------- 4. Echec, coups legaux, fin de partie ---------- */

// Le roi de ce camp est-il attaque ? (1 = oui, 0 = non)
int roi_en_echec(int camp) {
    char roi = 'K';              // le roi blanc s'ecrit K...
    if (camp == 2) roi = 'k';    // ...et le roi noir s'ecrit k


    int lr = -1, cr = -1;                     // 1) on cherche le roi (-1 = pas encore trouve)
    for (int l = 0; l < 8; l++)
        for (int c = 0; c < 8; c++)
            if (plateau[l][c] == roi) { lr = l; cr = c; }   // on note sa position
    if (lr == -1) return 0;                   // pas de roi sur le plateau : pas d'echec


    int ennemi = 3 - camp;                    // 2) une piece ennemie peut-elle le prendre ?
                                              // (3 - 1 = 2 et 3 - 2 = 1 : on obtient l'autre camp)
    for (int l = 0; l < 8; l++)
        for (int c = 0; c < 8; c++)
            // si c'est une piece ennemie ET qu'elle peut aller sur la case du roi : echec !
            if (appartient_au_joueur(plateau[l][c], ennemi) && mouvement_possible(l, c, lr, cr))
                return 1;
    return 0;   // personne ne peut prendre le roi
}


// Le coup est legal s'il respecte les regles ET ne laisse pas notre roi en echec
int coup_legal(int l1, int c1, int l2, int c2) {
    char piece = plateau[l1][c1];
    if (!appartient_au_joueur(piece, joueur)) return 0;   // pas notre piece
    if (!mouvement_possible(l1, c1, l2, c2)) return 0;    // mouvement interdit


    // On joue le coup "pour du faux", on regarde si notre roi est en danger, puis on annule.
    char ancienne_cible = plateau[l2][c2];    // on memorise ce qu'il y avait a l'arrivee
    plateau[l2][c2] = piece;                  // on essaie le coup...
    plateau[l1][c1] = '.';
    int echec = roi_en_echec(joueur);         // ...notre roi est-il attaque ?
    plateau[l1][c1] = piece;                  // ...et on annule l'essai (la piece revient)
    plateau[l2][c2] = ancienne_cible;         // (et la piece prise aussi)
    return !echec;                            // legal seulement s'il n'y a PAS d'echec
}


// Joue vraiment le coup sur le plateau (on suppose qu'il est deja verifie)
void jouer_coup(int l1, int c1, int l2, int c2) {
    plateau[l2][c2] = plateau[l1][c1];   // la piece arrive sur la case d'arrivee (elle ecrase une eventuelle piece prise)
    plateau[l1][c1] = '.';               // la case de depart devient vide
    // promotion : un pion arrive au bout devient une dame
    if (plateau[l2][c2] == 'P' && l2 == 0) plateau[l2][c2] = 'Q';   // pion blanc arrive en rangee 8
    if (plateau[l2][c2] == 'p' && l2 == 7) plateau[l2][c2] = 'q';   // pion noir arrive en rangee 1
}


// Existe-t-il au moins un coup legal pour le joueur ?
int a_un_coup_legal() {
    // On teste TOUTES les cases de depart possibles (l1, c1)
    // avec TOUTES les cases d'arrivee possibles (l2, c2).
    for (int l1 = 0; l1 < 8; l1++)
        for (int c1 = 0; c1 < 8; c1++)
            for (int l2 = 0; l2 < 8; l2++)
                for (int c2 = 0; c2 < 8; c2++)
                    if (coup_legal(l1, c1, l2, c2)) return 1;   // on en a trouve un : on s'arrete
    return 0;   // aucun coup possible
}


// Annonce mat ou pat si la partie est finie (renvoie 1), sinon renvoie 0
int partie_terminee() {
    if (a_un_coup_legal()) return 0;     // il reste un coup : la partie continue
    // Plus aucun coup possible : soit mat, soit pat
    if (roi_en_echec(joueur)) {          // roi attaque et aucun coup : echec et mat
        if (joueur == 1) printf("Echec et mat ! Les noirs gagnent.\n");
        else printf("Echec et mat ! Les blancs gagnent.\n");
    } else {                             // roi pas attaque mais aucun coup : pat (match nul)
        printf("Pat : partie nulle.\n");
    }
    return 1;   // la partie est finie
}


/* ---------- 5. Lire le coup tape par le joueur ---------- */

// Transforme "e2e4" en 4 nombres (dep_l, dep_c, arr_l, arr_c). Renvoie 1 si correct.
int lire_coup(char texte[]) {
    // On decoupe le texte tape : 1er caractere = colonne de depart, 2e = rangee de depart, etc.
    char colonne1 = texte[0], rangee1 = texte[1];
    char colonne2 = texte[2], rangee2 = texte[3];
    // Les colonnes doivent etre des lettres de 'a' a 'h'
    if (colonne1 < 'a' || colonne1 > 'h' || colonne2 < 'a' || colonne2 > 'h') return 0;
    // Les rangees doivent etre des chiffres de '1' a '8'
    if (rangee1 < '1' || rangee1 > '8' || rangee2 < '1' || rangee2 > '8') return 0;


    // On convertit les caracteres en numeros utilisables dans le tableau
    dep_c = colonne1 - 'a';          // 'e' - 'a' = 4 (les lettres se suivent dans la table des caracteres)
    dep_l = 8 - (rangee1 - '0');     // '2' - '0' = 2, puis 8 - 2 = 6 : la rangee 2 est la ligne 6
    arr_c = colonne2 - 'a';
    arr_l = 8 - (rangee2 - '0');
    return 1;   // le format est correct
}


/* ---------- 6. Le programme principal ---------- */

// main est la fonction qui demarre en premier : c'est le point d'entree du programme
int main() {
    char entree[100] = "";      // un tableau de 100 caracteres pour stocker ce que le joueur tape
    initialiser_plateau();      // on place les pieces
    printf("=== Jeu d'echecs simplifie ===\n");
    printf("Tapez un coup comme e2e4 (ou quitter).\n");


    // Boucle infinie (while (1) est toujours vrai) : un tour de jeu par passage.
    // On en sort uniquement avec "break".
    while (1) {
        afficher_plateau();
        if (partie_terminee()) break;                  // mat ou pat : on sort de la boucle
        if (roi_en_echec(joueur)) printf("Echec !\n"); // on previent le joueur si son roi est menace


        // On affiche a qui c'est de jouer
        if (joueur == 1) printf("Blancs : ");
        else printf("Noirs : ");
        // fgets lit une ligne tapee au clavier. Si la lecture echoue (NULL), on arrete.
        if (fgets(entree, 100, stdin) == NULL) break;
        // strncmp compare les 7 premiers caracteres : si le joueur a tape "quitter", on arrete (0 = identique)
        if (strncmp(entree, "quitter", 7) == 0) break;


        // Si le format n'est pas bon, on previent et "continue" recommence la boucle (meme joueur)
        if (!lire_coup(entree)) { printf("Format invalide. Exemple : e2e4\n"); continue; }
        // Si le coup est interdit par les regles, on previent et on recommence
        if (!coup_legal(dep_l, dep_c, arr_l, arr_c)) { printf("Coup illegal !\n"); continue; }


        jouer_coup(dep_l, dep_c, arr_l, arr_c);        // le coup est valide : on le joue
        joueur = 3 - joueur;                           // 1 devient 2, 2 devient 1
    }
    return 0;   // 0 = le programme s'est termine normalement
}