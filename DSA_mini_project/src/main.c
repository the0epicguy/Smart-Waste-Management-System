#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "gui.h"

// ── Globals ───────────────────────────────────────────────────
Dustbin*       head          = NULL;
queue*         front         = NULL;
queue*         rear          = NULL;
priorityqueue* priorityfront = NULL;
priorityqueue* priorityrear  = NULL;

AreaGraph g_areaGraph = { .count = 0 };

static DispatchSummary lastDispatchSummary = { 0 };

// ── Graph ─────────────────────────────────────────────────────
int graph_addArea(const char* name, float depotDist) {
    int idx = graph_findArea(name);
    if (idx >= 0) {
        g_areaGraph.nodes[idx].depotDistance = depotDist;
        return idx;
    }
    if (g_areaGraph.count >= MAX_AREAS) {
        printf("Warning: MAX_AREAS reached, cannot add '%s'\n", name);
        return -1;
    }
    idx = g_areaGraph.count++;
    strncpy(g_areaGraph.nodes[idx].name, name, 49);
    g_areaGraph.nodes[idx].name[49] = '\0';
    g_areaGraph.nodes[idx].depotDistance = depotDist;
    return idx;
}

int graph_findArea(const char* name) {
    for (int i = 0; i < g_areaGraph.count; i++)
        if (strcmp(g_areaGraph.nodes[i].name, name) == 0) return i;
    return -1;
}

void graph_addEdge(const char* a, const char* b, float roadKm) {
    int ia = graph_findArea(a);
    int ib = graph_findArea(b);
    if (ia < 0 || ib < 0) return;
    g_areaGraph.weight[ia][ib] = roadKm;
    g_areaGraph.weight[ib][ia] = roadKm;
}

float graph_getDepotDist(const char* area) {
    int idx = graph_findArea(area);
    return (idx < 0) ? -1.0f : g_areaGraph.nodes[idx].depotDistance;
}

void graph_clear(void) {
    g_areaGraph.count = 0;
    memset(g_areaGraph.weight, 0, sizeof(g_areaGraph.weight));
}

void graph_dijkstra(int src, float* dist, int* prev) {
    int n = g_areaGraph.count;
    int visited[MAX_AREAS] = { 0 };
    for (int i = 0; i < n; i++) { dist[i] = INF_DIST; prev[i] = -1; }
    dist[src] = 0.0f;

    for (int iter = 0; iter < n; iter++) {
        int u = -1;
        for (int i = 0; i < n; i++)
            if (!visited[i] && (u == -1 || dist[i] < dist[u])) u = i;
        if (u == -1 || dist[u] >= INF_DIST) break;
        visited[u] = 1;
        for (int v = 0; v < n; v++) {
            float w = g_areaGraph.weight[u][v];
            if (w > 0.0f && !visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
            }
        }
    }
}

// ── Validation ────────────────────────────────────────────────
static int validateBinID(int id) {
    Dustbin* cur = head;
    while (cur) { if (cur->binID == id) return 0; cur = cur->next; }
    return 1;
}
static int validateFillLevel(int f) { return (f >= 0 && f <= 100); }

// ── Bin list ──────────────────────────────────────────────────
Dustbin* createBin(int id, char* area, float distance, int fillLevel) {
    Dustbin* b = (Dustbin*)malloc(sizeof(Dustbin));
    if (!b) { printf("Memory allocation failed!\n"); return NULL; }
    b->binID = id;
    strncpy(b->area, area, 49); b->area[49] = '\0';
    b->distance  = distance;
    b->fillLevel = fillLevel;
    b->priority  = (fillLevel * 2) - (int)(distance * 5);
    b->next      = NULL;
    return b;
}

int addBin(int id, char* area, float distance, int fillLevel) {
    if (!validateBinID(id))     { printf("Error: Bin ID %d already exists!\n", id); return 0; }
    if (!validateFillLevel(fillLevel)) { printf("Error: Fill level must be 0-100!\n"); return 0; }
    if (distance < 0)           { printf("Error: Distance cannot be negative!\n"); return 0; }
    Dustbin* b = createBin(id, area, distance, fillLevel);
    if (!b) return 0;
    if (!head) { head = b; }
    else { Dustbin* t = head; while (t->next) t = t->next; t->next = b; }
    classify(b);
    return 1;
}

