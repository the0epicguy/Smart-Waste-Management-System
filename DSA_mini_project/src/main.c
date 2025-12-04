#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "gui.h"

Dustbin* head = NULL; // Global head pointer

// Function prototypes
Dustbin* createBin(int id, char* area, float distance, int fillLevel);
int addBin(int id, char* area, float distance, int fillLevel);
int deleteBin(int id);
void displayBins();
int updateFillLevel(int id, int newFillLevel);
int validateBinID(int id);
int validateFillLevel(int fillLevel);
void initializeRandomBins();
int getRandomFillLevel();
Dustbin* findBinByID(int id);
void freeLinkedList();
void classify(Dustbin* node);
void enqueue(Dustbin* node);
void deletefromqueue(int id);
void priorityenqueue(Dustbin* dustnode);
void deletefrompriorityqueue(int id);
void display();
void prioritydisplay();
void queueBinsByDistance();
void displayBSTInorder(BSTNode* root);
void inorderBSTtoQueue(BSTNode* root);
void inorderBSTtoPriorityQueue(BSTNode* root);
void displaySystemStatus();
void collectBinsFromArea(char* area);
void simulateTruckCollection();
void simulateFillLevelIncrease();
int popPriorityTarget(char *area_buf, float *dist, int *fill);
int popNormalTarget(char *area_buf, float *dist, int *fill);
void markBinCollectedAndRequeue(int binID);
float getAreaDistance(char* area);
void setAreaDistance(char* area, float distance);
void freeAreaDistances();

// Create a new bin node
Dustbin* createBin(int id, char* area, float distance, int fillLevel) {
    Dustbin* newBin = (Dustbin*)malloc(sizeof(Dustbin));
    if (!newBin) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    newBin->binID = id;
    strcpy(newBin->area, area);
    newBin->distance = distance;
    newBin->fillLevel = fillLevel;
    newBin->priority = (fillLevel * 2) - (distance * 5);
    newBin->next = NULL;
    return newBin;
    }

int validateBinID(int id) {
    Dustbin* current = head;
    while (current) {
        if (current->binID == id) return 0;
        current = current->next;
    }
    return 1;
}

int validateFillLevel(int fillLevel) {
    return (fillLevel >= 0 && fillLevel <= 100);
}
int addBin(int id, char* area, float distance, int fillLevel) {
    if (!validateBinID(id)) {
        printf("Error: Bin ID %d already exists!\n", id);
        return 0;
    }
    if (!validateFillLevel(fillLevel)) {
        printf("Error: Fill level must be between 0 and 100!\n");
        return 0;
    }
    if (distance < 0) {
        printf("Error: Distance cannot be negative!\n");
        return 0;
    }
    Dustbin* newBin = createBin(id, area, distance, fillLevel);
    if (!newBin) return 0;
    if (!head) {
        head = newBin;
    } else {
        Dustbin* temp = head;
        while (temp->next) temp = temp->next;
        temp->next = newBin;
    }
    classify(newBin);
     return 1;
}

int deleteBin(int id) {
    if (!head) {
        printf("No bins to delete!\n");
        return 0;
    }
    // Remove from queues before deleting
    deletefromqueue(id);
    deletefrompriorityqueue(id);
    
    if (head->binID == id) {
        Dustbin* temp = head;
        head = head->next;
        free(temp);
        printf("Bin %d deleted successfully!\n", id);
        return 1;
    }
    Dustbin* current = head;
    while (current->next && current->next->binID != id)
        current = current->next;
    if (!current->next) {
        printf("Bin %d not found!\n", id);
        return 0;
    }
     Dustbin* temp = current->next;
    current->next = temp->next;
    free(temp);
    printf("Bin %d deleted successfully!\n", id);
    return 1;
}

