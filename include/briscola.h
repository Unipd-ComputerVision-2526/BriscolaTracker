#ifndef BRISCOLA_H
#define BRISCOLA_H

#include<vector>

enum Seme{
    DENARI=1,
    COPPE=2,
    SPADE=3,
    BASTONI=4
};

class Briscola{
    public:
        Briscola(Seme seme, int players=2);

        void addCardToRound(Seme seme, int number);
        std::vector<int> getScores();

    private:
        int players;
        Seme briscolaSeed;
        std::vector<int> scores;    
};

#endif