-- AUREO: struttura del database

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS categorie (
    id_categoria INTEGER PRIMARY KEY,
    nome TEXT NOT NULL COLLATE NOCASE UNIQUE,
    CHECK (length(trim(nome)) > 0)
);

CREATE TABLE IF NOT EXISTS spese (
    id_spesa INTEGER PRIMARY KEY,
    data_spesa TEXT NOT NULL,
    importo_centesimi INTEGER NOT NULL,
    id_categoria INTEGER NOT NULL,
    descrizione TEXT,

    CHECK (
        typeof(importo_centesimi) = 'integer'
        AND importo_centesimi > 0
    ),

    FOREIGN KEY (id_categoria)
        REFERENCES categorie(id_categoria)
);

CREATE TABLE IF NOT EXISTS budget (
    id_budget INTEGER PRIMARY KEY,
    mese TEXT NOT NULL,
    id_categoria INTEGER NOT NULL,
    importo_centesimi INTEGER NOT NULL,

    CHECK (
        mese GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]'
        AND substr(mese, 6, 2) BETWEEN '01' AND '12'
    ),

    CHECK (
        typeof(importo_centesimi) = 'integer'
        AND importo_centesimi > 0
    ),

    UNIQUE (mese, id_categoria),

    FOREIGN KEY (id_categoria)
        REFERENCES categorie(id_categoria)
);
-- Categorie iniziali di Aureo

INSERT INTO categorie (nome) VALUES
    ('Casa'),
    ('Alimentari'),
    ('Mobilita'),
    ('Tempo libero')
ON CONFLICT(nome) DO NOTHING;
-- Budget dimostrativo: Alimentari, settembre 2026

INSERT INTO budget (mese, id_categoria, importo_centesimi)
SELECT '2026-09', id_categoria, 30000
FROM categorie
WHERE nome = 'Alimentari'
ON CONFLICT(mese, id_categoria) DO NOTHING;
-- Spesa dimostrativa: acquisto di alimentari

INSERT INTO spese (
    id_spesa,
    data_spesa,
    importo_centesimi,
    id_categoria,
    descrizione
)
SELECT
    1,
    '2026-09-14',
    4280,
    id_categoria,
    'Spesa settimanale al supermercato'
FROM categorie
WHERE nome = 'Alimentari'
ON CONFLICT(id_spesa) DO NOTHING;