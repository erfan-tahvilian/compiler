
// Non-Recursive Predictive parser
struct LL1Parser {
    Lexer &lexer; // Reference to the lexer for tokenization
    string grammar; // File path to the input grammar
    string start_symbol; // Starting symbol of the grammar
    Token currentToken; // Current token being processed
    pair<string, string> *gram; // Array to store grammar rules as pairs of LHS and RHS
    int prod_count; // Number of grammar productions
    set<string> non_terms; // Set of non-terminal symbols
    set<string> terms; // Set of terminal symbols
    map<string, set<string>> firsts; // FIRST sets for grammar symbols
    map<string, set<string>> follows; // FOLLOW sets for non-terminals
    int **parseTable; // Parsing table
    string *errors; // Array to store error messages
    int error_count; // Number of errors encountered
    string **process; // Array to log processing steps
    int process_count; // Count of processed steps
    bool accepted; // flag to indicate whether input is accepted

    // Constructor to initialize the parser with grammar and lexer
    LL1Parser(Lexer &lexer, string grammar) : lexer(lexer), grammar(grammar), currentToken(lexer.getNextToken()), prod_count(0), error_count(0), process_count(0), accepted(true) {
        // Allocate memory for grammar rules, errors, and process details
        gram = new pair<string, string>[MAX_GRAMMAR_SIZE];
        errors = new string[MAX_ERROR_SIZE];
        process = new string*[MAX_PROCESS_SIZE];
        for (int i = 0; i < MAX_PROCESS_SIZE; i++)
            process[i] = new string[4];
    }

    // Check if the grammar is LL(1) compliant
    bool LL1() {
        // Check for intersections in FIRST sets of productions with the same LHS
        for (int i = 0; i < prod_count - 1; i++) {
            string lhs1 = gram[i].first; // Left-hand side of the first production
            string rhs1 = gram[i].second; // Right-hand side of the first production
            for (int j = i + 1; j < prod_count; j++) {
                string lhs2 = gram[j].first; // Left-hand side of the second production
                if (lhs1 == lhs2) { // Both productions share the same LHS
                    string rhs2 = gram[j].second; // Right-hand side of the second production

                    // Ensure FIRST sets for both RHS are computed
                    if (firsts[rhs1].empty())
                        first(rhs1);
                    if (firsts[rhs2].empty())
                        first(rhs2);

                    // Find intersection of FIRST sets
                    set<string> intersection;
                    set_intersection(
                            firsts[rhs1].begin(), firsts[rhs1].end(),
                            firsts[rhs2].begin(), firsts[rhs2].end(),
                            inserter(intersection, intersection.begin()));

                    // If intersection is not empty, grammar is not LL(1)
                    if (!intersection.empty())
                        return false;
                }
            }
        }

        // Check for intersections between FOLLOW and FIRST for nullable productions
        for (int i = 0; i < prod_count; i++) {
            string lhs1 = gram[i].first; // Left-hand side of the production
            string rhs1 = gram[i].second; // Right-hand side of the production
            if (firsts[rhs1].empty())
                first(rhs1); // Compute FIRST set if not already computed
            if (firsts[rhs1].find("e") != firsts[rhs1].end()) { // Check for epsilon (nullable production)
                for (int j = 0; j < prod_count; j++) {
                    string lhs2 = gram[j].first; // Compare with other productions
                    if (lhs1 == lhs2) {
                        string rhs2 = gram[j].second;
                        if (rhs1 != rhs2) {
                            set<string> intersection;

                            // Ensure FOLLOW and FIRST sets are computed
                            if (follows[lhs1].empty())
                                follow(lhs1);
                            set_intersection(
                                    follows[lhs1].begin(), follows[lhs1].end(),
                                    firsts[rhs2].begin(), firsts[rhs2].end(),
                                    inserter(intersection, intersection.begin()));

                            // If intersection is not empty, grammar is not LL(1)
                            if (!intersection.empty())
                                return false;
                        }
                    }
                }
            }
        }

        return true; // Grammar is LL(1) compliant
    }

