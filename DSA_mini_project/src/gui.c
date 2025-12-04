#include <gtk/gtk.h>
#include <stdlib.h>
#include <time.h>
#include "gui.h"

// Global Widgets
GtkWidget *bin_table;
GtkWidget *priority_table;
GtkWidget *normal_table;
GtkWidget *status_label;
GtkWidget *analytics_area;
GtkWidget *analytics_info_label;
GtkWidget *truck_anim_area;
GtkWidget *event_log_view;

static GtkTextBuffer *event_log_buffer = NULL;
static double truck_anim_progress = 0.0;
static guint  truck_anim_timeout_id = 0;
static gboolean dark_mode_enabled = FALSE;
static GtkCssProvider *css_provider = NULL;
static gboolean css_provider_installed = FALSE;
static int analytics_selected_index = -1;
static int analytics_counts[4] = {0};

static GtkWidget* create_bins_table();
static GtkWidget* create_queue_tables();
static GtkWidget* create_dashboard_tab();
static GtkWidget* create_simulator_tab();
static GtkWidget* create_analytics_tab();
static void       load_app_css(void);
static gboolean   on_truck_anim_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean   truck_anim_tick(gpointer data);
static gboolean   on_dark_mode_switch(GtkSwitch *widget, GParamSpec *pspec, gpointer data);
static gboolean   on_analytics_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
static void       recompute_analytics_counts(void);
static void       update_analytics_info_label(void);

