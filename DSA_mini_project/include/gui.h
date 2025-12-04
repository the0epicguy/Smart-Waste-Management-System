#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "core.h"

// Exposed GTK widgets (defined in gui.c)
extern GtkWidget *bin_table;
extern GtkWidget *priority_table;
extern GtkWidget *normal_table;
extern GtkWidget *status_label;
extern GtkWidget *analytics_area;

// Main GTK initialization
void start_gui(int *argc, char ***argv);

// Functions to refresh GUI components
void refresh_bin_table();
void refresh_priority_queue();
void refresh_normal_queue();
void refresh_system_status();
void refresh_analytics();
void trigger_truck_animation();
void append_event_log(const char *message);

// Callback prototypes used across GUI files
void on_bin_row_activated(GtkTreeView *tree_view,
                          GtkTreePath *path,
                          GtkTreeViewColumn *column,
                          gpointer user_data);
void on_add_bin_clicked(GtkButton *button, gpointer user_data);
void on_delete_bin_clicked(GtkButton *button, gpointer user_data);
void on_update_bin_clicked(GtkButton *button, gpointer user_data);
void on_init_random_clicked(GtkButton *button, gpointer user_data);
void on_fill_time_clicked(GtkButton *button, gpointer user_data);
void on_sort_bins_clicked(GtkButton *button, gpointer user_data);
void on_truck_collect_clicked(GtkButton *button, gpointer user_data);

#endif