int deleteBin(int id) {
    if (!head) { printf("No bins to delete!\n"); return 0; }
    deletefromqueue(id);
    deletefrompriorityqueue(id);
    if (head->binID == id) {
        Dustbin* t = head; head = head->next; free(t);
        printf("Bin %d deleted successfully!\n", id); return 1;
    }
    Dustbin* cur = head;
    while (cur->next && cur->next->binID != id) cur = cur->next;
    if (!cur->next) { printf("Bin %d not found!\n", id); return 0; }
    Dustbin* t = cur->next; cur->next = t->next; free(t);
    printf("Bin %d deleted successfully!\n", id); return 1;
}

void displayBins(void) {
    if (!head) { printf("No bins available!\n"); return; }
    printf("\n=== BIN INVENTORY ===\n");
    printf("ID\tArea\t\tDistance\tFill Level\tStatus\n");
    printf("-----------------------------------------------------------------------\n");
    Dustbin* cur = head;
    while (cur) {
        char status[20];
        if      (cur->fillLevel >= 90) strcpy(status, "URGENT");
        else if (cur->fillLevel >= 70) strcpy(status, "HIGH");
        else if (cur->fillLevel >= 50) strcpy(status, "MEDIUM");
        else                           strcpy(status, "LOW");
        printf("%d\t%s\t\t%.2f\t\t%d%%\t\t%s\n",
               cur->binID, cur->area, cur->distance, cur->fillLevel, status);
        cur = cur->next;
    }
    printf("-----------------------------------------------------------------------\n");
}

int updateFillLevel(int id, int newFillLevel) {
    if (!validateFillLevel(newFillLevel)) { printf("Error: Fill level must be 0-100!\n"); return 0; }
    Dustbin* b = findBinByID(id);
    if (!b) { printf("Bin %d not found!\n", id); return 0; }
    int wasUrgent = (b->fillLevel >= 90);
    int isUrgent  = (newFillLevel >= 90);
    deletefromqueue(id);
    deletefrompriorityqueue(id);
    b->fillLevel = newFillLevel;
    b->priority  = (newFillLevel * 2) - (int)(b->distance * 5);
    classify(b);
    if (!wasUrgent && isUrgent)  printf("WARNING: Bin %d is now URGENT!\n", id);
    else if (wasUrgent && !isUrgent) printf("SUCCESS: Bin %d no longer urgent.\n", id);
    return 1;
}

Dustbin* findBinByID(int id) {
    Dustbin* cur = head;
    while (cur) { if (cur->binID == id) return cur; cur = cur->next; }
    return NULL;
}

void freeLinkedList(void) {
    Dustbin* cur = head;
    while (cur) { Dustbin* t = cur; cur = cur->next; free(t); }
    head = NULL;
}

// ── Queues ────────────────────────────────────────────────────
void classify(Dustbin* node) {
    if (node->fillLevel >= 90) priorityenqueue(node);
    else                       enqueue(node);
}

void enqueue(Dustbin* node) {
    queue* n = (queue*)malloc(sizeof(queue));
    n->binID = node->binID;
    strncpy(n->area, node->area, 49); n->area[49] = '\0';
    n->distance = node->distance; n->fillLevel = node->fillLevel;
    n->priority = node->priority; n->next = NULL;
    if (!front) { front = rear = n; return; }
    rear->next = n; rear = n;
}

void deletefromqueue(int id) {
    if (!front) return;
    queue* prev = NULL; queue* cur = front;
    while (cur && cur->binID != id) { prev = cur; cur = cur->next; }
    if (!cur) return;
    if (prev) prev->next = cur->next; else front = cur->next;
    if (cur == rear) rear = prev;
    free(cur);
}

