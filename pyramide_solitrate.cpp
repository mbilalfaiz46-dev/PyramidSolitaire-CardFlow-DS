#include "raylib.h"
#include <iostream>
#include <ctime>
#include <fstream>
#include <cstdlib>

using namespace std;

// Game State Enum
enum GameState {
    MAIN_MENU,
    INSTRUCTIONS,
    PLAYING,
    GAME_OVER
};

// ============================================
// MY CONTRIBUTION: DATA STRUCTURES
// ============================================

// Card class
class Card
{
public:
    int value;      // 1–13 (Ace to King)
    int suit;       // 0–3 (Hearts, Diamonds, Clubs, Spades)
    bool faceUp;
    bool inPlay;

    Card()
    {
        value = 0;
        suit = 0;
        faceUp = false;
        inPlay = true;
    }

    Card(int v, int s)
    {
        value = v;
        suit = s;
        faceUp = false;
        inPlay = true;
    }
};

// Linked List Node
template <class T>
class ListNode
{
public:
    T data;
    ListNode<T>* next;

    ListNode(T d)
    {
        data = d;
        next = NULL;
    }
};

// Linked List class
template<typename T>
class LinkedList {
private:
    ListNode<T>* head;
    ListNode<T>* tail;
    int size;

public:
    LinkedList()
    {
        head = NULL;
        tail = NULL;
        size = 0;
    }

    ~LinkedList() {
        clear();
    }

    void pushBack(T data) {
        ListNode<T>* newNode = new ListNode<T>(data);
        if (!head) {
            head = tail = newNode;
        }
        else {
            tail->next = newNode;
            tail = newNode;
        }
        size++;
    }

    void pushFront(T data) {
        ListNode<T>* newNode = new ListNode<T>(data);
        if (!head) {
            head = tail = newNode;
        }
        else {
            newNode->next = head;
            head = newNode;
        }
        size++;
    }

    T popBack() {
        if (!head)
            return T();

        if (head == tail) {
            T data = head->data;
            delete head;
            head = tail = nullptr;
            size--;
            return data;
        }

        ListNode<T>* current = head;
        while (current->next != tail) {
            current = current->next;
        }

        T data = tail->data;
        delete tail;
        tail = current;
        tail->next = nullptr;
        size--;
        return data;
    }

    T popFront() {
        if (!head)
            return T();

        T data = head->data;
        ListNode<T>* temp = head;
        head = head->next;
        delete temp;
        size--;

        if (!head)
            tail = nullptr;

        return data;
    }

    T back() {
        if (tail)
            return tail->data;

        return T();
    }

    T front() {
        if (head)
            return head->data;

        return T();
    }

    bool isEmpty() {
        return head == nullptr;
    }

    int getSize() {
        return size;
    }

    void clear() {
        while (head) {
            ListNode<T>* temp = head;
            head = head->next;
            delete temp;
        }

        tail = nullptr;
        size = 0;
    }

    ListNode<T>* getHead() {
        return head;
    }

    void remove(T data) {
        if (!head)
            return;

        if (head->data == data) {
            popFront();
            return;
        }

        ListNode<T>* current = head;
        while (current->next) {
            if (current->next->data == data) {
                ListNode<T>* temp = current->next;
                current->next = temp->next;
                if (temp == tail) tail = current;
                delete temp;
                size--;
                return;
            }
            current = current->next;
        }
    }
};

// Pyramid Node (from previous team member)
class PyramidNode
{
public:
    Card* card;
    PyramidNode* left;
    PyramidNode* right;
    PyramidNode* nextInRow;
    int row;
    int col;
    bool blocked;

    PyramidNode(Card* c, int r, int cl)
    {
        card = c;
        left = NULL;
        right = NULL;
        nextInRow = NULL;
        row = r;
        col = cl;
        blocked = true;
    }
};

// Game class
class PyramidSolitaire {
private:
    // MY CONTRIBUTION: Card Flow Data Members
    LinkedList<Card> deck;
    LinkedList<Card*> stock;
    LinkedList<Card*> stockBackup;
    LinkedList<Card*> wasteHistory;
    Card* currentWasteCard;
    Card allCards[52];
    int cardCount;
    int stockPosition;
    int moves;