void displayBins() {
    if (!head) {
        printf("No bins available!\n");
        return;
    }
    printf("\n=== BIN INVENTORY ===\n");
    printf("ID\tArea\t\tDistance\tFill Level\tStatus\n");
    printf("-----------------------------------------------------------------------\n");
    Dustbin* current = head;
    while (current) {
        char status[20];
        if (current->fillLevel >= 90) strcpy(status, "URGENT");
        else if (current->fillLevel >= 70) strcpy(status, "HIGH");
        else if (current->fillLevel >= 50) strcpy(status, "MEDIUM");
        else strcpy(status, "LOW");
         printf("%d\t%s\t\t%.2f\t\t%d%%\t\t%s\n", 
               current->binID, current->area, current->distance, 
               current->fillLevel, status);
        current = current->next;
    }
    printf("-----------------------------------------------------------------------\n");
}

int updateFillLevel(int id, int newFillLevel) {
    if (!validateFillLevel(newFillLevel)) {
        printf("Error: Fill level must be between 0 and 100!\n");
        return 0;
    }
    Dustbin* bin = findBinByID(id);
    if (!bin) {
        printf("Bin %d not found!\n", id);
        return 0;
    }
    int oldLevel = bin->fillLevel;
    int wasUrgent = (oldLevel >= 90);
    int isUrgent = (newFillLevel >= 90);
    
    // Remove bin from current queue before updating
    deletefromqueue(id);
    deletefrompriorityqueue(id);
    
    bin->fillLevel = newFillLevel;
    bin->priority = (newFillLevel * 2) - (bin->distance * 5);
    
    // Reclassify and add to appropriate queue
    classify(bin);
    
    if (!wasUrgent && isUrgent) {
        printf("WARNING: Bin %d is now URGENT and needs immediate collection!\n", id);
    } else if (wasUrgent && !isUrgent) {
        printf("SUCCESS: Bin %d is no longer urgent.\n", id);
    }
    return 1;
}

Dustbin* findBinByID(int id) {
    Dustbin* current = head;
    while (current) {
        if (current->binID == id) return current;
        current = current->next;
    }
    return NULL;
}

int getRandomFillLevel() {
    return rand() % 101;
}
void initializeRandomBins() {
    printf("\nInitializing waste management system with 10 bins...\n");
    char *areas[] = {
        "Shivajinagar", "Kothrud", "Koregaon Park", "Viman Nagar", "Hinjewadi",
        "Baner", "Kharadi", "Hadapsar", "Swargate", "Camp"
    };
    int totalAreas = 10;
    
    // Clear previous area distances
    freeAreaDistances();
    
    // Assign random distances to each area (between 2 and 20 km)
    for (int i = 0; i < totalAreas; i++) {
        float areaDistance = 2.0f + ((float)rand() / RAND_MAX) * 18.0f;
        setAreaDistance(areas[i], areaDistance);
    }
    
    // Create 10 bins with consistent area distances
    for (int i = 1; i <= 10; i++) {
        int randomAreaIndex = rand() % totalAreas;
        char* selectedArea = areas[randomAreaIndex];
        
        // Get the base distance for this area
        float baseDistance = getAreaDistance(selectedArea);
        
        // Add small variation (±0.5 km) to make it realistic
        float variation = ((float)rand() / RAND_MAX - 0.5f) * 1.0f;
        float binDistance = baseDistance + variation;
        if (binDistance < 0.5f) binDistance = 0.5f;
        
        int randomFill = getRandomFillLevel();
        addBin(i, selectedArea, binDistance, randomFill);
    }
    printf("10 bins initialized successfully with consistent area distances!\n");
}

void freeLinkedList() {
    Dustbin* current = head;
    while (current) {
        Dustbin* temp = current;
        current = current->next;
        free(temp);
    }
    head = NULL;
}

// BST Functions to insert and traverse bins sorted by distance
BSTNode* createBSTNode(Dustbin* bin) {
    BSTNode* node = (BSTNode*)malloc(sizeof(BSTNode));
    if (!node) {
        printf("Memory allocation error (BST node)!\n");
        return NULL;
    }
    node->binptr = bin;
    node->left = node->right = NULL;
    return node;
}

BSTNode* insertBST(BSTNode* root, Dustbin* bin) {
    if (!root) return createBSTNode(bin);
    if (bin->distance < root->binptr->distance) root->left = insertBST(root->left, bin);
    else root->right = insertBST(root->right, bin);
    return root;
}

