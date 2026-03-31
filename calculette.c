#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#ifdef DEBUG
    #define DEBUG_TRACE() \
        do {\
            fprintf(stderr, "%s\n", __func__); \
        }while(0)
#endif

char mon_token[256];
FILE* mon_fichier;
int mon_nombre;

void lire_caractere();
int est_en_fin();
void consommer(char attendu);
void terminer();
void amorcer(char* nom_fichier);
void passer_espaces();
int est_chiffre_non_nul();
int est_chiffre();

typedef enum e_operateur{
    PLUS, MOINS, FOIS, DIVISE, MODULO, NB_OPERATIONS
}t_operateur;

const char* t_operateur_image[NB_OPERATIONS] = {"PLUS", "MOINS", "FOIS", "DIVISE", "MODULO"};
t_operateur mon_operateur;

int est_operateur_additif();
int est_operateur_multiplicatif();
void operateur_additif();
void operateur_multiplicatif();
void chiffre_non_nul();
void sequence_chiffre();
void nombre_non_signe();
void nombre();
void facteur();
void terme();
void expression();
void calcul();
void salve();

void lire_token() {
    int c;
    int i = 0;

    do {
        c = fgetc(mon_fichier);
    } while (c != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r'));

    if (c == EOF) {
        mon_token[0] = '\0';
        return;
    }

    mon_token[i++] = (char)c;

    if (c == '<') {
        while (i < 255) {
            c = fgetc(mon_fichier);
            if (c == EOF) {
                fprintf(stderr, "Erreur : balise non fermee\n");
                exit(EXIT_FAILURE);
            }
            mon_token[i++] = (char)c;
            if (c == '>') break;
        }
    } else {
        while (i < 255) {
            c = fgetc(mon_fichier);
            if (c == EOF || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '<') {
                if (c == '<') ungetc(c, mon_fichier);
                break;
            }
            mon_token[i++] = (char)c;
        }
    }

    mon_token[i] = '\0';
}

int est_en_fin() {
    if(feof(mon_fichier)) {
        printf("\nFin du fichier");
        return 1;
    } else {
        return 0;
    }
}

void consommer_balise(const char *attendu) {
    if(mon_token == attendu) {
        lire_caractere();
    } else {
        fprintf(stderr, "Mauvais resultat pour : %c(%d), %c etait attendu \n", mon_token, mon_token, attendu);
        exit(EXIT_FAILURE);
    }
}

void terminer() {
    if(fclose(mon_fichier) != 0) {
        fprintf(stderr, "Erreur de fermeture du fichier");
    }
}

void amorcer(char* nom_fichier) {
    mon_fichier = fopen(nom_fichier, "r");
    if(mon_fichier == NULL) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s", nom_fichier);
        exit(EXIT_FAILURE);
    }
    lire_caractere();
}

void passer_espaces() {
    while (mon_token == ' ' || mon_token == '\t' || mon_token == '\n') {
        lire_caractere();
    }
}

int est_chiffre_non_nul() {
    return mon_token >= '1' && mon_token <= '9';
}

int est_chiffre() {
    return mon_token >= '0' && mon_token <= '9';
}

int est_operateur_additif() {
    return mon_token == '+' || mon_token == '-';
}

int est_operateur_multiplicatif() {
    return mon_token == '*' || mon_token == '/' || mon_token == '%';
}

void operateur_additif() {
    DEBUG_TRACE();
    if (est_operateur_additif()) {
        if(mon_token == '+'){
            mon_operateur = PLUS;
        }else if(mon_token == '-'){
            mon_operateur = MOINS;
        }
        lire_caractere();
        passer_espaces();
    }
}

void operateur_multiplicatif() {
    DEBUG_TRACE();
    if (est_operateur_multiplicatif()) {
        if(mon_token == '*'){
            mon_operateur = FOIS;
        }else if(mon_token == '/'){
            mon_operateur = DIVISE;
        }else if(mon_token == '%'){
            mon_operateur = MODULO;
        }
        lire_caractere();
        passer_espaces();
    }
}

void chiffre_non_nul() {
    DEBUG_TRACE();
    if (est_chiffre_non_nul()) {
        mon_nombre += mon_token - '0';
        lire_caractere();
    }
}

void sequence_chiffre() {
    DEBUG_TRACE();
    while (est_chiffre()) {
        mon_nombre *= 10;
        if(est_chiffre_non_nul()) {
            chiffre_non_nul();
        } else {
            consommer('0');
        }
    }
    passer_espaces();
}

void nombre_non_signe() {
    DEBUG_TRACE();
    if(est_chiffre_non_nul()) {
        chiffre_non_nul();
        sequence_chiffre();
    } else {
        consommer('0');
    }
    passer_espaces();
}

void nombre() {
    DEBUG_TRACE();
    mon_nombre = 0;
    char mon_signe = 1;
    if(mon_token == '-') {
        consommer('-');
        passer_espaces();
        mon_signe = - mon_signe;
    }
    nombre_non_signe();
    mon_nombre *= mon_signe;
    passer_espaces();
}

void facteur() {
    DEBUG_TRACE();
    if(mon_token == '(') {
        lire_caractere();
        passer_espaces();
        expression();
        consommer(')');
    } else {
        nombre();
    }
    passer_espaces();
}

void terme() {
    DEBUG_TRACE();
    int mon_accumulateur;
    facteur();
    mon_accumulateur = mon_nombre;
    passer_espaces();
    while(est_operateur_multiplicatif()) {
        operateur_multiplicatif();
        t_operateur mon_operateur_local = mon_operateur;
        facteur();
        switch(mon_operateur_local){
            case FOIS :
                mon_accumulateur *= mon_nombre;
                break;
            case DIVISE :
                mon_accumulateur /= mon_nombre;
                break;
            default : // MODULO
                printf("DEFAUT : (%d) %s!!\n", mon_operateur, t_operateur_image[mon_operateur]);
                mon_accumulateur %= mon_nombre;
        }        
        passer_espaces();
    }
    mon_nombre = mon_accumulateur;
}

void expression() {
    DEBUG_TRACE();
    int mon_accumulateur;
    terme();
    mon_accumulateur = mon_nombre;
    passer_espaces();
    while(est_operateur_additif()) {
        operateur_additif();
        t_operateur mon_operateur_local = mon_operateur;
        terme();
        if(mon_operateur_local == PLUS){
            mon_accumulateur += mon_nombre;
        }else{
            mon_accumulateur -= mon_nombre;
        }
        passer_espaces();
    }
    mon_nombre = mon_accumulateur;
}

void calcul() {
    DEBUG_TRACE();
    expression();
    printf(" %d", mon_nombre);
    consommer('=');
    passer_espaces();
}

void salve() {
    DEBUG_TRACE();
    while (!est_en_fin()) {
        calcul();
    }
}

int main(int argc, char *argv[])
{
    if(argc != 2){
        fprintf(stderr, "Erreur de parametrage : Usage \n\t %s FICHIER", argv[0]);
        exit(EXIT_FAILURE);
    }

    amorcer(argv[1]);
    salve();
    terminer();

    return 0;
}