// --------------------------------------------------------------
// MAIN GUI START
// --------------------------------------------------------------
void start_gui(int *argc, char ***argv) {
    // Initialize GTK
    gtk_init(argc, argv);

    // Seed RNG once and initialize backend bins
    srand((unsigned int)time(NULL));
    initializeRandomBins();
    // Build initial queues so Priority/Normal tabs have data
    queueBinsByDistance();

    // Load custom CSS for a more modern look
    load_app_css();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 720);

    // Client-side decorated header bar for a modern look
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Smart Waste Management System");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), "Real‑time bin monitoring & collection planning");
    gtk_widget_set_name(header, "main-header");

    GtkWidget *dark_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *dark_lbl = gtk_label_new("Dark");
    GtkWidget *dark_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(dark_switch), dark_mode_enabled);
    g_signal_connect(dark_switch, "notify::active", G_CALLBACK(on_dark_mode_switch), NULL);
    gtk_box_pack_start(GTK_BOX(dark_box), dark_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dark_box), dark_switch, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(dark_box, "Toggle light/dark theme");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), dark_box);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_set_name(notebook, "main-notebook");

    // Add tabs
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dashboard_tab(), gtk_label_new("Dashboard"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_bins_table(), gtk_label_new("Bins Overview"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_queue_tables(), gtk_label_new("Collection Queues"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_analytics_tab(), gtk_label_new("Analytics"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_simulator_tab(), gtk_label_new("Simulator"));

    gtk_container_add(GTK_CONTAINER(window), notebook);

    refresh_analytics();
    gtk_widget_show_all(window);

    gtk_main();
}

static GtkWidget* create_dashboard_tab() {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(root), 18);
    gtk_widget_set_name(root, "dashboard-root");

    // Title and subtitle
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size=\"xx-large\" weight=\"bold\">Smart Waste Management Dashboard</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    GtkWidget *subtitle = gtk_label_new(
        "Monitor fill levels, prioritize urgent bins, and dispatch trucks efficiently.");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_name(subtitle, "dashboard-subtitle");

    gtk_box_pack_start(GTK_BOX(root), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), subtitle, FALSE, FALSE, 0);

    // Top: System status + quick tips side by side
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    // Status frame
    GtkWidget *status_frame = gtk_frame_new("System Status");
    GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(status_box), 10);

    status_label = gtk_label_new("Initializing bins...");
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_widget_set_name(status_label, "status-label");

    gtk_box_pack_start(GTK_BOX(status_box), status_label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(status_frame), status_box);

    // Tips / legend frame
    GtkWidget *legend_frame = gtk_frame_new("Legend");
    GtkWidget *legend_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_container_set_border_width(GTK_CONTAINER(legend_box), 10);

    GtkWidget *lbl_u = gtk_label_new("URGENT (≥ 90%) – highest priority, immediate collection");
    GtkWidget *lbl_h = gtk_label_new("HIGH (70–89%) – should be collected soon");
    GtkWidget *lbl_m = gtk_label_new("MEDIUM (50–69%) – monitor regularly");
    GtkWidget *lbl_l = gtk_label_new("LOW (< 50%) – low priority");

    gtk_widget_set_halign(lbl_u, GTK_ALIGN_START);
    gtk_widget_set_halign(lbl_h, GTK_ALIGN_START);
    gtk_widget_set_halign(lbl_m, GTK_ALIGN_START);
    gtk_widget_set_halign(lbl_l, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(legend_box), lbl_u, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(legend_box), lbl_h, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(legend_box), lbl_m, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(legend_box), lbl_l, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(legend_frame), legend_box);

    gtk_box_pack_start(GTK_BOX(top_box), status_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(top_box), legend_frame, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(root), top_box, FALSE, FALSE, 10);

    // Controls frame
    GtkWidget *controls_frame = gtk_frame_new("Controls");
    gtk_widget_set_name(controls_frame, "controls-frame");

    GtkWidget *grid = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 18);

    GtkWidget *btn_add     = gtk_button_new_with_label("➕ Add New Bin");
    GtkWidget *btn_delete  = gtk_button_new_with_label("🗑 Delete Selected Bin");
    GtkWidget *btn_update  = gtk_button_new_with_label("✏️ Edit Fill Level");
    GtkWidget *btn_init    = gtk_button_new_with_label("🎲 Reinitialize Random Bins");
    GtkWidget *btn_fill    = gtk_button_new_with_label("⏱ Simulate Time Passage");
    GtkWidget *btn_sort    = gtk_button_new_with_label("📊 Sort Bins By Distance");
    GtkWidget *btn_collect = gtk_button_new_with_label("🚚 Dispatch Truck");

    gtk_widget_set_name(btn_add,     "primary-button");
    gtk_widget_set_name(btn_collect, "primary-button");

    // Hook callbacks
    g_signal_connect(btn_add,     "clicked", G_CALLBACK(on_add_bin_clicked), NULL);
    g_signal_connect(btn_delete,  "clicked", G_CALLBACK(on_delete_bin_clicked), NULL);
    g_signal_connect(btn_update,  "clicked", G_CALLBACK(on_update_bin_clicked), NULL);
    g_signal_connect(btn_init,    "clicked", G_CALLBACK(on_init_random_clicked), NULL);
    g_signal_connect(btn_fill,    "clicked", G_CALLBACK(on_fill_time_clicked), NULL);
    g_signal_connect(btn_sort,    "clicked", G_CALLBACK(on_sort_bins_clicked), NULL);
    g_signal_connect(btn_collect, "clicked", G_CALLBACK(on_truck_collect_clicked), NULL);

    // Layout buttons
    gtk_grid_attach(GTK_GRID(grid), btn_add,     0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_delete,  1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_update,  2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), btn_init,    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_fill,    1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_sort,    2, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), btn_collect, 1, 2, 1, 1);

    gtk_container_add(GTK_CONTAINER(controls_frame), grid);
    gtk_box_pack_start(GTK_BOX(root), controls_frame, FALSE, FALSE, 5);

    // Initial status text
    refresh_system_status();

    return root;
}

static GtkWidget* create_bins_table() {
    bin_table = gtk_tree_view_new();

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(bin_table), -1,
        "ID", renderer, "text", 0, NULL);

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(bin_table), -1,
        "Area", renderer, "text", 1, NULL);

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(bin_table), -1,
        "Distance", renderer, "text", 2, NULL);

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(bin_table), -1,
        "Fill %", renderer, "text", 3, NULL);

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(bin_table), -1,
        "Status", renderer, "text", 4, NULL);

    gtk_widget_set_name(bin_table, "bins-treeview");

    // Put tree view inside a scrolled window for better UX
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), bin_table);

    refresh_bin_table();

    return scrolled;
}

