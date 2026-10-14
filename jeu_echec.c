#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLANCS 1
#define NOIRS  (-1)

#define ROQUE_BLANC_ROI   1
#define ROQUE_BLANC_DAME  2
#define ROQUE_NOIR_ROI    4
#define ROQUE_NOIR_DAME   8

typedef struct {
    char plateau[8][8];
    int trait;
    int roques;
    int ep_ligne;
    int ep_colonne;
    int demi_coups;
} Partie;

typedef struct {
    int depart_ligne, depart_colonne;
    int arrivee_ligne, arrivee_colonne;
    char promotion;
    int prise_en_passant;
    int roque;
} Coup;

static int couleur_piece(char piece) {
    if (piece == '.') return 0;
    return isupper((unsigned char)piece) ? BLANCS : NOIRS;
}

static int dans_plateau(int ligne, int colonne) {
    return ligne >= 0 && ligne < 8 && colonne >= 0 && colonne < 8;
}

static void initialiser_partie(Partie *partie) {
    static const char *lignes[] = {
        "rnbqkbnr", "pppppppp", "........", "........",
        "........", "........", "PPPPPPPP", "RNBQKBNR"
    };
    int i;

    for (i = 0; i < 8; ++i)
        memcpy(partie->plateau[i], lignes[i], 8);
    partie->trait = BLANCS;
    partie->roques = ROQUE_BLANC_ROI | ROQUE_BLANC_DAME |
                     ROQUE_NOIR_ROI | ROQUE_NOIR_DAME;
    partie->ep_ligne = -1;
    partie->ep_colonne = -1;
    partie->demi_coups = 0;
}

static void afficher_plateau(const Partie *partie) {
    int ligne, colonne;
    puts("");
    for (ligne = 0; ligne < 8; ++ligne) {
        printf("%d  ", 8 - ligne);
        for (colonne = 0; colonne < 8; ++colonne)
            printf("%c ", partie->plateau[ligne][colonne]);
        putchar('\n');
    }
    puts("\n   a b c d e f g h\n");
    puts("Majuscules : blancs | Minuscules : noirs");
}

/* Indique si une case est attaquée par une pièce de la couleur demandée. */
static int case_attaquee(const Partie *partie, int ligne, int colonne, int couleur) {
    static const int cavaliers[8][2] = {
        {-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}
    };
    static const int directions[8][2] = {
        {-1,-1}, {-1,1}, {1,-1}, {1,1}, {-1,0}, {0,-1}, {0,1}, {1,0}
    };
    int i, l, c;
    char piece;

    /* Les pions attaquent en diagonale, dans le sens de leur déplacement. */
    l = ligne + (couleur == BLANCS ? 1 : -1);
    for (i = -1; i <= 1; i += 2) {
        c = colonne + i;
        if (dans_plateau(l, c)) {
            piece = partie->plateau[l][c];
            if (couleur_piece(piece) == couleur && tolower((unsigned char)piece) == 'p')
                return 1;
        }
    }

    for (i = 0; i < 8; ++i) {
        l = ligne + cavaliers[i][0];
        c = colonne + cavaliers[i][1];
        if (dans_plateau(l, c)) {
            piece = partie->plateau[l][c];
            if (couleur_piece(piece) == couleur && tolower((unsigned char)piece) == 'n')
                return 1;
        }
    }

    for (i = 0; i < 8; ++i) {
        l = ligne + directions[i][0];
        c = colonne + directions[i][1];
        while (dans_plateau(l, c)) {
            piece = partie->plateau[l][c];
            if (piece != '.') {
                if (couleur_piece(piece) == couleur) {
                    char type = (char)tolower((unsigned char)piece);
                    if (type == 'k' && l == ligne + directions[i][0] &&
                        c == colonne + directions[i][1])
                        return 1;
                    if ((i < 4 && (type == 'b' || type == 'q')) ||
                        (i >= 4 && (type == 'r' || type == 'q')))
                        return 1;
                }
                break;
            }
            l += directions[i][0];
            c += directions[i][1];
        }
    }
    return 0;
}

