#ifndef REPORTER_H
#define REPORTER_H

#include <vector>
#include <string>
#include <fstream>
#include "utils.h"

/**
 * @brief Data model for a single game round.
 *
 * This structure stores all round-level information used for export,
 * report generation, and comparison against ground-truth data.
 */
struct RoundData {
    /** @brief Round index (1-based). */
    int round;
    /** @brief Card number played by North. */
    int northNumber; 
    /** @brief Card suit played by North. */
    Suit northSuit;
    /** @brief Card number played by South. */
    int southNumber; 
    /** @brief Card suit played by South. */
    Suit southSuit;
    /** @brief Briscola card number for the round. */
    int briscolaNumber; 
    /** @brief Briscola card suit for the round. */
    Suit briscolaSuit;
    /** @brief Player who starts the round (for example "North" or "South"). */
    std::string leader; 
    /** @brief Player who wins the round (for example "North" or "South"). */
    std::string winner; 
    /** @brief Total points assigned in the round. */
    int points;
};

/**
 * @brief Per-suit counters used to evaluate recognition quality.
 */
struct SuitMetrics {
    /** @brief Total expected cards for this suit. */
    int expected = 0;
    /** @brief Cards with correctly predicted suit. */
    int correctSuit = 0;
    /** @brief Cards with both suit and number predicted correctly. */
    int exactMatch = 0;
    /** @brief Cards with incorrectly predicted suit. */
    int wrongSuit = 0;
    /** @brief Cards not evaluable because the round was missing/incomplete. */
    int incompleteRound = 0;
};

/**
 * @brief Aggregated performance metrics for one game evaluation.
 */
struct GameMetrics {
    /**
     * @brief Per-suit metrics indexed by Suit enum values.
     *
     * Valid indices are 1..4, corresponding to COINS, CUPS, SWORDS, CLUBS.
     */
    SuitMetrics suits[5]; // Index 1-4 correspond to Suit enum (COINS=1, CUPS=2, SWORDS=3, CLUBS=4)
    /** @brief Number of exact card matches (suit + number). */
    int correctCards = 0;
    /** @brief Number of correctly identified players (leader and winner checks). */
    int correctPlayers = 0;
    /** @brief Number of correctly recognized briscola cards. */
    int correctBriscola = 0;
    /** @brief Number of rounds that were actually evaluated. */
    int totalEvaluated = 0;
    /** @brief Total expected card comparisons. */
    int expectedCards = 0;
    /** @brief Briscola evaluation denominator. */
    int expectedBriscola = 1; 
    /** @brief Player accuracy denominator accumulator used during metric computation. */
    int totalPlayers = 40;
    /** @brief Number of correct game result fields. */
    int correctResultFields = 0;
    /** @brief Total expected game result fields. */
    int expectedResultFields = 3;

    /**
     * @brief Accumulates another metrics object into this one.
     * @param other Metrics object to add.
     */
    void add(const GameMetrics& other) {
        for(int i = 1; i <= 4; ++i) {
            suits[i].expected += other.suits[i].expected;
            suits[i].correctSuit += other.suits[i].correctSuit;
            suits[i].exactMatch += other.suits[i].exactMatch;
            suits[i].wrongSuit += other.suits[i].wrongSuit;
            suits[i].incompleteRound += other.suits[i].incompleteRound;
        }
        correctCards += other.correctCards;
        correctPlayers += other.correctPlayers;
        correctBriscola += other.correctBriscola;
        totalEvaluated += other.totalEvaluated;
        expectedCards += other.expectedCards;
        expectedBriscola += other.expectedBriscola;
        totalPlayers += other.totalPlayers;
        correctResultFields += other.correctResultFields;
        expectedResultFields += other.expectedResultFields;
    }
};

/**
 * @brief Report generator and metrics evaluator for Briscola rounds.
 */
class Reporter {
public:
    /**
     * @brief Appends one round to the internal history buffer.
     * @param data Round data to store.
     */
    void logRound(const RoundData& data);

    /**
     * @brief Exports the recorded rounds to a CSV file.
     * @param filename Output CSV path.
     */
    void exportCSV(const std::string& filename) const;

    /**
     * @brief Generates a human-readable final report file.
     * @param filename Output report path.
     * @param totalNorth Total score for North.
     * @param totalSouth Total score for South.
     */
    void generateFinalReport(const std::string& filename, int totalNorth, int totalSouth) const;

    /**
     * @brief Computes metrics by comparing recorded rounds against ground truth.
     * @param groundTruthPath Path to the ground-truth CSV file.
     * @param showDetailedStats If true, prints suit-level details to stdout.
     * @return Computed aggregate metrics.
     */
    GameMetrics calculateMetrics(const std::string& groundTruthPath, bool showDetailedStats = false) const;

private:
    /** @brief Internal storage of all recorded rounds. */
    std::vector<RoundData> history_;

    /**
     * @brief Parses all ground-truth rows from an already opened CSV stream.
     * @param file Input stream positioned at file start.
     * @return Parsed ground-truth rounds.
     */
    std::vector<RoundData> parseGroundTruth(std::ifstream& file) const;

    /**
     * @brief Parses one CSV row into a RoundData structure.
     * @param line Input CSV line.
     * @return Parsed round data.
     */
    RoundData parseGroundTruthLine(const std::string& line) const;

    /**
     * @brief Updates expected counters for the current ground-truth round.
     * @param metrics Metrics accumulator to update.
     * @param gtRound Current ground-truth round.
     */
    void updateExpectedMetrics(GameMetrics& metrics, const RoundData& gtRound) const;

    /**
     * @brief Updates counters for a round found in both prediction and ground truth.
     * @param detected Round extracted by the tracker.
     * @param gtRound Ground-truth round.
     * @param metrics Metrics accumulator to update.
     */
    void evaluateMatchedRound(const RoundData& detected, const RoundData& gtRound, GameMetrics& metrics) const;

    /**
     * @brief Updates counters when a ground-truth round is missing in history.
     * @param gtRound Ground-truth round missing from detected history.
     * @param metrics Metrics accumulator to update.
     */
    void markIncompleteRound(const RoundData& gtRound, GameMetrics& metrics) const;

    /**
     * @brief Prints metrics summary and optional detailed per-suit stats.
     * @param metrics Metrics to print.
     * @param showDetailedStats Enables detailed suit-level table.
     */
    void printMetricsReport(const GameMetrics& metrics, bool showDetailedStats) const;

    /**
     * @brief Converts a Suit value into its canonical text representation.
     * @param s Suit value to convert.
     * @return Suit string (for example "coins", "cups", "spades", "clubs").
     */
    std::string suitToString(Suit s) const;

    /**
     * @brief Converts a suit string into its Suit enum value.
     * @param s Suit string to parse.
     * @return Parsed Suit value; defaults to COINS for unknown input.
     */
    Suit stringToSuitInternal(const std::string& s) const;

    /**
     * @brief Validates that a file stream is open and usable.
     * @tparam T Stream type (for example std::ifstream or std::ofstream).
     * @param stream Stream instance to validate.
     * @param filename Related file path/name used for diagnostics.
     * @return true if the stream is open, false otherwise.
     */
    template <typename T>
    bool checkStream(const T& stream, const std::string& filename) const;
};

#endif // REPORTER_H
