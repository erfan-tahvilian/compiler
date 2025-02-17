
// item in a Canonical LR(1) state
struct item {
    pair<string, string> production; // A production rule (lhs, rhs)
    set<string> lookahead; // Lookahead symbols

    // Compares two items for ordering
    bool operator < (const item &other) const {
        return tie(production, lookahead) < tie(other.production, other.lookahead);
    }

    // Checks equality of two items
    bool operator == (const item &other) const {
        return production == other.production && lookahead == other.lookahead;
    }
};

// Canonical LR(1) parser
struct LR1Parser {
    Lexer &lexer; // Reference to the lexer for tokenization
    string grammar; // File path to the input grammar
    string start_symbol; // Starting symbol of the grammar
    Token currentToken; // Current token being processed
    pair<string, string> *gram; // Array to store grammar rules as pairs of LHS and RHS
    int prod_count; // Number of grammar productions
    set<string> non_terms; // Set of non-terminal symbols
    set<string> terms; // Set of terminal symbols
    map<string, set<string>> firsts; // FIRST sets for grammar symbols
    map<int, vector<item>> canonical; // Canonical LR(1) states
    int state_count; // Number of states in the automaton
    map<pair<int, string>, int> gotoMap; // GOTO transitions
    map<int, item> reduceMap; // REDUCE mappings for states
    int **parseTable; // Parsing table
    string *errors; // Array to store error messages
    int error_count; // Number of errors encountered
    string **process; // Array to log processing steps
    int process_count; // Count of processed steps
    bool accepted; // flag to indicate whether input is accepted

    // Constructor to initialize the parser with grammar and lexer
    LR1Parser(Lexer &lexer, string grammar) : lexer(lexer), grammar(grammar), currentToken(lexer.getNextToken()), prod_count(0), state_count(0), error_count(0), process_count(0), accepted(true) {
        // Allocate memory for grammar rules, errors, and process details
        gram = new pair<string, string>[MAX_GRAMMAR_SIZE];
        errors = new string[MAX_ERROR_SIZE];
        process = new string*[MAX_PROCESS_SIZE];
        for (int i = 0; i < MAX_PROCESS_SIZE; i++)
            process[i] = new string[4];
    }