static int roi_en_echec(const Partie *partie, int couleur) {
    int ligne, colonne;
    char roi = couleur == BLANCS ? 'K' : 'k';
    for (ligne = 0; ligne < 8; ++ligne) {
        for (colonne = 0; colonne < 8; ++colonne) {
            if (partie->plateau[ligne][colonne] == roi)
                return case_attaquee(partie, ligne, colonne, -couleur);
        }
    }
    return 1; /* Un roi absent rend la position invalide. */
}

static void ajouter_coup(Coup coups[], int *nombre, int dl, int dc,
                         int al, int ac, char promotion, int ep, int roque) {
    Coup *coup = &coups[*nombre];
    coup->depart_ligne = dl;
    coup->depart_colonne = dc;
    coup->arrivee_ligne = al;
    coup->arrivee_colonne = ac;
    coup->promotion = promotion;
    coup->prise_en_passant = ep;
    coup->roque = roque;
    ++*nombre;
}

static void ajouter_coup_pion(Coup coups[], int *nombre, int dl, int dc,
                              int al, int ac, int ep) {
    if (al == 0 || al == 7) {
        ajouter_coup(coups, nombre, dl, dc, al, ac, 'q', ep, 0);
        ajouter_coup(coups, nombre, dl, dc, al, ac, 'r', ep, 0);
        ajouter_coup(coups, nombre, dl, dc, al, ac, 'b', ep, 0);
        ajouter_coup(coups, nombre, dl, dc, al, ac, 'n', ep, 0);
    } else {
        ajouter_coup(coups, nombre, dl, dc, al, ac, 0, ep, 0);
    }
}

