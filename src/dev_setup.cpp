#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

extern "C" void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "CaptainLog");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *webview = webkit_web_view_new();

    // Register the custom scheme
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), "app://index.html");
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(webview), "<html><body><h1>Hello World !</h1></body></html>", NULL);

    gtk_container_add(GTK_CONTAINER(window), webview);
    gtk_widget_show_all(window);

}
