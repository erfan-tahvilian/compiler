#include <iostream>
#include <iomanip>
#include <windows.h>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <algorithm>

using namespace std;
using namespace chrono;

// High-resolution clock time points for measuring parsing times
time_point<high_resolution_clock> StartRD, StartLL1, StartLR1, StartLALR1;

// Variables to store parsing times for each parser
double TimeRD, TimeLL1, TimeLR1, TimeLALR1;

// Records the current time into the provided time point
void tic(time_point<high_resolution_clock>&);

// Calculates the duration since the recorded start time
double toc(time_point<high_resolution_clock>);

#define RUN 20
#define MAX_GRAMMAR_SIZE 1000
#define MAX_ERROR_SIZE 1000
#define MAX_PROCESS_SIZE 1000

#include "Lexer.hpp"
#include "RecursiveDescentParser.hpp"
#include "LL1Parser.hpp"
#include "LR1Parser.hpp"
#include "LALR1Parser.hpp"

// Display the main menu
int Menu();

// Measure and compare parsing times for different parsers
void time(string, string);

// Display a loading animation on the console
void loading();

int main() {
    SymbolTable *table;
    Lexer *lexer;
    RecursiveDescentParser *RDParser;
    LL1Parser *TopDownParser;
    LR1Parser *BottomUpParser1;
    LALR1Parser *BottomUpParser2;

    string inputString = "(xyz1+abc1)/(xyzkt2-ukm)"; // (xyz1+abc1)/(xyzkt2-ukm)
    fstream grammar_file;
    string path = "Grammars/";
    string inputGrammar = "g01.txt";

    bool flag;
    while (1) {
        switch (Menu()) {
            case 1:
                loading();
                flag = true;
                table = new SymbolTable;
                lexer = new Lexer(*table, inputString);
                RDParser = new RecursiveDescentParser(*lexer, path + inputGrammar);
                RDParser->parse();
                while (flag) {
                    switch (RecursiveDescentMenu(*RDParser)) {
                        case 1:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input string: ";
                            cin.ignore();
                            getline(cin, inputString);
                            delete RDParser;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            RDParser = new RecursiveDescentParser(*lexer, path + inputGrammar);
                            RDParser->parse();
                            loading();
                            break;

                        case 2:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                            cin >> inputGrammar;
                            inputGrammar += ".txt";
                            grammar_file.open(path + inputGrammar);
                            grammar_file.clear();
                            grammar_file.close();
                            while (grammar_file.fail()) {
                                grammar_file.clear();
                                grammar_file.close();
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                                cout << "\n\n\n\n\t\t\t\tError in opening grammar file!" << endl;
                                Sleep(2000);
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                                cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                                cin >> inputGrammar;
                                inputGrammar += ".txt";
                                grammar_file.open(path + inputGrammar);
                            }
                            delete RDParser;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            RDParser = new RecursiveDescentParser(*lexer, path + inputGrammar);
                            RDParser->parse();
                            loading();
                            break;

                        case 3:
                            loading();
                            printSymbolTable(table);
                            loading();
                            break;

                        case 4:
                            loading();
                            printFirstFollowTable(*RDParser);
                            loading();
                            break;

                        case 5:
                            loading();
                            printRecursiveDescentProcessingTable(*RDParser);
                            loading();
                            break;

                        case 0:
                            delete RDParser;
                            delete lexer;
                            delete table;
                            flag = false;
                            loading();
                            break;

                        default:
                            system("cls");
                            cout << "\n\n\n\n\t\t\t\tPlease enter correct option!";
                            Sleep(3000);
                    }
                }
                break;

            case 2:
                loading();
                flag = true;
                table = new SymbolTable;
                lexer = new Lexer(*table, inputString);
                TopDownParser = new LL1Parser(*lexer, path + inputGrammar);
                TopDownParser->parse();
                while (flag) {
                    switch (LL1Menu(*TopDownParser)) {
                        case 1:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input string: ";
                            cin.ignore();
                            getline(cin, inputString);
                            delete TopDownParser;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            TopDownParser = new LL1Parser(*lexer, path + inputGrammar);
                            TopDownParser->parse();
                            loading();
                            break;

                        case 2:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                            cin >> inputGrammar;
                            inputGrammar += ".txt";
                            grammar_file.open(path + inputGrammar);
                            grammar_file.clear();
                            grammar_file.close();
                            while (grammar_file.fail()) {
                                grammar_file.clear();
                                grammar_file.close();
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                                cout << "\n\n\n\n\t\t\t\tError in opening grammar file!" << endl;
                                Sleep(2000);
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                                cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                                cin >> inputGrammar;
                                inputGrammar += ".txt";
                                grammar_file.open(path + inputGrammar);
                            }
                            delete TopDownParser;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            TopDownParser = new LL1Parser(*lexer, path + inputGrammar);
                            TopDownParser->parse();
                            loading();
                            break;

                        case 3:
                            loading();
                            printSymbolTable(table);
                            loading();
                            break;

                        case 4:
                            loading();
                            printLL1ParsingTable(*TopDownParser);
                            loading();
                            break;

                        case 5:
                            loading();
                            printFirstFollowTable(*TopDownParser);
                            loading();
                            break;

                        case 6:
                            loading();
                            printLL1ProcessingTable(*TopDownParser);
                            loading();
                            break;

                        case 0:
                            delete TopDownParser;
                            delete lexer;
                            delete table;
                            flag = false;
                            loading();
                            break;

                        default:
                            system("cls");
                            cout << "\n\n\n\n\t\t\t\tPlease enter correct option!";
                            Sleep(3000);
                    }
                }
                break;

            case 3:
                loading();
                flag = true;
                table = new SymbolTable;
                lexer = new Lexer(*table, inputString);
                BottomUpParser1 = new LR1Parser(*lexer, path + inputGrammar);
                BottomUpParser1->parse();
                while (flag) {
                    switch (LR1Menu(*BottomUpParser1)) {
                        case 1:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input string: ";
                            cin.ignore();
                            getline(cin, inputString);
                            delete BottomUpParser1;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            BottomUpParser1 = new LR1Parser(*lexer, path + inputGrammar);
                            BottomUpParser1->parse();
                            loading();
                            break;

                        case 2:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                            cin >> inputGrammar;
                            inputGrammar += ".txt";
                            grammar_file.open(path + inputGrammar);
                            grammar_file.clear();
                            grammar_file.close();
                            while (grammar_file.fail()) {
                                grammar_file.clear();
                                grammar_file.close();
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                                cout << "\n\n\n\n\t\t\t\tError in opening grammar file!" << endl;
                                Sleep(2000);
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                                cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                                cin >> inputGrammar;
                                inputGrammar += ".txt";
                                grammar_file.open(path + inputGrammar);
                            }
                            delete BottomUpParser1;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            BottomUpParser1 = new LR1Parser(*lexer, path + inputGrammar);
                            BottomUpParser1->parse();
                            loading();
                            break;

                        case 3:
                            loading();
                            printSymbolTable(table);
                            loading();
                            break;

                        case 4:
                            loading();
                            printLR1ParsingTable(*BottomUpParser1);
                            loading();
                            break;

                        case 5:
                            loading();
                            printCanonicalItems(*BottomUpParser1);
                            loading();
                            break;

                        case 6:
                            loading();
                            printGoToTable(*BottomUpParser1);
                            loading();
                            break;

                        case 7:
                            loading();
                            printLR1ProcessingTable(*BottomUpParser1);
                            loading();
                            break;

                        case 0:
                            delete BottomUpParser1;
                            delete lexer;
                            delete table;
                            flag = false;
                            loading();
                            break;

                        default:
                            system("cls");
                            cout << "\n\n\n\n\t\t\t\tPlease enter correct option!";
                            Sleep(3000);
                    }
                }
                break;

            case 4:
                loading();
                flag = true;
                table = new SymbolTable;
                lexer = new Lexer(*table, inputString);
                BottomUpParser2 = new LALR1Parser(*lexer, path + inputGrammar);
                BottomUpParser2->parse();
                while (flag) {
                    switch (LALR1Menu(*BottomUpParser2)) {
                        case 1:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input string: ";
                            cin.ignore();
                            getline(cin, inputString);
                            delete BottomUpParser2;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            BottomUpParser2 = new LALR1Parser(*lexer, path + inputGrammar);
                            BottomUpParser2->parse();
                            loading();
                            break;

                        case 2:
                            loading();
                            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                            cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                            cin >> inputGrammar;
                            inputGrammar += ".txt";
                            grammar_file.open(path + inputGrammar);
                            grammar_file.clear();
                            grammar_file.close();
                            while (grammar_file.fail()) {
                                grammar_file.clear();
                                grammar_file.close();
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                                cout << "\n\n\n\n\t\t\t\tError in opening grammar file!" << endl;
                                Sleep(2000);
                                system("cls");
                                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                                cout << "\n\n\n\n\t\t\t\tPlease enter the input grammar file name: ";
                                cin >> inputGrammar;
                                inputGrammar += ".txt";
                                grammar_file.open(path + inputGrammar);
                            }
                            delete BottomUpParser2;
                            delete lexer;
                            delete table;
                            table = new SymbolTable;
                            lexer = new Lexer(*table, inputString);
                            BottomUpParser2 = new LALR1Parser(*lexer, path + inputGrammar);
                            BottomUpParser2->parse();
                            loading();
                            break;

                        case 3:
                            loading();
                            printSymbolTable(table);
                            loading();
                            break;

                        case 4:
                            loading();
                            printLR1ParsingTable(*BottomUpParser2);
                            loading();
                            break;

                        case 5:
                            loading();
                            printCanonicalItems(*BottomUpParser2);
                            loading();
                            break;

                        case 6:
                            loading();
                            printGoToTable(*BottomUpParser2);
                            loading();
                            break;

                        case 7:
                            loading();
                            printLR1ProcessingTable(*BottomUpParser2);
                            loading();
                            break;

                        case 0:
                            delete BottomUpParser2;
                            delete lexer;
                            delete table;
                            flag = false;
                            loading();
                            break;

                        default:
                            system("cls");
                            cout << "\n\n\n\n\t\t\t\tPlease enter correct option!";
                            Sleep(3000);
                    }
                }
                break;

            case 5:
                loading();
                time(inputString, path + inputGrammar);
                break;

            case 6:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                system("cls");
                cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\t\t\t\t\t\t\t\t\t\t\t\t*** Good Bye ***";
                Sleep(3000);
                exit(1);

            default:
                system("cls");
                cout << "\n\n\n\n\t\t\t\tPlease enter correct option!";
                Sleep(3000);
        }
    }

    return 0;
}