    // Eliminate left recursion from the grammar
    void eliminateLeftRecursion() {
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

    // Apply left factoring to the grammar
    void leftFactoring() {
        for (auto i = non_terms.begin(); i != non_terms.end(); i++) {
            set<string> strings; // Collect all productions for the current non-terminal
            for (int j = 0; j < prod_count; j++) {
                string lhs = gram[j].first;
                if (lhs == *i) {
                    string rhs = gram[j].second;
                    strings.insert(rhs);
                }
            }

            // Group productions with common prefixes
            map<string, set<string>> groups;
            for (auto itStr = strings.begin(); itStr != strings.end(); itStr++) {
                string str = *itStr;
                bool added = false;
                for (auto itGroup = groups.begin(); itGroup != groups.end(); itGroup++) {
                    string prefix = itGroup->first;
                    set<string> &group = itGroup->second;
                    int minLength = min(prefix.size(), str.size());
                    int index = 0;
                    // Find the common prefix length
                    while (index < minLength && prefix[index] == str[index])
                        index++;
                    // Adjust index to end at the last complete symbol
                    while (index > 0 && index < str.length() && str[index] != ' ')
                        index--;
                    string commonPrefix = prefix.substr(0, index);
                    if (!commonPrefix.empty()) {
                        group.insert(str);
                        added = true;
                        break;
                    }
                }
                if (!added)
                    groups[str] = {str};
            }

            // Process groups with more than one production
            for (auto it = groups.begin(); it != groups.end(); it++) {
                set<string> &group = it->second;
                if (group.size() > 1) {
                    string prefix = it->first;
                    string commonPrefix = *group.begin();
                    for (auto itGroup = next(group.begin()); itGroup != group.end(); itGroup++) {
                        int minLength = min(commonPrefix.size(), (*itGroup).size());
                        int index = 0;
                        // Refine the common prefix
                        while (index < minLength && commonPrefix[index] == (*itGroup)[index])
                            index++;
                        while (index > 0 && index < commonPrefix.length() && commonPrefix[index] != ' ')
                            index--;
                        commonPrefix = commonPrefix.substr(0, index);
                    }

                    // Create a new non-terminal for the common prefix
                    string new_non_term = *i + "^";
                    non_terms.insert(new_non_term);
                    gram[prod_count++] = make_pair(*i, commonPrefix + " " + new_non_term);

                    // Replace original productions with the new non-terminal
                    for (auto itGroup = group.begin(); itGroup != group.end(); itGroup++) {
                        for (int j = 0; j < prod_count; j++) {
                            string rhs = gram[j].second;
                            if (rhs == *itGroup) {
                                for (int k = j; k < prod_count - 1; k++)
                                    gram[k] = gram[k + 1];
                                prod_count--;
                            }
                        }
                        string new_rhs = "e";
                        if (itGroup->length() > commonPrefix.length())
                            new_rhs = itGroup->substr(commonPrefix.length() + 1);
                        gram[prod_count++] = make_pair(new_non_term, new_rhs);
                    }
                }
            }
        }
    }

    // Compute the FIRST set for a given symbol or string
    void first(string str) {
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
                            first(currentSymbol);

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
                            first(currentSymbol);

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

    // Compute the FOLLOW set for a given non-terminal
    void follow(string non_term) {

        for (int i = 0; i < prod_count; i++) {
            string lhs = gram[i].first; // Left-hand side of the production
            string rhs = gram[i].second; // Right-hand side of the production
            string currentSymbol;

            // Iterate over each symbol in the production's RHS
            for (auto ch = rhs.begin(); ch != rhs.end(); ch++) {
                if (*ch != ' ')
                    currentSymbol += *ch; // Build the current symbol character by character

                if (*ch == ' ' || next(ch) == rhs.end()) {
                    // Check if the current symbol matches the non-terminal
                    if (currentSymbol == non_term) {

                        // Case 1: If the symbol is at the end of the RHS and not left-recursive
                        if (next(ch) == rhs.end() && lhs != non_term) {
                            // Ensure FOLLOW set of LHS is computed
                            if (follows[lhs].empty())
                                follow(lhs);
                            // Add FOLLOW(LHS) to FOLLOW(non_term)
                            follows[non_term].insert(follows[lhs].begin(), follows[lhs].end());

                        } else {
                            // Case 2: Handle symbols after the current non-terminal
                            string str(next(ch), rhs.end()); // Extract the remaining RHS

                            // Ensure FIRST set of the remaining string is computed
                            if (firsts[str].empty())
                                first(str);

                            // Add FIRST(set) to FOLLOW(non_term) if it doesn't contain epsilon
                            if (firsts[str].find("e") == firsts[str].end()) {
                                follows[non_term].insert(firsts[str].begin(), firsts[str].end());
                                currentSymbol.clear();
                                continue;
                            }

                            // Include FOLLOW(LHS) if epsilon is in FIRST(set)
                            follows[non_term].insert(firsts[str].begin(), firsts[str].end());
                            follows[non_term].erase("e");
                            if (follows[lhs].empty())
                                follow(lhs);
                            follows[non_term].insert(follows[lhs].begin(), follows[lhs].end());
                        }
                    }

                    // Reset currentSymbol for the next iteration
                    currentSymbol.clear();
                }
            }
        }
    }

    // Compute FIRST sets for all non-terminals
    void findFirsts() {
        for (auto non_term = non_terms.begin(); non_term != non_terms.end(); non_term++) {
            // Compute the FIRST set for non-terminals if not already computed
            if (firsts[*non_term].empty())
                first(*non_term);
        }
    }

    // Compute FOLLOW sets for all non-terminals
    void findFollows() {
        // Initialize FOLLOW set for the start symbol with "$" (end of input marker)
        follows[start_symbol].insert("$");
        follow(start_symbol); // Compute FOLLOW set for the start symbol

        // Compute FOLLOW sets for all non-terminals
        for (auto non_term = non_terms.begin(); non_term != non_terms.end(); non_term++) {
            follow(*non_term);
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
                    gram[prod_count++] = make_pair(lhs, rhs); // Add the production
                    rhs.clear();
                    i += 2; // Skip over " | "
                }
            }
            gram[prod_count++] = make_pair(lhs, rhs); // Add the last production
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

    // generate the parsing table for LL(1) grammar
    void generateParsingTable() {
        // Initialize the parsing table with -1 (indicating errors by default)
        parseTable = new int*[non_terms.size()]; // Allocate memory for non-terminal rows
        for (int i = 0; i < non_terms.size(); i++)
            parseTable[i] = new int[terms.size()]; // Allocate memory for terminal columns

        // Initialize all cells of the table with -1 (error state)
        for (int i = 0; i < non_terms.size(); i++)
            fill(parseTable[i], parseTable[i] + terms.size(), -1);

        // Populate the parsing table based on grammar productions
        for (int i = 0; i < prod_count; i++) {
            string lhs = gram[i].first; // Left-hand side of the production
            string rhs = gram[i].second; // Right-hand side of the production

            // Ensure FIRST set of the RHS is computed
            if (firsts[rhs].empty())
                first(rhs);

            // For each terminal in the FIRST set of RHS (excluding epsilon), populate the table
            for (auto it = firsts[rhs].begin(); it != firsts[rhs].end(); it++) {
                if (*it != "e") { // Ignore epsilon
                    int row = distance(non_terms.begin(), non_terms.find(lhs)); // Row index for LHS non-terminal
                    int col = distance(terms.begin(), terms.find(*it)); // Column index for the terminal
                    parseTable[row][col] = i; // Set the production index
                }
            }

            // Handle epsilon in FIRST set and populate table using FOLLOW set of LHS
            for (auto it = follows[lhs].begin(); it != follows[lhs].end(); it++) {
                int row = distance(non_terms.begin(), non_terms.find(lhs));
                int col = distance(terms.begin(), terms.find(*it));
                if (firsts[rhs].find("e") != firsts[rhs].end()) {
                    // If epsilon is in FIRST(RHS), populate table with FOLLOW(LHS)
                    parseTable[row][col] = i; // Set the production index
                } else if (parseTable[row][col] == -1) {
                    // If FOLLOW set entry is not already filled, mark it as -2 (synchronization point)
                    parseTable[row][col] = -2;
                }
            }
        }
    }

    // parse the input string using the generated parsing table
    void parse() {
        // Generate the grammar and prepare the grammar for LL(1) parsing
        generateGrammar(); // Generate grammar from input file
        eliminateLeftRecursion(); // Eliminate left recursion from the grammar
        leftFactoring(); // Apply left factoring to the grammar

        tic(StartLL1); // Start timer for parsing

        findFirsts(); // Compute FIRST sets for all symbols
        findFollows(); // Compute FOLLOW sets for all non-terminals

        // Check if the grammar is LL(1)
        if (!LL1())
            return; // Exit if grammar is not LL(1)

        generateParsingTable(); // Generate the parsing table for the LL(1) grammar

        // Define lexical error messages
        map<string, string> lexicalErrors = {
                {"invalid-char", "Illegal character."},
                {"invalid-num", "Invalid number format."},
                {"invalid-id", "Invalid identifier."}
        };

        // Initialize parsing stack
        stack<string> st;
        st.push("$"); // Push end-of-input marker
        st.push(start_symbol); // Push start symbol

        stack<string> temp; // Temporary stack for processing
        string matched; // Track matched tokens
        string stackResult; // Store current stack state
        bool flag = false; // Flag for end of parsing
        bool flag2 = false; // Secondary flag for error handling
        bool flag3 = false; // Third flag for error handling
        string lookahead = currentToken.type; // Current lookahead token

        while (st.top() != "$" || lookahead != "$") {
            // Log the current parsing stack
            temp = st;
            stackResult.clear();
            while (!temp.empty()) {
                stackResult += temp.top() + " ";
                temp.pop();
            }
            stackResult.pop_back();

            // Store matched tokens and stack state for process logging
            process[process_count][0] = matched;
            process[process_count++][1] = stackResult;

            if (flag) // Exit condition for error handling
                break;

            // Determine the token representation based on its value
            string token = currentToken.value.empty() ? lookahead : currentToken.value;

            // Handle invalid tokens (not in the terminal set)
            if (terms.find(lookahead) == terms.end()) {
                process[process_count + 1][2] += lookahead + " ";
                process[process_count][3] = "error, skip '" + lookahead + "'.";

                // Log lexical errors with column position
                if (lexicalErrors.find(lookahead) != lexicalErrors.end())
                    errors[error_count++] = "Lexical Error: Invalid token '<" + token + ">': " + lexicalErrors[lookahead] + " Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                else
                    errors[error_count++] = "Lexical Error: Invalid token '<" + token + ">'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";

                // Advance to the next token and update lookahead
                currentToken = lexer.getNextToken();
                lookahead = currentToken.type;
                if (lookahead == "$")
                    flag = true;

                accepted = false;
                continue;
            }

            // If lookahead matches the top of the stack
            if (lookahead == st.top()) {
                matched += lookahead + " ";
                process[process_count + 1][2] += lookahead + " ";
                process[process_count][3] = "match " + lookahead;

                st.pop(); // Consume the terminal from the stack
                currentToken = lexer.getNextToken(); // Move to the next token
                lookahead = currentToken.type;
                continue;
            }
            // Handle syntax error for unexpected stack top
            else if (non_terms.find(st.top()) == non_terms.end()) {
                process[process_count][3] = "error, '" + st.top() + "' has been popped.";
                if (st.top() != "$")
                    st.pop();
                else
                    flag = true;

                if (token != "$")
                    errors[error_count++] = "Syntax Error: Expected end of input, but found '" + token + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                else if (!flag3) {
                    errors[error_count++] = "Syntax Error: Unexpected end of input. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    flag3 = true;
                }

                accepted = false;
                continue;
            }
            // Retrieve parsing table entry for the non-terminal and terminal pair
            else {
                string stack_top = st.top();
                int row = distance(non_terms.begin(), non_terms.find(stack_top));
                int col = distance(terms.begin(), terms.find(lookahead));
                int prod_num = parseTable[row][col];

                // Handle parsing errors based on parsing table entry
                if (prod_num == -1) {
                    set<string> expect;
                    expect.insert(firsts[st.top()].begin(), firsts[st.top()].end());
                    expect.erase("e");

                    string expected;
                    for (auto it = expect.begin(); it != expect.end(); it++) {
                        expected += *it;
                        if (next(it) != expect.end())
                            expected += "' or '";
                    }

                    process[process_count][3] = "error, '" + lookahead + "' has been popped.";
                    currentToken = lexer.getNextToken();
                    lookahead = currentToken.type;
                    if (token != "$")
                        errors[error_count++] = "Syntax Error: Unexpected token '" + token + "'. Expected one of: '" + expected + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    else
                        errors[error_count++] = "Syntax Error: Unexpected end of input. Expected one of: '" + expected + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    accepted = false;
                    continue;
                } else if (prod_num == -2) {
                    // Synchronization point error handling
                    set<string> expect;
                    expect.insert(firsts[st.top()].begin(), firsts[st.top()].end());
                    expect.erase("e");

                    string expected;
                    for (auto it = expect.begin(); it != expect.end(); it++) {
                        expected += *it;
                        if (next(it) != expect.end())
                            expected += "' or '";
                    }

                    string top = st.top();
                    st.pop();
                    if (st.top() == "$") {
                        st.push(top);
                        string skip;
                        if (firsts[top].find(lookahead) != firsts[top].end() && flag2) {
                            flag = true;
                            continue;
                        }
                        while (firsts[top].find(lookahead) == firsts[top].end()) {
                            if (lookahead == "$") {
                                flag = true;
                                break;
                            }
                            process[process_count + 1][2] += lookahead + " ";
                            skip += "'" + lookahead + "'";
                            currentToken = lexer.getNextToken();
                            lookahead = currentToken.type;
                            if (firsts[top].find(lookahead) == firsts[top].end())
                                skip += ", ";
                        }
                        process[process_count][3] = "error, skip " + skip + ". '" + lookahead + "' is in FIRST(" + top + ")";
                        if (token != "$")
                            errors[error_count++] = "Syntax Error: Unexpected token '" + token + "'. Expected one of: '" + expected + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                        else
                            errors[error_count++] = "Syntax Error: Unexpected end of input. Expected one of: '" + expected + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    } else {
                        process[process_count][3] = "error, M[" + top + "," + lookahead + "] = synch. '" + top +"' has been popped.";
                        if (token != "$")
                            errors[error_count++] = "Syntax Error: Missing '" + expected + "' before '" + token + "'. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                        else
                            errors[error_count++] = "Syntax Error: Missing '" + expected + "' before end of input. Column number: [" + to_string(lexer.pos - token.length() + 1) + "]";
                    }
                    flag2 = true;
                    accepted = false;
                    continue;
                }

                // Pop the current non-terminal and push the production RHS onto the stack
                st.pop();
                string lhs = gram[prod_num].first;
                string rhs = gram[prod_num].second;
                process[process_count][3] = "output " + lhs + " -> " + rhs;

                if (rhs == "e") // Skip empty productions
                    continue;

                // Push RHS symbols onto the stack in reverse order
                string currentSymbol;
                for (auto ch = rhs.rbegin(); ch != rhs.rend(); ch++) {
                    if (*ch != ' ')
                        currentSymbol = *ch + currentSymbol;
                    if (*ch == ' ' || next(ch) == rhs.rend()) {
                        st.push(currentSymbol);
                        currentSymbol.clear();
                    }
                }
            }
        }

        // Final processing for the stack and matched tokens
        temp = st;
        stackResult.clear();
        while (!temp.empty()) {
            stackResult += temp.top() + " ";
            temp.pop();
        }
        stackResult.pop_back();

        process[process_count][0] = matched;
        process[process_count++][1] = stackResult;

        TimeLL1 = toc(StartLL1); // Stop timer for parsing
    }

    // Destructor to clean up dynamically allocated resources
    ~LL1Parser() {
        if (LL1()) {
            for (int i = 0; i < non_terms.size(); i++)
                delete[] parseTable[i];
            delete[] parseTable;
        }
        delete[] gram;
        delete[] errors;
        for (int i = 0; i < MAX_PROCESS_SIZE; i++)
            delete[] process[i];
        delete[] process;
    }

};

// Display the menu for Non-Recursive Predictive Parsing options
int LL1Menu(LL1Parser &parser) {
    system("cls");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "\n\n\n\n\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\t[#] Non-Recursive LL(1) Predictive Parser [#]" << endl << endl;
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
    cout << "\t\t\t\t[*] Input Grammar [*]" << endl << endl;
    int num = 1;
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
    if (!parser.LL1()) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
        cout << "\t\t\t\tThe input grammar is not LL(1)." << endl << endl;
    }
    else if (parser.accepted) {
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
    cout << "\t\t\t\t[4] Non-Recursive LL(1) Predictive Parsing Table" << endl << endl;
    cout << "\t\t\t\t[5] Non Terminals FIRST & FOLLOW" << endl << endl;
    cout << "\t\t\t\t[6] Input Processing Table" << endl << endl;
    cout << "\t\t\t\t[0] Back to Main Menu" << endl << endl;
    cout << "\t\t\t\t------------------------------------------------------------------------" << endl << endl;
    cout << "\t\t\t\tPlease enter option : ";
    cin >> option;
    return option;
}

// Print the LL(1) predictive parsing table for the provided grammar
void printLL1ParsingTable(LL1Parser &parser) {
    if (!parser.LL1()) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
        cout << "\n\n\n\n\t\t\t\tThe input grammar is not LL(1)." << endl << endl;
        Sleep(2000);
        return;
    }
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Non Recursive Predictive Parsing Table [#]\n\n";

        int rows = parser.non_terms.size() + 1;
        int cols = parser.terms.size() + 1;
        int width = 16;
        string data[rows][cols];
        data[0][0] = "Non Terminal";

        auto non_term = parser.non_terms.begin();
        auto term = parser.terms.begin();
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (i == 0) {
                    if (j == 0)
                        continue;
                    data[i][j] = *term++;
                } else if (j == 0)
                    data[i][j] = *non_term++;
                else {
                    int prod_num = parser.parseTable[i - 1][j - 1];
                    if (prod_num == -2)
                        data[i][j] = "synch";
                    else if (prod_num != -1)
                        data[i][j] = parser.gram[prod_num].first + " -> " + parser.gram[prod_num].second;
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
        cout << "\n\t\t\t\t---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[0] Back To Menu" << endl << endl;
        cout << "\t\t\t\t---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\tPlease enter option : ";
        cin >> option;
    }
}

// Print the FIRST and FOLLOW sets for all non-terminals in a tabular format
void printFirstFollowTable(LL1Parser &parser) {
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Non Terminals FIRST & FOLLOW [#]\n\n";

        int rows = parser.non_terms.size() + 1;
        int cols = 3;
        int width = 30;
        string data[rows][cols];
        data[0][0] = "Non Terminal";
        data[0][1] = "FIRST";
        data[0][2] = "FOLLOW";

        auto non_term = parser.non_terms.begin();
        for (int i = 1; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (j == 0) {
                    data[i][j] = *non_term;
                } else if (j == 1) {
                    for (auto it = parser.firsts[*non_term].begin(); it != parser.firsts[*non_term].end(); it++) {
                        data[i][j] += *it;
                        if (next(it) != parser.firsts[*non_term].end())
                            data[i][j] += " , ";
                    }
                }
                else {
                    for (auto it = parser.follows[*non_term].begin(); it != parser.follows[*non_term].end(); it++) {
                        data[i][j] += *it;
                        if (next(it) != parser.follows[*non_term].end())
                            data[i][j] += " , ";
                    }
                    non_term++;
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

// Print the input processing table for the Non-Recursive Predictive Parser
void printLL1ProcessingTable(LL1Parser &parser) {
    if (!parser.LL1()) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
        cout << "\n\n\n\n\t\t\t\tThe input grammar is not LL(1)." << endl << endl;
        Sleep(2000);
        return;
    }
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t-------------------------------------------------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Input Processing Table [#]\n\n";

        int rows = parser.process_count + 1;
        int cols = 4;
        int width = 36;
        string data[rows][cols];
        data[0][0] = "Matched";
        data[0][1] = "Stack";
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