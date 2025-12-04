#include "gui.h"
#include <stdio.h>
#include <stdlib.h>

// Selected Bin ID (for edit/delete)
static int selected_bin_id = -1;

// --------------------------------------------------------------
// ROW ACTIVATED: Edit Bin Inline
// --------------------------------------------------------------
void on_bin_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                          GtkTreeViewColumn *column, gpointer user_data) {

    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *id_str;
        gtk_tree_model_get(model, &iter, 0, &id_str, -1);
        selected_bin_id = atoi(id_str);
        g_free(id_str);

        // Ask for new fill level via GTK dialog
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Edit Fill Level",
            NULL,
            GTK_DIALOG_MODAL,
            "_OK",
            GTK_RESPONSE_OK,
            "_Cancel",
            GTK_RESPONSE_CANCEL,
            NULL
        );

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter new fill %");
        gtk_container_add(GTK_CONTAINER(content_area), entry);
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
            int new_fill = atoi(text);
            updateFillLevel(selected_bin_id, new_fill);
            refresh_bin_table();
            refresh_priority_queue();
            refresh_normal_queue();
            refresh_system_status();
            refresh_analytics();
        }

        gtk_widget_destroy(dialog);
    }
}

// --------------------------------------------------------------
// BUTTON CALLBACKS
// --------------------------------------------------------------

void on_add_bin_clicked(GtkButton *button, gpointer user_data) {
    // Dialog to add a fully specified bin
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add New Bin",
        NULL,
        GTK_DIALOG_MODAL,
        "_Add", GTK_RESPONSE_OK,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

    GtkWidget *lbl_id   = gtk_label_new("Bin ID:");
    GtkWidget *lbl_area = gtk_label_new("Area:");
    GtkWidget *lbl_dist = gtk_label_new("Distance (km):");
    GtkWidget *lbl_fill = gtk_label_new("Fill Level (0-100):");

    GtkWidget *entry_id   = gtk_entry_new();
    GtkWidget *entry_area = gtk_entry_new();
    GtkWidget *entry_dist = gtk_entry_new();
    GtkWidget *entry_fill = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), lbl_id,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_id, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_area, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_area, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_dist, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_dist, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_fill, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_fill, 1, 3, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *id_txt   = gtk_entry_get_text(GTK_ENTRY(entry_id));
        const char *area_txt = gtk_entry_get_text(GTK_ENTRY(entry_area));
        const char *dist_txt = gtk_entry_get_text(GTK_ENTRY(entry_dist));
        const char *fill_txt = gtk_entry_get_text(GTK_ENTRY(entry_fill));

        int   id    = atoi(id_txt);
        float dist  = (float)atof(dist_txt);
        int   fill  = atoi(fill_txt);

        // Basic validation; backend will also validate
        if (strlen(area_txt) > 0) {
            addBin(id, (char*)area_txt, dist, fill);
            queueBinsByDistance();
            refresh_bin_table();
            refresh_priority_queue();
            refresh_normal_queue();
            refresh_system_status();
            refresh_analytics();
        }
    }

    gtk_widget_destroy(dialog);
}

void on_delete_bin_clicked(GtkButton *button, gpointer user_data) {
    if (selected_bin_id == -1) {
        GtkWidget *info = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Please select a bin to delete.");
        gtk_dialog_run(GTK_DIALOG(info));
        gtk_widget_destroy(info);
        return;
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE,
        "Delete Bin #%d?\nThis action cannot be undone.",
        selected_bin_id);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           "_Cancel", GTK_RESPONSE_CANCEL,
                           "_Delete", GTK_RESPONSE_ACCEPT,
                           NULL);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response != GTK_RESPONSE_ACCEPT) {
        return;
    }

    deleteBin(selected_bin_id);
    selected_bin_id = -1;
    queueBinsByDistance();
    refresh_bin_table();
    refresh_priority_queue();
    refresh_normal_queue();
    refresh_system_status();
    refresh_analytics();
}

void on_update_bin_clicked(GtkButton *button, gpointer user_data) {
    // just reuse the selected row edit
    if (selected_bin_id != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices(selected_bin_id, -1);
        on_bin_row_activated(GTK_TREE_VIEW(bin_table), path, NULL, NULL);
        gtk_tree_path_free(path);
    }
}

void on_init_random_clicked(GtkButton *button, gpointer user_data) {
    // Reinitialize the full system of bins
    clearQueue();
    clearPriorityQueue();
    freeLinkedList();
    freeAreaDistances();
    initializeRandomBins();
    queueBinsByDistance();
    refresh_bin_table();
    refresh_priority_queue();
    refresh_normal_queue();
    refresh_system_status();
    refresh_analytics();
}

void on_fill_time_clicked(GtkButton *button, gpointer user_data) {
    simulateFillLevelIncrease();
    queueBinsByDistance();
    refresh_bin_table();
    refresh_priority_queue();
    refresh_normal_queue();
    refresh_system_status();
    refresh_analytics();
}

void on_sort_bins_clicked(GtkButton *button, gpointer user_data) {
    queueBinsByDistance();
    refresh_bin_table();
    refresh_priority_queue();
    refresh_normal_queue();
    refresh_analytics();
}

void on_truck_collect_clicked(GtkButton *button, gpointer user_data) {
    simulateTruckCollection();
    refresh_bin_table();
    refresh_priority_queue();
    refresh_normal_queue();
    refresh_system_status();
    refresh_analytics();
    trigger_truck_animation();
    const DispatchSummary *summary = getLastDispatchSummary();
    if (summary) {
        gchar *msg = g_strdup_printf("Truck returned from %s (Bin #%d) • Collected %d bins in %.1f min",
                                     summary->area,
                                     summary->targetID,
                                     summary->binsCollected,
                                     summary->totalTimeMinutes);
        append_event_log(msg);
        g_free(msg);
    } else {
        append_event_log("No bins required dispatch.");
    }
}