int Menu() {
    system("cls");
    int option;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\n\n\n\n\t\t\t\t----------------------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\t[+] Main Menu [+]" << endl << endl;
    cout << "\t\t\t\t----------------------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\t[1] Recursive Descent Parser (Top-Down Parser)";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    cout << " => Parsing Time: " << TimeRD << " ns" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t[2] Non-Recursive LL(1) Predictive Parser (Top-Down Parser)";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    cout << " => Parsing Time: " << TimeLL1 << " ns" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t[3] Canonical LR(1) Parser (Bottom-Up Parser)";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    cout << " => Parsing Time: " << TimeLR1 << " ns" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t[4] Lookahead LR(1) Parser (Bottom-Up Parser)";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    cout << " => Parsing Time: " << TimeLALR1 << " ns" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t[5] Calculate Parsing Times" << endl << endl;
    cout << "\t\t\t\t[6] Exit" << endl << endl;
    cout << "\t\t\t\t----------------------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\tPlease enter option : ";
    cin >> option;
    return option;
}

void time(string input, string grammar) {
    SymbolTable *table;
    Lexer *lexer;
    RecursiveDescentParser *RDParser;
    LL1Parser *TopDownParser;
    LR1Parser *BottomUpParser1;
    LALR1Parser *BottomUpParser2;
    double time;

    time = 0;
    for (int i = 0; i < RUN; i++) {
        table = new SymbolTable;
        lexer = new Lexer(*table, input);
        RDParser = new RecursiveDescentParser(*lexer, grammar);
        RDParser->parse();
        delete RDParser;
        delete lexer;
        delete table;
        time += TimeRD;
    }
    TimeRD = time / RUN;

    time = 0;
    for (int i = 0; i < RUN; i++) {
        table = new SymbolTable;
        lexer = new Lexer(*table, input);
        TopDownParser = new LL1Parser(*lexer, grammar);
        TopDownParser->parse();
        delete TopDownParser;
        delete lexer;
        delete table;
        time += TimeLL1;
    }
    TimeLL1 = time / RUN;

    time = 0;
    for (int i = 0; i < RUN; i++) {
        table = new SymbolTable;
        lexer = new Lexer(*table, input);
        BottomUpParser1 = new LR1Parser(*lexer, grammar);
        BottomUpParser1->parse();
        delete BottomUpParser1;
        delete lexer;
        delete table;
        time += TimeLR1;
    }
    TimeLR1 = time / RUN;

    time = 0;
    for (int i = 0; i < RUN; i++) {
        table = new SymbolTable;
        lexer = new Lexer(*table, input);
        BottomUpParser2 = new LALR1Parser(*lexer, grammar);
        BottomUpParser2->parse();
        delete BottomUpParser2;
        delete lexer;
        delete table;
        time += TimeLALR1;
    }
    TimeLALR1 = time / RUN;

}

void loading() {
    system("cls");
    system("COLOR 0e");
    printf("\e[?25l");

    SetConsoleCP(437);
    SetConsoleOutputCP(437);
    int bar1 = 177, bar2 = 219;

    cout << "\n\n\n\n\t\t\t\tLoading...";
    cout << "\n\n\t\t\t\t";

    for(int i = 0; i < 25; i++)
        cout << (char)bar1;

    cout <<"\r";
    cout <<"\t\t\t\t";
    for(int i = 0; i < 25; i++) {
        cout << (char)bar2;
        Sleep(15);
    }
    Sleep(40);
    system("cls");
}

void tic(time_point<high_resolution_clock> &Start) {
    Start = high_resolution_clock::now();
}

double toc(time_point<high_resolution_clock> Start) {
    time_point<high_resolution_clock> End = high_resolution_clock::now();
    auto executionTime = duration<double, nano>(End - Start);
    return executionTime.count();
}