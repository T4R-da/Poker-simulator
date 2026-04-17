#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <fstream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <map>
#include <set>
#include <cstdlib>

// ANSI Colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"
#define GREEN   "\033[32m"
#define MAGENTA "\033[35m"

inline int playerBalance = 1000;
inline int currentBet = 0;
inline int startingRank = 2; 
inline int numCPUs = 1;
const int MAX_RAISES = 3; // Betting cap

enum class Rank   { TWO = 2, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE };
enum class Symbol { SPADES = 1, CLUBS = 2, DIAMONDS = 3, HEART = 4 };

enum HandValue {
    HIGH_CARD = 1, PAIR = 2, TWO_PAIR = 3, THREE_KIND = 4,
    STRAIGHT = 5, FLUSH = 6, FULL_HOUSE = 7, FOUR_KIND = 8,
    STRAIGHT_FLUSH = 9, ROYAL_FLUSH = 10
};

struct HandResult {
    HandValue value;
    int       totalScore;
    std::string name;
};

struct Card {
    Rank   rank;
    Symbol symbol;
    void print() const {
        std::string r[] = {"","","2","3","4","5","6","7","8","9","10","J","Q","K","A"};
        std::string s[] = {"","S","C","D","H"};
        if (symbol == Symbol::HEART || symbol == Symbol::DIAMONDS) std::cout << RED;
        else std::cout << CYAN;
        std::cout << "[" << r[static_cast<int>(rank)] << s[static_cast<int>(symbol)] << "]" << RESET << " ";
    }
};

// Forward declaration - fixes "evaluateHand was not declared in this scope"
HandResult evaluateHand(const std::vector<Card>& hand);

// --- TIMING UTILS ---
inline void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void sleepRandom() {
    sleepMs(1000 + (rand() % 1000)); // 1-2 seconds
}

// --- BETTING SYSTEM ---
enum class BetAction { FOLD, CHECK, CALL, RAISE };

struct Decision {
    BetAction action;
    int amount;
};

inline Decision cpuDecideBet(const std::vector<Card>& hand, int currentBetToCall, int raisesSoFar, int maxRaises) {
    HandResult hr = evaluateHand(hand);
    int score = hr.totalScore;
    int luck = rand() % 100;
    bool canRaise = (raisesSoFar < maxRaises);
    
    if (currentBetToCall == 0) {
        if (canRaise && score > 20000000 && luck < 60) {
            return {BetAction::RAISE, 20 + (rand() % 40)};
        } else if (canRaise && score > 10000000 && luck < 25) {
            return {BetAction::RAISE, 10 + (rand() % 20)};
        }
        return {BetAction::CHECK, 0};
    } else {
        if (score < 1000000 && currentBetToCall > 25) {
            return (luck < 15) ? Decision{BetAction::CALL, 0} : Decision{BetAction::FOLD, 0};
        }
        else if (score > 30000000) {
            if (canRaise && luck < 70) return {BetAction::RAISE, currentBetToCall * 2 + (rand() % 30)};
            return {BetAction::CALL, 0};
        }
        else if (score > 10000000 || currentBetToCall < 30) {
            if (canRaise && luck < 30 && score > 15000000) return {BetAction::RAISE, currentBetToCall + 25};
            return {BetAction::CALL, 0};
        }
        return (currentBetToCall > 40) ? Decision{BetAction::FOLD, 0} : Decision{BetAction::CALL, 0};
    }
}

// --- VISUALS (ASCII ART) ---
inline void printHeader() {
    std::cout << MAGENTA << BOLD;
    std::cout << "  ____     ____    _    ____  ____   ____      ____   ___   _  __ _____ ____  \n";
    std::cout << " | ___|   / ___|  / \\  |  _ \\|  _ \\ / ___|    |  _ \\ / _ \\ | |/ /| ____|  _ \\ \n";
    std::cout << " |___ \\  | |     / _ \\ | |_) | | | |\\___ \\    | |_) | | | || ' / |  _| | |_) |\n";
    std::cout << "  ___) | | |___ / ___ \\|  _ <| |_| | ___) |   |  __/| |_| || . \\ | |___|  _ < \n";
    std::cout << " |____/   \\____/_/   \\_\\_| \\_\\____/ |____/    |_|    \\___/ |_|\\_\\|_____|_| \\_\\\n";
    std::cout << "==================================================================================\n" << RESET;
}

inline void clearScreen() { system("cls"); }