static GtkWidget* create_queue_tables() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);

    GtkWidget *label1 = gtk_label_new("Priority Queue (Urgent Bins)");
    priority_table = gtk_tree_view_new();
    GtkWidget *label2 = gtk_label_new("Normal Queue");
    normal_table = gtk_tree_view_new();

    gtk_widget_set_name(priority_table, "queue-treeview");
    gtk_widget_set_name(normal_table, "queue-treeview");

    // Define columns for both queue views (same schema as main table)
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priority_table), -1,
        "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priority_table), -1,
        "Area", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priority_table), -1,
        "Distance", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priority_table), -1,
        "Fill %", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priority_table), -1,
        "Status", renderer, "text", 4, NULL);

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(normal_table), -1,
        "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(normal_table), -1,
        "Area", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(normal_table), -1,
        "Distance", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(normal_table), -1,
        "Fill %", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(normal_table), -1,
        "Status", renderer, "text", 4, NULL);

    // Wrap both tables in scrolled windows
    GtkWidget *prio_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prio_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(prio_scrolled), priority_table);

    GtkWidget *norm_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(norm_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(norm_scrolled), normal_table);

    gtk_box_pack_start(GTK_BOX(box), label1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), prio_scrolled, TRUE, TRUE, 5);

    gtk_box_pack_start(GTK_BOX(box), label2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), norm_scrolled, TRUE, TRUE, 5);

    refresh_priority_queue();
    refresh_normal_queue();

    return box;
}

static GtkWidget* create_simulator_tab() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 15);

    GtkWidget *heading = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(heading),
                         "<span size=\"x-large\" weight=\"bold\">Truck Dispatch Simulator</span>");
    gtk_widget_set_halign(heading, GTK_ALIGN_START);

    GtkWidget *desc = gtk_label_new(
        "Each click dispatches a truck to the highest‑priority area, collects all bins there,\n"
        "and updates the queues based on remaining fill levels and distances.");
    gtk_widget_set_halign(desc, GTK_ALIGN_START);

    GtkWidget *btn = gtk_button_new_with_label("🚚 Dispatch Next Truck");
    gtk_widget_set_name(btn, "primary-button");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_truck_collect_clicked), NULL);

    // Simple animation canvas
    truck_anim_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(truck_anim_area, 600, 120);
    gtk_widget_set_name(truck_anim_area, "truck-anim-area");
    g_signal_connect(truck_anim_area, "draw", G_CALLBACK(on_truck_anim_draw), NULL);

    gtk_box_pack_start(GTK_BOX(box), heading, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), desc, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), truck_anim_area, FALSE, FALSE, 10);

    GtkWidget *log_frame = gtk_frame_new("Truck Event Log");
    GtkWidget *log_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scrolled, -1, 160);
    gtk_container_set_border_width(GTK_CONTAINER(log_scrolled), 5);
    event_log_view = gtk_text_view_new();
    event_log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(event_log_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(event_log_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(event_log_view), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scrolled), event_log_view);
    gtk_container_add(GTK_CONTAINER(log_frame), log_scrolled);
    gtk_box_pack_start(GTK_BOX(box), log_frame, TRUE, TRUE, 5);

    return box;
}

// Simple analytics tab with bar chart for fill level distribution
gboolean on_analytics_draw(GtkWidget *widget, cairo_t *cr, gpointer data);

static GtkWidget* create_analytics_tab() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 15);

    GtkWidget *heading = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(heading),
                         "<span size=\"x-large\" weight=\"bold\">Fill Level Analytics</span>");
    gtk_widget_set_halign(heading, GTK_ALIGN_START);

    GtkWidget *desc = gtk_label_new(
        "Bar chart showing the count of bins in each fill-level band.\n"
        "Actions on the dashboard will automatically update this chart.");
    gtk_widget_set_halign(desc, GTK_ALIGN_START);

    analytics_info_label = gtk_label_new("Click a bar to view details.");
    gtk_widget_set_halign(analytics_info_label, GTK_ALIGN_START);

    analytics_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(analytics_area, 600, 260);
    g_signal_connect(analytics_area, "draw", G_CALLBACK(on_analytics_draw), NULL);
    gtk_widget_add_events(analytics_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(analytics_area, "button-press-event", G_CALLBACK(on_analytics_button_press), NULL);

    gtk_box_pack_start(GTK_BOX(box), heading, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), desc, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), analytics_info_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), analytics_area, TRUE, TRUE, 10);

    return box;
}

