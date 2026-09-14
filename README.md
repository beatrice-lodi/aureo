# AUREO
### Il valore delle tue scelte.

Aureo è un’applicazione da terminale per registrare le spese personali
e confrontarle con un budget mensile per categoria.

Il progetto è sviluppato in C++17 e utilizza SQLite per conservare
i dati anche dopo la chiusura del programma.

## Funzionalità

- Creazione di categorie, con controllo dei nomi vuoti e dei duplicati.
- Inserimento di spese con data, importo, categoria e descrizione facoltativa.
- Definizione e aggiornamento del budget mensile per categoria.
- Visualizzazione del totale delle spese per categoria.
- Confronto mensile tra spese e budget.
- Elenco delle spese ordinate dalla più vecchia alla più recente.

Il confronto mensile distingue quattro situazioni:
budget non definito, budget disponibile, budget raggiunto e budget superato.

## Requisiti

- Compilatore compatibile con C++17, come Clang o GCC.
- Libreria SQLite 3 con intestazioni di sviluppo (`sqlite3.h`).
- Programma `sqlite3` da terminale per inizializzare il database.
- SQLite versione 3.24.0 o successiva, per la sintassi UPSERT utilizzata.

Visual Studio Code è l’editor utilizzato durante lo sviluppo,
ma non è necessario per eseguire l’applicazione.

## File del progetto

| File | Contenuto |
| --- | --- |
| `main.cpp` | Menu principale, collegamento al database e gestione delle categorie |
| `spese.h` | Dichiarazione della funzione di inserimento delle spese |
| `spese.cpp` | Validazione e salvataggio delle spese |
| `budget.h` | Dichiarazione della funzione di gestione del budget |
| `budget.cpp` | Inserimento e aggiornamento dei budget |
| `report.h` | Dichiarazione del menu dei report |
| `report.cpp` | Query e visualizzazione dei report |
| `database.sql` | Struttura del database e dati dimostrativi |
| `README.md` | Istruzioni del progetto |

Il file `aureo.db` viene generato inizializzando il database.
Il file eseguibile `aureo` viene generato dalla compilazione.

## Preparazione del database

Aprire un terminale nella cartella del progetto ed eseguire:

```bash
sqlite3 -bail aureo.db < database.sql
```

Lo script crea le tabelle e inserisce questi dati dimostrativi:

- Categorie: Casa, Alimentari, Mobilita e Tempo libero.
- Budget Alimentari di settembre 2026: 300,00 euro.
- Spesa Alimentari del 14 settembre 2026: 42,80 euro.

I comandi di inserimento gestiscono i conflitti per evitare duplicati
quando lo script viene eseguito nuovamente. I budget già presenti
non vengono sovrascritti dai dati dimostrativi.

Il programma richiede un database già inizializzato.

## Compilazione

Da un ambiente con compilatore e SQLite configurati:

```bash
clang++ main.cpp spese.cpp budget.cpp report.cpp -std=c++17 -lsqlite3 -o aureo
```

Con GCC:

```bash
g++ main.cpp spese.cpp budget.cpp report.cpp -std=c++17 -lsqlite3 -o aureo
```

### Configurazione utilizzata sul Mac di sviluppo

Sul Mac utilizzato per il progetto è stato necessario indicare
esplicitamente il percorso delle intestazioni standard C++:

```bash
clang++ main.cpp spese.cpp budget.cpp report.cpp -std=c++17 -isystem /Library/Developer/CommandLineTools/SDKs/MacOSX15.2.sdk/usr/include/c++/v1 -lsqlite3 -o aureo
```

Questo percorso dipende dall’SDK installato sul computer.
Non è un requisito generale del progetto.

## Avvio

Dalla cartella del progetto:

```bash
./aureo
```

Il programma cerca `aureo.db` nella cartella da cui viene avviato.

## Utilizzo

Il menu principale presenta le seguenti opzioni:

1. Gestione Categorie
2. Inserisci Spesa
3. Definisci Budget Mensile
4. Visualizza Report
5. Esci

Digitare il numero desiderato e premere Invio.

### Categorie

Inserire un nome non vuoto.
Se la categoria esiste già, il programma segnala il duplicato.

Gli spazi iniziali e finali vengono rimossi.
Il confronto dei nomi utilizza la collazione SQLite `NOCASE`,
che ignora le differenze tra maiuscole e minuscole per i caratteri ASCII.

### Spese

Inserire:

- Data nel formato `YYYY-MM-DD`, ad esempio `2026-09-14`.
- Importo positivo, ad esempio `12,50` oppure `12.50`.
- Nome di una categoria esistente.
- Descrizione facoltativa; premere Invio per ometterla.

Sono accettati al massimo due decimali.
Non sono ammessi separatori delle migliaia.

Le date vengono controllate anche rispetto al calendario,
compresa la gestione degli anni bisestili.

### Budget

Inserire:

- Mese nel formato `YYYY-MM`, ad esempio `2026-09`.
- Nome di una categoria esistente.
- Importo positivo.

Se esiste già un budget per quel mese e quella categoria,
il nuovo importo sostituisce quello precedente.

### Report

Il sottomenu consente di visualizzare:

1. Totale delle spese per categoria, considerando tutte le date.
2. Confronto tra spese e budget per un mese.
3. Elenco delle spese ordinato per data.
4. Ritorno al menu principale.

Nel confronto mensile inserire il mese senza spazi, ad esempio `2026-09`.

## Struttura del database

### categorie

- `id_categoria`: chiave primaria.
- `nome`: obbligatorio, univoco e non vuoto.

### spese

- `id_spesa`: chiave primaria.
- `data_spesa`: data obbligatoria.
- `importo_centesimi`: intero positivo obbligatorio.
- `id_categoria`: chiave esterna obbligatoria verso `categorie`.
- `descrizione`: testo facoltativo.

### budget

- `id_budget`: chiave primaria.
- `mese`: mese obbligatorio nel formato previsto.
- `id_categoria`: chiave esterna obbligatoria verso `categorie`.
- `importo_centesimi`: intero positivo obbligatorio.
- Coppia `mese, id_categoria`: univoca.

Una categoria può essere associata a più spese e a più budget,
ma può avere un solo budget per ciascun mese.

## Scelte tecniche

Gli importi sono memorizzati in centesimi interi:
ad esempio, 42,80 euro corrispondono a 4280 centesimi.

La conversione avviene direttamente dal testo, senza utilizzare
numeri in virgola mobile, per evitare arrotondamenti monetari.

Le query che ricevono dati dall’utente utilizzano parametri SQL.
I valori vengono associati tramite le funzioni `sqlite3_bind_*`.

I vincoli delle chiavi esterne vengono attivati a ogni apertura
del database con `PRAGMA foreign_keys = ON`.

La validità delle date delle spese viene verificata dal codice C++.
Inserimenti effettuati direttamente tramite SQL devono rispettare
il formato e la validità del calendario.

## Verifiche effettuate

Durante le prove manuali sono stati verificati:

- Inserimento di una spesa valida.
- Rifiuto di una data inesistente.
- Rifiuto di una categoria inesistente.
- Aggiornamento del budget senza duplicazione.
- Confronto mensile tra spese e budget.
- Visualizzazione dell’elenco delle spese.
- Totali delle spese per categoria.

Esempio verificato: con un budget Alimentari di 350,00 euro
e spese di 42,80 euro, il report mostra 307,20 euro disponibili.

Questo esempio deriva da una modifica effettuata durante le prove:
il budget dimostrativo iniziale dello script resta di 300,00 euro.