/* Génère les coups qui respectent le déplacement des pièces, avant le test d'échec. */
static int coups_candidats(const Partie *partie, int couleur, Coup coups[]) {
    static const int cavaliers[8][2] = {
        {-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}
    };
    static const int directions[8][2] = {
        {-1,-1}, {-1,1}, {1,-1}, {1,1}, {-1,0}, {0,-1}, {0,1}, {1,0}
    };
    int nombre = 0, ligne, colonne, i, l, c, pas, depart_pion;
    char piece, cible, type;

    for (ligne = 0; ligne < 8; ++ligne) {
        for (colonne = 0; colonne < 8; ++colonne) {
            piece = partie->plateau[ligne][colonne];
            if (couleur_piece(piece) != couleur) continue;
            type = (char)tolower((unsigned char)piece);

            if (type == 'p') {
                pas = couleur == BLANCS ? -1 : 1;
                depart_pion = couleur == BLANCS ? 6 : 1;
                l = ligne + pas;
                if (dans_plateau(l, colonne) && partie->plateau[l][colonne] == '.') {
                    ajouter_coup_pion(coups, &nombre, ligne, colonne, l, colonne, 0);
                    if (ligne == depart_pion && partie->plateau[l + pas][colonne] == '.')
                        ajouter_coup(coups, &nombre, ligne, colonne, l + pas, colonne, 0, 0, 0);
                }
                for (i = -1; i <= 1; i += 2) {
                    c = colonne + i;
                    if (!dans_plateau(l, c)) continue;
                    cible = partie->plateau[l][c];
                    if (couleur_piece(cible) == -couleur)
                        ajouter_coup_pion(coups, &nombre, ligne, colonne, l, c, 0);
                    else if (l == partie->ep_ligne && c == partie->ep_colonne)
                        ajouter_coup_pion(coups, &nombre, ligne, colonne, l, c, 1);
                }
            } else if (type == 'n') {
                for (i = 0; i < 8; ++i) {
                    l = ligne + cavaliers[i][0];
                    c = colonne + cavaliers[i][1];
                    if (dans_plateau(l, c) && couleur_piece(partie->plateau[l][c]) != couleur)
                        ajouter_coup(coups, &nombre, ligne, colonne, l, c, 0, 0, 0);
                }
            } else if (type == 'k') {
                for (i = 0; i < 8; ++i) {
                    l = ligne + directions[i][0];
                    c = colonne + directions[i][1];
                    if (dans_plateau(l, c) && couleur_piece(partie->plateau[l][c]) != couleur)
                        ajouter_coup(coups, &nombre, ligne, colonne, l, c, 0, 0, 0);
                }

                /* Le roi ne peut pas roquer en étant en échec ou en traversant une case attaquée. */
                if (!roi_en_echec(partie, couleur) && ligne == (couleur == BLANCS ? 7 : 0) && colonne == 4) {
                    int droit_roi = couleur == BLANCS ? ROQUE_BLANC_ROI : ROQUE_NOIR_ROI;
                    int droit_dame = couleur == BLANCS ? ROQUE_BLANC_DAME : ROQUE_NOIR_DAME;
                    int adversaire = -couleur;
                    if ((partie->roques & droit_roi) &&
                        partie->plateau[ligne][7] == (couleur == BLANCS ? 'R' : 'r') &&
                        partie->plateau[ligne][5] == '.' && partie->plateau[ligne][6] == '.' &&
                        !case_attaquee(partie, ligne, 5, adversaire) &&
                        !case_attaquee(partie, ligne, 6, adversaire))
                        ajouter_coup(coups, &nombre, ligne, colonne, ligne, 6, 0, 0, 1);
                    if ((partie->roques & droit_dame) &&
                        partie->plateau[ligne][0] == (couleur == BLANCS ? 'R' : 'r') &&
                        partie->plateau[ligne][1] == '.' && partie->plateau[ligne][2] == '.' &&
                        partie->plateau[ligne][3] == '.' &&
                        !case_attaquee(partie, ligne, 3, adversaire) &&
                        !case_attaquee(partie, ligne, 2, adversaire))
                        ajouter_coup(coups, &nombre, ligne, colonne, ligne, 2, 0, 0, 1);
                }
            } else {
                for (i = 0; i < 8; ++i) {
                    int diagonale = i < 4;
                    if ((type == 'b' && !diagonale) || (type == 'r' && diagonale)) continue;
                    l = ligne + directions[i][0];
                    c = colonne + directions[i][1];
                    while (dans_plateau(l, c)) {
                        cible = partie->plateau[l][c];
                        if (couleur_piece(cible) == couleur) break;
                        ajouter_coup(coups, &nombre, ligne, colonne, l, c, 0, 0, 0);
                        if (cible != '.') break;
                        l += directions[i][0];
                        c += directions[i][1];
                    }
                }
            }
        }
    }
    return nombre;
}

