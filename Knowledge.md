# Knowledge Base: Integrazione Template Matching & Metriche
*Documento riepilogativo per future implementazioni e refactoring.*

## 1. Parametri Ottimali per il Template Matching (Carte Numerali)
Dopo svariati esperimenti, abbiamo scoperto che il Template Matching funziona incredibilmente bene (85% di accuratezza) per contare i Denari sulle carte numerali, a patto di rispettare questi rigidi paletti:

*   **Template da utilizzare (SOLO DUE)**:
    *   **GRANDE (dal 3 di Denari)**: Estratto dal file `3-coins.JPG` alle coordinate `Rect(222, 84, 127, 131)`.
    *   **MEDIO (dal 7 di Denari)**: Estratto dal file `7-coins.JPG` alle coordinate `Rect(406, 47, 94, 89)`.
    *   *Nota: Mai inserire il Cavallo o altre figure tra i template base. Creano troppi falsi positivi.*

*   **Range di Scaling (Identico per entrambi)**:
    *   `scaleStart = 0.3`
    *   `scaleEnd = 1.5`
    *   `scaleStep = 0.1`

*   **Soglia di Riconoscimento**:
    *   Metodo: `TM_CCOEFF_NORMED`
    *   Soglia Minima (`thresholdScore`): `0.60`

*   **La Regola d'Oro (Filtro Pixel)**:
    *   L'NMS impazzisce se si scalano i template a dimensioni minuscole. 
    *   **Codice salvavita**: Subito dopo aver ridimensionato il template con `resize`, si deve scartare la scala corrente se il template risultante è più piccolo di `38x38` pixel o ha un'area inferiore a `1500` pixel.
    ```cpp
    if (resizedTempl.cols < 38 || resizedTempl.rows < 38 || (resizedTempl.cols * resizedTempl.rows) < 1500) continue;
    ```

*   **Non-Maximum Suppression (NMS)**:
    *   Raccogliere tutte le detection di tutte le scale e tutti i template in una singola lista.
    *   Ordinare in modo decrescente per `score`.
    *   Eliminare le detection con IoU (Intersection over Union) `> 0.2` rispetto a una detection con score più alto.

*   **Rotazioni Scartate**:
    *   Ruotare i template (0-315 gradi) è controproducente. Provoca artefatti sui bordi durante il resize che portano il CCOEFF_NORMED a segnare falsi positivi massicci (fino a 100+ per frame). Da non riutilizzare.

## 2. Integrazione nel Modulo Eye (L'Approccio Ibrido)
*   **Ruoli**: 
    *   `SIFT`: Strato primario. Infallibile sulle Figure (Fante, Cavallo, Re) e Asso grazie alle texture complesse.
    *   `Template Matching`: Paracadute di Fallback. Ottimo per contare i cerchietti quando le texture confondono il SIFT.
*   **Come si parlano**: 
    *   Quando `findCardValue` (SIFT) rileva `< 10` match, invece di restituire `false`, chiama la funzione `countDenari(img, diffMask)`.
    *   Il template matching deve ritagliare l'immagine originale seguendo strettamente la `diffMask` calcolata (che esclude tovaglia e briscola statica), in modo da cercare solo sulla carta giocata.
    *   Se trova $N$ monete (con $2 \le N \le 7$), restituisce la carta `(COINS, N)`.

## 3. Risoluzione dei Problemi di Metriche (Perché il 35%?)
L'accuratezza del 35% riportata alla fine dei game nel log CSV è **fuorviante**. 

1.  **Bug del Leader Iniziale nel `main.cpp`**: 
    *   Il sistema imposta hardcoded North come primo giocatore (`leaderIdx = 0`). 
    *   Tuttavia, nel Ground Truth (es. `game1`), a iniziare è South!
    *   Questo sfasamento inverte tutte le mani: il tracker legge correttamente le carte presenti sul tavolo, ma le attribuisce ai giocatori sbagliati. Il `reporter.cpp` segna 0% per quelle mani. Se si calcolasse l'accuracy ignorando l'attribuzione, si scoprirebbe un massiccio **75%**.
2.  **Limite del Fallback (Bug "Spade vs Denari")**:
    *   L'attuale logica di fallback assume automaticamente che qualsiasi cosa stia contando siano Denari. 
    *   Esempio: Se SIFT fallisce sul *2 di Spade*, il Fallback rileva le due lame tondeggianti delle spade, conta "2" e forza `(COINS, 2)`.
    *   *Soluzione futura*: Prima di convertire ciecamente $N$ in denari, applicare un filtro colore all'interno della detection (i denari sono giallo-arancio/oro, le spade no).