void priorityenqueue(Dustbin* dustnode) {
    priorityqueue* n = (priorityqueue*)malloc(sizeof(priorityqueue));
    n->binID = dustnode->binID;
    strncpy(n->area, dustnode->area, 49); n->area[49] = '\0';
    n->distance = dustnode->distance; n->fillLevel = dustnode->fillLevel;
    n->priority = dustnode->priority; n->next = NULL;
    if (!priorityfront) { priorityfront = priorityrear = n; return; }
    if (n->priority >= priorityfront->priority) {
        n->next = priorityfront; priorityfront = n; return;
    }
    if (n->priority < priorityrear->priority) {
        priorityrear->next = n; priorityrear = n; return;
    }
    priorityqueue* cur = priorityfront; priorityqueue* prev = NULL;
    while (cur && n->priority < cur->priority) { prev = cur; cur = cur->next; }
    n->next = cur; prev->next = n;
}

void deletefrompriorityqueue(int id) {
    if (!priorityfront) return;
    priorityqueue* prev = NULL; priorityqueue* cur = priorityfront;
    while (cur && cur->binID != id) { prev = cur; cur = cur->next; }
    if (!cur) return;
    if (prev) prev->next = cur->next; else priorityfront = cur->next;
    if (cur == priorityrear) priorityrear = prev;
    free(cur);
}

void display(void) {
    if (!front) { printf("Normal queue is empty.\n"); return; }
    printf("ID\tArea\t\tDistance\tFill Level\n");
    printf("--------------------------------------------------------\n");
    queue* t = front;
    while (t) { printf("%d\t%s\t\t%.2f\t\t%d%%\n", t->binID, t->area, t->distance, t->fillLevel); t = t->next; }
}

void prioritydisplay(void) {
    if (!priorityfront) { printf("Priority queue is empty.\n"); return; }
    printf("ID\tArea\t\tDistance\tFill Level\n");
    printf("--------------------------------------------------------\n");
    priorityqueue* t = priorityfront;
    while (t) { printf("%d\t%s\t\t%.2f\t\t%d%%\n", t->binID, t->area, t->distance, t->fillLevel); t = t->next; }
    printf("--------------------------------------------------------\n");
}

void clearQueue(void) {
    while (front) { queue* t = front; front = front->next; free(t); } rear = NULL;
}

void clearPriorityQueue(void) {
    while (priorityfront) { priorityqueue* t = priorityfront; priorityfront = priorityfront->next; free(t); }
    priorityrear = NULL;
}