void displayBSTInorderHelper(BSTNode* root, int printHeader) {
    if (root == NULL) return;
    if (printHeader) {
        printf("--------------------------------------------------------\n");
        printf("ID\tArea\t\tDistance\tFill Level\n");
        printf("--------------------------------------------------------\n");
    }
    displayBSTInorderHelper(root->left, 0);
    Dustbin* b = root->binptr;
    printf("%-8d %-15s %-10.2f %d%%\n", b->binID, b->area, b->distance, b->fillLevel);
    displayBSTInorderHelper(root->right, 0);
    if (printHeader) printf("--------------------------------------------------------\n");
}

void displayBSTInorder(BSTNode* root) {
    displayBSTInorderHelper(root, 1);
}

void inorderBSTtoQueue(BSTNode* root);
void inorderBSTtoPriorityQueue(BSTNode* root);

void freeBST(BSTNode* root) {
    if (!root) return;
    freeBST(root->left);
    freeBST(root->right);
    free(root);
}

// Queue and Priority Queue globals (types in core.h)
queue* front = NULL;
queue* rear = NULL;
priorityqueue* priorityfront = NULL;
priorityqueue* priorityrear = NULL;

static DispatchSummary lastDispatchSummary = {0};

void classify(Dustbin* node) {
    if (node->fillLevel >= 90) {
        priorityenqueue(node);
    } else {
        enqueue(node);
    }
}

void enqueue(Dustbin* node) {
    queue*new=(queue*)malloc(sizeof(queue));
    new->binID=node->binID;
    strcpy(new->area, node->area);
    new->distance=node->distance;
    new->fillLevel=node->fillLevel;
    new->priority=node->priority;
    new->next=NULL;
    if(front==NULL && rear==NULL){
        front=rear=new;
        return;
    }
    rear->next=new;
    rear=new;
    return;
}

void deletefromqueue(int id) {
    queue*temp=front;
    queue*prev=NULL;
    if(front==NULL && rear==NULL){
        return;
    }
    if(front->binID == id){
        front=temp->next;
        if(front==NULL) rear=NULL;  
        free(temp);
        return;
    }
    while(temp!=NULL && temp->binID != id){
        prev=temp;
        temp=temp->next;
    }
    if(temp==NULL){
        return;
    }
    prev->next=temp->next;
    if(temp==rear) rear=prev;  
    temp->next=NULL;
    free(temp);
}

void priorityenqueue(Dustbin* dustnode) {
    priorityqueue*node=(priorityqueue*)malloc(sizeof(priorityqueue));
    node->binID=dustnode->binID;
    strcpy(node->area, dustnode->area);
    node->distance=dustnode->distance;
    node->fillLevel=dustnode->fillLevel;
    node->priority=dustnode->priority;
    node->next = NULL;
    if(priorityfront==NULL && priorityrear==NULL){
        priorityfront=priorityrear=node;
        return;
    }else if(node->priority >= priorityfront->priority){
        node->next = priorityfront; 
        priorityfront = node;
        return;
    }else if(node->priority < priorityrear->priority){
        node->next = NULL; 
        priorityrear->next= node;
        priorityrear=node;
        return;
    }
    priorityqueue*current=priorityfront;
    priorityqueue*prev=NULL;
    while(current != NULL && node->priority >= current->priority){
        prev=current;
        current= current->next;
    }
    node->next = current;
    prev->next = node;
}

void deletefrompriorityqueue(int id) {
    priorityqueue*temp=priorityfront;
    priorityqueue*prev=NULL;
    if(priorityfront==NULL && priorityrear==NULL){
        return;
    }
    if(priorityfront->binID == id){
        priorityfront=temp->next;
        if(priorityfront==NULL) priorityrear=NULL;  
        free(temp);
        return;
    }
    while(temp!=NULL && temp->binID != id){
        prev=temp;
        temp=temp->next;
    }
    if(temp==NULL){
        return;
    }
    prev->next=temp->next;
    if(temp==priorityrear) priorityrear=prev;  
    temp->next=NULL;
    free(temp);
}

