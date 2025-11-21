#include "package_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>

// Global package manager state
static PackageManager *global_pm = NULL;

// HTTP response structure
typedef struct {
    char *data;
    size_t size;
} HTTPResponse;

// Callback for curl to write response data
static size_t write_callback(void *contents, size_t size, size_t nmemb, HTTPResponse *response) {
    size_t total_size = size * nmemb;
    char *new_data = realloc(response->data, response->size + total_size + 1);
    
    if (!new_data) {
        return 0;
    }
    
    response->data = new_data;
    memcpy(&(response->data[response->size]), contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

PackageManager *package_manager_create(const char *registry_url, const char *cache_dir) {
    PackageManager *pm = malloc(sizeof(PackageManager));
    pm->registry_url = strdup(registry_url);
    pm->cache_dir = strdup(cache_dir);
    pm->auth_token = NULL;
    pm->installed_packages = NULL;
    pm->package_count = 0;
    pm->package_capacity = 0;
    
    // Create cache directory
    mkdir(cache_dir, 0755);
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    global_pm = pm;
    return pm;
}

void package_manager_destroy(PackageManager *pm) {
    if (!pm) return;
    
    free(pm->registry_url);
    free(pm->cache_dir);
    free(pm->auth_token);
    
    for (size_t i = 0; i < pm->package_count; i++) {
        package_info_cleanup(&pm->installed_packages[i]);
    }
    free(pm->installed_packages);
    
    curl_global_cleanup();
    free(pm);
}

bool package_manager_login(PackageManager *pm, const char *username, const char *password) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;
    
    HTTPResponse response = {0};
    
    // Prepare login data
    json_object *login_data = json_object_new_object();
    json_object *username_obj = json_object_new_string(username);
    json_object *password_obj = json_object_new_string(password);
    json_object_object_add(login_data, "username", username_obj);
    json_object_object_add(login_data, "password", password_obj);
    
    const char *json_string = json_object_to_json_string(login_data);
    
    char url[512];
    snprintf(url, sizeof(url), "%s/api/users/login", pm->registry_url);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_object_put(login_data);
    
    bool success = false;
    if (res == CURLE_OK && response_code == 200) {
        json_object *root = json_tokener_parse(response.data);
        json_object *token_obj;
        
        if (json_object_object_get_ex(root, "token", &token_obj)) {
            const char *token = json_object_get_string(token_obj);
            pm->auth_token = strdup(token);
            success = true;
        }
        
        json_object_put(root);
    }
    
    free(response.data);
    return success;
}

PackageInfo *package_manager_search(PackageManager *pm, const char *query, size_t *result_count) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    
    HTTPResponse response = {0};
    
    char url[512];
    char *encoded_query = curl_easy_escape(curl, query, 0);
    snprintf(url, sizeof(url), "%s/api/search?q=%s", pm->registry_url, encoded_query);
    curl_free(encoded_query);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    PackageInfo *packages = NULL;
    *result_count = 0;
    
    if (res == CURLE_OK && response_code == 200) {
        json_object *root = json_tokener_parse(response.data);
        json_object *results_obj;
        
        if (json_object_object_get_ex(root, "results", &results_obj)) {
            int array_len = json_object_array_length(results_obj);
            packages = malloc(sizeof(PackageInfo) * array_len);
            
            for (int i = 0; i < array_len; i++) {
                json_object *pkg_obj = json_object_array_get_idx(results_obj, i);
                parse_package_info(&packages[*result_count], pkg_obj);
                (*result_count)++;
            }
        }
        
        json_object_put(root);
    }
    
    free(response.data);
    return packages;
}