// ── Initialization (builds Pune road graph) ───────────────────
void initializeRandomBins(void) {
    printf("\nInitializing waste management system with 10 bins...\n");
    graph_clear();

    graph_addArea("DEPOT", 0.0f);

    const char* areaNames[] = {
        "Shivajinagar", "Kothrud", "Koregaon Park", "Viman Nagar", "Hinjewadi",
        "Baner", "Kharadi", "Hadapsar", "Swargate", "Camp"
    };
    float depotDists[] = { 3.0f, 6.0f, 5.0f, 11.0f, 14.0f,
                           10.0f, 15.0f, 12.0f, 4.0f, 6.0f };
    int totalAreas = 10;

    for (int i = 0; i < totalAreas; i++)
        graph_addArea(areaNames[i], depotDists[i]);

    // Road edges (approximate Pune road km)
    graph_addEdge("Shivajinagar",  "Kothrud",        5.0f);
    graph_addEdge("Shivajinagar",  "Koregaon Park",  4.0f);
    graph_addEdge("Shivajinagar",  "Swargate",        3.0f);
    graph_addEdge("Koregaon Park", "Viman Nagar",     6.0f);
    graph_addEdge("Koregaon Park", "Kharadi",         9.0f);
    graph_addEdge("Viman Nagar",   "Kharadi",         5.0f);
    graph_addEdge("Kharadi",       "Hadapsar",        7.0f);
    graph_addEdge("Hadapsar",      "Swargate",        8.0f);
    graph_addEdge("Swargate",      "Camp",            4.0f);
    graph_addEdge("Kothrud",       "Hinjewadi",       8.0f);
    graph_addEdge("Kothrud",       "Baner",           6.0f);
    graph_addEdge("Baner",         "Hinjewadi",       5.0f);

    // Depot direct roads
    graph_addEdge("DEPOT", "Shivajinagar",   3.0f);
    graph_addEdge("DEPOT", "Kothrud",        6.0f);
    graph_addEdge("DEPOT", "Baner",         10.0f);
    graph_addEdge("DEPOT", "Hinjewadi",     14.0f);
    graph_addEdge("DEPOT", "Koregaon Park",  5.0f);
    graph_addEdge("DEPOT", "Viman Nagar",   11.0f);
    graph_addEdge("DEPOT", "Kharadi",       15.0f);
    graph_addEdge("DEPOT", "Hadapsar",      12.0f);
    graph_addEdge("DEPOT", "Swargate",       4.0f);
    graph_addEdge("DEPOT", "Camp",           6.0f);

    for (int i = 1; i <= 10; i++) {
        int aIdx         = rand() % totalAreas;
        const char* area = areaNames[aIdx];
        float baseDist   = depotDists[aIdx];
        float variation  = ((float)rand() / RAND_MAX - 0.5f) * 1.0f;
        float binDist    = baseDist + variation;
        if (binDist < 0.5f) binDist = 0.5f;
        addBin(i, (char*)area, binDist, rand() % 101);
    }
    printf("10 bins initialized with Pune road network graph.\n");
}

void freeAreaDistances(void) { graph_clear(); }

// ── Graph-based queue sorting (replaces BST) ──────────────────
void queueBinsByDistance(void) {
    if (!head) { printf("No bins available to sort!\n"); return; }

    clearQueue();
    clearPriorityQueue();

    float dijkDist[MAX_AREAS];
    int   dijkPrev[MAX_AREAS];
    graph_dijkstra(0, dijkDist, dijkPrev);

    // Collect areas that have bins
    typedef struct { int areaIdx; float roadDist; } AreaEntry;
    AreaEntry entries[MAX_AREAS];
    int entryCount = 0;

    for (int i = 1; i < g_areaGraph.count; i++) {
        Dustbin* d = head;
        int hasBin = 0;
        while (d) {
            if (strcmp(d->area, g_areaGraph.nodes[i].name) == 0) { hasBin = 1; break; }
            d = d->next;
        }
        if (!hasBin) continue;
        float rd = (dijkDist[i] < INF_DIST) ? dijkDist[i]
                                             : g_areaGraph.nodes[i].depotDistance;
        entries[entryCount++] = (AreaEntry){ i, rd };
    }

    // Insertion sort by road distance
    for (int i = 1; i < entryCount; i++) {
        AreaEntry key = entries[i]; int j = i - 1;
        while (j >= 0 && entries[j].roadDist > key.roadDist) {
            entries[j+1] = entries[j]; j--;
        }
        entries[j+1] = key;
    }

    // Enqueue bins area by area in road-optimal order
    printf("\n=== Areas sorted by shortest road distance from depot ===\n");
    for (int e = 0; e < entryCount; e++) {
        const char* aname = g_areaGraph.nodes[entries[e].areaIdx].name;
        printf("  [%.2f km road] %s\n", entries[e].roadDist, aname);
        Dustbin* d = head;
        while (d) {
            if (strcmp(d->area, aname) == 0) {
                if (d->fillLevel >= 90) priorityenqueue(d);
                else                   enqueue(d);
            }
            d = d->next;
        }
    }
    printf("Bins enqueued in road-optimal order.\n");
}

// ── Truck simulation ──────────────────────────────────────────
static float travelTime(float d, float speed) { return (d / speed) * 60.0f; }

static void markBinCollectedAndRequeue(int binID) {
    Dustbin* b = findBinByID(binID);
    if (!b) return;
    deletefromqueue(binID);
    deletefrompriorityqueue(binID);
    b->fillLevel = 0;
    b->priority  = -(int)(b->distance * 5);
    enqueue(b);
}