    // Previous team member's data
    PyramidNode* pyramidRows[7];
    Card* selectedCard1;
    Card* selectedCard2;
    PyramidNode* selectedNode1;
    PyramidNode* selectedNode2;

    int score;
    float gameTime;
    bool gameWon;
    bool gameLost;

    Texture2D stockTexture;
    Texture2D background;
    Texture2D cardTextures[4][13];

    GameState state;

    const int CARD_WIDTH = 90;
    const int CARD_HEIGHT = 130;
    const int CARD_SPACING = 20;

    Rectangle stockRect;

public:
    PyramidSolitaire() {
        for (int i = 0; i < 7; i++) {
            pyramidRows[i] = nullptr;
        }

        selectedCard1 = nullptr;
        selectedCard2 = nullptr;
        selectedNode1 = nullptr;
        selectedNode2 = nullptr;
        currentWasteCard = nullptr;
        score = 0;
        moves = 0;
        gameTime = 0.0f;
        gameWon = false;
        gameLost = false;
        cardCount = 0;
        stockPosition = 0;
        state = MAIN_MENU;

        loadCardTextures();
        stockRect = { 0, 0, 0, 0 };
    }

    ~PyramidSolitaire() {
        for (int s = 0; s < 4; s++) {
            for (int v = 0; v < 13; v++) {
                UnloadTexture(cardTextures[s][v]);
            }
        }
        UnloadTexture(background);
        UnloadTexture(stockTexture);
        clearPyramid();
    }

    void loadCardTextures() {
        const char* suits[4] = { "H", "D", "C", "S" };
        const char* values[13] = {
            "A","2","3","4","5","6","7","8","9","10","J","Q","K"
        };

        for (int s = 0; s < 4; s++) {
            for (int v = 0; v < 13; v++) {
                string path = "images/";
                path += values[v];
                path += suits[s];
                path += ".JPG";
                cardTextures[s][v] = LoadTexture(path.c_str());
            }
        }

        stockTexture = LoadTexture("images/stock.jpg");
        background = LoadTexture("images/background.jpg");
    }

    // ============================================
    // MY CONTRIBUTION: CARD FLOW LOGIC
    // ============================================

    void createDeck() {
        cardCount = 0;
        for (int suit = 0; suit < 4; suit++) {
            for (int value = 1; value <= 13; value++) {
                allCards[cardCount] = Card(value, suit);
                deck.pushBack(allCards[cardCount]);
                cardCount++;
            }
        }
    }

    void shuffleDeck() {
        srand(time(nullptr));

        Card tempDeck[52];
        int index = 0;
        ListNode<Card>* current = deck.getHead();
        while (current) {
            tempDeck[index++] = current->data;
            current = current->next;
        }

        for (int i = 51; i > 0; i--) {
            int j = rand() % (i + 1);
            swap(tempDeck[i], tempDeck[j]);
        }

        for (int i = 0; i < 52; i++) {
            allCards[i] = tempDeck[i];
        }

        deck.clear();
        for (int i = 0; i < 52; i++) {
            deck.pushBack(allCards[i]);
        }
    }

    void drawCardFromStock() {
        moves++;

        if (stock.isEmpty()) {
            LinkedList<Card*> tempList;
            ListNode<Card*>* backupNode = stockBackup.getHead();
            while (backupNode) {
                if (backupNode->data->inPlay) {
                    tempList.pushBack(backupNode->data);
                }
                backupNode = backupNode->next;
            }

            stock.clear();
            ListNode<Card*>* tempNode = tempList.getHead();
            while (tempNode) {
                stock.pushBack(tempNode->data);
                tempNode = tempNode->next;
            }

            stockPosition = 0;
        }

        Card* card = stock.popFront();
        card->faceUp = true;
        currentWasteCard = card;
        wasteHistory.pushBack(card);
        stockPosition++;
    }