bool package_manager_install(PackageManager *pm, const char *name, const char *version) {
    // Check if already installed
    if (package_manager_is_installed(pm, name, version)) {
        printf("Package %s@%s is already installed\n", name, version);
        return true;
    }
    
    // Get package info
    PackageInfo pkg_info;
    if (!get_package_info(pm, name, version, &pkg_info)) {
        printf("Failed to get package info for %s@%s\n", name, version);
        return false;
    }
    
    // Resolve dependencies
    DependencyGraph *dep_graph = resolve_dependencies(pm, &pkg_info);
    if (!dep_graph) {
        printf("Failed to resolve dependencies for %s@%s\n", name, version);
        return false;
    }
    
    // Install dependencies in order
    for (size_t i = 0; i < dep_graph->node_count; i++) {
        DependencyNode *node = &dep_graph->nodes[i];
        
        if (!package_manager_is_installed(pm, node->name, node->version)) {
            if (!download_and_install_package(pm, node->name, node->version)) {
                printf("Failed to install dependency %s@%s\n", node->name, node->version);
                dependency_graph_cleanup(dep_graph);
                return false;
            }
        }
    }
    
    // Install the main package
    bool success = download_and_install_package(pm, name, version);
    
    dependency_graph_cleanup(dep_graph);
    return success;
}

bool package_manager_uninstall(PackageManager *pm, const char *name) {
    // Find installed package
    int pkg_index = -1;
    for (size_t i = 0; i < pm->package_count; i++) {
        if (strcmp(pm->installed_packages[i].name, name) == 0) {
            pkg_index = i;
            break;
        }
    }
    
    if (pkg_index == -1) {
        printf("Package %s is not installed\n", name);
        return false;
    }
    
    PackageInfo *pkg = &pm->installed_packages[pkg_index];
    
    // Remove package files
    char package_dir[512];
    snprintf(package_dir, sizeof(package_dir), "%s/packages/%s", pm->cache_dir, name);
    
    if (!remove_directory_recursive(package_dir)) {
        printf("Failed to remove package directory: %s\n", package_dir);
        return false;
    }
    
    // Remove from installed packages list
    package_info_cleanup(pkg);
    
    // Shift remaining packages
    for (size_t i = pkg_index; i < pm->package_count - 1; i++) {
        pm->installed_packages[i] = pm->installed_packages[i + 1];
    }
    pm->package_count--;
    
    printf("Successfully uninstalled %s\n", name);
    return true;
}

bool package_manager_update(PackageManager *pm, const char *name) {
    // Get current installed version
    PackageInfo *installed_pkg = NULL;
    for (size_t i = 0; i < pm->package_count; i++) {
        if (strcmp(pm->installed_packages[i].name, name) == 0) {
            installed_pkg = &pm->installed_packages[i];
            break;
        }
    }
    
    if (!installed_pkg) {
        printf("Package %s is not installed\n", name);
        return false;
    }
    
    // Get latest version info
    PackageInfo latest_info;
    if (!get_package_info(pm, name, NULL, &latest_info)) {
        printf("Failed to get latest version info for %s\n", name);
        return false;
    }
    
    // Compare versions
    int version_cmp = compare_versions(installed_pkg->version, latest_info.version);
    if (version_cmp >= 0) {
        printf("Package %s is already up to date (%s)\n", name, installed_pkg->version);
        package_info_cleanup(&latest_info);
        return true;
    }
    
    printf("Updating %s from %s to %s\n", name, installed_pkg->version, latest_info.version);
    
    // Uninstall old version
    if (!package_manager_uninstall(pm, name)) {
        package_info_cleanup(&latest_info);
        return false;
    }
    
    // Install new version
    bool success = package_manager_install(pm, name, latest_info.version);
    package_info_cleanup(&latest_info);
    
    return success;
}

bool package_manager_is_installed(PackageManager *pm, const char *name, const char *version) {
    for (size_t i = 0; i < pm->package_count; i++) {
        if (strcmp(pm->installed_packages[i].name, name) == 0) {
            if (!version || strcmp(pm->installed_packages[i].version, version) == 0) {
                return true;
            }
        }
    }
    return false;
}

void package_manager_list_installed(PackageManager *pm) {
    printf("Installed packages:\n");
    
    if (pm->package_count == 0) {
        printf("  No packages installed\n");
        return;
    }
    
    for (size_t i = 0; i < pm->package_count; i++) {
        PackageInfo *pkg = &pm->installed_packages[i];
        printf("  %s@%s - %s\n", pkg->name, pkg->version, pkg->description);
    }
}