static int popPriorityTarget(char* area_buf, float* dist, int* fill) {
    if (!priorityfront) return -1;
    priorityqueue* node = priorityfront;
    int id = node->binID;
    Dustbin* bin = findBinByID(id);
    if (bin) {
        if (area_buf) { strncpy(area_buf, bin->area, 49); area_buf[49] = '\0'; }
        if (dist) *dist = bin->distance;
        if (fill) *fill = bin->fillLevel;
    } else {
        if (area_buf) { strncpy(area_buf, node->area, 49); area_buf[49] = '\0'; }
        if (dist) *dist = node->distance;
        if (fill) *fill = node->fillLevel;
    }
    priorityfront = node->next;
    if (!priorityfront) priorityrear = NULL;
    free(node);
    return id;
}

static int popNormalTarget(char* area_buf, float* dist, int* fill) {
    if (!front) return -1;
    queue* node = front;
    int id = node->binID;
    Dustbin* bin = findBinByID(id);
    if (bin) {
        if (area_buf) { strncpy(area_buf, bin->area, 49); area_buf[49] = '\0'; }
        if (dist) *dist = bin->distance;
        if (fill) *fill = bin->fillLevel;
    } else {
        if (area_buf) { strncpy(area_buf, node->area, 49); area_buf[49] = '\0'; }
        if (dist) *dist = node->distance;
        if (fill) *fill = node->fillLevel;
    }
    front = node->next;
    if (!front) rear = NULL;
    free(node);
    return id;
}

void simulateTruckCollection(void) {
    lastDispatchSummary.valid = 0;
    printf("\n         TRUCK DISPATCH SIMULATION (GRAPH-ROUTED)\n");
    printf("---------------------------------------------------------------\n");

    queueBinsByDistance();

    char  targetArea[50];
    float targetDist;
    int   targetFill;

    int targetID = popPriorityTarget(targetArea, &targetDist, &targetFill);
    if (targetID == -1)
        targetID = popNormalTarget(targetArea, &targetDist, &targetFill);

    if (targetID == -1) {
        printf("\nAll bins are empty — no trucks to dispatch.\n");
        printf("---------------------------------------------------------------\n");
        return;
    }

    Dustbin* targetBin = findBinByID(targetID);
    if (!targetBin || targetBin->fillLevel == 0) {
        printf("\nTarget bin is already empty.\n");
        printf("---------------------------------------------------------------\n");
        return;
    }

    // Use Dijkstra road distance for travel time
    float dijkDist[MAX_AREAS]; int dijkPrev[MAX_AREAS];
    graph_dijkstra(0, dijkDist, dijkPrev);
    int areaIdx = graph_findArea(targetArea);
    float roadDist = (areaIdx >= 0 && dijkDist[areaIdx] < INF_DIST)
                     ? dijkDist[areaIdx] : targetDist;

    float go = travelTime(roadDist, 30.0f);
    const char* priorityStatus = (targetFill >= 90) ? "URGENT" : "NORMAL";

    printf("\n   TRUCK DISPATCHED\n");
    printf("---------------------------------------------------------------\n");
    printf("Target: Bin #%d in '%s'\n", targetID, targetArea);
    printf("Road distance (Dijkstra): %.2f km | Fill: %d%% | Priority: %s\n",
           roadDist, targetFill, priorityStatus);
    printf("Travel time (one way): %.1f min\n\n", go);
    printf("Collecting bins in area '%s':\n", targetArea);

    int binsCollected = 0;
    Dustbin* d = head;
    while (d) {
        if (strcmp(d->area, targetArea) == 0 && d->fillLevel > 0) {
            printf("    Bin #%d (Fill %d%%) - COLLECTED\n", d->binID, d->fillLevel);
            markBinCollectedAndRequeue(d->binID);
            binsCollected++;
        }
        d = d->next;
    }

    float totalLoad = binsCollected * 3.0f;
    float totalTime = go + go + totalLoad;

    printf("\n  Route Summary:\n");
    printf("   Road distance:    %.2f km\n", roadDist);
    printf("   Travel outward:   %.1f min\n", go);
    printf("   Loading time:     %.1f min\n", totalLoad);
    printf("   Return travel:    %.1f min\n", go);
    printf("   -----------------------------------\n");
    printf("   TOTAL ROUTE TIME: %.1f minutes\n", totalTime);
    printf("   Bins collected:   %d\n", binsCollected);

    printf("\n=== QUEUE STATUS AFTER COLLECTION ===\n");
    printf("\nPriority Queue:\n"); prioritydisplay();
    printf("\nNormal Queue:\n"); display();
    printf("---------------------------------------------------------------\n");

    int remaining = 0;
    Dustbin* tmp = head;
    while (tmp) { if (tmp->fillLevel > 0) remaining++; tmp = tmp->next; }
    if (remaining > 0)
        printf("\n Remaining non-empty bins: %d\n", remaining);

    lastDispatchSummary.valid            = 1;
    lastDispatchSummary.targetID         = targetID;
    strncpy(lastDispatchSummary.area, targetArea, 49);
    lastDispatchSummary.area[49]         = '\0';
    lastDispatchSummary.distance         = roadDist;
    lastDispatchSummary.startFill        = targetFill;
    lastDispatchSummary.binsCollected    = binsCollected;
    lastDispatchSummary.totalTimeMinutes = totalTime;
    lastDispatchSummary.wasPriority      = (targetFill >= 90);
}

