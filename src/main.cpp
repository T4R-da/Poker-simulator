#include "functions.hpp"
#include <windows.h>
#include "miniaudio.h"

void playerDrawPhase(std::vector<Card>& hand, Deck& deck, int currentTurn, int totalTurns, int potSize) {
    std::vector<bool> discard(5, false);
    while (true) {
        clearScreen();
        printHeader();
        std::cout << CYAN << BOLD << "TURN " << currentTurn << " OF " << totalTurns << RESET << "\n";
        std::cout << YELLOW << "POT: $" << potSize << " | BAL: $" << playerBalance << RESET << "\n\n";
        std::cout << BOLD << "YOUR HAND:\n" << RESET;
        printHand(hand, discard);
        std::cout << "\n  (1)       (2)       (3)       (4)       (5)\n";
        std::cout << "\nNumbers to toggle DISCARD, ENTER to DRAW.\n";
        
        int currentDiscards = std::count(discard.begin(), discard.end(), true);
        if (currentDiscards == 4) {
            std::cout << YELLOW << "(MAX 4 CARDS SELECTED)" << RESET << "\n";
        } else if (currentDiscards > 0) {
            std::cout << "Discarding: " << currentDiscards << "/4 cards\n";
        }
        std::cout << "> ";

        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) break; 

        for (char ch : line) {
            if (ch >= '1' && ch <= '5') {
                int idx = ch - '1';
                if (!discard[idx] && currentDiscards >= 4) continue;
                discard[idx] = !discard[idx];
            }
        }
    }
    
    for (int i = 0; i < 5; i++) {
        if (discard[i] && !deck.isEmpty()) {
            hand[i] = deck.drawCard();
        }
    }
}

bool bettingRound(std::vector<Card>& pHand, std::vector<std::vector<Card>>& cpuHands,
                  std::vector<bool>& active, int& pot, int& betToCall,
                  int turnNum, int totalTurns, bool isPreDraw) {
    
    int raiseCount = 0;
    int startPlayer = 0;
    int current = startPlayer;
    int lastRaiser = -1;
    bool firstTime = true;
    int activeCount = std::count(active.begin(), active.end(), true);
    
    while (firstTime || current != lastRaiser) {
        firstTime = false;
        
        if (!active[current]) {
            current = (current + 1) % (numCPUs + 1);
            continue;
        }
        
        if (current == 0) { // Player
            clearScreen();
            printHeader();
            std::cout << CYAN << BOLD << "TURN " << turnNum << " OF " << totalTurns 
                      << (isPreDraw ? " - PRE-DRAW" : " - POST-DRAW") << RESET << "\n";
            std::cout << YELLOW << "POT: $" << pot << " | TO CALL: $" << betToCall 
                      << " | BAL: $" << playerBalance << RESET << "\n";
            if (raiseCount >= MAX_RAISES) std::cout << RED << "BETTING CAPPED" << RESET << "\n";
            std::cout << "\n" << BOLD << "YOUR HAND:\n" << RESET;
            printHand(pHand);
            std::cout << "\n";
            
            if (betToCall == 0) {
                if (raiseCount < MAX_RAISES) std::cout << "[C]heck  [R]aise  [F]old\n> ";
                else std::cout << "[C]heck  [F]old (capped)\n> ";
            } else {
                if (raiseCount < MAX_RAISES) std::cout << "[C]all $" << betToCall << "  [R]aise  [F]old\n> ";
                else std::cout << "[C]all $" << betToCall << "  [F]old (capped)\n> ";
            }
            
            char cmd;
            std::cin >> cmd;
            std::cin.ignore(1000, '\n');
            cmd = toupper(cmd);
            
            if (cmd == 'F') {
                std::cout << RED << "\nYou fold." << RESET << "\n";
                active[0] = false;
                activeCount--;
                sleepMs(1000);
                return (activeCount > 1);
            }
            else if (cmd == 'C') {
                if (betToCall == 0) std::cout << GREEN << "You check." << RESET << "\n";
                else {
                    std::cout << GREEN << "You call $" << betToCall << "." << RESET << "\n";
                    pot += betToCall;
                    playerBalance -= betToCall;
                }
                sleepMs(800);
            }
            else if (cmd == 'R') {
                if (raiseCount >= MAX_RAISES) {
                    std::cout << RED << "Max 3 raises! Calling instead." << RESET << "\n";
                    if (betToCall > 0) { pot += betToCall; playerBalance -= betToCall; }
                    sleepMs(1000);
                } else {
                    int minRaise = betToCall * 2;
                    std::cout << "Enter total bet (min $" << minRaise << "): ";
                    int newBet;
                    std::cin >> newBet;
                    std::cin.ignore(1000, '\n');
                    
                    if (newBet > betToCall && newBet <= playerBalance) {
                        pot += newBet;
                        playerBalance -= newBet;
                        betToCall = newBet;
                        lastRaiser = 0;
                        raiseCount++;
                        std::cout << YELLOW << "You raise to $" << betToCall << "! (" << raiseCount << "/3)" << RESET << "\n";
                    } else {
                        std::cout << "Invalid. You call $" << betToCall << ".\n";
                        pot += betToCall; playerBalance -= betToCall;
                    }
                    sleepMs(800);
                }
            }
        } 
        else { // CPU
            int cpuIdx = current - 1;
            std::cout << "\nCPU " << (cpuIdx + 1) << " thinking..." << std::flush;
            sleepRandom();
            
            Decision d = cpuDecideBet(cpuHands[cpuIdx], betToCall, raiseCount, MAX_RAISES);
            
            if (d.action == BetAction::RAISE && raiseCount >= MAX_RAISES) 
                d = {BetAction::CALL, 0};
            
            switch (d.action) {
                case BetAction::FOLD:
                    std::cout << RED << " CPU " << (cpuIdx + 1) << " folds." << RESET << "\n";
                    active[current] = false; activeCount--;
                    break;
                case BetAction::CHECK:
                    std::cout << CYAN << " CPU " << (cpuIdx + 1) << " checks." << RESET << "\n";
                    break;
                case BetAction::CALL:
                    std::cout << CYAN << " CPU " << (cpuIdx + 1) << " calls $" << betToCall << "." << RESET << "\n";
                    pot += betToCall;
                    break;
                case BetAction::RAISE:
                    raiseCount++;
                    betToCall = d.amount;
                    pot += betToCall;
                    lastRaiser = current;
                    std::cout << YELLOW << " CPU " << (cpuIdx + 1) << " raises to $" << betToCall 
                              << "! (" << raiseCount << "/3)" << RESET << "\n";
                    break;
            }
            sleepMs(1000);
        }
        
        if (activeCount <= 1) return false;
        current = (current + 1) % (numCPUs + 1);
        if (current == lastRaiser || (lastRaiser == -1 && current == startPlayer)) break;
    }
    return true;
}

