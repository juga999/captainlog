#include <archive.h>
#include <archive_entry.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

static void handle_app_uri_scheme(WebKitURISchemeRequest *request, gpointer user_data)
{
    const gchar *uri = webkit_uri_scheme_request_get_uri(request);
    g_print("Handling request for URI: %s\n", uri);

    const gchar *html = "<html><body><h1>Hello from chorem:// protocol!</h1></body></html>";
    GInputStream *stream = g_memory_input_stream_new_from_data(html, strlen(html), NULL);

    // Set the content type and content length
	webkit_uri_scheme_request_finish(request, stream, strlen(html), "text/html");

    // Clean up
    g_object_unref(stream);
}

extern "C" void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "CaptainLog");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *webview = webkit_web_view_new();
    WebKitWebContext *context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview));

    // Register the custom scheme
    webkit_web_context_register_uri_scheme(context, "app", handle_app_uri_scheme, NULL, NULL);
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), "app://index.html");

    gtk_container_add(GTK_CONTAINER(window), webview);
    gtk_widget_show_all(window);

}
