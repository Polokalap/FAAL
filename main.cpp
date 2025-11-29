#include <algorithm>
#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <unordered_map>
#include <glib.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fs = std::filesystem;
using namespace std;

enum log_type {
    INFO,
    WARN,
    ERROR,
    FATAL
};

void logger(log_type type, string message) {

    string prefix;
    string color = "\033[0m";

    switch (type) {

        case INFO:
            prefix = "[INFO] ";
            break;
        case WARN:
            prefix = "[WARN] ";
            color = "\033[0;33m";
            break;
        case ERROR:
            prefix = "[ERROR] ";
            color = "\033[0;31m";
            break;
        case FATAL:
            prefix = "[FATAL] ";
            color = "\033[41m";
            break;

    }

    cout << color << prefix << message << endl;

}

struct EntryData {
    GtkWidget *vbox;
    vector<string> *names;
};

int cursor = 0;
unordered_map<string, string> apps;
unordered_map<string, string> icons;
unordered_map<string, vector<string>> keywords;
vector<string> names;
vector<string> currentAppNames;
string entryText;
extern char **environ;

static void make_new_app_list() {

    currentAppNames.clear();

    string text_edited;

    for (char c : entryText) {

        if (c != '.' && c != '_' && c != '-' && c != ' ') {

            if (isupper(c)) c = tolower(c);

            text_edited += c;

        }

    }

    for (const string &name : names) {

        string name_edited;

        for (char c : name) {

            if (c != '.' && c != '_' && c != '-' && c != ' ') {

                if (std::isupper(c)) c = std::tolower(c);

                name_edited += c;

            }

        }

        if (name_edited.find(text_edited) != string::npos) {

            currentAppNames.push_back(name);

        } else {

            for (string keyword : keywords[name]) {

                string keyword_edited;

                for (char c : keyword) {

                    if (c != '.' && c != '_' && c != '-' && c != ' ') {

                        if (std::isupper(c)) c = std::tolower(c);

                        keyword_edited += c;

                    }

                }

                if (keyword_edited.find(text_edited) != string::npos) {

                    currentAppNames.push_back(name);

                }

            }

        }

    }

    const string home = std::getenv("HOME");

    fs::path path = fs::path(home) / ".config" / "faal";

    try {

        fs::path filePath = path / "uses.pds";

        std::unordered_map<std::string, int> counter;
        if (fs::exists(filePath)) {
            std::ifstream infile(filePath);
            std::string line;
            while (std::getline(infile, line)) {
                std::istringstream iss(line);
                std::string app;
                int count;
                if (std::getline(iss, app, '=') && iss >> count) {
                    counter[app] = count;
                }
            }
            infile.close();
        }

        std::sort(currentAppNames.begin(), currentAppNames.end(),
        [&counter](const std::string &a, const std::string &b) {
            int countA = counter.count(a) ? counter.at(a) : 0;
            int countB = counter.count(b) ? counter.at(b) : 0;
            return countA > countB;
        });

    } catch (std::exception &e) {

        logger(INFO, e.what());

    }

}

static void reload(GtkWidget *vbox) {

    gtk_container_foreach(GTK_CONTAINER(vbox),
        [](GtkWidget *child, gpointer entry_ptr) {
            if (!GTK_IS_ENTRY(child))
                gtk_widget_destroy(child);
        },
        NULL
    );

    if (currentAppNames.size() - 1 < cursor) cursor = currentAppNames.size() - 1;

    int i = 0;

    for (const string &name : currentAppNames) {

        GtkWidget *label = gtk_label_new(name.c_str());
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
        gtk_widget_show(label);

        // GtkIconTheme *theme = gtk_icon_theme_get_default();
        // GtkIconInfo *info  = gtk_icon_theme_lookup_icon(theme, name.c_str(), 16, GTK_ICON_LOOKUP_DIR_LTR);
        //
        // if (info) {
        //
        //     const char *path = gtk_icon_info_get_filename(info);
        //     GtkWidget *image = gtk_image_new_from_file(path);
        //
        //     gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 5);
        //     gtk_widget_show(image);
        //
        //     gtk_icon_info_free(info);
        //
        // }

        if (i == cursor) {

            GtkStyleContext *context = gtk_widget_get_style_context(label);
            gtk_style_context_add_class(context, "cursor");

        }

        i++;

    }

}

static void on_entry_changed(GtkEntry *entry, gpointer user_data) {

    EntryData *data = static_cast<EntryData*>(user_data);
    GtkWidget *vbox = data->vbox;

    entryText = gtk_entry_get_text(entry);

    make_new_app_list();
    reload(vbox);

}