void playGame(ma_engine* pMainEngine) {
    clearScreen(); printHeader();
    
    int totalTurns;
    std::cout << CYAN << "How many turns? (1-5): " << RESET;
    while (!(std::cin >> totalTurns) || totalTurns < 1 || totalTurns > 5) {
        std::cin.clear(); std::cin.ignore(1000, '\n');
        std::cout << RED << "Invalid. Enter 1-5: " << RESET;
    }
    std::cin.ignore(1000, '\n');
    
    chooseOpponents();
    srand(time(nullptr));

    for (int t = 1; t <= totalTurns; t++) {
        // FIX: Create deck with startingRank based on CPU count
        Deck myDeck(startingRank);
        myDeck.shuffleDeck();
        
        std::vector<Card> pHand;
        std::vector<std::vector<Card>> cpuHands(numCPUs);
        std::vector<bool> inHand(numCPUs + 1, true);
        
        // Deal
        for (int i = 0; i < 5; i++) {
            if (!myDeck.isEmpty()) pHand.push_back(myDeck.drawCard());
            for (int j = 0; j < numCPUs; j++) 
                if (!myDeck.isEmpty()) cpuHands[j].push_back(myDeck.drawCard());
        }

        int pot = (numCPUs + 1) * 10;
        playerBalance -= 10;
        int currentBetAmt = 0;
        
        std::cout << "\nTurn " << t << "... Everyone antes $10\n";
        sleepMs(1500);
        
        bool handContinues = bettingRound(pHand, cpuHands, inHand, pot, currentBetAmt, t, totalTurns, true);
        
        if (handContinues && inHand[0]) 
            playerDrawPhase(pHand, myDeck, t, totalTurns, pot);
        
        // CPU Draw
        for (int j = 0; j < numCPUs; j++) {
            if (!inHand[j+1]) continue;
            std::cout << "\nCPU " << (j + 1) << " drawing..." << std::flush;
            sleepRandom();
            
            std::vector<bool> cDiscard = cpuDecide1(cpuHands[j]);
            int drawn = 0;
            for (int i = 0; i < 5; i++) {
                if (cDiscard[i] && !myDeck.isEmpty()) {
                    cpuHands[j][i] = myDeck.drawCard();
                    drawn++;
                }
            }
            std::cout << " (" << drawn << " cards)" << std::endl;
            sleepMs(800);
        }
        
        int remaining = std::count(inHand.begin(), inHand.end(), true);
        if (remaining > 1 && inHand[0]) {
            currentBetAmt = 0;
            handContinues = bettingRound(pHand, cpuHands, inHand, pot, currentBetAmt, t, totalTurns, false);
        }
        
        // Showdown
        clearScreen(); printHeader();
        std::cout << RED << BOLD << "\n*** SHOWDOWN (TURN " << t << ") ***" << RESET << "\n";
        std::cout << YELLOW << "POT: $" << pot << RESET << "\n\n";
        
        int bestScore = -1;
        int winner = -1;  // FIX: Only track one winner (index: 0=player, 1+=CPU)
        
        if (inHand[0]) {
            HandResult pRes = evaluateHand(pHand);
            std::cout << "YOUR HAND: "; printHand(pHand);
            std::cout << " >> " << CYAN << pRes.name << RESET << "\n";
            bestScore = pRes.totalScore;
            winner = 0;
        } else std::cout << "YOU (folded)\n";
        
        // CPU hands in Yellow
        for (int j = 0; j < numCPUs; j++) {
            if (!inHand[j+1]) {
                std::cout << YELLOW << "CPU " << (j+1) << " HAND: " << RESET << "(folded)\n";
                continue;
            }
            HandResult cRes = evaluateHand(cpuHands[j]);
            std::cout << YELLOW << "CPU " << (j+1) << " HAND: " << RESET;
            printHand(cpuHands[j]);
            std::cout << " >> " << CYAN << cRes.name << RESET << "\n";
            
            // FIX: Strictly greater than - no ties allowed, first player with highest score wins
            if (cRes.totalScore > bestScore) {
                bestScore = cRes.totalScore;
                winner = j+1;
            }
            // Removed: else if equal - we keep the first winner (player wins ties)
        }
        
        // Award to single winner
        std::cout << "\n" << BOLD;
        if (winner == 0) {
            playerBalance += pot;
            std::cout << GREEN << ">> YOU WIN $" << pot << "! <<\n";
        } else if (winner > 0) {
            std::cout << RED << ">> CPU " << winner << " WINS $" << pot << "! <<\n";
        } else {
            std::cout << "No winner.\n";
        }
        std::cout << RESET;
        
        // Sounds
        ma_engine_stop(pMainEngine);
        ma_engine eng; 
        ma_engine_init(NULL, &eng);
        ma_engine_play_sound(&eng, (winner == 0) ? "jackpot.wav" : "fart.wav", NULL);
        sleepMs(2000);
        ma_engine_uninit(&eng);
        ma_engine_start(pMainEngine);
        
        if (t < totalTurns) {
            std::cout << CYAN << "\nPrepare for Turn " << (t + 1) << "..." << RESET << "\n";
            sleepMs(2000);
        }
    }
    
    std::cout << MAGENTA << BOLD << "\nFINAL BALANCE: $" << playerBalance << RESET << "\n";
    waitForEnter();
}

int main() {
    bootingSequence();
    ma_engine engine1;
    if (ma_engine_init(NULL, &engine1) != MA_SUCCESS) return -1;
    
    ma_sound bgSound;
    if (ma_sound_init_from_file(&engine1, "sound.wav", MA_SOUND_FLAG_STREAM, NULL, NULL, &bgSound) == MA_SUCCESS) {
        ma_sound_set_looping(&bgSound, true);
        ma_sound_start(&bgSound);
    }

    int choice = 0;
    std::string options[] = { "Start Game", "Rules", "Hand Rankings", "Quit" };

    while (true) {
        clearScreen(); printHeader();
        std::cout << YELLOW << "BALANCE: $" << playerBalance << RESET << "\n\n";
        for (int i = 0; i < 4; i++) {
            if (i == choice) std::cout << GREEN << BOLD << "  > " << options[i] << " <" << RESET << "\n";
            else std::cout << "    " << options[i] << "\n";
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
    
    ma_sound_stop(&bgSound);
    ma_sound_uninit(&bgSound);
    ma_engine_uninit(&engine1);
    return 0;
}
