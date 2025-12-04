#ifndef CORE_H
#define CORE_H

#include <stdio.h>

// ----------------------------
// Core data structures
// ----------------------------

typedef struct Dustbin {
    int binID;
    char area[50];
    float distance;
    int fillLevel;
    int priority;
    struct Dustbin* next;
} Dustbin;

// BST node for sorting bins by distance
typedef struct BSTNode {
    Dustbin* binptr;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

// Normal queue node
typedef struct queue {
    int binID;
    char area[50];
    float distance;
    int fillLevel;
    int priority;
    struct queue* next;
} queue;

// Priority queue node
typedef struct priorityqueue {
    int binID;
    char area[50];
    float distance;
    int fillLevel;
    int priority;
    struct priorityqueue* next;
} priorityqueue;

// ----------------------------
// Global lists / queues
// (defined in main.c)
// ----------------------------

extern Dustbin* head;
extern queue* front;
extern queue* rear;
extern priorityqueue* priorityfront;
extern priorityqueue* priorityrear;

// ----------------------------
// Core API used by GUI
// ----------------------------

// Bin / list operations
Dustbin* createBin(int id, char* area, float distance, int fillLevel);
int addBin(int id, char* area, float distance, int fillLevel);
int deleteBin(int id);
void displayBins();
int updateFillLevel(int id, int newFillLevel);
Dustbin* findBinByID(int id);
void freeLinkedList();

// Queue / priority queue and sorting
void classify(Dustbin* node);
void enqueue(Dustbin* node);
void deletefromqueue(int id);
void priorityenqueue(Dustbin* dustnode);
void deletefrompriorityqueue(int id);
void display();
void prioritydisplay();
void queueBinsByDistance();
void clearQueue();
void clearPriorityQueue();

// Simulation / system helpers
void initializeRandomBins();
void collectBinsFromArea(char* area);
void simulateTruckCollection();
void simulateFillLevelIncrease();
void displaySystemStatus();
void freeAreaDistances();

typedef struct DispatchSummary {
    int  valid;
    int  targetID;
    char area[50];
    float distance;
    int  startFill;
    int  binsCollected;
    float totalTimeMinutes;
    int  wasPriority;
} DispatchSummary;

const DispatchSummary* getLastDispatchSummary(void);

#endif


