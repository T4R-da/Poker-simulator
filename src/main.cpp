#include "functions.hpp"
#include <windows.h>
#include "miniaudio.h"

void printHand(const std::vector<Card>& hand, const std::vector<bool>& discard = {}) {
    for (const auto& c : hand) c.print();
    std::cout << "\n";
    if (!discard.empty()) {
        for (int i = 0; i < 5; i++) {
            if (discard[i]) std::cout << RED << "[DISCARD]" << RESET << " ";
            else            std::cout << "         ";
            if (hand[i].rank == Rank::TEN) std::cout << " "; 
        }
        std::cout << "\n";
    }
}

void playerDrawPhase(std::vector<Card>& hand, Deck& deck) {
    std::vector<bool> discard(5, false);
    while (true) {
        clearScreen();
        printHeader();
        std::cout << YELLOW << "POT: $" << currentBet << " | BAL: $" << playerBalance - currentBet << RESET << "\n\n";
        std::cout << BOLD << "YOUR HAND:\n" << RESET;
        printHand(hand, discard);
        std::cout << "\n  (1)       (2)       (3)       (4)       (5)\n";
        std::cout << "\nNumbers to toggle DISCARD, ENTER to DRAW.\n> ";

        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) break; 

        for (char ch : line) {
            if (ch >= '1' && ch <= '5') discard[ch - '1'] = !discard[ch - '1'];
        }
    }
    for (int i = 0; i < 5; i++) if (discard[i]) hand[i] = deck.drawCard();
}

void playGame(ma_engine* pMainEngine) {
    clearScreen(); printHeader();
    placeBet();
    chooseOpponents(); // New function replaces numcards()

    Deck myDeck;
    myDeck.shuffleDeck();

    std::vector<Card> pHand;
    std::vector<std::vector<Card>> cpuHands(numCPUs);

    // Deal
    for (int i = 0; i < 5; i++) {
        pHand.push_back(myDeck.drawCard());
        for (int j = 0; j < numCPUs; j++) {
            cpuHands[j].push_back(myDeck.drawCard());
        }
    }

    playerDrawPhase(pHand, myDeck);

    // CPU Draw Phases
    for (int j = 0; j < numCPUs; j++) {
        std::vector<bool> cDiscard = cpuDecide1(cpuHands[j]);
        for (int i = 0; i < 5; i++) if (cDiscard[i]) cpuHands[j][i] = myDeck.drawCard();
    }

    HandResult pRes = evaluateHand(pHand);
    
    clearScreen(); printHeader();
    std::cout << RED << BOLD << "\n*** SHOWDOWN ***" << RESET << "\n\n";
    std::cout << "YOUR HAND: "; printHand(pHand);
    std::cout << " >> " << CYAN << pRes.name << RESET << "\n\n";

    int highestCpuScore = 0;
    int winnerIdx = -1;

    for (int j = 0; j < numCPUs; j++) {
        HandResult cRes = evaluateHand(cpuHands[j]);
        std::cout << "CPU " << (j + 1) << " HAND: "; printHand(cpuHands[j]);
        std::cout << " >> " << CYAN << cRes.name << RESET << "\n";
        
        if (cRes.totalScore > highestCpuScore) {
            highestCpuScore = cRes.totalScore;
            winnerIdx = j;
        }
    }

    std::cout << "\n-----------------------------------------------------\n";

    if (pRes.totalScore > highestCpuScore) {
        int win = currentBet * static_cast<int>(pRes.value);
        playerBalance += win;
        std::cout << GREEN << BOLD << ">> YOU WIN! YOU BEAT ALL CPUS. WINNINGS: $" << win << " <<" << RESET << "\n";
        
        ma_engine_stop(pMainEngine);
        ma_engine engWin; ma_engine_init(NULL, &engWin);
        ma_engine_play_sound(&engWin, "jackpot.wav", NULL);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        ma_engine_uninit(&engWin);
        ma_engine_start(pMainEngine);
    } else if (highestCpuScore > pRes.totalScore) {
        playerBalance -= currentBet;
        std::cout << RED << BOLD << ">> CPU " << (winnerIdx + 1) << " WINS! YOU LOST $" << currentBet << " <<" << RESET << "\n";
        
        ma_engine_stop(pMainEngine);
        ma_engine engLose; ma_engine_init(NULL, &engLose);
        ma_engine_play_sound(&engLose, "fart.wav", NULL);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ma_engine_uninit(&engLose);
        ma_engine_start(pMainEngine);
    } else {
        std::cout << YELLOW << BOLD << ">> DRAW! POT RETURNED <<" << RESET << "\n";
    }
    waitForEnter();
}

int main() {
    bootingSequence();
    ma_engine engine1;
    if (ma_engine_init(NULL, &engine1) != MA_SUCCESS) return -1;
    ma_engine_play_sound(&engine1, "sound.wav", NULL);

    int choice = 0;
    std::string options[] = { "Start Game", "Rules", "Hand Rankings", "Quit" };

    while (true) {
        clearScreen();
        printHeader();
        std::cout << YELLOW << "BALANCE: $" << playerBalance << RESET << "\n\n";
        for (int i = 0; i < 4; i++) {
            if (i == choice) std::cout << GREEN << BOLD << "  > " << options[i] << " <" << RESET << "\n";
            else             std::cout << "    " << options[i] << "\n";
        }

        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == 72) choice = (choice - 1 + 4) % 4;
            if (key == 80) choice = (choice + 1) % 4;
        } else if (key == 13) {
            if      (choice == 0) playGame(&engine1);
            else if (choice == 1) showFile("rules.txt");
            else if (choice == 2) showFile("points.txt");
            else if (choice == 3) break;
        }
    }
    ma_engine_uninit(&engine1);
    return 0;
}
