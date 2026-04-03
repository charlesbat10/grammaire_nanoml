#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEBUG 0          /* ← mettre 1 pour réactiver le debug */
#if DEBUG
    #define DEBUG_TRACE() \
        do {\
            fprintf(stderr, "%s\n", __func__); \
        }while(0)
#else
    #define DEBUG_TRACE() do {} while(0)   /* ← ne fait rien */
#endif


char mon_token[256];
FILE* mon_fichier;
int niveau = 0;
int pos_ligne = -1;
int indent_courant = 0;

/* ---- Prototypes ---- */
void lire_token(void);
int  est_en_fin(void);
void consommer(const char* attendu);
void terminer(void);
void amorcer(char* nom_fichier);
int  est_balise(const char* b);
int  est_mot_simple(void);

int  largeur_contenu(void);
void ouvrir_boite(void);
void fermer_boite(void);
void debut_ligne(void);
void fin_ligne(void);
void retour_a_la_ligne(void);
void ecrire_mot(const char* mot, int majuscule);

void mot_simple(int majuscule);
void mot_important(void);
void texte_enrichi(int majuscule);
void texte(int majuscule);
void titre(void);
void item(void);
void liste(void);
void section(void);
void contenu(void);
void document(void);
void annexe_unique(void);
void annexes(void);
void salve(void);

/* ================================================================
   LECTURE
   ================================================================ */

void lire_token(void) {
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

int est_en_fin(void) {
    return mon_token[0] == '\0';
}

void consommer(const char* attendu) {
    if (strcmp(mon_token, attendu) == 0) {
        lire_token();
    } else {
        fprintf(stderr, "Erreur : '%s' attendu, '%s' lu\n", attendu, mon_token);
        exit(EXIT_FAILURE);
    }
}

void terminer(void) {
    if (fclose(mon_fichier) != 0) {
        fprintf(stderr, "Erreur de fermeture du fichier");
    }
}

void amorcer(char* nom_fichier) {
    mon_fichier = fopen(nom_fichier, "r");
    if (mon_fichier == NULL) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s", nom_fichier);
        exit(EXIT_FAILURE);
    }
    lire_token();
}

/* ================================================================
   HELPERS
   ================================================================ */

int est_balise(const char* b) {
    return strcmp(mon_token, b) == 0;
}

int est_mot_simple(void) {
    return mon_token[0] != '\0' && mon_token[0] != '<';
}

/* ================================================================
   AFFICHAGE EN BOITES ASCII
   ================================================================ */

int largeur_contenu(void) {
    return 50 - 2 * niveau;
}

void ouvrir_boite(void) {
    for (int i = 0; i < niveau; i++) printf("|");
    printf("+");
    for (int i = 0; i < 48 - 2 * niveau; i++) printf("-");
    printf("+");
    for (int i = 0; i < niveau; i++) printf("|");
    printf("\n");
    niveau++;
    pos_ligne = -1;
}

void fermer_boite(void) {
    if (pos_ligne >= 0) fin_ligne();
    niveau--;
    for (int i = 0; i < niveau; i++) printf("|");
    printf("+");
    for (int i = 0; i < 48 - 2 * niveau; i++) printf("-");
    printf("+");
    for (int i = 0; i < niveau; i++) printf("|");
    printf("\n");
}

void debut_ligne(void) {
    for (int i = 0; i < niveau; i++) printf("|");
    for (int i = 0; i < indent_courant; i++) printf(" ");
    pos_ligne = indent_courant;
}

void fin_ligne(void) {
    int larg = largeur_contenu();
    while (pos_ligne < larg) { printf(" "); pos_ligne++; }
    for (int i = 0; i < niveau; i++) printf("|");
    printf("\n");
    pos_ligne = -1;
}

void retour_a_la_ligne(void) {
    if (pos_ligne >= 0) fin_ligne();
}

