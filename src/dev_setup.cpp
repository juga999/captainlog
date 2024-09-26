#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

using json = nlohmann::json;

constexpr std::string_view DEV_CONF_FILE_NAME = "captainlog-dev.conf";


extern "C" void on_activate(GtkApplication* app, gpointer user_data) {
	std::filesystem::path current_path = std::filesystem::current_path();
	fmt::println("Current working directory: {}", current_path.c_str());

	fs::path app_config_path{ static_cast<std::string>(DEV_CONF_FILE_NAME) };
	if (!fs::exists(fs::status(app_config_path))) {
		throw std::runtime_error(fmt::format("File not found: {}", DEV_CONF_FILE_NAME));
	}

	json config_json;
    std::ifstream config_stream(app_config_path);
    config_stream >> config_json;

    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "CaptainLog");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget* webview = webkit_web_view_new();

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), config_json["ui-dev-url"].get<std::string>().c_str());

    gtk_container_add(GTK_CONTAINER(window), webview);
    gtk_widget_show_all(window);
}