// --------------------------------------------------------------
// Truck dispatch animation helpers
// --------------------------------------------------------------
void trigger_truck_animation() {
    if (!truck_anim_area)
        return;

    truck_anim_progress = 0.0;
    if (truck_anim_timeout_id) {
        g_source_remove(truck_anim_timeout_id);
    }
    truck_anim_timeout_id = g_timeout_add(30, truck_anim_tick, NULL);
    gtk_widget_queue_draw(truck_anim_area);
}

static gboolean truck_anim_tick(gpointer data) {
    (void)data;
    truck_anim_progress += 0.01;
    if (truck_anim_progress >= 1.0) {
        truck_anim_progress = 1.0;
        truck_anim_timeout_id = 0;
        gtk_widget_queue_draw(truck_anim_area);
        return G_SOURCE_REMOVE;
    }
    gtk_widget_queue_draw(truck_anim_area);
    return G_SOURCE_CONTINUE;
}

static gboolean on_truck_anim_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;
    double width = gtk_widget_get_allocated_width(widget);
    double height = gtk_widget_get_allocated_height(widget);

    // Background
    cairo_set_source_rgb(cr, 0.94, 0.96, 0.99);
    cairo_paint(cr);

    // Road
    double road_y = height * 0.65;
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.75);
    cairo_rectangle(cr, 20, road_y - 15, width - 40, 30);
    cairo_fill(cr);

    // Road divider
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 25, road_y);
    cairo_line_to(cr, width - 25, road_y);
    cairo_set_dash(cr, (double[]){10.0, 6.0}, 2, 0);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);

    // Calculate truck position
    double start_x = 30;
    double end_x = width - 110;
    double truck_x = start_x + (end_x - start_x) * truck_anim_progress;
    double truck_y = road_y - 10;

    // Truck body
    cairo_set_source_rgb(cr, 0.2, 0.45, 0.9);
    cairo_rectangle(cr, truck_x, truck_y - 20, 70, 30);
    cairo_fill(cr);

    // Truck cab
    cairo_set_source_rgb(cr, 0.15, 0.35, 0.75);
    cairo_rectangle(cr, truck_x + 50, truck_y - 30, 30, 40);
    cairo_fill(cr);

    // Wheels
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.15);
    cairo_arc(cr, truck_x + 15, truck_y + 10, 8, 0, 2 * G_PI);
    cairo_arc(cr, truck_x + 55, truck_y + 10, 8, 0, 2 * G_PI);
    cairo_fill(cr);

    // Progress text
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.3);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);
    char buf[64];
    snprintf(buf, sizeof(buf), "Dispatch progress: %d%%", (int)(truck_anim_progress * 100));
    cairo_move_to(cr, 30, 30);
    cairo_show_text(cr, buf);

    return FALSE;
}

void append_event_log(const char *message) {
    if (!event_log_buffer || !event_log_view || !message) return;

    time_t now = time(NULL);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm_now);

    gchar *formatted = g_strdup_printf("[%s] %s\n", timebuf, message);

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(event_log_buffer, &end);
    gtk_text_buffer_insert(event_log_buffer, &end, formatted, -1);

    GtkTextMark *mark = gtk_text_buffer_create_mark(event_log_buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(event_log_view), mark);
    gtk_text_buffer_delete_mark(event_log_buffer, mark);

    g_free(formatted);
}

// --------------------------------------------------------------
// Analytics helpers & drawing
// --------------------------------------------------------------
static void recompute_analytics_counts(void) {
    analytics_counts[0] = analytics_counts[1] = analytics_counts[2] = analytics_counts[3] = 0;

    Dustbin *cur = head;
    while (cur) {
        if (cur->fillLevel >= 90) analytics_counts[0]++;
        else if (cur->fillLevel >= 70) analytics_counts[1]++;
        else if (cur->fillLevel >= 50) analytics_counts[2]++;
        else analytics_counts[3]++;
        cur = cur->next;
    }
}