void display() {
    if (!front) {
        printf("Normal queue is empty.\n");
        return;
    }
    queue* temp = front;
    printf("ID\tArea\t\tDistance\tFill Level\n");
    printf("--------------------------------------------------------\n");
    while (temp) {
        printf("%d\t%s\t\t%.2f\t\t%d%%\n", temp->binID, temp->area, temp->distance, temp->fillLevel);
        temp = temp->next;
    }
    printf("\n");
}

void prioritydisplay() {
    if (!priorityfront) {
        printf("Priority queue is empty.\n");
        return;
    }
    priorityqueue* temp = priorityfront;
    printf("ID\tArea\t\tDistance\tFill Level\n");
    printf("--------------------------------------------------------\n");
    while (temp) {
        printf("%d\t%s\t\t%.2f\t\t%d%%\n", temp->binID, temp->area, temp->distance, temp->fillLevel);
        temp = temp->next;
          }
    printf("--------------------------------------------------------\n");
}

void markBinCollectedAndRequeue(int binID) {
    Dustbin* bin = findBinByID(binID);
    if (!bin) {
        return;
    }

    deletefromqueue(binID);
    deletefrompriorityqueue(binID);
    
    bin->fillLevel = 0;
    bin->priority = (0 * 2) - (bin->distance * 5);
    
    enqueue(bin);
}

// Add this structure and functions for area distance tracking
typedef struct AreaDistance {
    char area[50];
    float distance;
    struct AreaDistance* next;
} AreaDistance;

AreaDistance* areaDistanceHead = NULL;

float getAreaDistance(char* area) {
    AreaDistance* current = areaDistanceHead;
    while (current) {
        if (strcmp(current->area, area) == 0) {
            return current->distance;
        }
        current = current->next;
    }
    return -1.0f;
}

void setAreaDistance(char* area, float distance) {
    AreaDistance* current = areaDistanceHead;
    while (current) {
        if (strcmp(current->area, area) == 0) {
            current->distance = distance;
            return;
        }
        current = current->next;
    }
    
    AreaDistance* newAreaDist = (AreaDistance*)malloc(sizeof(AreaDistance));
    strcpy(newAreaDist->area, area);
    newAreaDist->distance = distance;
    newAreaDist->next = areaDistanceHead;
    areaDistanceHead = newAreaDist;
}

void freeAreaDistances() {
    AreaDistance* current = areaDistanceHead;
    while (current) {
        AreaDistance* temp = current;
        current = current->next;
        free(temp);
    }
    areaDistanceHead = NULL;
}

void collectBinsFromArea(char* area) {
    if (!area || strlen(area) == 0) return;

    // Build list of binIDs in this area from the master 'head' list.
    int ids[256];
    int count = 0;
    Dustbin* d = head;
    while (d) {
        if (strcmp(d->area, area) == 0) {
            if (count < 256) ids[count++] = d->binID;
        }
        d = d->next;
    }
    if (count == 0) {
        printf("    No other bins in area '%s' to collect.\n", area);
        return;
    }


    int collected = 0;
    for (int i = 0; i < count; ++i) {
        int id = ids[i];
        Dustbin *b = findBinByID(id);
        if (!b) continue;
        if (b->fillLevel == 0) continue; // already empty
        printf("    Bin #%d (Fill: %d%%) - COLLECTED\n", id, b->fillLevel);
        updateFillLevel(id, 0);
        deletefromqueue(id);
        deletefrompriorityqueue(id);
        collected++;
    }
    if (collected > 0)
        printf("    Total bins collected from %s: %d\n", area, collected);
    else
        printf("    No non-empty bins found in %s.\n", area);
}


int popPriorityTarget(char *area_buf, float *dist, int *fill) {
    if (!priorityfront) return -1;
    priorityqueue *node = priorityfront;
    int id = node->binID;
    
    // Get current data from actual bin
    if (bin) {
        if (area_buf) strncpy(area_buf, bin->area, 49), area_buf[49] = '\0';
        if (dist) *dist = bin->distance;
        if (fill) *fill = bin->fillLevel;
    } else {
        // Fallback to queue data if bin not found
        if (area_buf) strncpy(area_buf, node->area, 49), area_buf[49] = '\0';
        if (dist) *dist = node->distance;
        if (fill) *fill = node->fillLevel;
    }

    // remove top
    priorityfront = node->next;
    if (!priorityfront) priorityrear = NULL;
    free(node);
    return id;
}