static void make_applications_list(fs::path path, string name) {

    for (const auto& entry : fs::directory_iterator(path)) {

        if (entry.path().string().ends_with(".desktop")) {

            ifstream file(entry.path());
            string line;

            while (getline(file, line)) {

                string name;

                if (line.starts_with("Name=")) {

                    if (find(names.begin(), names.end(), line.substr(5)) != names.end()) break;

                    names.push_back(line.substr(5));

                    name = line.substr(5);

                }

                if (line.starts_with("Keywords=")) {

                    if (find(names.begin(), names.end(), name) != names.end()) break;

                    string keywordsLine = line.substr(9);
                    size_t start = 0;
                    size_t end = 0;

                    while ((end = keywordsLine.find(';', start)) != string::npos) {
                        string token = keywordsLine.substr(start, end - start);
                        keywords[names.back()].push_back(token);
                        start = end + 1;
                    }

                    if (start < keywordsLine.size()) {
                        string token = keywordsLine.substr(start);
                        keywords[names.back()].push_back(token);
                    }

                }

                if (line.starts_with("Exec=")) {

                    if (find(names.begin(), names.end(), name) != names.end()) break;

                    string exec_cmd = line.substr(5);

                    std::vector<std::string> placeholders = {"%U", "%u", "%F", "%f", "%i"};

                    for (const auto& ph : placeholders) {
                        size_t pos = 0;
                        while ((pos = exec_cmd.find(ph, pos)) != std::string::npos) {
                            exec_cmd.replace(pos, ph.length(), "");
                        }
                    }

                    if (line.starts_with("Icon=")) {

                        string icon = line.substr(5);
                        icons[names.back()] = icon;

                    }

                    apps[names.back()] = exec_cmd;

                }

            }

            file.close();

            logger(INFO, "Recorded " + entry.path().string() + " from " + name);

        } else if (entry.path().string().ends_with(".bfs")) {

            ifstream file(entry.path());
            string line;

            while (getline(file, line)) {

                if (line.starts_with("Name=")) {

                    if (find(names.begin(), names.end(), line.substr(5)) == names.end()) {

                        names.push_back(line.substr(5));

                    }

                }

                if (line.starts_with("Keywords=")) {

                    string keywordsLine = line.substr(9);
                    size_t start = 0;
                    size_t end = 0;

                    while ((end = keywordsLine.find(';', start)) != string::npos) {
                        string token = keywordsLine.substr(start, end - start);
                        keywords[names.back()].push_back(token);
                        start = end + 1;
                    }

                    if (start < keywordsLine.size()) {
                        string token = keywordsLine.substr(start);
                        keywords[names.back()].push_back(token);
                    }

                }

            }

            file.close();

            apps[names.back()] = "faal " + entry.path().string();

            logger(INFO, "Recorded " + entry.path().string() + " from " + name);

        }

    }

}

static void add_to_application_counter(string application) {

    const string home = std::getenv("HOME");

    fs::path path = fs::path(home) / ".config" / "faal";

    try {

        fs::path filePath = path / "uses.pds";

        std::unordered_map<std::string, int> counter;
        if (fs::exists(filePath)) {
            std::ifstream infile(filePath);
            std::string line;
            while (std::getline(infile, line)) {
                std::istringstream iss(line);
                std::string app;
                int count;
                if (std::getline(iss, app, '=') && iss >> count) {
                    counter[app] = count;
                }
            }
            infile.close();
        }

        counter[application]++;

        std::ofstream outfile(filePath, std::ios::trunc);
        for (auto &[app, count] : counter) {
            outfile << app << "=" << count << "\n";
        }
        outfile.close();

        logger(INFO, "Updated usage file.");

    } catch (fs::filesystem_error &e) {
        logger(FATAL, "Error creating config file.");
    }

}

static void open_app_on_cursor() {

    const char* cmd = apps[currentAppNames[cursor]].c_str();

    add_to_application_counter(currentAppNames[cursor]);

    logger(INFO, cmd);

    pid_t pid = fork();
    if (pid == 0) {

        setsid();

        int fd = open("/dev/null", O_RDWR);
        if (fd != -1) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2) close(fd);
        }

        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        _exit(1);

    }

}

