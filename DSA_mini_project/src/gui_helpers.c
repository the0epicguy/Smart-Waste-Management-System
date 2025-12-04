#include "gui.h"
#include <gtk/gtk.h>

// Data comes directly from core lists/queues declared in core.h
extern Dustbin* head;
extern queue* front;
extern priorityqueue* priorityfront;
extern GtkWidget *analytics_area;

// --------------------------------------------------------------
// GENERIC TREE MODEL REFRESH FROM DUSTBIN NODES
// --------------------------------------------------------------
static void fill_tree_view_from_dustbins(GtkWidget *tree_view, Dustbin *list_head) {
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter iter;

    Dustbin *current = list_head;
    while (current != NULL) {
        char id_str[8], dist_str[8], fill_str[8];
        sprintf(id_str, "%d", current->binID);
        sprintf(dist_str, "%.2f", current->distance);
        sprintf(fill_str, "%d", current->fillLevel);

        // Derive status label from fill level (same logic as console)
        char status[20];
        if (current->fillLevel >= 90) sprintf(status, "URGENT");
        else if (current->fillLevel >= 70) sprintf(status, "HIGH");
        else if (current->fillLevel >= 50) sprintf(status, "MEDIUM");
        else sprintf(status, "LOW");

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, id_str,
                           1, current->area,
                           2, dist_str,
                           3, fill_str,
                           4, status,
                           -1);
        current = current->next;
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(store));
    g_object_unref(store);

    // Make rows clickable for inline editing
    g_signal_connect(tree_view, "row-activated",
                     G_CALLBACK(on_bin_row_activated), NULL);
}

// --------------------------------------------------------------
void refresh_bin_table() {
    fill_tree_view_from_dustbins(bin_table, head);
}

void refresh_priority_queue() {
    // Priority queue is already built from priority bins; iterate queue nodes
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter iter;

    priorityqueue *current = priorityfront;
    while (current != NULL) {
        char id_str[8], dist_str[8], fill_str[8];
        sprintf(id_str, "%d", current->binID);
        sprintf(dist_str, "%.2f", current->distance);
        sprintf(fill_str, "%d", current->fillLevel);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, id_str,
                           1, current->area,
                           2, dist_str,
                           3, fill_str,
                           4, "URGENT",
                           -1);
        current = current->next;
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(priority_table), GTK_TREE_MODEL(store));
    g_object_unref(store);
}

void refresh_normal_queue() {
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter iter;

    queue *current = front;
    while (current != NULL) {
        char id_str[8], dist_str[8], fill_str[8];
        sprintf(id_str, "%d", current->binID);
        sprintf(dist_str, "%.2f", current->distance);
        sprintf(fill_str, "%d", current->fillLevel);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, id_str,
                           1, current->area,
                           2, dist_str,
                           3, fill_str,
                           4, "NORMAL",
                           -1);
        current = current->next;
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(normal_table), GTK_TREE_MODEL(store));
    g_object_unref(store);
}

// High-level system status -> dashboard label
void refresh_system_status() {
    if (!status_label) return;

    int total = 0, urgent = 0, high = 0, medium = 0, low = 0;
    Dustbin *cur = head;
    while (cur) {
        total++;
        if (cur->fillLevel >= 90) urgent++;
        else if (cur->fillLevel >= 70) high++;
        else if (cur->fillLevel >= 50) medium++;
        else low++;
        cur = cur->next;
    }

    char buf[256];
    snprintf(buf, sizeof(buf),
             "Total bins: %d | Urgent: %d | High: %d | Medium: %d | Low: %d",
             total, urgent, high, medium, low);

    gtk_label_set_text(GTK_LABEL(status_label), buf);
}

