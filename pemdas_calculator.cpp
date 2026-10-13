#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#include <cctype>

using namespace std;

// ---------------------------------------------------------------------------
// Token: either a number or a single-character operator / parenthesis
// ---------------------------------------------------------------------------
struct Token {
    bool isNumber;
    double value;   // used when isNumber is true
    char symbol;    // used when isNumber is false: + - * / ^ ( )
};

// ---------------------------------------------------------------------------
// Stack implemented with a singly linked list
// ---------------------------------------------------------------------------
template <typename T>
class LinkedStack {
private:
    struct Node {
        T data;
        Node* next;
    };
    Node* topNode;

public:
    LinkedStack() : topNode(nullptr) {}
    ~LinkedStack() { clear(); }

    bool isEmpty() const { return topNode == nullptr; }

    void push(const T& item) {
        Node* newNode = new Node{item, topNode};
        topNode = newNode;
    }

    T pop() {
        if (isEmpty()) throw string("Stack underflow.");
        Node* temp = topNode;
        T item = temp->data;
        topNode = topNode->next;
        delete temp;
        return item;
    }

    T peek() const {
        if (isEmpty()) throw string("Stack is empty.");
        return topNode->data;
    }

    void clear() {
        while (!isEmpty()) pop();
    }
};

// ---------------------------------------------------------------------------
// Queue implemented with a singly linked list (holds token sequences)
// ---------------------------------------------------------------------------
class TokenQueue {
private:
    struct Node {
        Token data;
        Node* next;
    };
    Node* front;
    Node* rear;

public:
    TokenQueue() : front(nullptr), rear(nullptr) {}
    ~TokenQueue() { clear(); }

    bool isEmpty() const { return front == nullptr; }

    void enqueue(const Token& t) {
        Node* newNode = new Node{t, nullptr};
        if (rear == nullptr) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
    }

    Token dequeue() {
        if (isEmpty()) throw string("Queue is empty.");
        Node* temp = front;
        Token t = temp->data;
        front = front->next;
        if (front == nullptr) rear = nullptr;
        delete temp;
        return t;
    }

    void clear() {
        while (!isEmpty()) dequeue();
    }

    // Builds a printable string of the queue contents (without removing them)
    string toString() const {
        string result;
        for (Node* cur = front; cur != nullptr; cur = cur->next) {
            if (!result.empty()) result += ' ';
            if (cur->data.isNumber) {
                result += to_string((long long)cur->data.value);
            } else {
                result += cur->data.symbol;
            }
        }
        return result;
    }
};

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------
bool isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

int precedence(char op) {
    if (op == '^') return 3;               // Exponents
    if (op == '*' || op == '/') return 2;  // Multiplication / Division
    if (op == '+' || op == '-') return 1;  // Addition / Subtraction
    return 0;                              // Parentheses
}

bool isRightAssociative(char op) {
    return op == '^';   // 2^3^2 = 2^(3^2)
}

// ---------------------------------------------------------------------------
// Step 1: Tokenize and validate (string manipulation)
// Throws a string describing the error if the expression is invalid.
// ---------------------------------------------------------------------------
void tokenize(const string& expr, TokenQueue& tokens) {
    bool expectOperand = true;   // true when the next item must be a number or '('
    int parenDepth = 0;
    size_t i = 0;

    while (i < expr.length()) {
        char c = expr[i];

        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }

        if (isdigit(c)) {
            if (!expectOperand)
                throw string("Missing operator before number at position ") + to_string(i + 1) + ".";
            double number = 0;
            while (i < expr.length() && isdigit(expr[i])) {
                number = number * 10 + (expr[i] - '0');
                i++;
            }
            if (i < expr.length() && expr[i] == '.')
                throw string("Only integers are allowed (found a decimal point).");
            tokens.enqueue({true, number, 0});
            expectOperand = false;
            continue;
        }

        if (c == '(') {
            if (!expectOperand)
                throw string("Missing operator before '(' at position ") + to_string(i + 1) + ".";
            tokens.enqueue({false, 0, '('});
            parenDepth++;
        } else if (c == ')') {
            if (expectOperand)
                throw string("Unexpected ')' at position ") + to_string(i + 1) + ".";
            parenDepth--;
            if (parenDepth < 0)
                throw string("Unmatched ')' at position ") + to_string(i + 1) + ".";
            tokens.enqueue({false, 0, ')'});
        } else if (isOperator(c)) {
            if (expectOperand) {
                if (c == '-')
                    throw string("Negative numbers are not allowed (position ") + to_string(i + 1) + ").";
                throw string("Operator '") + c + "' is missing a left operand (position " + to_string(i + 1) + ").";
            }
            tokens.enqueue({false, 0, c});
            expectOperand = true;
        } else {
            throw string("Invalid character '") + c + "' at position " + to_string(i + 1) + ".";
        }
        i++;
    }

    if (tokens.isEmpty())
        throw string("Expression is empty.");
    if (expectOperand)
        throw string("Expression ends with an operator or '('.");
    if (parenDepth != 0)
        throw string("Unmatched '('.");
}

