#include "spese.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

// Rimuove spazi iniziali e finali.
static string pulisciTesto(const string& testo) {
    size_t inizio = testo.find_first_not_of(" \t\r\n");

    if (inizio == string::npos) {
        return "";
    }

    size_t fine = testo.find_last_not_of(" \t\r\n");
    return testo.substr(inizio, fine - inizio + 1);
}

// Controlla anche che il giorno esista nel calendario.
static bool dataValida(const string& data) {
    if (data.size() != 10 || data[4] != '-' || data[7] != '-') {
        return false;
    }

    for (size_t i = 0; i < data.size(); i++) {
        if (i == 4 || i == 7) {
            continue;
        }

        if (data[i] < '0' || data[i] > '9') {
            return false;
        }
    }

    int anno = stoi(data.substr(0, 4));
    int mese = stoi(data.substr(5, 2));
    int giorno = stoi(data.substr(8, 2));

    if (anno < 1 || mese < 1 || mese > 12) {
        return false;
    }

    int giorniMese[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    bool bisestile =
        (anno % 400 == 0) ||
        (anno % 4 == 0 && anno % 100 != 0);

    if (bisestile) {
        giorniMese[1] = 29;
    }

    return giorno >= 1 && giorno <= giorniMese[mese - 1];
}

// Converte gli euro in centesimi senza usare numeri decimali.
// Accetta, per esempio: 12, 12.5, 12.50 oppure 12,50.
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

    // Completa i centesimi: 12 diventa 1200, 12.5 diventa 1250.
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

void inserisciSpesa(sqlite3* db) {
    string data;
    string importo;
    string categoria;
    string descrizione;
    sqlite3_int64 centesimi = 0;

    cout << "\nNUOVA SPESA\n";

    cout << "Data (YYYY-MM-DD): ";
    if (!getline(cin, data)) {
        return;
    }

    data = pulisciTesto(data);

    if (!dataValida(data)) {
        cout << "Errore: inserisci una data reale nel formato YYYY-MM-DD.\n";
        return;
    }

    cout << "Importo in euro (esempio 12,50, senza separatori delle migliaia): ";
    if (!getline(cin, importo)) {
        return;
    }

    if (!convertiImporto(importo, centesimi)) {
        cout << "Errore: l'importo deve essere maggiore di zero, "
             << "con massimo due decimali e nei limiti supportati.\n";
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

    cout << "Descrizione (facoltativa, Invio per saltare): ";
    if (!getline(cin, descrizione)) {
        return;
    }

    descrizione = pulisciTesto(descrizione);

    // Cerca l'identificativo della categoria.
    sqlite3_stmt* query = nullptr;

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

    // Salva la spesa usando parametri separati dal testo SQL.
    esito = sqlite3_prepare_v2(
        db,
        "INSERT INTO spese "
        "(data_spesa, importo_centesimi, id_categoria, descrizione) "
        "VALUES (?, ?, ?, ?);",
        -1,
        &query,
        nullptr
    );

    if (esito != SQLITE_OK) {
        cerr << "Errore nella preparazione della spesa: "
             << sqlite3_errmsg(db) << "\n";
        return;
    }

    esito = sqlite3_bind_text(
        query, 1, data.c_str(), -1, SQLITE_TRANSIENT
    );

    if (esito == SQLITE_OK) {
        esito = sqlite3_bind_int64(query, 2, centesimi);
    }

    if (esito == SQLITE_OK) {
        esito = sqlite3_bind_int64(query, 3, idCategoria);
    }

    if (esito == SQLITE_OK) {
        if (descrizione.empty()) {
            esito = sqlite3_bind_null(query, 4);
        } else {
            esito = sqlite3_bind_text(
                query, 4, descrizione.c_str(), -1, SQLITE_TRANSIENT
            );
        }
    }

    if (esito == SQLITE_OK) {
        esito = sqlite3_step(query);
    }

    if (esito == SQLITE_DONE) {
        cout << "Spesa inserita correttamente.\n";
    } else {
        cerr << "Errore nel salvataggio della spesa: "
             << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(query);
}