void simulateFillLevelIncrease(void) {
    printf("\nSimulating passage of time — bins filling up...\n");
    Dustbin* cur = head; int updated = 0;
    while (cur) {
        int newLevel = cur->fillLevel + (rand() % 20) + 5;
        if (newLevel > 100) newLevel = 100;
        if (newLevel > cur->fillLevel) { updateFillLevel(cur->binID, newLevel); updated++; }
        cur = cur->next;
    }
    printf("%d bins updated.\n", updated);
    queueBinsByDistance();
}

void displaySystemStatus(void) {
    printf("\n                    SYSTEM STATUS OVERVIEW\n");
    int total=0, urgent=0, high=0, medium=0, low=0;
    Dustbin* cur = head;
    while (cur) {
        total++;
        if      (cur->fillLevel >= 90) urgent++;
        else if (cur->fillLevel >= 70) high++;
        else if (cur->fillLevel >= 50) medium++;
        else                           low++;
        cur = cur->next;
    }
    printf("  Total: %d | Urgent: %d | High: %d | Medium: %d | Low: %d\n",
           total, urgent, high, medium, low);
    printf("  Graph nodes: %d areas + DEPOT\n", g_areaGraph.count - 1);
    if (urgent > 0) printf("  WARNING: %d bin(s) need immediate attention!\n", urgent);
    else            printf("  System operating normally.\n");
}

void collectBinsFromArea(char* area) {
    if (!area || !strlen(area)) return;
    int ids[256]; int count = 0;
    Dustbin* d = head;
    while (d) {
        if (strcmp(d->area, area) == 0 && count < 256) ids[count++] = d->binID;
        d = d->next;
    }
    int collected = 0;
    for (int i = 0; i < count; i++) {
        Dustbin* b = findBinByID(ids[i]);
        if (!b || b->fillLevel == 0) continue;
        printf("    Bin #%d (Fill: %d%%) - COLLECTED\n", ids[i], b->fillLevel);
        updateFillLevel(ids[i], 0);
        deletefromqueue(ids[i]);
        deletefrompriorityqueue(ids[i]);
        collected++;
    }
    printf("    Collected %d bins from %s.\n", collected, area);
}

const DispatchSummary* getLastDispatchSummary(void) {
    return lastDispatchSummary.valid ? &lastDispatchSummary : NULL;
}

// ── Entry point ───────────────────────────────────────────────
int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));
    start_gui(&argc, &argv);
    return 0;
}