// Helper functions
bool get_package_info(PackageManager *pm, const char *name, const char *version, PackageInfo *info) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;
    
    HTTPResponse response = {0};
    
    char url[512];
    if (version) {
        snprintf(url, sizeof(url), "%s/api/packages/%s?version=%s", pm->registry_url, name, version);
    } else {
        snprintf(url, sizeof(url), "%s/api/packages/%s", pm->registry_url, name);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    bool success = false;
    if (res == CURLE_OK && response_code == 200) {
        json_object *root = json_tokener_parse(response.data);
        success = parse_package_info(info, root);
        json_object_put(root);
    }
    
    free(response.data);
    return success;
}

bool parse_package_info(PackageInfo *info, json_object *json_obj) {
    json_object *name_obj, *version_obj, *desc_obj, *deps_obj;
    
    if (!json_object_object_get_ex(json_obj, "name", &name_obj) ||
        !json_object_object_get_ex(json_obj, "version", &version_obj)) {
        return false;
    }
    
    info->name = strdup(json_object_get_string(name_obj));
    info->version = strdup(json_object_get_string(version_obj));
    
    if (json_object_object_get_ex(json_obj, "description", &desc_obj)) {
        info->description = strdup(json_object_get_string(desc_obj));
    } else {
        info->description = strdup("");
    }
    
    // Parse dependencies
    info->dependencies = NULL;
    info->dependency_count = 0;
    
    if (json_object_object_get_ex(json_obj, "dependencies", &deps_obj)) {
        json_object_object_foreach(deps_obj, dep_name, dep_version_obj) {
            info->dependency_count++;
            info->dependencies = realloc(info->dependencies, 
                                       sizeof(Dependency) * info->dependency_count);
            
            Dependency *dep = &info->dependencies[info->dependency_count - 1];
            dep->name = strdup(dep_name);
            dep->version_spec = strdup(json_object_get_string(dep_version_obj));
        }
    }
    
    return true;
}

bool download_and_install_package(PackageManager *pm, const char *name, const char *version) {
    // Download package
    char download_path[512];
    snprintf(download_path, sizeof(download_path), "%s/%s-%s.tgz", pm->cache_dir, name, version);
    
    if (!download_package(pm, name, version, download_path)) {
        printf("Failed to download package %s@%s\n", name, version);
        return false;
    }
    
    // Extract package
    char extract_dir[512];
    snprintf(extract_dir, sizeof(extract_dir), "%s/packages/%s", pm->cache_dir, name);
    
    if (!extract_tarball(download_path, extract_dir)) {
        printf("Failed to extract package %s@%s\n", name, version);
        return false;
    }
    
    // Add to installed packages
    if (pm->package_count >= pm->package_capacity) {
        pm->package_capacity = pm->package_capacity ? pm->package_capacity * 2 : 16;
        pm->installed_packages = realloc(pm->installed_packages, 
                                       sizeof(PackageInfo) * pm->package_capacity);
    }
    
    PackageInfo *pkg = &pm->installed_packages[pm->package_count];
    if (!get_package_info(pm, name, version, pkg)) {
        printf("Failed to get package info after installation\n");
        return false;
    }
    
    pm->package_count++;
    
    printf("Successfully installed %s@%s\n", name, version);
    return true;
}

bool download_package(PackageManager *pm, const char *name, const char *version, const char *output_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;
    
    FILE *file = fopen(output_path, "wb");
    if (!file) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    char url[512];
    snprintf(url, sizeof(url), "%s/api/packages/%s/%s/download", pm->registry_url, name, version);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    fclose(file);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK || response_code != 200) {
        unlink(output_path);
        return false;
    }
    
    return true;
}

bool extract_tarball(const char *tarball_path, const char *extract_dir) {
    // Create extraction directory
    mkdir(extract_dir, 0755);
    
    char command[1024];
    snprintf(command, sizeof(command), "tar -xzf \"%s\" -C \"%s\"", tarball_path, extract_dir);
    
    int result = system(command);
    return result == 0;
}