void ecrire_mot(const char* mot, int majuscule) {
    int len = (int)strlen(mot);
    int larg = largeur_contenu();

    if (pos_ligne < 0) debut_ligne();

    int besoin = (pos_ligne > indent_courant) ? len + 1 : len;

    if (pos_ligne + besoin > larg) {
        fin_ligne();
        debut_ligne();
    }

    if (pos_ligne > indent_courant) { printf(" "); pos_ligne++; }

    for (int i = 0; mot[i] != '\0'; i++) {
        char c = majuscule ? (char)toupper((unsigned char)mot[i]) : mot[i];
        printf("%c", c);
        pos_ligne++;
    }
}

/* ================================================================
   PARSING NANOML
   ================================================================ */

void mot_simple(int majuscule) {
    DEBUG_TRACE();
    ecrire_mot(mon_token, majuscule);
    lire_token();
}

void mot_important(void) {
    DEBUG_TRACE();
    consommer("<important>");
    while (est_mot_simple()) {
        mot_simple(0);
    }
    consommer("</important>");
}

void texte_enrichi(int majuscule) {
    DEBUG_TRACE();
    if (est_balise("<br/>")) {
        retour_a_la_ligne();
        consommer("<br/>");
    } else if (est_balise("<important>")) {
        mot_important();
    } else {
        mot_simple(majuscule);
    }
}

void texte(int majuscule) {
    DEBUG_TRACE();
    while (!est_en_fin()
        && !est_balise("</titre>")
        && !est_balise("</item>")
        && !est_balise("</important>")
        && !est_balise("<section>")
        && !est_balise("</section>")
        && !est_balise("<titre>")
        && !est_balise("<liste>")
        && !est_balise("</document>")
        && !est_balise("</annexe>")) {
        texte_enrichi(majuscule);
    }
}

void titre(void) {
    DEBUG_TRACE();
    retour_a_la_ligne();
    consommer("<titre>");
    texte(1);
    consommer("</titre>");
    retour_a_la_ligne();
}

void item(void) {
    DEBUG_TRACE();
    int indent_sauvegarde = indent_courant;
    consommer("<item>");

    retour_a_la_ligne();
    debut_ligne();
    printf("#  ");
    pos_ligne += 3;
    indent_courant = indent_courant + 3;

    while (!est_en_fin() && !est_balise("</item>")) {
        if (est_balise("<liste>")) {
            liste();
        } else {
            texte_enrichi(0);
        }
    }
    consommer("</item>");
    retour_a_la_ligne();
    indent_courant = indent_sauvegarde;
}

void liste(void) {
    DEBUG_TRACE();
    int indent_sauvegarde = indent_courant;
    consommer("<liste>");
    indent_courant += 2;
    while (est_balise("<item>")) {
        item();
    }
    indent_courant = indent_sauvegarde;
    consommer("</liste>");
}

void section(void) {
    DEBUG_TRACE();
    consommer("<section>");
    ouvrir_boite();
    contenu();
    fermer_boite();
    consommer("</section>");
}

void contenu(void) {
    DEBUG_TRACE();
    while (!est_en_fin()
        && !est_balise("</document>")
        && !est_balise("</section>")
        && !est_balise("</annexe>")) {
        if (est_balise("<section>")) {
            section();
        } else if (est_balise("<titre>")) {
            titre();
        } else if (est_balise("<liste>")) {
            liste();
        } else {
            texte_enrichi(0);
        }
    }
}

void document(void) {
    DEBUG_TRACE();
    consommer("<document>");
    ouvrir_boite();
    contenu();
    fermer_boite();
    consommer("</document>");
}

void annexe_unique(void) {
    DEBUG_TRACE();
    consommer("<annexe>");
    ouvrir_boite();
    contenu();
    fermer_boite();
    consommer("</annexe>");
}

void annexes(void) {
    DEBUG_TRACE();
    while (est_balise("<annexe>")) {
        annexe_unique();
    }
}

void salve(void) {
    DEBUG_TRACE();
    document();
    annexes();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Erreur de parametrage : Usage \n\t %s FICHIER", argv[0]);
        exit(EXIT_FAILURE);
    }

    amorcer(argv[1]);
    salve();
    terminer();

    return 0;
}