int popNormalTarget(char *area_buf, float *dist, int *fill) {
    if (!front) return -1;
    queue *node = front;
    int id = node->binID;
    
    // Get current data from actual bin
    Dustbin* bin = findBinByID(id);
    if (bin) {
        if (area_buf) strncpy(area_buf, bin->area, 49), area_buf[49] = '\0';
        if (dist) *dist = bin->distance;
        if (fill) *fill = bin->fillLevel;
    } else {
        // Fallback to queue data if bin not found 
        if (area_buf) strncpy(area_buf, node->area, 49), area_buf[49] = '\0';
        if (dist) *dist = node->distance;
        if (fill) *fill = node->fillLevel;
    }

    front = node->next;
    if (!front) rear = NULL;
    free(node);
    return id;
}


// Helper function to convert km → minutes
static float travelTime(float d, float speed_kmph) {
    return (d / speed_kmph) * 60.0;
}

// SINGLE AREA TRUCK COLLECTION (ONE AT A TIME)

void simulateTruckCollection() {
    lastDispatchSummary.valid = 0;

    printf("\n");
    printf("                 TRUCK DISPATCH SIMULATION (TIMED)            \n");
    printf("---------------------------------------------------------------\n");
    

    queueBinsByDistance();
    
    char targetArea[50];
    float targetDist;
    int targetFill;
    int targetID;

    float SPEED_KMPH = 30.0;
    float LOAD_TIME = 3.0;

 
    targetID = popPriorityTarget(targetArea, &targetDist, &targetFill);
    

    if (targetID == -1) {
        targetID = popNormalTarget(targetArea, &targetDist, &targetFill);
    }
    
 
    if (targetID == -1) {
        printf("\nAll bins are empty — no trucks to dispatch.\n");
        printf("---------------------------------------------------------------\n");
        return;
    }
    

    Dustbin* targetBin = findBinByID(targetID);
    if (!targetBin || targetBin->fillLevel == 0) {
        printf("\nTarget bin is already empty. Try again.\n");
        printf("---------------------------------------------------------------\n");
        return;
    }
    
    // Calculate travel times
    float go = travelTime(targetDist, SPEED_KMPH);
    float ret = go;

    // Determine if this is priority or normal
    char priorityStatus[20];
    if (targetFill >= 90) {
        strcpy(priorityStatus, "URGENT");
    } else {
        strcpy(priorityStatus, "NORMAL");
    }

    printf("\n   TRUCK DISPATCHED\n");
    printf("---------------------------------------------------------------\n");
    printf("Target: Bin #%d in '%s'\n", targetID, targetArea);
    printf("Distance: %.2f km | Fill: %d%% | Priority: %s\n", targetDist, targetFill, priorityStatus);
    printf("Travel Time (one way): %.1f min\n\n", go);

    printf("Collecting bins in area '%s':\n", targetArea);
    
    // Collect ALL bins from this area and requeue them
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

    float totalLoad = binsCollected * LOAD_TIME;
    float totalTime = go + ret + totalLoad;

    printf("\n  Route Summary:\n");
    printf("   Travel Outward:      %.1f min\n", go);
    printf("   Loading Time:        %.1f min\n", totalLoad);
    printf("   Return Travel:       %.1f min\n", ret);
    printf("   -----------------------------------\n");
    printf("   TOTAL ROUTE TIME:   %.1f minutes\n", totalTime);
    printf("   Bins Collected:      %d\n", binsCollected);
    
    // Display queues after collection
    printf("\n=== QUEUE STATUS AFTER COLLECTION ===\n");
    printf("\nPriority Queue (Urgent):\n");
    prioritydisplay();
    printf("\nNormal Queue:\n");
    display();
    printf("---------------------------------------------------------------\n");
    printf("   Collection complete! Truck returned to depot.\n");
    printf("   Select option 9 again to dispatch next truck.\n");
    printf("---------------------------------------------------------------\n");

    // Count remaining non-empty bins
    int remainingBins = 0;
    Dustbin* temp = head;
    while (temp) {
        if (temp->fillLevel > 0) remainingBins++;
        temp = temp->next;
    }

    if (remainingBins > 0) {
        printf("\n Remaining bins to collect: %d\n", remainingBins);
        printf("   Select option 9 again to continue collection.\n");
    }

    lastDispatchSummary.valid = 1;
    lastDispatchSummary.targetID = targetID;
    strncpy(lastDispatchSummary.area, targetArea, sizeof(lastDispatchSummary.area) - 1);
    lastDispatchSummary.area[sizeof(lastDispatchSummary.area) - 1] = '\0';
    lastDispatchSummary.distance = targetDist;
    lastDispatchSummary.startFill = targetFill;
    lastDispatchSummary.binsCollected = binsCollected;
    lastDispatchSummary.totalTimeMinutes = totalTime;
    lastDispatchSummary.wasPriority = (targetFill >= 90);
}


