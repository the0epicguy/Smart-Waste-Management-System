#ifndef CORE_H
#define CORE_H

#include <stdio.h>

typedef struct Dustbin {
    int binID;
    char area[50];
    float distance;
    int fillLevel;
    int priority;
    struct Dustbin* next;
} Dustbin;

#define MAX_AREAS 20
#define INF_DIST  1e9f

typedef struct {
    char name[50];
    float depotDistance;
} AreaNode;

typedef struct {
    AreaNode nodes[MAX_AREAS];
    float    weight[MAX_AREAS][MAX_AREAS];
    int      count;
} AreaGraph;

extern AreaGraph g_areaGraph;

int   graph_addArea(const char* name, float depotDist);
int   graph_findArea(const char* name);
void  graph_addEdge(const char* a, const char* b, float roadKm);
float graph_getDepotDist(const char* area);
void  graph_clear(void);
void  graph_dijkstra(int src, float* dist, int* prev);

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

extern Dustbin*       head;
extern queue*         front;
extern queue*         rear;
extern priorityqueue* priorityfront;
extern priorityqueue* priorityrear;

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
void queueBinsByDistance(void);
void clearQueue(void);
void clearPriorityQueue(void);

void initializeRandomBins(void);
void collectBinsFromArea(char* area);
void simulateTruckCollection(void);
void simulateFillLevelIncrease(void);
void displaySystemStatus(void);
void freeAreaDistances(void);

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

#endif
