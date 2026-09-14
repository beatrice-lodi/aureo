#include "budget.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

static string pulisciTesto(const string& testo) {
    size_t inizio = testo.find_first_not_of(" \t\r\n");

    if (inizio == string::npos) {
        return "";
    }

    size_t fine = testo.find_last_not_of(" \t\r\n");
    return testo.substr(inizio, fine - inizio + 1);
}

// Verifica il formato YYYY-MM e il numero del mese.
static bool meseValido(const string& mese) {
    if (mese.size() != 7 || mese[4] != '-') {
        return false;
    }

    for (size_t i = 0; i < mese.size(); i++) {
        if (i == 4) {
            continue;
        }

        if (mese[i] < '0' || mese[i] > '9') {
            return false;
        }
    }

    int anno = stoi(mese.substr(0, 4));
    int numeroMese = stoi(mese.substr(5, 2));

    return anno >= 1 && numeroMese >= 1 && numeroMese <= 12;
}

// Converte euro in centesimi senza arrotondamenti.
static bool convertiImporto(
    const string& testo,
    sqlite3_int64& centesimi
) {
    string valore = pulisciTesto(testo);

    if (valore.empty()) {
        return false;
    }

    string cifre;
    bool separatorePresente = false;
    int decimali = 0;
    int cifreIntere = 0;

    for (char carattere : valore) {
        if (carattere == '.' || carattere == ',') {
            if (separatorePresente) {
                return false;
            }

            separatorePresente = true;
        } else if (carattere >= '0' && carattere <= '9') {
            cifre += carattere;

            if (separatorePresente) {
                decimali++;
            } else {
                cifreIntere++;
            }
        } else {
            return false;
        }
    }

    if (cifreIntere == 0 || decimali > 2) {
        return false;
    }

    if (separatorePresente && decimali == 0) {
        return false;
    }

    while (decimali < 2) {
        cifre += '0';
        decimali++;
    }

    centesimi = 0;
    sqlite3_int64 massimo = numeric_limits<sqlite3_int64>::max();

    for (char cifra : cifre) {
        int numero = cifra - '0';

        if (centesimi > (massimo - numero) / 10) {
            return false;
        }

        centesimi = centesimi * 10 + numero;
    }

    return centesimi > 0;
}

void definisciBudget(sqlite3* db) {
    string mese;
    string categoria;
    string importo;
    sqlite3_int64 centesimi = 0;

    cout << "\nBUDGET MENSILE\n";
    cout << "Se un budget esiste gia per questo mese e categoria,\n";
    cout << "il nuovo importo lo sostituira.\n\n";

    cout << "Mese (YYYY-MM): ";
    if (!getline(cin, mese)) {
        return;
    }

    mese = pulisciTesto(mese);

    if (!meseValido(mese)) {
        cout << "Errore: inserisci un mese valido nel formato YYYY-MM.\n";
        return;
    }

    cout << "Nome della categoria: ";
    if (!getline(cin, categoria)) {
        return;
    }

    categoria = pulisciTesto(categoria);

    if (categoria.empty()) {
        cout << "Errore: indica una categoria.\n";
        return;
    }

    cout << "Budget in euro (esempio 300,00, senza separatori delle migliaia): ";
    if (!getline(cin, importo)) {
        return;
    }

    if (!convertiImporto(importo, centesimi)) {
        cout << "Errore: il budget deve essere maggiore di zero,\n";
        cout << "con massimo due decimali e nei limiti supportati.\n";
        return;
    }

    sqlite3_stmt* query = nullptr;

    // Cerca la categoria nel database.
    int esito = sqlite3_prepare_v2(
        db,
        "SELECT id_categoria FROM categorie WHERE nome = ?;",
        -1,
        &query,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Errore nella ricerca della categoria: "
             << sqlite3_errmsg(db) << "\n";
        return;
    }

    esito = sqlite3_bind_text(
        query, 1, categoria.c_str(), -1, SQLITE_TRANSIENT
    );

    if (esito == SQLITE_OK) {
        esito = sqlite3_step(query);
    }

    if (esito == SQLITE_DONE) {
        cout << "Errore: la categoria non esiste.\n";
        sqlite3_finalize(query);
        return;
    }

    if (esito != SQLITE_ROW) {
        cerr << "Errore nella ricerca della categoria: "
             << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(query);
        return;
    }

    sqlite3_int64 idCategoria = sqlite3_column_int64(query, 0);

    sqlite3_finalize(query);
    query = nullptr;

    // Inserisce il budget oppure aggiorna quello esistente.
    esito = sqlite3_prepare_v2(
        db,
        "INSERT INTO budget "
        "(mese, id_categoria, importo_centesimi) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(mese, id_categoria) DO UPDATE SET "
        "importo_centesimi = excluded.importo_centesimi;",
        -1,
        &query,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Errore nella preparazione del budget: "
             << sqlite3_errmsg(db) << "\n";
        return;
    }

    esito = sqlite3_bind_text(
        query, 1, mese.c_str(), -1, SQLITE_TRANSIENT
    );

    if (esito == SQLITE_OK) {
        esito = sqlite3_bind_int64(query, 2, idCategoria);
    }

    if (esito == SQLITE_OK) {
        esito = sqlite3_bind_int64(query, 3, centesimi);
    }

    if (esito == SQLITE_OK) {
        esito = sqlite3_step(query);
    }

    if (esito == SQLITE_DONE) {
        cout << "Budget mensile salvato correttamente.\n";
    } else {
        cerr << "Errore nel salvataggio del budget: "
             << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(query);
}