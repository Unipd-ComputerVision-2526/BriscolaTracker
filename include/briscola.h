#ifndef BRISCOLA_H
#define BRISCOLA_H

#include<vector>
#include "utils.h"


class Briscola{
    public:
        // Costruttore che accetta la briscola
        Briscola(Seme seme, int players=2);
        // Aggiunge carta al round corrente, ritorna true se il round è completo
        boolean addCardToRound(Seme seme, int number);
        // Ritorna i punteggi attuali
        std::vector<int> getScores();
        // Ritorna il seme della briscola
        Seme getBriscolaSeed();

    private:
        // Aggiorna i punteggi.
        void setScores(const std::vector<int>& newScores);
        // Controlla se il round è completo (tutti i giocatori hanno giocato)
        boolean isRoundComplete();
        // Calcola il vincitore del round e aggiorna i punteggi e ritorna il vincitore del round.
        int computeRound();

        // Numero di giocatori
        int players;
        // Indice del giocatore che inizia il round, vincitore del round precedente.
        int nextFirstPlayer;
        // Seme della briscola
        Seme briscolaSeed;
        // Carte giocate nel round corrente, con seme e numero.
        std::vector<std::pair<Seme, int>> currentRoundCards;
        // Punteggi attuali dei giocatori, per tutta la partita.
        std::vector<int> scores;    
};

#endif