static const char *analytics_labels[4] = { "URGENT (≥ 90%)", "HIGH (70–89%)", "MEDIUM (50–69%)", "LOW (< 50%)" };

static void update_analytics_info_label(void) {
    if (!analytics_info_label) return;

    if (analytics_selected_index < 0) {
        gtk_label_set_text(GTK_LABEL(analytics_info_label), "Click a bar to view category details.");
        return;
    }

    int count = analytics_counts[analytics_selected_index];
    gchar *text = g_strdup_printf("%s: %d bins", analytics_labels[analytics_selected_index], count);
    gtk_label_set_text(GTK_LABEL(analytics_info_label), text);
    g_free(text);
}

void refresh_analytics() {
    recompute_analytics_counts();
    update_analytics_info_label();
    if (analytics_area) {
        gtk_widget_queue_draw(analytics_area);
    }
}

gboolean on_analytics_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;
    double width  = gtk_widget_get_allocated_width(widget);
    double height = gtk_widget_get_allocated_height(widget);

    cairo_set_source_rgb(cr, dark_mode_enabled ? 0.16 : 0.97,
                             dark_mode_enabled ? 0.18 : 0.98,
                             dark_mode_enabled ? 0.22 : 1.0);
    cairo_paint(cr);

    double margin = 40.0;
    double chart_w = width - 2 * margin;
    double chart_h = height - 2 * margin;
    if (chart_w <= 0 || chart_h <= 0) return FALSE;

    cairo_set_source_rgb(cr, dark_mode_enabled ? 0.85 : 0.2,
                             dark_mode_enabled ? 0.85 : 0.2,
                             dark_mode_enabled ? 0.9 : 0.3);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, margin, margin);
    cairo_line_to(cr, margin, margin + chart_h);
    cairo_line_to(cr, margin + chart_w, margin + chart_h);
    cairo_stroke(cr);

    int max = 1;
    for (int i = 0; i < 4; ++i) {
        if (analytics_counts[i] > max) max = analytics_counts[i];
    }

    double spacing = chart_w / 4.0;
    double bar_w = spacing * 0.45;

    for (int i = 0; i < 4; ++i) {
        double cx = margin + spacing * (i + 0.5);
        double bar_h = (max > 0) ? (chart_h * analytics_counts[i] / (double)max) : 0.0;
        double x = cx - bar_w / 2.0;
        double y = margin + chart_h - bar_h;

        if (i == 0)      cairo_set_source_rgb(cr, 0.85, 0.15, 0.15);
        else if (i == 1) cairo_set_source_rgb(cr, 0.95, 0.6, 0.0);
        else if (i == 2) cairo_set_source_rgb(cr, 0.4, 0.7, 0.2);
        else             cairo_set_source_rgb(cr, 0.2, 0.5, 0.9);

        if (analytics_selected_index == i) {
            cairo_save(cr);
            cairo_rectangle(cr, x - 2, y - 2, bar_w + 4, bar_h + 4);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
            cairo_fill(cr);
            cairo_restore(cr);
        }

        cairo_rectangle(cr, x, y, bar_w, bar_h);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, dark_mode_enabled ? 0.9 : 0.1,
                                 dark_mode_enabled ? 0.9 : 0.1,
                                 dark_mode_enabled ? 0.95 : 0.2);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10.0);
        cairo_move_to(cr, cx - 30, margin + chart_h + 15);
        cairo_show_text(cr, analytics_labels[i]);

        char numbuf[16];
        snprintf(numbuf, sizeof(numbuf), "%d", analytics_counts[i]);
        cairo_move_to(cr, cx - 5, y - 5);
        cairo_show_text(cr, numbuf);
    }

    return FALSE;
}