    // Eliminate left recursion from the grammar
    void eliminateLeftRecursion(pair<string, string> *gram, int &prod_count) {
        string arrangement[non_terms.size()]; // Array to keep track of non-terminals in order
        int index = 0;

        // Populate arrangement array with unique non-terminals
        for (int i = 0; i < prod_count; i++) {
            if (find(arrangement, arrangement + non_terms.size(), gram[i].first) == arrangement + non_terms.size())
                arrangement[index++] = gram[i].first;
        }

        // Process each non-terminal in the arrangement
        for (int i = 0; i < index; i++) {
            for (int j = 0; j < i; j++) {
                for (int k = 0; k < prod_count; k++) {
                    string lhs = gram[k].first; // Left-hand side of the production
                    if (lhs != arrangement[i])
                        continue;
                    string rhs = gram[k].second; // Right-hand side of the production
                    string firstSymbol;

                    // Extract the first symbol from RHS
                    for (auto ch = rhs.begin(); ch != rhs.end(); ch++) {
                        if (*ch != ' ')
                            firstSymbol += *ch;
                        if (*ch == ' ' || next(ch) == rhs.end()) {
                            // Replace indirect left recursion
                            if (firstSymbol == arrangement[j]) {
                                bool flag1 = true;
                                for (int l = 0; l < prod_count; l++) {
                                    string lhs2 = gram[l].first;
                                    if (lhs2 != arrangement[j])
                                        continue;
                                    string rhs2 = gram[l].second;
                                    string new_rhs = rhs2;

                                    // Handle epsilon or concatenate remaining RHS
                                    if (new_rhs == "e")
                                        new_rhs = rhs.substr(lhs2.length() + 1);
                                    else if (next(ch) != rhs.end())
                                        new_rhs += rhs.substr(lhs2.length());

                                    if (flag1) {
                                        gram[k] = make_pair(lhs, new_rhs);
                                        flag1 = false;
                                    } else {
                                        for (int m = prod_count; m >= k + 1; m--)
                                            gram[m] = gram[m - 1];
                                        gram[k] = make_pair(lhs, new_rhs);
                                        prod_count++;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }

            // Detect and eliminate direct left recursion
            bool flag = false;
            for (int j = 0; j < prod_count; j++) {
                if (flag)
                    break;
                string lhs = gram[j].first;
                if (lhs != arrangement[i])
                    continue;
                string rhs = gram[j].second;
                string firstSymbol;

                // Extract first symbol and check for direct recursion
                for (auto ch = rhs.begin(); ch != rhs.end(); ch++) {
                    if (*ch != ' ')
                        firstSymbol += *ch;
                    if (*ch == ' ' || next(ch) == rhs.end()) {
                        if (firstSymbol == lhs)
                            flag = true;
                        break;
                    }
                }
            }

            if (flag) {
                int last;
                string new_non_term = arrangement[i] + "'"; // Create a new non-terminal
                non_terms.insert(new_non_term);

                for (int j = 0; j < prod_count; j++) {
                    string lhs = gram[j].first;
                    if (lhs != arrangement[i])
                        continue;
                    string rhs = gram[j].second;
                    string firstSymbol;

                    for (auto ch = rhs.begin(); ch != rhs.end(); ch++) {
                        if (*ch != ' ')
                            firstSymbol += *ch;
                        if (*ch == ' ' || next(ch) == rhs.end()) {
                            last = j;
                            string new_rhs;

                            // Handle left-recursive and non-left-recursive productions
                            if (firstSymbol == lhs) {
                                if (next(ch) != rhs.end())
                                    new_rhs += rhs.substr(firstSymbol.length() + 1);
                                new_rhs += " " + new_non_term;
                                gram[j] = make_pair(new_non_term, new_rhs);
                            } else {
                                if (rhs != "e")
                                    new_rhs = rhs + " " + new_non_term;
                                else
                                    new_rhs = new_non_term;
                                gram[j] = make_pair(lhs, new_rhs);
                            }
                            break;
                        }
                    }
                }

                // Add epsilon production for the new non-terminal
                for (int j = prod_count; j >= last + 2; j--)
                    gram[j] = gram[j - 1];
                gram[last + 1] = make_pair(new_non_term, "e");
                prod_count++;
            }
        }
    }

    // Compute the FIRST set for a given symbol or string
    void first(string str, pair<string, string> *gram, int prod_count) {
        // Check if the input is a terminal or epsilon
        if (terms.find(str) != terms.end() || str == "e") {
            firsts[str].insert(str);
        } else if (non_terms.find(str) != non_terms.end()) {
            // If input is a non-terminal, process its productions
            for (int i = 0; i < prod_count; i++) {
                if (gram[i].first != str)
                    continue;
                string rhs = gram[i].second;
                string currentSymbol;

                // Iterate over the symbols in the production's RHS
                for (auto ch = rhs.begin(); ch != rhs.end(); ch++) {
                    if (*ch != ' ')
                        currentSymbol += *ch;
                    if (*ch == ' ' || next(ch) == rhs.end()) {
                        // Compute FIRST set for the current symbol if not already done
                        if (firsts[currentSymbol].empty())
                            first(currentSymbol, gram, prod_count);

                        // Add the FIRST set of the current symbol to the FIRST set of the non-terminal
                        if (firsts[currentSymbol].find("e") == firsts[currentSymbol].end()) {
                            firsts[str].insert(firsts[currentSymbol].begin(), firsts[currentSymbol].end());
                            break;
                        }

                        // Handle nullable symbols
                        if (next(ch) != rhs.end()) {
                            firsts[str].insert(firsts[currentSymbol].begin(), firsts[currentSymbol].end());
                            firsts[str].erase("e");
                            currentSymbol.clear();
                            continue;
                        }

                        firsts[str].insert(firsts[currentSymbol].begin(), firsts[currentSymbol].end());
                    }
                }
            }
        } else {
            // Handle complex strings with multiple symbols
            string currentSymbol;
            for (int i = 0; i < str.length(); i++) {
                if (str[i] != ' ')
                    currentSymbol += str[i];
                if (str[i] == ' ' || i + 1 == str.length()) {
                    // If current symbol is a terminal, add it to the FIRST set
                    if (terms.find(currentSymbol) != terms.end()) {
                        firsts[str].insert(currentSymbol);
                        break;
                    } else if (non_terms.find(currentSymbol) != non_terms.end()) {
                        // If current symbol is a non-terminal, compute its FIRST set
                        if (firsts[currentSymbol].empty())
                            first(currentSymbol, gram, prod_count);

                        // Add the FIRST set of the current symbol to the FIRST set of the string
                        if (firsts[currentSymbol].find("e") == firsts[currentSymbol].end()) {
                            firsts[str].insert(firsts[currentSymbol].begin(), firsts[currentSymbol].end());
                            break;
                        }

                        // Handle nullable symbols within strings
                        if (i + 1 != str.length()) {
                            firsts[str].insert(firsts[currentSymbol].begin(), firsts[currentSymbol].end());
                            firsts[str].erase("e");
                            currentSymbol.clear();
                            continue;
                        }

                        firsts[str].insert(firsts[currentSymbol].begin(), firsts[currentSymbol].end());
                    }
                }
            }
        }
    }

    // Compute FIRST sets for all non-terminals
    void findFirsts() {
        pair<string, string> *temp = new pair<string, string>[MAX_GRAMMAR_SIZE];
        copy(gram, gram + prod_count, temp);

        int count = prod_count;
        eliminateLeftRecursion(temp, count);

        for (auto non_term = non_terms.begin(); non_term != non_terms.end(); non_term++) {
            // Compute the FIRST set for non-terminals if not already computed
            if (firsts[*non_term].empty())
                first(*non_term, temp, count);
        }

        delete[] temp;
    }

    // Expand the closure of a set of LR(1) items
    void closure(vector<item> &items) {
        bool updated = true;

        while (updated) {
            updated = false;
            vector<item> newItems = items; // Copy of the current set of items

            // Iterate through all items in the current closure
            for (auto it = items.begin(); it != items.end(); it++) {
                string rhs = it->production.second; // Right-hand side of the production

                // Find the position of the dot (.) in the production
                int dotPos = rhs.find(".");
                int nextPos = dotPos + 2; // Position of the symbol after the dot

                // If there is no symbol after the dot, skip this item
                if (nextPos >= rhs.length())
                    continue;

                // Extract the next symbol after the dot
                int spacePos = rhs.find(" ", nextPos);
                if (spacePos == string::npos)
                    spacePos = rhs.length();

                string nextSymbol = rhs.substr(nextPos, spacePos - nextPos);

                // If the next symbol is a non-terminal, add its productions to the closure
                if (non_terms.find(nextSymbol) != non_terms.end()) {
                    for (int i = 0; i < prod_count; i++) {
                        if (gram[i].first == nextSymbol) {
                            string new_rhs = "."; // Start the new production with a dot
                            if (gram[i].second != "e")
                                new_rhs += " " + gram[i].second;

                            set<string> lookaheadSet;
                            if (spacePos + 1 < rhs.length()) {
                                // Compute the lookahead set for the new item
                                string rest = rhs.substr(spacePos + 1);
                                if (firsts[rest].empty())
                                    first(rest, gram, prod_count);
                                lookaheadSet = firsts[rest];
                                if (firsts[rest].find("e") != firsts[rest].end()) {
                                    lookaheadSet.erase("e");
                                    lookaheadSet.insert(it->lookahead.begin(), it->lookahead.end());
                                }
                            } else
                                lookaheadSet = it->lookahead;

                            // Check if the new item already exists in the closure
                            auto existing = newItems.end();
                            for (auto itItem = newItems.begin(); itItem != newItems.end(); itItem++) {
                                if (itItem->production == make_pair(nextSymbol, new_rhs)) {
                                    existing = itItem;
                                    break;
                                }
                            }
                            // Merge lookahead sets or add a new item
                            if (existing != newItems.end()) {
                                set<string> mergedLookahead = existing->lookahead;
                                int prevSize = mergedLookahead.size();
                                mergedLookahead.insert(lookaheadSet.begin(), lookaheadSet.end());

                                if (mergedLookahead.size() > prevSize) {
                                    newItems.erase(existing);
                                    newItems.push_back({ {nextSymbol, new_rhs}, mergedLookahead });
                                    updated = true;
                                }
                            } else {
                                newItems.push_back({ {nextSymbol, new_rhs}, lookaheadSet });
                                updated = true;
                            }
                        }
                    }
                }
            }

            items = newItems; // Update the closure with the new items
        }
    }

    // Compute the set of items transitioned to by a given symbol from the current items
    vector<item> GoTo(vector<item> items, string symbol) {
        vector<item> newItems; // Store the resulting items after transition

        // Iterate through all items to find transitions on the given symbol
        for (auto it = items.begin(); it != items.end(); it++) {
            string lhs = it->production.first; // Left-hand side of the production
            string rhs = it->production.second; // Right-hand side of the production

            int dotPos = rhs.find("."); // Position of the dot in the production
            int nextPos = dotPos + 2; // Position of the symbol after the dot

            // Skip items where the dot is at the end of the production
            if (nextPos >= rhs.length())
                continue;

            // Extract the symbol immediately after the dot
            int spacePos = rhs.find(" ", nextPos);
            if (spacePos == string::npos)
                spacePos = rhs.length();

            string nextSymbol = rhs.substr(nextPos, spacePos - nextPos);

            // If the next symbol matches the given symbol, create a new item
            if (nextSymbol == symbol) {
                string new_rhs = rhs.substr(0, dotPos) + nextSymbol + " ." + rhs.substr(spacePos); // Move the dot
                newItems.push_back({{lhs, new_rhs}, {it->lookahead}}); // Add the new item

                // Expand the closure for the new set of items
                closure(newItems);
            }
        }

        return newItems; // Return the set of items after transition
    }

    // Generate the canonical collection of LR(1) items for the grammar
    void canonicalItems() {
        // Initialize the start item with the augmented grammar's start production
        vector<item> startItem = {{{start_symbol, ". " + gram[0].second}, {"$"}}};
        closure(startItem); // Compute the closure of the start item
        canonical[state_count++] = startItem; // Add the start item to the canonical collection

        // Combine non-terminals and terminals into a single symbol set
        set<string> symbols;
        symbols.insert(non_terms.begin(), non_terms.end());
        symbols.insert(terms.begin(), terms.end());

        bool updated = true; // Flag to track if new states are added
        while (updated) {
            updated = false;
            map<int, vector<item>> newCanonical = canonical; // Copy the current states

            // Iterate over all existing states
            for (auto it = newCanonical.begin(); it != newCanonical.end(); it++) {
                for (auto symbol = symbols.begin(); symbol != symbols.end(); symbol++) {
                    vector<item> items; // Items transitioning on the current symbol

                    // Find items in the current state with a transition on the symbol
                    for (auto itItem = it->second.begin(); itItem != it->second.end(); itItem++) {
                        string lhs = itItem->production.first;
                        string rhs = itItem->production.second;
                        int dotPos = rhs.find(".");
                        int nextPos = dotPos + 2;

                        if (nextPos >= rhs.length()) {
                            reduceMap[it->first] = *itItem; // Mark the item for reduction
                            continue;
                        }

                        int spacePos = rhs.find(" ", nextPos);
                        if (spacePos == string::npos)
                            spacePos = rhs.length();

                        string nextSymbol = rhs.substr(nextPos, spacePos - nextPos);

                        if (nextSymbol == *symbol)
                            items.push_back(*itItem); // Add items transitioning on the symbol
                    }

                    if (!items.empty()) {
                        vector<item> state = GoTo(items, *symbol); // Compute the new state
                        bool exists = false;

                        // Check if the state already exists in the canonical collection
                        for (auto itState = newCanonical.begin(); itState != newCanonical.end(); itState++) {
                            if (itState->second == state) {
                                exists = true;
                                gotoMap[{it->first, *symbol}] = itState->first; // Link the transition
                                break;
                            }
                        }

                        // If the state is new, add it to the collection
                        if (!exists) {
                            gotoMap[{it->first, *symbol}] = state_count;
                            newCanonical[state_count++] = state;
                            updated = true; // Mark that the collection has been updated
                        }
                    }
                }
            }
            canonical = newCanonical; // Update the canonical collection
        }
    }

    // Generate grammar rules from an input file
    void generateGrammar() {
        fstream grammar_file;
        grammar_file.open(grammar); // Open the grammar file

        // Read grammar rules from the file
        while (!grammar_file.eof() && prod_count < MAX_GRAMMAR_SIZE) {
            char buffer[100]; // Buffer to store each line of the grammar
            grammar_file.getline(buffer, 99); // Read a line from the file
            string lhs; // Left-hand side of the production
            int i = 0;

            // Extract the left-hand side (LHS) of the production
            while (buffer[i] != ' ')
                lhs += buffer[i++];
            i += 4; // Skip over the " -> "

            string rhs; // Right-hand side of the production
            // Extract the right-hand side (RHS) and handle multiple productions
            while (buffer[i] != '\0') {
                rhs += buffer[i++];
                if (buffer[i] == '|') { // Handle multiple productions separated by '|'
                    rhs.pop_back(); // Remove trailing space before '|'
                    if (prod_count == 0)
                        gram[prod_count++] = make_pair(lhs + "'", lhs); // Augmented grammar start rule
                    gram[prod_count++] = make_pair(lhs, rhs); // Add the production
                    rhs.clear();
                    i += 2; // Skip over " | "
                }
            }
            if (prod_count == 0)
                gram[prod_count++] = make_pair(lhs + "'", lhs); // Augmented grammar start rule
            gram[prod_count++] = make_pair(lhs, rhs);  // Add the last production
        }

        // Identify all non-terminals from the grammar
        for (int i = 0; i < prod_count; i++)
            non_terms.insert(gram[i].first);

        // Identify all terminals from the grammar
        for (int i = 0; i < prod_count; i++) {
            string currentSymbol;
            for (auto ch = gram[i].second.begin(); ch != gram[i].second.end(); ch++) {
                if (*ch != ' ')
                    currentSymbol += *ch; // Build symbols character by character
                if (*ch == ' ' || next(ch) == gram[i].second.end()) {
                    if (non_terms.find(currentSymbol) == non_terms.end())
                        terms.insert(currentSymbol); // Add to terminals if not a non-terminal
                    currentSymbol.clear();
                }
            }
        }

        terms.erase("e"); // Remove epsilon from terminals
        terms.insert("$"); // Add the end of input marker as a terminal

        start_symbol = gram[0].first; // Set the starting symbol from the first production
    }

    // generate the parsing table for LR(1) grammar
    void generateParsingTable() {
        int numSymbols = non_terms.size() + terms.size() - 1; // Total number of symbols

        // Allocate memory for the parsing table and initialize it
        parseTable = new int*[state_count];
        for (int i = 0; i < state_count; i++)
            parseTable[i] = new int[numSymbols];
        for (int i = 0; i < state_count; i++)
            fill(parseTable[i], parseTable[i] + numSymbols, -100); // Initialize all cells with a default value

        // Populate the table with REDUCE actions from reduceMap
        for (auto it = reduceMap.begin(); it != reduceMap.end(); it++) {
            int state = it->first;
            item reduceItem = it->second;
            string lhs = reduceItem.production.first;
            string rhs = "e"; // Default RHS for empty production

            if (reduceItem.production.second != ".")
                rhs = reduceItem.production.second.substr(0, reduceItem.production.second.length() - 2);

            int col = distance(terms.begin(), terms.find("$")); // Column index for end-of-input symbol
            if (lhs == start_symbol) {
                parseTable[state][col] = 100; // ACCEPT action for the start production
            } else {
                for (auto itLookahead = reduceItem.lookahead.begin(); itLookahead != reduceItem.lookahead.end(); itLookahead++) {
                    col = distance(terms.begin(), terms.find(*itLookahead)); // Column index for the lookahead symbol
                    parseTable[state][col] = -(distance(gram, find(gram, gram + prod_count, make_pair(lhs, rhs)))); // REDUCE action
                }
            }
        }

        // Populate the table with SHIFT and GOTO actions from gotoMap
        set<string> new_non_terms = non_terms;
        new_non_terms.erase(start_symbol); // Exclude the start symbol from GOTO actions

        for (auto it = gotoMap.begin(); it != gotoMap.end(); it++) {
            pair<int, string> key = it->first;
            int state = key.first;
            string symbol = key.second;
            int nextState = it->second;
            int col;

            if (terms.find(symbol) != terms.end()) {
                col = distance(terms.begin(), terms.find(symbol)); // Column index for terminals
                parseTable[state][col] = nextState; // SHIFT action
            } else if (new_non_terms.find(symbol) != new_non_terms.end()) {
                col = distance(new_non_terms.begin(), new_non_terms.find(symbol)) + terms.size(); // Column index for non-terminals
                parseTable[state][col] = nextState; // GOTO action
            }
        }
    }

    // parse the input string using the generated parsing table
    void parse() {
        generateGrammar(); // Generate the grammar rules, terminals, and non-terminals

        findFirsts(); // Compute FIRST sets for all symbols

        tic(StartLR1); // Start timer for parsing

        canonicalItems(); // Generate the canonical collection of LR(1) items

        generateParsingTable(); // Generate the parsing table for the grammar

        // Define lexical error messages
        map<string, string> lexicalErrors = {
                {"invalid-char", "Illegal character."},
                {"invalid-num", "Invalid number format."},
                {"invalid-id", "Invalid identifier."}
        };

        stack<int> st, stTemp; // Stack for states
        st.push(0);

        stack<string> symbols, symbolsTemp; // Stack for grammar symbols

        string lookahead = currentToken.type; // Initialize the lookahead symbol

        int col = distance(terms.begin(), terms.find(lookahead)); // Find column index for lookahead
        int action = parseTable[st.top()][col]; // Get the parsing table action

        set<string> new_non_terms = non_terms;
        new_non_terms.erase(start_symbol); // Remove start symbol from non-terminals

        string stackResult;
        while (1) {
            // Log the current state stack
            stTemp = st;
            stackResult.clear();
            while (!stTemp.empty()) {
                stackResult = to_string(stTemp.top()) + " " + stackResult;
                stTemp.pop();
            }
            process[process_count][0] = stackResult;

            // Log the current symbol stack
            symbolsTemp = symbols;
            stackResult.clear();
            while (!symbolsTemp.empty()) {
                stackResult = symbolsTemp.top() + " " + stackResult;
                symbolsTemp.pop();
            }
            process[process_count++][1] = stackResult;

            // Determine the token representation based on its value
            string token = currentToken.value.empty() ? lookahead : currentToken.value;

            // Handle invalid tokens
            if (terms.find(lookahead) == terms.end()) {
                process[process_count + 1][2] += lookahead + " ";
                process[process_count - 1][3] = "error, skip '" + lookahead + "'.";
                if (lexicalErrors.find(lookahead) != lexicalErrors.end())
                    errors[error_count++] = "Lexical Error: Invalid token '<" + token + ">': " + lexicalErrors[lookahead] + " Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                else
                    errors[error_count++] = "Lexical Error: Invalid token '<" + token + ">'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                currentToken = lexer.getNextToken();
                lookahead = currentToken.type;
                accepted = false;
                continue;
            }

            // Handle errors and termination cases
            if (action == -100) {
                set<string> expect;
                for (int i = 0; i < terms.size(); i++) {
                    if (parseTable[st.top()][i] >= 0 && parseTable[st.top()][i] != 100) {
                        auto it = terms.begin();
                        advance(it, i);
                        expect.insert(*it);
                    }
                }

                string expected;
                for (auto it = expect.begin(); it != expect.end(); it++) {
                    expected += *it;
                    if (next(it) != expect.end())
                        expected += "' or '";
                }

                process[process_count - 1][3] = "error";
                if (token != "$") {
                    if (!expect.empty())
                        errors[error_count++] = "Syntax Error: Unexpected token '" + token + "'. Expected one of: '" + expected + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    else
                        errors[error_count++] = "Syntax Error: Unexpected token '" + token + "'. Expected end of input. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                } else {
                    if (!expect.empty())
                        errors[error_count++] = "Syntax Error: Unexpected end of input. Expected one of: '" + expected + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    else
                        errors[error_count++] = "Syntax Error: Unexpected end of input. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                }
                accepted = false;
                break;
            } else if (action == 100) { // ACCEPT case
                process[process_count - 1][3] = "accept";
                break;
            } else if (action >= 0) { // SHIFT case
                process[process_count + 1][2] += lookahead + " ";
                process[process_count - 1][3] = "shift";
                st.push(action);
                symbols.push(lookahead);
                currentToken = lexer.getNextToken();
                lookahead = currentToken.type;
                col = distance(terms.begin(), terms.find(lookahead));
                action = parseTable[st.top()][col];
            } else { // REDUCE case
                string lhs = gram[-action].first;
                string rhs = gram[-action].second;
                process[process_count - 1][3] = "reduce by " + lhs + " -> " + rhs;
                if (rhs != "e") {
                    int count = 0;
                    for (auto ch = rhs.begin(); ch != rhs.end(); ch++)
                        if (*ch == ' ' || next(ch) == rhs.end())
                            count++;
                    for (int i = 0; i < count; i++) {
                        st.pop();
                        symbols.pop();
                    }
                }
                int col2 = distance(new_non_terms.begin(), new_non_terms.find(lhs)) + terms.size();
                st.push(parseTable[st.top()][col2]);
                symbols.push(lhs);
                action = parseTable[st.top()][col];
            }
        }

        TimeLR1 = toc(StartLR1); // Stop timer for parsing
    }

    // Destructor to clean up dynamically allocated resources
    ~LR1Parser() {
        for (int i = 0; i < state_count; i++)
            delete[] parseTable[i];
        delete[] parseTable;
        delete[] gram;
        delete[] errors;
        for (int i = 0; i < MAX_PROCESS_SIZE; i++)
            delete[] process[i];
        delete[] process;
    }

};

// Display the menu for Canonical LR(1) Parsing options
int LR1Menu(LR1Parser &parser) {
    system("cls");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\n\n\n\n\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\t[#] Canonical LR(1) Parsing Parser [#]" << endl << endl;
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
    cout << "\t\t\t\t[*] Input Grammar [*]" << endl << endl;
    int num = 0;
    for (int i = 0; i < parser.prod_count; i++)
        if (parser.gram[i].first == parser.start_symbol)
            cout << "\t\t\t\t" << num++ << ". " << parser.gram[i].first << " -> " << parser.gram[i].second << endl;
    for (int i = 0; i < parser.prod_count; i++)
        if (parser.gram[i].first != parser.start_symbol)
            cout << "\t\t\t\t" << num++ << ". " << parser.gram[i].first << " -> " << parser.gram[i].second << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\n\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);
    cout << "\t\t\t\t[*] Input String [*]" << endl << endl;
    cout << "\t\t\t\t" << parser.lexer.input << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    if (parser.accepted) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
        cout << "\t\t\t\t[+] The Input String Was Accepted [+]" << endl << endl;
    } else {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
        cout << "\t\t\t\t[-] The Input String Was Rejected [-]" << endl << endl;
        for (int i = 0; i < parser.error_count; i++)
            cout << "\t\t\t\t[" << i + 1 << "] " << parser.errors[i] << endl << endl;
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;

    int option;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\t\t\t\t[+] Menu [+]" << endl << endl;
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\t[1] Change Input String" << endl << endl;
    cout << "\t\t\t\t[2] Change Input Grammar" << endl << endl;
    cout << "\t\t\t\t[3] Symbol Table" << endl << endl;
    cout << "\t\t\t\t[4] Canonical LR(1) Parsing Table" << endl << endl;
    cout << "\t\t\t\t[5] CLR Canonical Items" << endl << endl;
    cout << "\t\t\t\t[6] GoTo Table" << endl << endl;
    cout << "\t\t\t\t[7] Input Processing Table" << endl << endl;
    cout << "\t\t\t\t[0] Back to Main Menu" << endl << endl;
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\tPlease enter option : ";
    cin >> option;
    return option;
}

// Print the canonical collection of LR(1) items
void printCanonicalItems(LR1Parser &parser) {
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t-------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] CLR Canonical Items [#]\n\n";
        cout << "\t\t\t\t-------------------------------------------" << endl << endl;
        int count = 0;
        for (auto state = parser.canonical.begin(); state != parser.canonical.end(); state++) {
            cout << "\t\t\t\tI" << count++ << endl << endl;
            for (auto itItem = state->second.begin(); itItem != state->second.end(); itItem++) {
                cout << "\t\t\t\t[" <<itItem->production.first << " -> " << itItem->production.second << ", ";
                for (auto it = itItem->lookahead.begin(); it != itItem->lookahead.end(); it++) {
                    if (next(it) != itItem->lookahead.end())
                        cout << *it << " / ";
                    else
                        cout << *it;
                }
                cout << "]" << endl << endl;
            }
            cout << "\n\t\t\t\t-------------------------------------------" << endl << endl;
        }
        cout << "\t\t\t\t[0] Back To Menu" << endl << endl;
        cout << "\t\t\t\t-------------------------------------------" << endl << endl;
        cout << "\t\t\t\tPlease enter option : ";
        cin >> option;
    }
}

// Print the GOTO table for the LR(1) parser, showing state transitions for symbols
void printGoToTable(LR1Parser &parser) {
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t-------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Goto Table [#]\n\n";

        int rows = parser.gotoMap.size() + 1;
        int cols = 2;
        int width = 29;
        string data[rows][cols];
        data[0][0] = "GoTo";
        data[0][1] = "State";

        int count = 1;
        for (int i = 0; i < parser.state_count; i++) {
            for (auto it = parser.gotoMap.begin(); it != parser.gotoMap.end(); it++) {
                if (it->first.first == i) {
                    if (parser.non_terms.find(it->first.second) != parser.non_terms.end()) {
                        data[count][0] = "GoTo ( " + to_string(i) + ", " + it->first.second + " )";
                        data[count++][1] = to_string(it->second);
                    }
                }
            }
            for (auto it = parser.gotoMap.begin(); it != parser.gotoMap.end(); it++) {
                if (it->first.first == i) {
                    if (parser.terms.find(it->first.second) != parser.terms.end()) {
                        data[count][0] = "GoTo ( " + to_string(i) + ", " + it->first.second + " )";
                        data[count++][1] = to_string(it->second);
                    }
                }
            }
        }

        cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        for (int i = 0; i < rows; i++) {
            cout << "\t\t\t\t";
            cout << (char)186;
            for (int j = 0; j < cols; j++) {
                int padding = (width - data[i][j].length()) / 2;
                cout << right << setfill(' ') << setw(padding) << ' ';
                cout << left << setfill(' ') << setw(width - padding) << data[i][j];
                cout << (char)186;
            }
            cout << endl;
            if (i != rows - 1)
                cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        }
        cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        cout << "\n\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[0] Back To Menu" << endl << endl;
        cout << "\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\tPlease enter option : ";
        cin >> option;
    }
}

// Print the LL(1) predictive parsing table for the provided grammar
void printLR1ParsingTable(LR1Parser &parser) {
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t-------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[*] Input Grammar [*]" << endl << endl;
        int num = 0;
        for (int i = 0; i < parser.prod_count; i++)
            if (parser.gram[i].first == parser.start_symbol)
                cout << "\t\t\t\t" << num++ << ". " << parser.gram[i].first << " -> " << parser.gram[i].second << endl;
        for (int i = 0; i < parser.prod_count; i++)
            if (parser.gram[i].first != parser.start_symbol)
                cout << "\t\t\t\t" << num++ << ". " << parser.gram[i].first << " -> " << parser.gram[i].second << endl;

        cout << "\n\t\t\t\t-------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Canonical LR(1) Parsing Table [#]\n\n";

        int rows = parser.state_count + 2;
        int cols = parser.non_terms.size() + parser.terms.size();
        int width = 11;
        string data[rows][cols];
        data[0][1] = "Action";
        data[0][2] = "GoTo";
        data[1][0] = "State";

        set<string> new_non_terms = parser.non_terms;
        new_non_terms.erase(parser.start_symbol);
        auto non_term = new_non_terms.begin();
        auto term = parser.terms.begin();
        for (int i = 1; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (i == 1) {
                    if (j == 0)
                        continue;
                    if (term != parser.terms.end())
                        data[i][j] = *term++;
                    else
                        data[i][j] = *non_term++;
                } else if (j == 0) {
                    data[i][j] = to_string(i - 2);
                } else {
                    int action = parser.parseTable[i - 2][j - 1];
                    if (j - 1 < parser.terms.size()) {
                        if (action == 100)
                            data[i][j] = "acc";
                        else if (action >= 0)
                            data[i][j] = "s" + to_string(action);
                        else if (action > -100)
                            data[i][j] = "r" + to_string(-action);
                    } else {
                        if (action > -100)
                            data[i][j] = to_string(action);
                    }
                }
            }
        }

        cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        for (int i = 0; i < rows; i++) {
            cout << "\t\t\t\t";
            cout << (char)186;
            int padding;
            if (i == 0) {
                padding = (width - data[1][0].length()) / 2;
                cout << right << setfill(' ') << setw(padding) << ' ';
                cout << left << setfill(' ') << setw(width - padding) << data[0][0];
                cout << (char)186;

                padding = (parser.terms.size() * (width + 1) - 1 - data[0][1].length()) / 2;
                cout << right << setfill(' ') << setw(padding) << ' ';
                cout << left << setfill(' ') << setw(parser.terms.size() * (width + 1) - 1 - padding) << data[0][1];
                cout << (char)186;

                padding = (new_non_terms.size() * (width + 1) - 1 - data[0][2].length()) / 2;
                cout << right << setfill(' ') << setw(padding) << ' ';
                cout << left << setfill(' ') << setw(new_non_terms.size() * (width + 1) - 1 - padding) << data[0][2];
                cout << (char)186;

                cout << endl;
                cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;

                continue;
            }
            for (int j = 0; j < cols; j++) {
                padding = (width - data[i][j].length()) / 2;
                cout << right << setfill(' ') << setw(padding) << ' ';
                cout << left << setfill(' ') << setw(width - padding) << data[i][j];
                cout << (char)186;
            }
            cout << endl;
            if (i != rows - 1)
                cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        }
        cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        cout << "\n\t\t\t\t-------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[0] Back To Menu" << endl << endl;
        cout << "\t\t\t\t-------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\tPlease enter option : ";
        cin >> option;
    }
}

// Print the input processing table for the Non-Recursive Predictive Parser
void printLR1ProcessingTable(LR1Parser &parser) {
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Input Processing Table [#]\n\n";

        int rows = parser.process_count + 2;
        int cols = 4;
        int width = 36;
        string data[rows][cols];
        data[0][0] = "Stack";
        data[0][1] = "Symbols";
        data[0][2] = "Input";
        data[0][3] = "Action";

        data[1][2] = parser.lexer.getAllTokens();
        for (int i = 1; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (j != 2)
                    data[i][j] = parser.process[i - 1][j];
                else if (i > 1) {
                    if (!parser.process[i - 1][j].empty())
                        data[i][j] = data[i - 1][j].erase(0, parser.process[i - 1][j].length());
                    else
                        data[i][j] = data[i - 1][j];
                }
            }
        }

        cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        for (int i = 0; i < rows - 1; i++) {
            cout << "\t\t\t\t";
            cout << (char)186;
            for (int j = 0; j < cols; j++) {
                int padding = (width - data[i][j].length()) / 2;
                cout << right << setfill(' ') << setw(padding) << ' ';
                cout << left << setfill(' ') << setw(width - padding) << data[i][j];
                cout << (char)186;
            }
            cout << endl;
            if (i != rows - 2)
                cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        }
        cout << "\t\t\t\t " << setfill((char)205) << setw(width*cols+cols-1) << (char)205 << endl;
        cout << "\n\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[0] Back To Menu" << endl << endl;
        cout << "\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\tPlease enter option : ";
        cin >> option;
    }
}