static void jouer_coup(Partie *partie, const Coup *coup) {
    int dl = coup->depart_ligne, dc = coup->depart_colonne;
    int al = coup->arrivee_ligne, ac = coup->arrivee_colonne;
    char piece = partie->plateau[dl][dc];
    char capture = partie->plateau[al][ac];
    int couleur = couleur_piece(piece);

    if (coup->prise_en_passant) {
        capture = partie->plateau[dl][ac];
        partie->plateau[dl][ac] = '.';
    }

    partie->plateau[al][ac] = piece;
    partie->plateau[dl][dc] = '.';

    if (coup->roque) {
        if (ac == 6) {
            partie->plateau[al][5] = partie->plateau[al][7];
            partie->plateau[al][7] = '.';
        } else {
            partie->plateau[al][3] = partie->plateau[al][0];
            partie->plateau[al][0] = '.';
        }
    }

    if (coup->promotion) {
        partie->plateau[al][ac] = couleur == BLANCS
            ? (char)toupper((unsigned char)coup->promotion) : coup->promotion;
    }

    if (piece == 'K') partie->roques &= ~(ROQUE_BLANC_ROI | ROQUE_BLANC_DAME);
    if (piece == 'k') partie->roques &= ~(ROQUE_NOIR_ROI | ROQUE_NOIR_DAME);
    if (piece == 'R' && dl == 7 && dc == 0) partie->roques &= ~ROQUE_BLANC_DAME;
    if (piece == 'R' && dl == 7 && dc == 7) partie->roques &= ~ROQUE_BLANC_ROI;
    if (piece == 'r' && dl == 0 && dc == 0) partie->roques &= ~ROQUE_NOIR_DAME;
    if (piece == 'r' && dl == 0 && dc == 7) partie->roques &= ~ROQUE_NOIR_ROI;
    if (capture == 'R' && al == 7 && ac == 0) partie->roques &= ~ROQUE_BLANC_DAME;
    if (capture == 'R' && al == 7 && ac == 7) partie->roques &= ~ROQUE_BLANC_ROI;
    if (capture == 'r' && al == 0 && ac == 0) partie->roques &= ~ROQUE_NOIR_DAME;
    if (capture == 'r' && al == 0 && ac == 7) partie->roques &= ~ROQUE_NOIR_ROI;

    partie->ep_ligne = -1;
    partie->ep_colonne = -1;
    if (tolower((unsigned char)piece) == 'p' && abs(al - dl) == 2) {
        partie->ep_ligne = (al + dl) / 2;
        partie->ep_colonne = dc;
    }

    if (tolower((unsigned char)piece) == 'p' || capture != '.')
        partie->demi_coups = 0;
    else
        ++partie->demi_coups;
    partie->trait = -partie->trait;
}

static int coup_legal(const Partie *partie, const Coup *coup, int couleur) {
    Partie copie = *partie;
    jouer_coup(&copie, coup);
    return !roi_en_echec(&copie, couleur);
}

static int coups_legaux(const Partie *partie, int couleur, Coup legaux[]) {
    Coup candidats[256];
    int n = coups_candidats(partie, couleur, candidats);
    int i, nombre = 0;
    for (i = 0; i < n; ++i) {
        if (coup_legal(partie, &candidats[i], couleur))
            legaux[nombre++] = candidats[i];
    }
    return nombre;
}

static int convertir_case(char colonne, char rangee, int *ligne, int *col) {
    colonne = (char)tolower((unsigned char)colonne);
    if (colonne < 'a' || colonne > 'h' || rangee < '1' || rangee > '8') return 0;
    *col = colonne - 'a';
    *ligne = 8 - (rangee - '0');
    return 1;
}