void simulateFillLevelIncrease() {
    printf("\nSimulating passage of time - bins filling up...\n");
    
    Dustbin* current = head;
    int updated = 0;
    
    while (current) {
        int increase = (rand() % 20) + 5; // Random increase 5-24%
        int newLevel = current->fillLevel + increase;
        if (newLevel > 100) newLevel = 100;
        
        if (current->fillLevel < newLevel) {
            updateFillLevel(current->binID, newLevel);
            updated++;
        }
        current = current->next;
    }
    
    printf("%d bins updated with new fill levels\n", updated);
    queueBinsByDistance();
}

void displaySystemStatus() {
    printf("\n");
    printf("                    SYSTEM STATUS OVERVIEW                      \n");
    
    int totalBins = 0, urgentBins = 0, highBins = 0, mediumBins = 0, lowBins = 0;
    
    Dustbin* current = head;
    while (current) {
        totalBins++;
        if (current->fillLevel >= 90) urgentBins++;
        else if (current->fillLevel >= 70) highBins++;
        else if (current->fillLevel >= 50) mediumBins++;
        else lowBins++;
        current = current->next;
    }
    
    printf("\n Statistics:\n");
    printf("   Total Bins: %d\n", totalBins);
    printf("Urgent (≥90%%): %d bins\n", urgentBins);
    printf("High (70-89%%): %d bins\n", highBins);
    printf("Medium (50-69%%): %d bins\n", mediumBins);
    printf("Low (<50%%): %d bins\n", lowBins);
    
    if (urgentBins > 0) {
        printf("\nWARNING: %d bins require immediate attention!\n", urgentBins);
    } else {
        printf("\nNo urgent bins - System operating normally\n");
    }
}

void clearQueue() {
    while (front) {
        queue* t = front;
        front = front->next;
        free(t);
    }
    rear = NULL;
}

void clearPriorityQueue() {
    while (priorityfront) {
        priorityqueue* t = priorityfront;
        priorityfront = priorityfront->next;
        free(t);
    }
    priorityrear = NULL;
}

// Convert BST inorder into queues
void inorderBSTtoQueue(BSTNode* root) {
    if (!root) return;
    inorderBSTtoQueue(root->left);
    enqueue(root->binptr);
    inorderBSTtoQueue(root->right);
}

void inorderBSTtoPriorityQueue(BSTNode* root) {
    if (!root) return;
    inorderBSTtoPriorityQueue(root->left);
    priorityenqueue(root->binptr);
    inorderBSTtoPriorityQueue(root->right);
}

// Sort bins using BST and enqueue in sorted order
void queueBinsByDistance() {
    if (!head) {
        printf("No bins available to sort!\n");
        return;
    }
    
    clearQueue();
    clearPriorityQueue();

    BSTNode* bstNormal = NULL;
    BSTNode* bstPriority = NULL;
    Dustbin* temp = head;
    
    while (temp) {
        if (temp->fillLevel >= 90) 
            bstPriority = insertBST(bstPriority, temp);
        else 
            bstNormal = insertBST(bstNormal, temp);
        temp = temp->next;
    }
    
    printf("\n=== Priority Bins Sorted by Distance ===\n");
    if (bstPriority) {
        displayBSTInorder(bstPriority);
    } else {
        printf("No priority bins.\n");
    }
    
    printf("\n=== Normal Bins Sorted by Distance ===\n");
    if (bstNormal) {
        displayBSTInorder(bstNormal);
    } else {
        printf("No normal bins.\n");
    }

    inorderBSTtoPriorityQueue(bstPriority);
    inorderBSTtoQueue(bstNormal);

    freeBST(bstPriority);
    freeBST(bstNormal);
    
    printf("\nBins have been sorted and enqueued by distance successfully!\n");
}

