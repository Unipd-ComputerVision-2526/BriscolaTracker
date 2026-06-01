#ifndef OCCHI_H
#define OCCHI_H

#include "utils.h"
#include <tuple>

class Occhi{
    public:
        // Costruttore
        Occhi();
        //Grande classico di Daniele: svuoto tutto.
        void clear();
        // Allena il modello con un set di immagini, seme e numero associati (SIFT). 
        void fit(std::vector<std::tuple<cv::Mat, Seme, int>> trainingset);
        // Riconosce la carta in un'immagine, ritorna seme e numero.
        std::pair<Seme, int> recognize(cv::Mat image);


    private:
        // Carta riconosciuta, con seme e numero.
        std::pair<Seme, int> card;
        // Mappa che associa descrittori SIFT (vettore di punti chiave) a carte (seme e numero).
        std::map<std::vector<cv::Point2f>, std::pair<Seme, int>> cardMap;
        //Array di carte già riconosciute, per evitare di riconoscere più volte la stessa carta.
        std::vector<std::pair<Seme, int>> recognizedCards;
        //Svuota array di carte già riconosciute, da chiamare ad ogni nuova partita.
        void resetRecognizedCards();
        

}


#endif