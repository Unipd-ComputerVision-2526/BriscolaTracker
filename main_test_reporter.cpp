#include <iostream>
#include <string>
#include "reporter.h"
#include "utils.h"

int main() {
    std::cout << "--- Test Modulo Reporter ---" << std::endl;

    Reporter reporter;

    // 1. Simulazione di alcuni round di gioco
    RoundData r1;
    r1.round = 1;
    r1.northNumber = 8; r1.northSuit = SWORDS; // 8-spades
    r1.southNumber = 5; r1.southSuit = COINS;  // 5-coins
    r1.briscolaNumber = 1; r1.briscolaSuit = CLUBS; // 1-clubs
    r1.leader = "South";
    r1.winner = "South";
    r1.points = 2;
    reporter.logRound(r1);

    RoundData r2;
    r2.round = 2;
    r2.northNumber = 7; r2.northSuit = CLUBS;
    r2.southNumber = 3; r2.southSuit = CLUBS;
    r2.briscolaNumber = 1; r2.briscolaSuit = CLUBS;
    r2.leader = "South";
    r2.winner = "North";
    r2.points = 10;
    reporter.logRound(r2);

    // 2. Test Esportazione CSV
    std::cout << "[Reporter] Esportazione CSV in corso..." << std::endl;
    reporter.exportCSV("results/game1_results.csv");

    // 3. Test Generazione Report TXT
    std::cout << "[Reporter] Generazione Report TXT in corso..." << std::endl;
    reporter.generateFinalReport("results/game1_output.txt", 10, 2);

    // 4. Test Calcolo Metriche (usando il Ground Truth di Game 1)
    // Nota: Avendo simulato solo i primi 2 round identici al GT di game1,
    // ci aspettiamo un'accuratezza parziale ma corretta per quei round.
    std::cout << "[Reporter] Calcolo metriche su dataset/game1results.csv..." << std::endl;
    reporter.calculateMetrics("dataset/game1results.csv");

    std::cout << "\n--- TEST REPORTER COMPLETATO ---" << std::endl;
    std::cout << "Controlla i file 'results/game1_results.csv' e 'results/game1_output.txt' per i risultati." << std::endl;

    return 0;
}