const DispatchSummary* getLastDispatchSummary(void) {
    if (!lastDispatchSummary.valid) {
        return NULL;
    }
    return &lastDispatchSummary;
}
/* Main function with menu-driven interface
int main() {
    srand(time(NULL));
    int choice, id, fillLevel;
    char area[50];
    float distance;

    printf("====================================================\n");
    printf("SMART WASTE MANAGEMENT SYSTEM.\n");
    printf("====================================================\n");

    initializeRandomBins();

    while (1) {
        printf("\n=== MENU ===\n");
        printf("1. Add New Bin\n");
        printf("2. Delete Bin\n");
        printf("3. Update Fill Level\n");
        printf("4. Display All Bins\n");
        printf("5. Find Bin by ID\n");
        printf("6. Reinitialize Random Bins\n");
        printf("7. Display the order of bins\n");
        printf("8. Sort and enqueue bins by distance\n");
        printf("9. Simulate Truck Collection (AUTO) \n");
        printf("10. Simulate Time Passage (Bins Fill Up)\n");
        printf("11. Display System Status \n");
        printf("12. Reinitialize System\n");
        printf("13. Exit..\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        printf("--------------------------------------------------------\n\n");

        switch (choice) {
            case 1:
                printf("Enter Bin ID: ");
                scanf("%d", &id);
                getchar(); // consume newline
                printf("Enter Area: ");
                fgets(area, sizeof(area), stdin);
                area[strcspn(area, "\n")] = '\0';
                printf("Enter Distance from Collection Center: ");
                scanf("%f", &distance);
                printf("Enter Fill Level (0-100): ");
                scanf("%d", &fillLevel);
                addBin(id, area, distance, fillLevel);
                printf("Bin %d added successfully!\n", id);

                break;

            case 2:
                printf("Enter Bin ID to delete: ");
                scanf("%d", &id);
                deleteBin(id);
                break;

            case 3:
                printf("Enter Bin ID: ");
                scanf("%d", &id);
                printf("Enter new Fill Level (0-100): ");
                scanf("%d", &fillLevel);
                updateFillLevel(id, fillLevel);
                break;

            case 4:
                displayBins();
                break;

            case 5:
                printf("Enter Bin ID to find: ");
                scanf("%d", &id);
                Dustbin* found = findBinByID(id);
                if (found)
                    printf("Bin Found: ID=%d, Area=%s, Distance=%.2f, Fill Level=%d%%\n", found->binID, found->area, found->distance, found->fillLevel);
                else
                    printf("Bin %d not found!\n", id);
                break;

            case 6:
                clearQueue();
                clearPriorityQueue();
                freeLinkedList();
                freeAreaDistances();  // Add this line
                initializeRandomBins();
                break;


            case 7:
                printf("Priority Bins: \n");
                prioritydisplay();
                printf("Normal Bins: \n");
                display();
                break;

            case 8:
                printf("\nPriority Bins (Urgent) By Distance:\n");
                queueBinsByDistance();
                break;

            case 9:
                simulateTruckCollection();
                break;

            case 10:
                simulateFillLevelIncrease();
                break;

            case 11:
                displaySystemStatus();
                break;

            case 12:
                printf("\nREINITIALIZING SYSTEM\n");
                clearQueue();
                clearPriorityQueue();
                freeLinkedList();
                initializeRandomBins();
                break;

            case 13:
                printf("\n");
                printf("    Thank you for using Smart Waste Management System v2.0!     \n");
                printf("                    Exiting program...                          \n");
                clearQueue();
                clearPriorityQueue();
                freeLinkedList();
                freeAreaDistances();  // Add this line
                return 0;

            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
    return 0;
} */

int main(int argc, char **argv) {
    start_gui(&argc, &argv);
    return 0;
}