    void removeCards() {
        if (selectedCard1 && isKing(selectedCard1)) {
            selectedCard1->inPlay = false;

            if (currentWasteCard == selectedCard1) {
                wasteHistory.remove(currentWasteCard);
                if (wasteHistory.isEmpty())
                {
                    currentWasteCard = NULL;
                }
                else
                {
                    currentWasteCard = wasteHistory.back();
                }
            }

            score += 10;
            moves++;
            selectedCard1 = nullptr;
            selectedNode1 = nullptr;
            updateBlockedStatus();
            checkWinCondition();
            return;
        }

        if (selectedCard1 && selectedCard2 && isValidMove(selectedCard1, selectedCard2)) {
            selectedCard1->inPlay = false;
            selectedCard2->inPlay = false;

            if (currentWasteCard == selectedCard1 || currentWasteCard == selectedCard2) {
                if (currentWasteCard == selectedCard1) {
                    wasteHistory.remove(selectedCard1);
                }
                if (currentWasteCard == selectedCard2) {
                    wasteHistory.remove(selectedCard2);
                }

                if (wasteHistory.isEmpty())
                    currentWasteCard = NULL;
                else
                    currentWasteCard = wasteHistory.back();
            }

            score += 20;
            moves++;

            selectedCard1 = nullptr;
            selectedCard2 = nullptr;
            selectedNode1 = nullptr;
            selectedNode2 = nullptr;

            updateBlockedStatus();
            checkWinCondition();
        }
        else if (selectedCard1 && selectedCard2) {
            moves++;
            selectedCard1 = nullptr;
            selectedCard2 = nullptr;
            selectedNode1 = nullptr;
            selectedNode2 = nullptr;
        }
    }

    void initGame() {
        clearPyramid();
        deck.clear();
        stock.clear();
        stockBackup.clear();
        wasteHistory.clear();

        selectedCard1 = nullptr;
        selectedCard2 = nullptr;
        selectedNode1 = nullptr;
        selectedNode2 = nullptr;
        currentWasteCard = nullptr;
        score = 0;
        moves = 0;
        gameTime = 0.0f;
        gameWon = false;
        gameLost = false;
        cardCount = 0;
        stockPosition = 0;

        createDeck();
        shuffleDeck();
        createPyramid();

        for (int i = 28; i < 52; i++) {
            stock.pushBack(&allCards[i]);
            stockBackup.pushBack(&allCards[i]);
        }

        state = PLAYING;
    }

    // ============================================
    // PREVIOUS TEAM MEMBER'S FUNCTIONS
    // ============================================

    void createPyramid() {
        int cardIndex = 0;
        PyramidNode* prevRowHeads[7] = { nullptr };

        for (int row = 0; row < 7; row++) {
            PyramidNode* rowHead = nullptr;
            PyramidNode* rowTail = nullptr;

            for (int col = 0; col <= row; col++) {
                Card* card = &allCards[cardIndex++];
                card->faceUp = true;
                PyramidNode* node = new PyramidNode(card, row, col);

                if (!rowHead) {
                    rowHead = node;
                    rowTail = node;
                }
                else {
                    rowTail->nextInRow = node;
                    rowTail = node;
                }

                if (row > 0) {
                    PyramidNode* prevRowNode = prevRowHeads[row - 1];
                    int count = 0;
                    while (prevRowNode && count < col) {
                        prevRowNode = prevRowNode->nextInRow;
                        count++;
                    }

                    if (prevRowNode && count == col) {
                        if (col < row) {
                            prevRowNode->left = node;
                        }
                        if (col > 0) {
                            PyramidNode* leftParent = prevRowHeads[row - 1];
                            count = 0;
                            while (leftParent && count < col - 1) {
                                leftParent = leftParent->nextInRow;
                                count++;
                            }
                            if (leftParent) {
                                leftParent->right = node;
                            }
                        }
                    }
                }

                if (row == 6) {
                    node->blocked = false;
                }
            }

            pyramidRows[row] = rowHead;
            prevRowHeads[row] = rowHead;
        }
    }

