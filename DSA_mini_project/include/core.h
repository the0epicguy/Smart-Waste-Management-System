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

// ----------------------------
// Graph: Areas as nodes, road
// distances between them as edges
// ----------------------------

#define MAX_AREAS 20
#define INF_DIST  1e9f

typedef struct {
    char name[50];
    float depotDistance; // straight-line km from depot (kept for bin's .distance field)
} AreaNode;

typedef struct {
    AreaNode nodes[MAX_AREAS];
    float    weight[MAX_AREAS][MAX_AREAS]; // road km between areas; 0 = no edge
    int      count;
} AreaGraph;

extern AreaGraph g_areaGraph; // single global graph (defined in core.c)

// Graph API
int   graph_addArea(const char* name, float depotDist);
int   graph_findArea(const char* name);               // returns index, -1 if not found
void  graph_addEdge(const char* a, const char* b, float roadKm);
float graph_getDepotDist(const char* area);
void  graph_clear(void);

// Dijkstra: fills dist[0..count-1] with shortest road km from depot (index 0).
// prev[i] = predecessor index in shortest path (-1 if none).
void  graph_dijkstra(int src, float* dist, int* prev);

// ----------------------------
// Queue nodes  (unchanged)
// ----------------------------

typedef struct queue {
    int binID;
    char area[50];
    float distance;
    int fillLevel;
    int priority;
    struct queue* next;
} queue;

typedef struct priorityqueue {
    int binID;
    char area[50];
    float distance;
    int fillLevel;
    int priority;
    struct priorityqueue* next;
} priorityqueue;

// ----------------------------
// Globals (defined in core.c)
// ----------------------------

extern Dustbin*       head;
extern queue*         front;
extern queue*         rear;
extern priorityqueue* priorityfront;
extern priorityqueue* priorityrear;

// ----------------------------
// Core API  (GUI-visible; signatures unchanged)
// ----------------------------

Dustbin* createBin(int id, char* area, float distance, int fillLevel);
int      addBin(int id, char* area, float distance, int fillLevel);
int      deleteBin(int id);
void     displayBins(void);
int      updateFillLevel(int id, int newFillLevel);
Dustbin* findBinByID(int id);
void     freeLinkedList(void);

void classify(Dustbin* node);
void enqueue(Dustbin* node);
void deletefromqueue(int id);
void priorityenqueue(Dustbin* dustnode);
void deletefrompriorityqueue(int id);
void display(void);
void prioritydisplay(void);
void queueBinsByDistance(void);   // now uses Dijkstra instead of BST
void clearQueue(void);
void clearPriorityQueue(void);

void initializeRandomBins(void);
void collectBinsFromArea(char* area);
void simulateTruckCollection(void);
void simulateFillLevelIncrease(void);
void displaySystemStatus(void);
void freeAreaDistances(void);     // kept for API compat – now calls graph_clear()

// ----------------------------
// Dispatch summary (unchanged)
// ----------------------------

typedef struct DispatchSummary {
    int   valid;
    int   targetID;
    char  area[50];
    float distance;
    int   startFill;
    int   binsCollected;
    float totalTimeMinutes;
    int   wasPriority;
} DispatchSummary;

const DispatchSummary* getLastDispatchSummary(void);

#endif /* CORE_H */