static int texte_egal_sans_casse(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int analyser_entree(const char *texte, int *dl, int *dc, int *al, int *ac,
                           char *promotion, int *a_promotion) {
    char copie[64];
    size_t n = strlen(texte), debut = 0, fin;
    while (n > 0 && isspace((unsigned char)texte[n - 1])) --n;
    while (debut < n && isspace((unsigned char)texte[debut])) ++debut;
    fin = n - debut;
    if (fin >= sizeof(copie)) return 0;
    memcpy(copie, texte + debut, fin);
    copie[fin] = '\0';
    if (fin != 4 && fin != 5) return 0;
    if (!convertir_case(copie[0], copie[1], dl, dc) ||
        !convertir_case(copie[2], copie[3], al, ac)) return 0;
    *promotion = 0;
    *a_promotion = fin == 5;
    if (fin == 5) {
        *promotion = (char)tolower((unsigned char)copie[4]);
        if (*promotion != 'q' && *promotion != 'r' &&
            *promotion != 'b' && *promotion != 'n') return 0;
    }
    return 1;
}

static int materiel_insuffisant(const Partie *partie) {
    int ligne, colonne, pieces_mineures = 0, couleur_fou = -1;
    char piece;
    for (ligne = 0; ligne < 8; ++ligne) {
        for (colonne = 0; colonne < 8; ++colonne) {
            piece = (char)tolower((unsigned char)partie->plateau[ligne][colonne]);
            if (piece == 'p' || piece == 'q' || piece == 'r') return 0;
            if (piece == 'b' || piece == 'n') {
                ++pieces_mineures;
                if (piece == 'b') {
                    int couleur_case = (ligne + colonne) % 2;
                    if (couleur_fou != -1 && couleur_fou != couleur_case) return 0;
                    couleur_fou = couleur_case;
                } else {
                    couleur_fou = -2;
                }
            }
        }
    }
    return pieces_mineures <= 1 || (pieces_mineures > 0 && couleur_fou >= 0);
}

static void afficher_aide(void) {
    puts("Entrez un coup avec les cases de depart et d'arrivee : e2e4, g1f3.");
    puts("Pour promouvoir un pion, ajoutez q (dame), r (tour), b (fou) ou n (cavalier), par exemple e7e8q.");
    puts("Le roque s'ecrit e1g1 ou e1c1 pour les blancs, e8g8 ou e8c8 pour les noirs.");
    puts("Commandes : aide, quitter.");
}

int main(void) {
    Partie partie;
    char entree[128], promotion;
    int dl, dc, al, ac, avec_promotion;
    int couleur, n, i, trouve, coup_force;
    Coup legaux[256];

    initialiser_partie(&partie);
    puts("=== Jeu d'echecs ===");
    afficher_aide();

    for (;;) {
        afficher_plateau(&partie);
        couleur = partie.trait;
        printf("Trait des %s%s. Coup (ou aide/quitter) : ",
               couleur == BLANCS ? "blancs" : "noirs",
               roi_en_echec(&partie, couleur) ? " - echec" : "");

        if (!fgets(entree, sizeof(entree), stdin)) {
            puts("\nFin de la partie.");
            break;
        }
        entree[strcspn(entree, "\r\n")] = '\0';
        if (texte_egal_sans_casse(entree, "quitter") ||
            texte_egal_sans_casse(entree, "quit") ||
            texte_egal_sans_casse(entree, "exit")) {
            puts("Partie terminee.");
            break;
        }
        if (texte_egal_sans_casse(entree, "aide") ||
            texte_egal_sans_casse(entree, "help") ||
            strcmp(entree, "?") == 0) {
            afficher_aide();
            continue;
        }

        if (!analyser_entree(entree, &dl, &dc, &al, &ac, &promotion, &avec_promotion)) {
            puts("Format invalide. Exemple : e2e4. Tapez aide pour les commandes.");
            continue;
        }

        n = coups_legaux(&partie, couleur, legaux);
        trouve = -1;
        for (i = 0; i < n; ++i) {
            if (legaux[i].depart_ligne == dl && legaux[i].depart_colonne == dc &&
                legaux[i].arrivee_ligne == al && legaux[i].arrivee_colonne == ac) {
                if (legaux[i].promotion) {
                    char choix = avec_promotion ? promotion : 'q';
                    if (legaux[i].promotion == choix) trouve = i;
                } else if (!avec_promotion) {
                    trouve = i;
                }
            }
        }

        if (trouve < 0) {
            puts("Coup illegal. Verifiez les regles, les cases et votre roi.");
            continue;
        }

        jouer_coup(&partie, &legaux[trouve]);
        if (partie.demi_coups >= 100) {
            afficher_plateau(&partie);
            puts("Partie nulle : regle des 50 coups.");
            break;
        }
        if (materiel_insuffisant(&partie)) {
            afficher_plateau(&partie);
            puts("Partie nulle : materiel insuffisant pour mater.");
            break;
        }

        coup_force = coups_legaux(&partie, partie.trait, legaux);
        if (coup_force == 0) {
            afficher_plateau(&partie);
            if (roi_en_echec(&partie, partie.trait))
                printf("Echec et mat ! Les %s gagnent.\n", partie.trait == BLANCS ? "noirs" : "blancs");
            else
                puts("Partie nulle : pat.");
            break;
        }
    }
    return 0;
}
