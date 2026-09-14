#include <iostream>
#include <sstream>
#include <string>
#include <sqlite3.h>

#include "spese.h"
#include "budget.h"
#include "report.h"

using namespace std;

void mostraIntestazione() {
    cout << "\nAUREO\n";
    cout << "Il valore delle tue scelte.\n";
    cout << "Gestione delle spese personali e del budget\n\n";
}

void mostraMenu() {
    cout << "\nMENU PRINCIPALE\n";
    cout << "1. Gestione Categorie\n";
    cout << "2. Inserisci Spesa\n";
    cout << "3. Definisci Budget Mensile\n";
    cout << "4. Visualizza Report\n";
    cout << "5. Esci\n";
    cout << "Inserisci la tua scelta: ";
}

void gestisciCategorie(sqlite3* db) {
    string nome;

    cout << "\nNUOVA CATEGORIA\n";
    cout << "Nome della categoria: ";

    if (!getline(cin, nome)) {
        return;
    }

    // Rimuove gli spazi iniziali e finali.
    size_t inizio = nome.find_first_not_of(" \t\r\n");

    if (inizio == string::npos) {
        cout << "Errore: il nome della categoria non puo essere vuoto.\n";
        return;
    }

    size_t fine = nome.find_last_not_of(" \t\r\n");
    nome = nome.substr(inizio, fine - inizio + 1);

    sqlite3_stmt* query = nullptr;

    // Verifica se la categoria esiste gia.
    int esito = sqlite3_prepare_v2(
        db,
        "SELECT id_categoria FROM categorie WHERE nome = ?;",
        -1,
        &query,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Errore nella ricerca: "
             << sqlite3_errmsg(db) << "\n";
        return;
    }

    esito = sqlite3_bind_text(
        query,
        1,
        nome.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (esito != SQLITE_OK) {
        cerr << "Errore nella lettura del nome: "
             << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(query);
        return;
    }

    esito = sqlite3_step(query);

    if (esito == SQLITE_ROW) {
        cout << "La categoria esiste gia.\n";
        sqlite3_finalize(query);
        return;
    }

    if (esito != SQLITE_DONE) {
        cerr << "Errore nella ricerca: "
             << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(query);
        return;
    }

    sqlite3_finalize(query);
    query = nullptr;

    // Salva la nuova categoria.
    esito = sqlite3_prepare_v2(
        db,
        "INSERT INTO categorie (nome) VALUES (?);",
        -1,
        &query,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Errore nell'inserimento: "
             << sqlite3_errmsg(db) << "\n";
        return;
    }

    esito = sqlite3_bind_text(
        query,
        1,
        nome.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (esito == SQLITE_OK) {
        esito = sqlite3_step(query);
    }

    if (esito == SQLITE_DONE) {
        cout << "Categoria inserita correttamente.\n";
    } else {
        cerr << "Errore nell'inserimento: "
             << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(query);
}

int main() {
    sqlite3* db = nullptr;

    int esito = sqlite3_open_v2(
        "aureo.db",
        &db,
        SQLITE_OPEN_READWRITE,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Impossibile aprire il database: "
             << (db ? sqlite3_errmsg(db) : "memoria insufficiente")
             << "\n";

        sqlite3_close(db);
        return 1;
    }

    esito = sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Impossibile attivare i vincoli: "
             << sqlite3_errmsg(db) << "\n";

        sqlite3_close(db);
        return 1;
    }

    mostraIntestazione();
    cout << "Database Aureo collegato correttamente.\n";
    cout << "Benvenuto in Aureo!\n";

    int scelta = 0;
    string input;

    do {
        mostraMenu();

        if (!getline(cin, input)) {
            cout << "\nInput terminato. Chiusura di Aureo.\n";
            break;
        }

        // Legge tutta la riga e rifiuta caratteri aggiuntivi.
        istringstream lettura(input);
        char extra;
        scelta = 0;

        if (!(lettura >> scelta) || (lettura >> extra)) {
            scelta = 0;
            cout << "Scelta non valida. Riprovare.\n";
            continue;
        }

        switch (scelta) {
            case 1:
                gestisciCategorie(db);
                break;

            case 2:
                inserisciSpesa(db);
                break;

            case 3:
                definisciBudget(db);
                break;

            case 4:
                visualizzaReport(db);
                break;

            case 5:
                cout << "\nArrivederci da Aureo.\n";
                break;

            default:
                cout << "Scelta non valida. Riprovare.\n";
        }

    } while (scelta != 5);

    esito = sqlite3_close(db);

    if (esito != SQLITE_OK) {
        cerr << "Errore durante la chiusura del database.\n";
        return 1;
    }

    return 0;
}