DependencyGraph *resolve_dependencies(PackageManager *pm, PackageInfo *root_package) {
    DependencyGraph *graph = malloc(sizeof(DependencyGraph));
    graph->nodes = NULL;
    graph->node_count = 0;
    graph->node_capacity = 0;
    
    // Use BFS to resolve dependencies
    PackageInfo **queue = malloc(sizeof(PackageInfo *) * 1000);
    size_t queue_start = 0;
    size_t queue_end = 0;
    
    queue[queue_end++] = root_package;
    
    while (queue_start < queue_end) {
        PackageInfo *current = queue[queue_start++];
        
        // Add current package to graph
        add_dependency_node(graph, current->name, current->version);
        
        // Process dependencies
        for (size_t i = 0; i < current->dependency_count; i++) {
            Dependency *dep = &current->dependencies[i];
            
            // Check if already processed
            if (find_dependency_node(graph, dep->name)) {
                continue;
            }
            
            // Resolve version
            char *resolved_version = resolve_version_spec(pm, dep->name, dep->version_spec);
            if (!resolved_version) {
                printf("Failed to resolve dependency %s@%s\n", dep->name, dep->version_spec);
                continue;
            }
            
            // Get dependency info
            PackageInfo *dep_info = malloc(sizeof(PackageInfo));
            if (get_package_info(pm, dep->name, resolved_version, dep_info)) {
                queue[queue_end++] = dep_info;
            }
            
            free(resolved_version);
        }
    }
    
    free(queue);
    return graph;
}

void add_dependency_node(DependencyGraph *graph, const char *name, const char *version) {
    if (graph->node_count >= graph->node_capacity) {
        graph->node_capacity = graph->node_capacity ? graph->node_capacity * 2 : 16;
        graph->nodes = realloc(graph->nodes, sizeof(DependencyNode) * graph->node_capacity);
    }
    
    DependencyNode *node = &graph->nodes[graph->node_count];
    node->name = strdup(name);
    node->version = strdup(version);
    node->dependencies = NULL;
    node->dependency_count = 0;
    
    graph->node_count++;
}

DependencyNode *find_dependency_node(DependencyGraph *graph, const char *name) {
    for (size_t i = 0; i < graph->node_count; i++) {
        if (strcmp(graph->nodes[i].name, name) == 0) {
            return &graph->nodes[i];
        }
    }
    return NULL;
}

char *resolve_version_spec(PackageManager *pm, const char *name, const char *version_spec) {
    // Simple version resolution - in a real implementation, this would handle
    // semantic versioning ranges like "^1.0.0", "~2.1.0", etc.
    
    if (strcmp(version_spec, "latest") == 0 || version_spec[0] == '^' || version_spec[0] == '~') {
        // Get latest version
        PackageInfo info;
        if (get_package_info(pm, name, NULL, &info)) {
            char *version = strdup(info.version);
            package_info_cleanup(&info);
            return version;
        }
        return NULL;
    } else {
        // Exact version
        return strdup(version_spec);
    }
}

int compare_versions(const char *version1, const char *version2) {
    // Simple version comparison - in a real implementation, this would
    // properly handle semantic versioning
    
    int major1, minor1, patch1;
    int major2, minor2, patch2;
    
    sscanf(version1, "%d.%d.%d", &major1, &minor1, &patch1);
    sscanf(version2, "%d.%d.%d", &major2, &minor2, &patch2);
    
    if (major1 != major2) return major1 - major2;
    if (minor1 != minor2) return minor1 - minor2;
    return patch1 - patch2;
}

bool remove_directory_recursive(const char *path) {
    char command[512];
    snprintf(command, sizeof(command), "rm -rf \"%s\"", path);
    return system(command) == 0;
}

void package_info_cleanup(PackageInfo *info) {
    free(info->name);
    free(info->version);
    free(info->description);
    
    for (size_t i = 0; i < info->dependency_count; i++) {
        free(info->dependencies[i].name);
        free(info->dependencies[i].version_spec);
    }
    free(info->dependencies);
}

void dependency_graph_cleanup(DependencyGraph *graph) {
    for (size_t i = 0; i < graph->node_count; i++) {
        free(graph->nodes[i].name);
        free(graph->nodes[i].version);
        free(graph->nodes[i].dependencies);
    }
    free(graph->nodes);
    free(graph);
}