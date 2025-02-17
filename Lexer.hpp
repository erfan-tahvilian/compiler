
// lexical token with its type and value
struct Token {
    string type; // Type of the token
    string value; // Lexical value of the token

    // Constructor to initialize token with type and value
    Token(string t = "", string v = "") : type(t), value(v) {}

    // Set the type of the token
    void setType(string t) {
        type = t;
    }

    // Set the value of the token
    void setValue(string v) {
        value = v;
    }
};

// symbol table for storing tokens
struct SymbolTable {
    Token token; // Token stored in the current entry
    SymbolTable *next; // Pointer to the next entry
    static int count; // Static counter to track the number of symbols

    // Default constructor initializes the linked list node
    SymbolTable() : next(NULL) {}

    // Constructor to initialize a node with a token
    SymbolTable(Token token) : token(token), next(NULL) {}

    // Insert a new token into the symbol table
    void insert(Token newToken) {
        SymbolTable *current = this;
        if (current->token.type.empty()) {
            // If the first node is empty, initialize it with the new token
            current->token.type = newToken.type;
            current->token.value = newToken.value;
        } else {
            // Traverse to the end of the linked list to insert the new token
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = new SymbolTable(newToken);
        }
        count++;
    }

    // Remove a token with the specified value from the symbol table
    bool remove(string value) {
        SymbolTable *current = this;
        SymbolTable *prev = NULL;

        while (current != NULL) {
            if (current->token.value == value) {
                if (prev == NULL) {
                    // Handle removal from the first node
                    if (current->next != NULL) {
                        token = current->next->token;
                        SymbolTable *temp = current->next;
                        next = current->next->next;
                        delete temp;
                    } else {
                        token = Token();
                        next = NULL;
                    }
                } else {
                    // Link the previous node to the next node, skipping the current node
                    prev->next = current->next;
                    delete current;
                }
                return true;
            }
            prev = current;
            current = current->next;
        }
        return false;
    }

    // Search for a token by its type and return a pointer to it
    Token* searchType(string type) {
        SymbolTable *current = this;
        while (current != NULL) {
            if (current->token.type == type)
                return &current->token;
            current = current->next;
        }
        return NULL;
    }

    // Search for a token by its value and return a pointer to it
    Token* searchValue(string value) {
        SymbolTable *current = this;
        while (current != NULL) {
            if (current->token.value == value)
                return &current->token;
            current = current->next;
        }
        return NULL;
    }

    // Destructor to clean up the linked list
    ~SymbolTable() {
        SymbolTable *current = next;
        while (current != NULL) {
            SymbolTable *temp = current->next;
            current->next = NULL;
            delete current;
            current = temp;
        }
        count--;
    }
};
int SymbolTable::count = 0;

// Lexer, Handles lexical analysis
struct Lexer {
    SymbolTable &table; // Reference to the symbol table for token management
    string input; // The input string being processed
    int pos; // Current position in the input string

    // Constructor initializes the lexer with a symbol table and input string
    Lexer(SymbolTable &t, const string input) : table(t), input(input), pos(0) {
        // Insert predefined tokens into the symbol table
        table.insert({"if"});
        table.insert({"else"});
        table.insert({"true"});
        table.insert({"false"});
        table.insert({"and"});
        table.insert({"or"});
        table.insert({"not"});
        table.insert({"int"});
        table.insert({"float"});
    }

    // Get the next token from the input string
    Token getNextToken() {
        // Skip whitespace characters to locate the next meaningful token
        while (isspace(input[pos]))
            pos++;

        // Check if the end of the input string is reached
        if (pos >= input.length())
            return {"$"};

        char current = input[pos];
        if (isalpha(current) || current == '_') {
            // Handle identifiers and keywords starting with an alphabet
            string id;
            id += input[pos++];
            while (isalnum(input[pos]) || input[pos] == '_')
                id += input[pos++];

            // Check if the identifier matches an existing token in the table
            Token *result = table.searchType(id);
            if (result == NULL) {
                Token token("id", id);
                result = table.searchValue(id);
                if (result == NULL)
                    table.insert(token); // Insert new identifiers into the symbol table
                return token;
            } else
                return *result; // Return existing token if found
        } else if (isdigit(current)) {
            // Handle numeric literals (integers and floating-point numbers)
            string num;
            bool flag = true; // Flag to distinguish between integer and float
            num += input[pos++];
            while (isdigit(input[pos]))
                num += input[pos++];
            if (input[pos] == '.') {
                flag = false; // Mark as floating-point number
                num += input[pos++];
                if (isdigit(input[pos])) {
                    num += input[pos++];
                    while (isdigit(input[pos]))
                        num += input[pos++];
                } else
                    return {"invalid-num", num}; // Handle invalid floating-point format
            }
            if (input[pos] == 'E' || input[pos] == 'e') {
                // Handle scientific notation for numbers
                num += input[pos++];
                if (input[pos] == '+' || input[pos] == '-')
                    num += input[pos++];
                if (isdigit(input[pos])) {
                    num += input[pos++];
                    while (isdigit(input[pos]))
                        num += input[pos++];
                } else if (!isalpha(input[pos]))
                    return {"invalid-num", num};
            }
            if (isalpha(input[pos]) || input[pos] == '_') {
                // Handle invalid numeric format with trailing alphabets
                num += input[pos++];
                while (isalnum(input[pos]))
                    num += input[pos++];
                return {"invalid-id", num};
            }
            Token token("", num);
            if (flag)
                token.setType("intNum"); // Assign type as integer
            else
                token.setType("floatNum"); // Assign type as float
            if (table.searchValue(num) == NULL)
                table.insert(token); // Insert number into the symbol table
            return token;
        } else if (current == '+' || current == '-' || current == '*' || current == '/' || current == '(' || current == ')' || current == '=') {
            // Handle operators and punctuations
            pos++;
            string s(1, current);
            Token token(s);
            if (table.searchType(s) == NULL)
                table.insert(token); // Insert operator/punctuation into the symbol table if not present
            return token;
        } else {
            // Handle invalid or unrecognized characters
            pos++;
            return {"invalid-char", string(1, current)};
        }
    }

    // Get all tokens from the input as a single string
    string getAllTokens() {
        SymbolTable temp;
        Lexer lexer(temp, input);
        string allTokens, nextTokenType;
        while (nextTokenType != "$") {
            nextTokenType = lexer.getNextToken().type;
            allTokens += nextTokenType + " ";
        }
        allTokens.pop_back(); // Remove trailing space
        return allTokens;
    }
};

// Display the contents of the symbol table
void printSymbolTable(SymbolTable *table) {
    int option = 1;
    while (option) {
        system("cls");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
        cout << "\n\n\n\n\t\t\t\t--------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[#] Symbol Table [#]\n\n";

        int rows = SymbolTable::count + 1;
        int cols = 3;
        int width = 30;
        string data[rows][cols];
        data[0][0] = "Row Number";
        data[0][1] = "Type";
        data[0][2] = "Attribute Value";

        SymbolTable *current = table;
        for (int i = 1; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (j == 0)
                    data[i][j] = to_string(i);
                else if (j == 1) {
                    data[i][j] = current->token.type;
                }
                else {
                    if (current->token.value.empty())
                        data[i][j] = "N/A";
                    else
                        data[i][j] = current->token.value;
                }
            }
            current = current->next;
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
        cout << "\n\t\t\t\t--------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\t[0] Back To Menu" << endl << endl;
        cout << "\t\t\t\t--------------------------------------------------------" << endl << endl;
        cout << "\t\t\t\tPlease enter option : ";
        cin >> option;
    }
}