    void clearPyramid() {
        for (int row = 0; row < 7; row++) {
            PyramidNode* current = pyramidRows[row];
            while (current) {
                PyramidNode* next = current->nextInRow;
                delete current;
                current = next;
            }
            pyramidRows[row] = nullptr;
        }
    }

    void updateBlockedStatus() {
        for (int row = 0; row < 7; row++) {
            PyramidNode* current = pyramidRows[row];
            while (current) {
                if (current->card && current->card->inPlay) {
                    bool leftBlocking = (current->left && current->left->card && current->left->card->inPlay);
                    bool rightBlocking = (current->right && current->right->card && current->right->card->inPlay);
                    current->blocked = leftBlocking || rightBlocking;
                }
                current = current->nextInRow;
            }
        }
    }

    bool isCardFree(PyramidNode* node) {
        if (!node || !node->card || !node->card->inPlay)
            return false;

        return !node->blocked;
    }

    bool isValidMove(Card* c1, Card* c2) {
        if (!c1 || !c2)
            return false;

        if (!c1->inPlay || !c2->inPlay)
            return false;

        return (c1->value + c2->value) == 13;
    }

    bool isKing(Card* c) {
        if (!c)
            return false;

        return c->value == 13;
    }

    void selectCard(Card* card, PyramidNode* node) {
        if (!card || !card->inPlay)
            return;

        if (node && !isCardFree(node))
            return;

        if (isKing(card)) {
            selectedCard1 = card;
            selectedNode1 = node;
            selectedCard2 = nullptr;
            selectedNode2 = nullptr;
            removeCards();
            return;
        }

        if (!selectedCard1) {
            selectedCard1 = card;
            selectedNode1 = node;
        }
        else if (selectedCard1 == card) {
            selectedCard1 = nullptr;
            selectedNode1 = nullptr;
        }
        else {
            selectedCard2 = card;
            selectedNode2 = node;
            removeCards();
        }
    }

    void checkWinCondition() {
        bool allRemoved = true;
        for (int row = 0; row < 7; row++) {
            PyramidNode* current = pyramidRows[row];
            while (current) {
                if (current->card && current->card->inPlay) {
                    allRemoved = false;
                    break;
                }
                current = current->nextInRow;
            }
            if (!allRemoved)
                break;
        }

        if (allRemoved) {
            gameWon = true;
        }
    }

    void checkLoseCondition() {
        LinkedList<Card*> freeCards;

        for (int row = 0; row < 7; row++) {
            PyramidNode* current = pyramidRows[row];
            while (current) {
                if (isCardFree(current)) {
                    freeCards.pushBack(current->card);
                }
                current = current->nextInRow;
            }
        }

        if (currentWasteCard && currentWasteCard->inPlay) {
            freeCards.pushBack(currentWasteCard);
        }

        ListNode<Card*>* checkNode = freeCards.getHead();
        while (checkNode) {
            if (isKing(checkNode->data))
                return;

            checkNode = checkNode->next;
        }

        ListNode<Card*>* node1 = freeCards.getHead();
        while (node1) {
            ListNode<Card*>* node2 = node1->next;
            while (node2) {
                if (isValidMove(node1->data, node2->data)) {
                    return;
                }
                node2 = node2->next;
            }
            node1 = node1->next;
        }

        if (!stock.isEmpty())
            return;

        ListNode<Card*>* backupCheckNode = stockBackup.getHead();
        while (backupCheckNode) {
            if (backupCheckNode->data->inPlay)
                return;

            backupCheckNode = backupCheckNode->next;
        }

        gameLost = true;
    }