static gboolean on_analytics_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)data;
    if (event->button != GDK_BUTTON_PRIMARY) return FALSE;

    double width  = gtk_widget_get_allocated_width(widget);
    double height = gtk_widget_get_allocated_height(widget);
    double margin = 40.0;
    double chart_w = width - 2 * margin;
    double chart_h = height - 2 * margin;
    if (chart_w <= 0 || chart_h <= 0) return FALSE;

    int max = 1;
    for (int i = 0; i < 4; ++i) if (analytics_counts[i] > max) max = analytics_counts[i];
    double spacing = chart_w / 4.0;
    double bar_w = spacing * 0.45;
    double chart_bottom = margin + chart_h;

    for (int i = 0; i < 4; ++i) {
        double cx = margin + spacing * (i + 0.5);
        double bar_h = (max > 0) ? (chart_h * analytics_counts[i] / (double)max) : 0.0;
        double x = cx - bar_w / 2.0;
        double y = chart_bottom - bar_h;

        if (event->x >= x && event->x <= x + bar_w &&
            event->y >= y && event->y <= chart_bottom) {
            analytics_selected_index = i;
            update_analytics_info_label();
            gtk_widget_queue_draw(widget);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean on_dark_mode_switch(GtkSwitch *widget, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    (void)data;
    dark_mode_enabled = gtk_switch_get_active(widget);
    load_app_css();
    if (analytics_area) gtk_widget_queue_draw(analytics_area);
    if (truck_anim_area) gtk_widget_queue_draw(truck_anim_area);
    return FALSE;
}

// --------------------------------------------------------------
// CSS styling
// --------------------------------------------------------------
static void load_app_css(void) {
    static const gchar *light_css =
        "#main-header {"
        "  background: linear-gradient(to right, #1e3c72, #2a5298);"
        "  color: #ffffff;"
        "}"
        "#main-notebook tab {"
        "  padding: 6px 14px;"
        "}"
        "#dashboard-root {"
        "  background-color: #f5f7fb;"
        "}"
        "#dashboard-subtitle {"
        "  color: #555555;"
        "}"
        "#controls-frame {"
        "  background-color: #ffffff;"
        "  border-radius: 6px;"
        "}"
        "#primary-button {"
        "  background: #2a5298;"
        "  color: #ffffff;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "}"
        "#primary-button:hover {"
        "  background: #1e3c72;"
        "}"
        "#bins-treeview, #queue-treeview {"
        "  font-family: Sans;"
        "  font-size: 10pt;"
        "}"
        "#status-label {"
        "  font-weight: bold;"
        "}"
        "#truck-anim-area {"
        "  border: 1px solid #d0d6e2;"
        "  border-radius: 6px;"
        "}";

    static const gchar *dark_css =
        "#main-header {"
        "  background: linear-gradient(to right, #0f2027, #203a43);"
        "  color: #e0e6f5;"
        "}"
        "#main-notebook {"
        "  background-color: #11161f;"
        "}"
        "#main-notebook tab {"
        "  padding: 6px 14px;"
        "  color: #e0e6f5;"
        "}"
        "#dashboard-root {"
        "  background-color: #141b24;"
        "  color: #e0e6f5;"
        "}"
        "#dashboard-subtitle {"
        "  color: #a0aec0;"
        "}"
        "#controls-frame {"
        "  background-color: #1f2733;"
        "  border-radius: 6px;"
        "  color: #e0e6f5;"
        "}"
        "#primary-button {"
        "  background: #2d71b8;"
        "  color: #ffffff;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "}"
        "#primary-button:hover {"
        "  background: #1d4f86;"
        "}"
        "#bins-treeview, #queue-treeview {"
        "  font-family: Sans;"
        "  font-size: 10pt;"
        "  color: #e0e6f5;"
        "}"
        "#status-label {"
        "  font-weight: bold;"
        "}"
        "#truck-anim-area {"
        "  border: 1px solid #394355;"
        "  border-radius: 6px;"
        "}";

    if (!css_provider) {
        css_provider = gtk_css_provider_new();
    }

    const gchar *css = dark_mode_enabled ? dark_css : light_css;
    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);

    if (!css_provider_installed) {
        GdkScreen *screen = gdk_screen_get_default();
        if (screen) {
            gtk_style_context_add_provider_for_screen(
                screen,
                GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            css_provider_installed = TRUE;
        }
    }
}

