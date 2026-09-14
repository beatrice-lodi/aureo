#include "report.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

static string formattaEuro(sqlite3_int64 centesimi) {
    ostringstream testo;

    testo << centesimi / 100 << ','
          << setfill('0') << setw(2) << centesimi % 100
          << " EUR";

    return testo.str();
}

static string leggiTesto(sqlite3_stmt* query, int colonna) {
    const unsigned char* testo = sqlite3_column_text(query, colonna);

    return testo ? reinterpret_cast<const char*>(testo) : "";
}

static bool meseValido(const string& mese) {
    if (mese.size() != 7 || mese[4] != '-') {
        return false;
    }

    for (size_t i = 0; i < mese.size(); ++i) {
        if (i != 4 && (mese[i] < '0' || mese[i] > '9')) {
            return false;
        }
    }

    int anno = stoi(mese.substr(0, 4));
    int numeroMese = stoi(mese.substr(5, 2));

    return anno >= 1 && numeroMese >= 1 && numeroMese <= 12;
}

static void mostraTotali(sqlite3* db) {
    const char* sql = R"(
        SELECT c.nome, COALESCE(SUM(s.importo_centesimi), 0)
        FROM categorie c
        LEFT JOIN spese s ON s.id_categoria = c.id_categoria
        GROUP BY c.id_categoria, c.nome
        ORDER BY c.nome;
    )";

    sqlite3_stmt* query = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &query, nullptr) != SQLITE_OK) {
        cerr << "Errore nel report: " << sqlite3_errmsg(db) << "\n";
        return;
    }

    cout << "\nSPESE TOTALI PER CATEGORIA\n";
    cout << "Comprende tutte le date registrate.\n\n";

    bool presenti = false;
    int esito;

    while ((esito = sqlite3_step(query)) == SQLITE_ROW) {
        presenti = true;

        cout << leggiTesto(query, 0) << ": "
             << formattaEuro(sqlite3_column_int64(query, 1))
             << "\n";
    }

    if (esito != SQLITE_DONE) {
        cerr << "Errore nella lettura: " << sqlite3_errmsg(db) << "\n";
    } else if (!presenti) {
        cout << "Nessuna categoria presente.\n";
    }

    sqlite3_finalize(query);
}

static void mostraConfrontoBudget(sqlite3* db) {
    string mese;

    cout << "\nCONFRONTO SPESE E BUDGET\n";
    cout << "Mese (YYYY-MM): ";

    if (!getline(cin, mese)) {
        return;
    }

    if (!meseValido(mese)) {
        cout << "Errore: inserisci un mese valido nel formato YYYY-MM.\n";
        return;
    }

    const char* sql = R"(
        SELECT c.nome,
               COALESCE(s.totale, 0),
               b.importo_centesimi
        FROM categorie c
        LEFT JOIN (
            SELECT id_categoria, SUM(importo_centesimi) AS totale
            FROM spese
            WHERE substr(data_spesa, 1, 7) = ?1
            GROUP BY id_categoria
        ) s ON s.id_categoria = c.id_categoria
        LEFT JOIN budget b
            ON b.id_categoria = c.id_categoria
            AND b.mese = ?1
        ORDER BY c.nome;
    )";

    sqlite3_stmt* query = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &query, nullptr) != SQLITE_OK) {
        cerr << "Errore nel confronto: " << sqlite3_errmsg(db) << "\n";
        return;
    }

    if (sqlite3_bind_text(
            query, 1, mese.c_str(), -1, SQLITE_TRANSIENT
        ) != SQLITE_OK) {
        cerr << "Errore nel mese: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(query);
        return;
    }

    cout << "\nSituazione del mese " << mese << "\n";

    bool presenti = false;
    int esito;

    while ((esito = sqlite3_step(query)) == SQLITE_ROW) {
        presenti = true;

        sqlite3_int64 speso = sqlite3_column_int64(query, 1);

        cout << "\n" << leggiTesto(query, 0) << "\n";
        cout << "  Speso: " << formattaEuro(speso) << "\n";

        if (sqlite3_column_type(query, 2) == SQLITE_NULL) {
            cout << "  Budget non definito.\n";
        } else {
            sqlite3_int64 budget = sqlite3_column_int64(query, 2);

            cout << "  Budget: " << formattaEuro(budget) << "\n";

            if (speso < budget) {
                cout << "  Entro il budget. Disponibili: "
                     << formattaEuro(budget - speso) << "\n";
            } else if (speso == budget) {
                cout << "  Budget raggiunto.\n";
            } else {
                cout << "  Budget superato di "
                     << formattaEuro(speso - budget) << "\n";
            }
        }
    }

    if (esito != SQLITE_DONE) {
        cerr << "Errore nella lettura: " << sqlite3_errmsg(db) << "\n";
    } else if (!presenti) {
        cout << "Nessuna categoria presente.\n";
    }

    sqlite3_finalize(query);
}

static void mostraElencoSpese(sqlite3* db) {
    const char* sql = R"(
        SELECT s.id_spesa, s.data_spesa, c.nome,
               s.importo_centesimi, s.descrizione
        FROM spese s
        JOIN categorie c ON c.id_categoria = s.id_categoria
        ORDER BY s.data_spesa ASC, s.id_spesa ASC;
    )";

    sqlite3_stmt* query = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &query, nullptr) != SQLITE_OK) {
        cerr << "Errore nell'elenco: " << sqlite3_errmsg(db) << "\n";
        return;
    }

    cout << "\nELENCO SPESE\n";
    cout << "Dalla piu vecchia alla piu recente.\n";

    bool presenti = false;
    int esito;

    while ((esito = sqlite3_step(query)) == SQLITE_ROW) {
        presenti = true;

        cout << "\nSpesa #" << sqlite3_column_int64(query, 0)
             << " | " << leggiTesto(query, 1)
             << " | " << leggiTesto(query, 2)
             << " | " << formattaEuro(sqlite3_column_int64(query, 3))
             << "\n";

        string descrizione = leggiTesto(query, 4);

        if (!descrizione.empty()) {
            cout << "  " << descrizione << "\n";
        }
    }

    if (esito != SQLITE_DONE) {
        cerr << "Errore nella lettura: " << sqlite3_errmsg(db) << "\n";
    } else if (!presenti) {
        cout << "Nessuna spesa registrata.\n";
    }

    sqlite3_finalize(query);
}

void visualizzaReport(sqlite3* db) {
    string input;

    while (true) {
        cout << "\nREPORT AUREO\n";
        cout << "1. Totale spese per categoria\n";
        cout << "2. Confronto mensile con il budget\n";
        cout << "3. Elenco spese per data\n";
        cout << "4. Torna al menu principale\n";
        cout << "Inserisci la tua scelta: ";

        if (!getline(cin, input)) {
            return;
        }

        // Accetta un solo numero, con eventuali spazi ai lati.
        istringstream lettura(input);
        int scelta;
        char extra;

        if (!(lettura >> scelta) || (lettura >> extra)) {
            cout << "Scelta non valida. Riprovare.\n";
            continue;
        }

        switch (scelta) {
            case 1:
                mostraTotali(db);
                break;

            case 2:
                mostraConfrontoBudget(db);
                break;

            case 3:
                mostraElencoSpese(db);
                break;

            case 4:
                return;

            default:
                cout << "Scelta non valida. Riprovare.\n";
        }
    }
}