    void handleMouseClick(int mouseX, int mouseY) {
        if (gameWon || gameLost)
            return;

        for (int row = 0; row < 7; row++) {
            PyramidNode* current = pyramidRows[row];
            while (current) {
                if (current->card && current->card->inPlay) {
                    Rectangle cardRect = getPyramidCardRect(row, current->col);
                    if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, cardRect)) {
                        selectCard(current->card, current);
                        return;
                    }
                }
                current = current->nextInRow;
            }
        }

        if (currentWasteCard && currentWasteCard->inPlay) {
            int uiStartY = 150 + 7 * (CARD_HEIGHT / 2 + CARD_SPACING);
            Rectangle wasteRect = { 50, (float)uiStartY, CARD_WIDTH, CARD_HEIGHT };
            if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, wasteRect)) {
                selectCard(currentWasteCard, nullptr);
                return;
            }
        }

        if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, stockRect)) {
            drawCardFromStock();
            selectedCard1 = nullptr;
            selectedCard2 = nullptr;
            selectedNode1 = nullptr;
            selectedNode2 = nullptr;
            return;
        }
    }

    Rectangle getPyramidCardRect(int row, int col) {
        int startX = (GetScreenWidth() / 2) - (row * (CARD_WIDTH + CARD_SPACING) / 2);
        int x = startX + col * (CARD_WIDTH + CARD_SPACING);
        int y = 100 + row * (CARD_HEIGHT / 2 + CARD_SPACING);
        return { (float)x, (float)y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
    }

    void drawCard(Card* card, Rectangle rect, bool selected) {
        if (!card)
            return;

        if (!card->faceUp) {
            DrawRectangleRec(rect, DARKBLUE);
            DrawRectangleLinesEx(rect, 2, BLACK);
            DrawText("?", rect.x + rect.width / 2 - 10, rect.y + rect.height / 2 - 10, 30, WHITE);
            return;
        }

        Texture2D tex = cardTextures[card->suit][card->value - 1];

        if (tex.id != 0) {
            DrawTexturePro(
                tex,
                { 0, 0, (float)tex.width, (float)tex.height },
                rect,
                { 0, 0 },
                0,
                WHITE
            );
        }
        else {
            Color suitColor = (card->suit == 0 || card->suit == 1) ? RED : BLACK;
            DrawRectangleRec(rect, WHITE);
            DrawRectangleLinesEx(rect, 2, BLACK);

            const char* values[13] = { "A","2","3","4","5","6","7","8","9","10","J","Q","K" };
            DrawText(values[card->value - 1], rect.x + 10, rect.y + 10, 20, suitColor);

            const char* suitSymbols[4] = { "♥", "♦", "♣", "♠" };
            DrawText(suitSymbols[card->suit], rect.x + 10, rect.y + 35, 25, suitColor);
        }

        if (selected) {
            DrawRectangleLinesEx(rect, 3, YELLOW);
        }
    }

    void drawBackground() {
        if (background.id != 0) {
            DrawTexturePro(background,
                { 0, 0, (float)background.width, (float)background.height },
                { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },
                { 0, 0 }, 0, WHITE
            );
        }
        else {
            ClearBackground(DARKGREEN);
        }
    }

    void renderMainMenu() {
        BeginDrawing();
        drawBackground();

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        DrawText("PYRAMID SOLITAIRE", sw / 2 - 250, sh / 2 - 250, 50, GOLD);

        Rectangle playBtn = { (float)(sw / 2 - 150), (float)(sh / 2 - 130), 300, 60 };
        Rectangle instructBtn = { (float)(sw / 2 - 150), (float)(sh / 2 - 40), 300, 60 };
        Rectangle exitBtn = { (float)(sw / 2 - 150), (float)(sh / 2 + 50), 300, 60 };

        DrawRectangleRec(playBtn, DARKGREEN);
        DrawRectangleLinesEx(playBtn, 3, GREEN);
        DrawText("NEW GAME", sw / 2 - 130, sh / 2 - 110, 25, WHITE);

        DrawRectangleRec(instructBtn, DARKBLUE);
        DrawRectangleLinesEx(instructBtn, 3, BLUE);
        DrawText("INSTRUCTIONS", sw / 2 - 110, sh / 2 - 20, 25, WHITE);

        DrawRectangleRec(exitBtn, DARKGRAY);
        DrawRectangleLinesEx(exitBtn, 3, BLACK);
        DrawText("EXIT GAME", sw / 2 - 80, sh / 2 + 70, 25, WHITE);

        EndDrawing();
    }

    void renderInstructions() {
        BeginDrawing();
        drawBackground();

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        DrawRectangle(sw / 2 - 400, 100, 800, 700, { 0, 0, 0, 200 });
        DrawRectangleLinesEx({ (float)(sw / 2 - 400), 100, 800, 700 }, 3, GOLD);

        DrawText("HOW TO PLAY PYRAMID SOLITAIRE", sw / 2 - 280, 130, 30, GOLD);

        int y = 190;
        int spacing = 35;

        DrawText("OBJECTIVE:", sw / 2 - 350, y, 25, YELLOW);
        y += spacing;
        DrawText("Remove all cards from the pyramid by pairing cards", sw / 2 - 350, y, 20, WHITE);
        y += spacing - 5;
        DrawText("that add up to 13.", sw / 2 - 350, y, 20, WHITE);
        y += spacing + 10;

        DrawText("RULES:", sw / 2 - 350, y, 25, YELLOW);
        y += spacing;
        DrawText("1. Only uncovered cards can be selected.", sw / 2 - 350, y, 20, WHITE);
        y += spacing + 5;
        DrawText("2. Pair two cards that sum to 13:", sw / 2 - 350, y, 20, WHITE);
        y += spacing - 5;
        DrawText("   Ace(1) + Queen(12), 2 + Jack(11), 3 + 10,", sw / 2 - 350, y, 20, WHITE);
        y += spacing - 5;
        DrawText("   4 + 9, 5 + 8, 6 + 7", sw / 2 - 350, y, 20, WHITE);
        y += spacing + 5;
        DrawText("3. Kings (13) can be removed individually.", sw / 2 - 350, y, 20, WHITE);
        y += spacing + 5;
        DrawText("4. Click the STOCK pile to draw cards.", sw / 2 - 350, y, 20, WHITE);
        y += spacing + 10;

        DrawText("SCORING:", sw / 2 - 350, y, 25, YELLOW);
        y += spacing;
        DrawText("King removed: +10 points", sw / 2 - 350, y, 20, WHITE);
        y += spacing - 5;
        DrawText("Pair removed: +20 points", sw / 2 - 350, y, 20, WHITE);

        Rectangle backBtn = { (float)(sw / 2 - 100), (float)(sh - 120), 200, 50 };
        DrawRectangleRec(backBtn, DARKGRAY);
        DrawRectangleLinesEx(backBtn, 2, WHITE);
        DrawText("BACK TO MENU", sw / 2 - 85, sh - 105, 20, WHITE);

        EndDrawing();
    }

    void handleMainMenuClick(int mouseX, int mouseY) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        Rectangle playBtn = { (float)(sw / 2 - 150), (float)(sh / 2 - 130), 300, 60 };
        Rectangle instructBtn = { (float)(sw / 2 - 150), (float)(sh / 2 - 40), 300, 60 };
        Rectangle exitBtn = { (float)(sw / 2 - 150), (float)(sh / 2 + 50), 300, 60 };

        if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, playBtn)) {
            initGame();
        }
        else if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, instructBtn)) {
            state = INSTRUCTIONS;
        }
        else if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, exitBtn)) {
            CloseWindow();
            exit(0);
        }
    }

    void handleInstructionsClick(int mouseX, int mouseY) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        Rectangle backBtn = { (float)(sw / 2 - 100), (float)(sh - 120), 200, 50 };
        if (CheckCollisionPointRec({ (float)mouseX, (float)mouseY }, backBtn)) {
            state = MAIN_MENU;
        }
    }

    void render() {
        if (state == MAIN_MENU) {
            renderMainMenu();
            return;
        }

        if (state == INSTRUCTIONS) {
            renderInstructions();
            return;
        }

        BeginDrawing();
        drawBackground();

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        DrawText(TextFormat("Moves: %d", moves), sw - 150, 20, 25, YELLOW);

        for (int row = 0; row < 7; row++) {
            PyramidNode* current = pyramidRows[row];
            while (current) {
                if (current->card && current->card->inPlay) {
                    Rectangle rect = getPyramidCardRect(row, current->col);
                    bool selected = (current == selectedNode1 || current == selectedNode2);
                    drawCard(current->card, rect, selected);

                    if (current->blocked) {
                        DrawRectangle(rect.x, rect.y, rect.width, 5, RED);
                    }
                }
                current = current->nextInRow;
            }
        }

        int uiStartY = 150 + 7 * (CARD_HEIGHT / 2 + CARD_SPACING);

        DrawText("WASTE", 50, uiStartY - 30, 20, WHITE);
        if (currentWasteCard && currentWasteCard->inPlay) {
            Rectangle wasteRect = { 50, (float)uiStartY, CARD_WIDTH, CARD_HEIGHT };
            bool selected = (currentWasteCard == selectedCard1 || currentWasteCard == selectedCard2);
            drawCard(currentWasteCard, wasteRect, selected);
        }

        stockRect = { 180.0f, (float)uiStartY, (float)CARD_WIDTH, (float)CARD_HEIGHT };
        DrawText("STOCK", 180, uiStartY - 30, 20, WHITE);

        if (stockTexture.id != 0) {
            DrawTexturePro(
                stockTexture,
                { 0, 0, (float)stockTexture.width, (float)stockTexture.height },
                stockRect, { 0, 0 }, 0, WHITE
            );
        }
        else {
            DrawRectangleRec(stockRect, BLUE);
            DrawRectangleLinesEx(stockRect, 2, WHITE);
        }
        int totalSeconds = (int)gameTime;
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;

        DrawText(TextFormat("Score: %d", score), sw / 2 - 60, sh - 60, 25, WHITE);
        DrawText(TextFormat("Time: %02d:%02d:%02d", hours, minutes, seconds), sw / 2 - 80, sh - 30, 25, WHITE);

        Rectangle restartBtn = { (float)(sw - 150), (float)(sh - 60), 120, 50 };
        DrawRectangleRec(restartBtn, MAROON);
        DrawRectangleLinesEx(restartBtn, 2, WHITE);
        DrawText("RESTART", sw - 140, sh - 45, 20, WHITE);

        if (gameWon) {
            DrawRectangle(0, 0, sw, sh, { 0, 0, 0, 150 });
            DrawText("YOU WIN!", sw / 2 - 100, sh / 2 - 50, 40, GOLD);
            DrawText(TextFormat("Score: %d", score), sw / 2 - 80, sh / 2 + 10, 30, WHITE);
        }
        else if (gameLost) {
            DrawRectangle(0, 0, sw, sh, { 0, 0, 0, 150 });
            DrawText("NO MOVES LEFT!", sw / 2 - 150, sh / 2 - 50, 40, RED);
            DrawText(TextFormat("Score: %d", score), sw / 2 - 80, sh / 2 + 10, 30, WHITE);
        }

        EndDrawing();
    }

    void update(float deltaTime) {
        if (state == MAIN_MENU) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                handleMainMenuClick((int)mousePos.x, (int)mousePos.y);
            }
            return;
        }

        if (state == INSTRUCTIONS) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                handleInstructionsClick((int)mousePos.x, (int)mousePos.y);
            }
            return;
        }

        if (!gameWon && !gameLost) {
            gameTime += deltaTime;

            static float checkTimer = 0.0f;
            checkTimer += deltaTime;
            if (checkTimer >= 1.0f) {
                checkLoseCondition();
                checkTimer = 0.0f;
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();

            int sw = GetScreenWidth();
            int sh = GetScreenHeight();
            Rectangle restartBtn = { (float)(sw - 150), (float)(sh - 60), 120, 50 };
            if (CheckCollisionPointRec(mousePos, restartBtn)) {
                initGame();
                return;
            }

            handleMouseClick((int)mousePos.x, (int)mousePos.y);
        }
    }
    };
int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Pyramid Solitaire Game");
    SetTargetFPS(60);

    PyramidSolitaire game;

    while (!WindowShouldClose()) {
        game.update(GetFrameTime());
        game.render();
    }

    CloseWindow();
    return 0;
}