inline void bootingSequence() {
    clearScreen();
    std::cout << CYAN << BOLD << "[SYSTEM]: INITIALIZING ASSETS..." << RESET << "\n";
    std::cout << "[";
    for(int i=0; i<30; i++) {
        std::cout << "#";
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    std::cout << "] 100%\n";
    std::cout << GREEN << "ASSETS LOADED." << RESET << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// --- CARD DISPLAY ---
inline void printHand(const std::vector<Card>& hand, const std::vector<bool>& discard = {}) {
    for (const auto& c : hand) {
        c.print();
        std::cout << " ";
    }
    std::cout << "\n";
    
    if (!discard.empty()) {
        for (size_t i = 0; i < hand.size() && i < 5; i++) {
            if (discard[i]) {
                std::cout << RED << "[DISCARD]" << RESET;
            } else {
                std::cout << "         ";
            }
            if (hand[i].rank == Rank::TEN) std::cout << " "; 
        }
        std::cout << "\n";
    }
}

// --- GAME SETUP ---
inline void chooseOpponents() {
    std::cout << CYAN << "\nHow many CPUs do you want to play against? (1-4): " << RESET;
    while (!(std::cin >> numCPUs) || numCPUs < 1 || numCPUs > 4) {
        std::cin.clear(); std::cin.ignore(1000, '\n');
        std::cout << RED << "Select 1 to 4: " << RESET;
    }
    // 1 CPU = start from 9, 2 CPUs = start from 8, etc.
    startingRank = 10 - numCPUs;
    std::cin.ignore(1000, '\n');
}

inline void placeBet() {
    if (playerBalance <= 0) {
        std::cout << RED << "\n[!] BANKRUPT! Dealer grants $200 charity.\n" << RESET;
        playerBalance = 200;
    }
    while (true) {
        std::cout << GREEN << BOLD << "\nCURRENT BALANCE: $" << playerBalance << RESET << "\n";
        std::cout << "Enter your bet amount: $";
        if (!(std::cin >> currentBet)) {
            std::cin.clear(); std::cin.ignore(1000, '\n');
            std::cout << RED << "Invalid input." << RESET << "\n";
        } else if (currentBet <= 0 || currentBet > playerBalance) {
            std::cout << RED << "Invalid amount." << RESET << "\n";
        } else { 
            std::cin.ignore(1000, '\n'); 
            break; 
        }
    }
}

// --- HAND EVALUATION (FIXED: Proper scoring to avoid ties) ---
inline HandResult evaluateHand(const std::vector<Card>& hand) {
    std::vector<Card> h = hand;
    // Sort descending (high to low)
    std::sort(h.begin(), h.end(), [](const Card& a, const Card& b){ return a.rank > b.rank; });
    
    std::map<Rank, int> rc;
    std::map<Symbol, int> sc;
    for (const auto& c : h) { rc[c.rank]++; sc[c.symbol]++; }
    
    // Check flush
    bool isFlush = false;
    Symbol flushSuit;
    for (auto const& [suit, cnt] : sc) {
        if (cnt >= 5) {
            isFlush = true;
            flushSuit = suit;
            break;
        }
    }
    
    // Get flush cards sorted descending
    std::vector<int> flushRanks;
    if (isFlush) {
        for (const auto& c : h) {
            if (c.symbol == flushSuit) {
                flushRanks.push_back(static_cast<int>(c.rank));
            }
        }
        std::sort(flushRanks.rbegin(), flushRanks.rend());
    }
    
    // Check straight (using unique ranks)
    std::set<int> uniqueRanksSet;
    for (const auto& c : h) uniqueRanksSet.insert(static_cast<int>(c.rank));
    std::vector<int> uniqueRanks(uniqueRanksSet.rbegin(), uniqueRanksSet.rend());
    
    bool isStraight = false;
    int straightHigh = 0;
    
    // Check for 5 consecutive cards
    for (size_t i = 0; i + 4 < uniqueRanks.size(); ++i) {
        if (uniqueRanks[i] - uniqueRanks[i+4] == 4) {
            isStraight = true;
            straightHigh = uniqueRanks[i];
            break;
        }
    }
    
    // Check ace-low straight (A-5-4-3-2)
    if (!isStraight && uniqueRanksSet.count(14) && uniqueRanksSet.count(5) && 
        uniqueRanksSet.count(4) && uniqueRanksSet.count(3) && uniqueRanksSet.count(2)) {
        isStraight = true;
        straightHigh = 5; // Ace is low
    }
    
    // Count frequencies
    int quads = 0, trips = 0, pairs = 0;
    int quadRank = 0, tripRank = 0, pair1Rank = 0, pair2Rank = 0;
    std::vector<int> kickers;
    
    for (auto const& [r, count] : rc) {
        int rankVal = static_cast<int>(r);
        if (count == 4) { quads++; quadRank = rankVal; }
        else if (count == 3) { trips++; tripRank = rankVal; }
        else if (count == 2) { 
            pairs++; 
            if (rankVal > pair1Rank) {
                pair2Rank = pair1Rank;
                pair1Rank = rankVal;
            } else if (rankVal > pair2Rank) {
                pair2Rank = rankVal;
            }
        }
        else if (count == 1) {
            kickers.push_back(rankVal);
        }
    }
    std::sort(kickers.rbegin(), kickers.rend()); // Descending
    
    HandValue v = HIGH_CARD;
    int score = 0;
    const int BASE = 100000000; // 10^8 - ensures categories don't overlap
    
    // Royal Flush check (10-J-Q-K-A of same suit)
    bool isRoyal = (isFlush && isStraight && straightHigh == 14 && 
                    uniqueRanksSet.count(13) && uniqueRanksSet.count(12) && 
                    uniqueRanksSet.count(11) && uniqueRanksSet.count(10));
    
    if (isRoyal) {
        v = ROYAL_FLUSH;
        score = 9 * BASE;
    }
    else if (isFlush && isStraight) {
        v = STRAIGHT_FLUSH;
        score = 8 * BASE + straightHigh;
    }
    else if (quads) {
        v = FOUR_KIND;
        score = 7 * BASE + quadRank * 10000 + kickers[0];
    }
    else if (trips && pairs) {
        v = FULL_HOUSE;
        score = 6 * BASE + tripRank * 10000 + pair1Rank;
    }
    else if (isFlush) {
        v = FLUSH;
        score = 5 * BASE;
        // Top 5 flush cards
        for (int i = 0; i < std::min(5, (int)flushRanks.size()); i++) {
            score += flushRanks[i] * std::pow(100, 4-i); // 100^4, 100^3, etc.
        }
        score /= 100; // Scale down to fit in int
    }
    else if (isStraight) {
        v = STRAIGHT;
        score = 4 * BASE + straightHigh;
    }
    else if (trips) {
        v = THREE_KIND;
        score = 3 * BASE + tripRank * 1000000 + kickers[0] * 10000 + kickers[1] * 100 + kickers[2];
    }
    else if (pairs >= 2) {
        v = TWO_PAIR;
        score = 2 * BASE + pair1Rank * 1000000 + pair2Rank * 10000 + kickers[0];
    }
    else if (pairs == 1) {
        v = PAIR;
        score = 1 * BASE + pair1Rank * 1000000 + kickers[0] * 10000 + kickers[1] * 100 + kickers[2];
    }
    else {
        v = HIGH_CARD;
        score = 0;
        // Use top 5 cards
        for (int i = 0; i < std::min(5, (int)h.size()); i++) {
            score += static_cast<int>(h[i].rank) * std::pow(100, 4-i);
        }
        score /= 100;
    }
    
    std::string n[] = {"","HIGH CARD","PAIR","TWO PAIR","3 OF A KIND","STRAIGHT","FLUSH","FULL HOUSE","4 OF A KIND","STRAIGHT FLUSH","ROYAL FLUSH"};
    return { v, score, n[v] };
}

inline void waitForEnter() {
    std::cout << "\n" << YELLOW << "Press ENTER to continue..." << RESET;
    while (_getch() != 13);
}

inline void showFile(const std::string& f) {
    clearScreen(); printHeader();
    std::ifstream file(f);
    if (file.is_open()) {
        std::string l;
        while (std::getline(file, l)) std::cout << l << "\n";
        file.close();
    } else std::cout << RED << "File Error." << RESET << "\n";
    waitForEnter();
}

// --- DECK CLASS (FIXED: Accepts starting rank) ---
class Deck {
private:
    std::vector<Card> cards;
public:
    Deck(int minRank = 2) {
        // Create deck from minRank to Ace (14)
        for (int r = minRank; r <= 14; ++r)
            for (int s = 1; s <= 4; ++s)
                cards.push_back({ static_cast<Rank>(r), static_cast<Symbol>(s) });
    }
    void shuffleDeck() {
        std::random_device rd; 
        std::mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    }
    Card drawCard() { 
        if(cards.empty()) return {Rank::TWO, Symbol::SPADES};
        Card c = cards.back(); 
        cards.pop_back(); 
        return c; 
    }
    bool isEmpty() const { return cards.empty(); }
    size_t remaining() const { return cards.size(); }
};

// --- CPU LOGIC ---
inline std::vector<bool> cpuDecide1(const std::vector<Card>& hand) {
    std::map<Rank, int> rc;
    for (const auto& c : hand) rc[c.rank]++;
    std::vector<bool> d(5, false);
    for (int i=0; i<5; i++) if (rc[hand[i].rank] < 2) d[i] = true;
    return d;
}

#endif