int main(int argc, char* argv[]) {

    logger(INFO, "Killing any already running processes");

    pid_t self = getpid();

    std::string cmd = "for pid in $(pidof faal); do "
                      "if [ \"$pid\" != \"" + std::to_string(self) + "\" ]; then "
                      "kill -9 $pid; "
                      "fi; "
                      "done";

    system(cmd.c_str());


    const string home = std::getenv("HOME");

    fs::path confPath = fs::path(home) / ".config" / "faal";

    if (!fs::exists(confPath) || !fs::is_directory(confPath) || !fs::exists(confPath / "config.css")) {

        logger(WARN, "Config path doesn't exist.");
        logger(INFO, "Creating config file.");

        try {

            fs::create_directory(confPath);

            ofstream configFile(confPath / "config.css");

            logger(INFO, "Writing config file.");

            // yes I forgor to fix this in v1

            configFile << "window {"
            "\n   border-radius: 15px;"
            "\n   background-color: #00b7ff;"
            "\n   font-family: 'Poppins', sans-serif;"
            "\n}"
            "\nlabel.cursor {"
            "\n   background-color: #000000;"
            "\n}"
            ;

            configFile.close();

            logger(INFO, "Config file created.");

        } catch (fs::filesystem_error &e) {

            logger(FATAL, "Error creating config file.");

            return 1;

        }

    }

    fs::path scriptsPath = fs::path(home) / ".faal" / "scripts";

    if (!fs::exists(scriptsPath) || !fs::is_directory(scriptsPath) || !fs::exists(scriptsPath / "themes.bfs")) {

        logger(WARN, "Script path doesn't exist.");
        logger(INFO, "Creating Theme Script file.");

        try {

            fs::create_directories(scriptsPath);

            ofstream scriptFile(scriptsPath / "themes.bfs");

            logger(INFO, "Writing script file.");

            scriptFile << "Name=Themes"
            "\nOption=Name=Default;Exec=mkdir -p ~/.config/faal && [ -f ~/.config/faal/config.css ] && mv ~/.config/faal/config.css ~/.config/faal/config.css.bak"
            "\nOption=Name=Polokalap;Exec=mkdir -p ~/.config/faal/themes && curl -f -L https://raw.githubusercontent.com/Polokalap/FAAL-configs/main/Polokalap/config.css -o ~/.config/faal/themes/config.css && cp ~/.config/faal/themes/config.css ~/.config/faal/config.css && curl -f -L https://raw.githubusercontent.com/Polokalap/FAAL-configs/main/Polokalap/bg.png -o ~/.config/faal/bg.png"
            ;

            scriptFile.close();

            logger(INFO, "Script file created.");

        } catch (fs::filesystem_error &e) {

            logger(FATAL, "Error creating script file.");

            return 1;

        }

    }

    if (argc > 1) {

        fs::path file_to_run = argv[1];

        if (!fs::exists(file_to_run)) {

            std::cerr << "File doesn't exist.\n";
            return 1;

        }

        ifstream file(file_to_run);
        string line;

        while (getline(file, line)) {

            if (line.starts_with("Option=")) {

                string chopped_line = line.substr(12, line.length());

                string name = chopped_line.substr(0, chopped_line.find(';'));

                names.push_back(name);

                apps[names.back()] = chopped_line.substr(name.length() + 6, chopped_line.length());

            }

        }

        file.close();

    } else {

        make_applications_list("/usr/share/applications", "Applications");
        make_applications_list(fs::path(home) / ".local" / "share" / "applications", "Applications");
        make_applications_list(fs::path(home) / ".faal" / "scripts", "Built-in scripts");

        logger(INFO, "Registered " + std::to_string(names.size()) + " applications.");

    }

    gtk_init(0, nullptr);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, (confPath / "config.css").c_str(), NULL);
    GdkScreen *screen = gtk_widget_get_screen(window);
    gtk_style_context_add_provider_for_screen(
    screen,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

    gtk_widget_set_opacity(window, 0.99);
    gtk_widget_set_size_request(window, 500, 10);
    gtk_window_set_accept_focus(GTK_WINDOW(window), TRUE);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
    GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, 500, 700);

    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scrolled_window), vbox);

    gtk_widget_set_hexpand(vbox, TRUE);
    gtk_widget_set_vexpand(vbox, FALSE);

    GtkWidget *searchBar = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), searchBar, FALSE, FALSE, 5);

    EntryData data_struct;
    data_struct.vbox = vbox;
    data_struct.names = &names;

    g_signal_connect(searchBar, "changed", G_CALLBACK(on_entry_changed), &data_struct);

    make_new_app_list();
    reload(vbox);

    g_signal_connect(window, "key-press-event", G_CALLBACK(+[](GtkWidget *widget, GdkEventKey *event, gpointer data) -> gboolean {

        GtkWidget *vbox = GTK_WIDGET(data);

        if (event->keyval == GDK_KEY_Up) {
            if (cursor > 0) {
                cursor--;
                make_new_app_list();
                reload(vbox);
            }
            return TRUE;
        }
        if (event->keyval == GDK_KEY_Down) {

            if (cursor < currentAppNames.size() - 1) {

                cursor++;
                make_new_app_list();
                reload(vbox);

            }
            return TRUE;
        }
        return FALSE;

    }), vbox);

    g_signal_connect(window, "key-press-event", G_CALLBACK(+[](GtkWidget *widget, GdkEventKey *event, gpointer data) -> gboolean {
        if (event->keyval == GDK_KEY_Escape) {
            gtk_main_quit();
            logger(INFO, "Program closed by user.");
            return TRUE;
        }
        return FALSE;
    }), NULL);

    g_signal_connect(window, "key-press-event", G_CALLBACK(+[](GtkWidget *widget, GdkEventKey *event, gpointer data) -> gboolean {

        if (event->keyval == GDK_KEY_Return) {
            gtk_main_quit();
            logger(INFO, "Program closed by user.");

            open_app_on_cursor();

            return TRUE;
        }
        return FALSE;
    }), NULL);

    gtk_widget_show_all(window);
    gtk_widget_show_all(scrolled_window);
    gtk_widget_grab_focus(searchBar);
    gtk_main();

    return 0;

}