// ---------------------------------------------------------------------------
// Step 2: Infix to postfix (Shunting-yard with a linked-list stack)
// ---------------------------------------------------------------------------
void infixToPostfix(TokenQueue& infix, TokenQueue& postfix) {
    LinkedStack<char> opStack;

    while (!infix.isEmpty()) {
        Token t = infix.dequeue();

        if (t.isNumber) {
            postfix.enqueue(t);
        } else if (t.symbol == '(') {
            opStack.push('(');
        } else if (t.symbol == ')') {
            while (opStack.peek() != '(') {
                postfix.enqueue({false, 0, opStack.pop()});
            }
            opStack.pop();   // discard '('
        } else {   // operator
            char op = t.symbol;
            while (!opStack.isEmpty() && opStack.peek() != '(') {
                char top = opStack.peek();
                bool popTop = precedence(top) > precedence(op) ||
                              (precedence(top) == precedence(op) && !isRightAssociative(op));
                if (!popTop) break;
                postfix.enqueue({false, 0, opStack.pop()});
            }
            opStack.push(op);
        }
    }

    while (!opStack.isEmpty()) {
        postfix.enqueue({false, 0, opStack.pop()});
    }
}

// ---------------------------------------------------------------------------
// Step 3: Evaluate postfix (linked-list stack of doubles)
// ---------------------------------------------------------------------------
double evaluatePostfix(TokenQueue& postfix) {
    LinkedStack<double> valStack;

    while (!postfix.isEmpty()) {
        Token t = postfix.dequeue();

        if (t.isNumber) {
            valStack.push(t.value);
        } else {
            double right = valStack.pop();
            double left = valStack.pop();
            double result = 0;

            switch (t.symbol) {
                case '+': result = left + right; break;
                case '-': result = left - right; break;
                case '*': result = left * right; break;
                case '/':
                    if (right == 0) throw string("Division by zero.");
                    result = left / right;
                    break;
                case '^': result = pow(left, right); break;
            }
            valStack.push(result);
        }
    }

    double finalResult = valStack.pop();
    if (!valStack.isEmpty()) throw string("Malformed expression.");
    return finalResult;
}

// ---------------------------------------------------------------------------
// Main program (follows the Main Program Flowchart)
// ---------------------------------------------------------------------------
int main() {
    string expression;
    char again = 'y';

    cout << "========================================\n";
    cout << "        PEMDAS EXPRESSION EVALUATOR      \n";
    cout << "========================================\n";
    cout << "Operators: + - * / ^ and parentheses ( )\n";
    cout << "Use positive integers only.\n";

    while (again == 'y' || again == 'Y') {
        cout << "\nEnter an expression: ";
        if (!getline(cin, expression)) break;

        TokenQueue infix, postfix;

        try {
            tokenize(expression, infix);              // Tokenize and validate
            infixToPostfix(infix, postfix);           // Infix to postfix
            cout << "Postfix: " << postfix.toString() << "\n";
            double result = evaluatePostfix(postfix); // Evaluate postfix

            if (isinf(result) || isnan(result))
                throw string("Result is too large to display.");

            cout << fixed << setprecision(4);
            cout << "Result:  " << result << "\n";    // Display result
        } catch (const string& error) {
            cout << "Error: " << error << "\n";       // Display error
            continue;                                  // Back to input
        }

        cout << "\nEvaluate another expression? (y/n): ";
        string answer;
        if (!getline(cin, answer)) break;
        again = answer.empty() ? 'n' : answer[0];
    }

    cout << "\nProgram terminated. Goodbye!\n";
    return 0;
}
