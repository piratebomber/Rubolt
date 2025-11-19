#include "package_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <json-c/json.h>

PackageManifest* package_manifest_create(const char* name, const char* version) {
    PackageManifest* manifest = malloc(sizeof(PackageManifest));
    manifest->name = strdup(name);
    manifest->version = strdup(version);
    manifest->description = strdup("");
    manifest->author = strdup("");
    manifest->license = strdup("MIT");
    manifest->repository = strdup("");
    manifest->dependencies = NULL;
    manifest->dependency_count = 0;
    manifest->dev_dependencies = NULL;
    manifest->dev_dependency_count = 0;
    return manifest;
}

PackageManifest* package_manifest_load(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    
    json_object* root = json_tokener_parse(content);
    free(content);
    
    if (!root) return NULL;
    
    PackageManifest* manifest = malloc(sizeof(PackageManifest));
    
    // Parse basic fields
    json_object* name_obj, *version_obj, *desc_obj, *author_obj, *license_obj, *repo_obj;
    
    if (json_object_object_get_ex(root, "name", &name_obj)) {
        manifest->name = strdup(json_object_get_string(name_obj));
    } else {
        manifest->name = strdup("");
    }
    
    if (json_object_object_get_ex(root, "version", &version_obj)) {
        manifest->version = strdup(json_object_get_string(version_obj));
    } else {
        manifest->version = strdup("1.0.0");
    }
    
    if (json_object_object_get_ex(root, "description", &desc_obj)) {
        manifest->description = strdup(json_object_get_string(desc_obj));
    } else {
        manifest->description = strdup("");
    }
    
    if (json_object_object_get_ex(root, "author", &author_obj)) {
        manifest->author = strdup(json_object_get_string(author_obj));
    } else {
        manifest->author = strdup("");
    }
    
    if (json_object_object_get_ex(root, "license", &license_obj)) {
        manifest->license = strdup(json_object_get_string(license_obj));
    } else {
        manifest->license = strdup("MIT");
    }
    
    if (json_object_object_get_ex(root, "repository", &repo_obj)) {
        manifest->repository = strdup(json_object_get_string(repo_obj));
    } else {
        manifest->repository = strdup("");
    }
    
    // Parse dependencies
    json_object* deps_obj;
    if (json_object_object_get_ex(root, "dependencies", &deps_obj)) {
        json_object_object_foreach(deps_obj, key, val) {
            manifest->dependency_count++;
        }
        
        if (manifest->dependency_count > 0) {
            manifest->dependencies = malloc(manifest->dependency_count * sizeof(char*));
            size_t i = 0;
            
            json_object_object_foreach(deps_obj, key, val) {
                char* dep_spec = malloc(strlen(key) + strlen(json_object_get_string(val)) + 2);
                sprintf(dep_spec, "%s@%s", key, json_object_get_string(val));
                manifest->dependencies[i++] = dep_spec;
            }
        }
    } else {
        manifest->dependencies = NULL;
        manifest->dependency_count = 0;
    }
    
    // Parse dev dependencies
    json_object* dev_deps_obj;
    if (json_object_object_get_ex(root, "devDependencies", &dev_deps_obj)) {
        json_object_object_foreach(dev_deps_obj, key, val) {
            manifest->dev_dependency_count++;
        }
        
        if (manifest->dev_dependency_count > 0) {
            manifest->dev_dependencies = malloc(manifest->dev_dependency_count * sizeof(char*));
            size_t i = 0;
            
            json_object_object_foreach(dev_deps_obj, key, val) {
                char* dep_spec = malloc(strlen(key) + strlen(json_object_get_string(val)) + 2);
                sprintf(dep_spec, "%s@%s", key, json_object_get_string(val));
                manifest->dev_dependencies[i++] = dep_spec;
            }
        }
    } else {
        manifest->dev_dependencies = NULL;
        manifest->dev_dependency_count = 0;
    }
    
    json_object_put(root);
    return manifest;
}

void package_manifest_save(PackageManifest* manifest, const char* path) {
    json_object* root = json_object_new_object();
    
    json_object_object_add(root, "name", json_object_new_string(manifest->name));
    json_object_object_add(root, "version", json_object_new_string(manifest->version));
    json_object_object_add(root, "description", json_object_new_string(manifest->description));
    json_object_object_add(root, "author", json_object_new_string(manifest->author));
    json_object_object_add(root, "license", json_object_new_string(manifest->license));
    json_object_object_add(root, "repository", json_object_new_string(manifest->repository));
    
    // Add dependencies
    if (manifest->dependency_count > 0) {
        json_object* deps = json_object_new_object();
        for (size_t i = 0; i < manifest->dependency_count; i++) {
            char* dep = strdup(manifest->dependencies[i]);
            char* at_pos = strchr(dep, '@');
            if (at_pos) {
                *at_pos = '\0';
                json_object_object_add(deps, dep, json_object_new_string(at_pos + 1));
            }
            free(dep);
        }
        json_object_object_add(root, "dependencies", deps);
    }
    
    // Add dev dependencies
    if (manifest->dev_dependency_count > 0) {
        json_object* dev_deps = json_object_new_object();
        for (size_t i = 0; i < manifest->dev_dependency_count; i++) {
            char* dep = strdup(manifest->dev_dependencies[i]);
            char* at_pos = strchr(dep, '@');
            if (at_pos) {
                *at_pos = '\0';
                json_object_object_add(dev_deps, dep, json_object_new_string(at_pos + 1));
            }
            free(dep);
        }
        json_object_object_add(root, "devDependencies", dev_deps);
    }
    
    const char* json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    
    FILE* file = fopen(path, "w");
    if (file) {
        fputs(json_str, file);
        fclose(file);
    }
    
    json_object_put(root);
}

void package_manifest_free(PackageManifest* manifest) {
    if (!manifest) return;
    
    free(manifest->name);
    free(manifest->version);
    free(manifest->description);
    free(manifest->author);
    free(manifest->license);
    free(manifest->repository);
    
    for (size_t i = 0; i < manifest->dependency_count; i++) {
        free(manifest->dependencies[i]);
    }
    free(manifest->dependencies);
    
    for (size_t i = 0; i < manifest->dev_dependency_count; i++) {
        free(manifest->dev_dependencies[i]);
    }
    free(manifest->dev_dependencies);
    
    free(manifest);
}

void package_registry_init(PackageRegistry* registry, const char* registry_url) {
    registry->packages = NULL;
    registry->package_count = 0;
    registry->registry_url = strdup(registry_url);
    registry->cache_dir = strdup(".rubolt/cache");
    
    // Create cache directory
    mkdir(registry->cache_dir, 0755);
}

void package_registry_free(PackageRegistry* registry) {
    for (size_t i = 0; i < registry->package_count; i++) {
        free(registry->packages[i].name);
        free(registry->packages[i].version);
        free(registry->packages[i].download_url);
        free(registry->packages[i].checksum);
    }
    free(registry->packages);
    free(registry->registry_url);
    free(registry->cache_dir);
}

typedef struct {
    char* data;
    size_t size;
} HttpResponse;

static size_t write_callback(void* contents, size_t size, size_t nmemb, HttpResponse* response) {
    size_t total_size = size * nmemb;
    char* ptr = realloc(response->data, response->size + total_size + 1);
    
    if (!ptr) return 0;
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

PackageInfo* package_registry_search(PackageRegistry* registry, const char* name) {
    CURL* curl = curl_easy_init();
    if (!curl) return NULL;
    
    char url[512];
    snprintf(url, sizeof(url), "%s/packages/%s", registry->registry_url, name);
    
    HttpResponse response = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK || !response.data) {
        free(response.data);
        return NULL;
    }
    
    json_object* root = json_tokener_parse(response.data);
    free(response.data);
    
    if (!root) return NULL;
    
    PackageInfo* package = malloc(sizeof(PackageInfo));
    
    json_object* name_obj, *version_obj, *url_obj, *checksum_obj, *size_obj;
    
    if (json_object_object_get_ex(root, "name", &name_obj)) {
        package->name = strdup(json_object_get_string(name_obj));
    }
    
    if (json_object_object_get_ex(root, "version", &version_obj)) {
        package->version = strdup(json_object_get_string(version_obj));
    }
    
    if (json_object_object_get_ex(root, "downloadUrl", &url_obj)) {
        package->download_url = strdup(json_object_get_string(url_obj));
    }
    
    if (json_object_object_get_ex(root, "checksum", &checksum_obj)) {
        package->checksum = strdup(json_object_get_string(checksum_obj));
    }
    
    if (json_object_object_get_ex(root, "size", &size_obj)) {
        package->size = json_object_get_int64(size_obj);
    }
    
    json_object_put(root);
    return package;
}

void package_manager_init(PackageManager* pm, const char* project_root) {
    pm->project_root = strdup(project_root);
    pm->packages_dir = malloc(strlen(project_root) + 20);
    sprintf(pm->packages_dir, "%s/.rubolt/packages", project_root);
    
    package_registry_init(&pm->registry, "https://packages.rubolt.dev");
    
    // Create packages directory
    mkdir(pm->packages_dir, 0755);
    
    // Load lock file
    char lock_path[512];
    snprintf(lock_path, sizeof(lock_path), "%s/rubolt.lock", project_root);
    PackageLock* lock = package_lock_load(lock_path);
    if (lock) {
        pm->lock = *lock;
        free(lock);
    } else {
        pm->lock.dependencies = NULL;
        pm->lock.dependency_count = 0;
        pm->lock.lock_file_path = strdup(lock_path);
    }
}

void package_manager_free(PackageManager* pm) {
    free(pm->project_root);
    free(pm->packages_dir);
    package_registry_free(&pm->registry);
    package_lock_free(&pm->lock);
}

bool package_manager_install(PackageManager* pm, const char* package_spec) {
    // Parse package spec (name@version)
    char* name = strdup(package_spec);
    char* version = "latest";
    
    char* at_pos = strchr(name, '@');
    if (at_pos) {
        *at_pos = '\0';
        version = at_pos + 1;
    }
    
    // Search for package
    PackageInfo* package = package_registry_search(&pm->registry, name);
    if (!package) {
        printf("Package '%s' not found\n", name);
        free(name);
        return false;
    }
    
    // Download package
    char dest_path[512];
    snprintf(dest_path, sizeof(dest_path), "%s/%s-%s.tar.gz", 
             pm->packages_dir, package->name, package->version);
    
    bool success = package_registry_download(&pm->registry, package, dest_path);
    
    if (success) {
        printf("Installed %s@%s\n", package->name, package->version);
        
        // Add to lock file
        pm->lock.dependencies = realloc(pm->lock.dependencies,
                                      (pm->lock.dependency_count + 1) * sizeof(PackageDependency));
        
        PackageDependency* dep = &pm->lock.dependencies[pm->lock.dependency_count];
        dep->name = strdup(package->name);
        dep->version = strdup(version);
        dep->resolved_version = strdup(package->version);
        dep->path = strdup(dest_path);
        dep->is_dev_dependency = false;
        
        pm->lock.dependency_count++;
        
        // Save lock file
        package_lock_save(&pm->lock, pm->lock.lock_file_path);
    }
    
    free(name);
    free(package->name);
    free(package->version);
    free(package->download_url);
    free(package->checksum);
    free(package);
    
    return success;
}

bool package_registry_download(PackageRegistry* registry, PackageInfo* package, const char* dest_path) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    FILE* file = fopen(dest_path, "wb");
    if (!file) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, package->download_url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    
    fclose(file);
    curl_easy_cleanup(curl);
    
    return res == CURLE_OK;
}

SemanticVersion* version_parse(const char* version_str) {
    SemanticVersion* version = malloc(sizeof(SemanticVersion));
    version->major = 0;
    version->minor = 0;
    version->patch = 0;
    version->prerelease = NULL;
    
    char* str = strdup(version_str);
    char* token = strtok(str, ".");
    
    if (token) {
        version->major = atoi(token);
        token = strtok(NULL, ".");
        
        if (token) {
            version->minor = atoi(token);
            token = strtok(NULL, ".-");
            
            if (token) {
                version->patch = atoi(token);
                token = strtok(NULL, "");
                
                if (token) {
                    version->prerelease = strdup(token);
                }
            }
        }
    }
    
    free(str);
    return version;
}

int version_compare(SemanticVersion* a, SemanticVersion* b) {
    if (a->major != b->major) return a->major - b->major;
    if (a->minor != b->minor) return a->minor - b->minor;
    if (a->patch != b->patch) return a->patch - b->patch;
    
    // Handle prerelease comparison
    if (!a->prerelease && !b->prerelease) return 0;
    if (!a->prerelease && b->prerelease) return 1;
    if (a->prerelease && !b->prerelease) return -1;
    
    return strcmp(a->prerelease, b->prerelease);
}

bool version_satisfies(SemanticVersion* version, const char* constraint) {
    // Simple constraint checking - would be expanded for full semver
    if (strncmp(constraint, "^", 1) == 0) {
        // Caret range - compatible within major version
        SemanticVersion* constraint_version = version_parse(constraint + 1);
        bool satisfies = (version->major == constraint_version->major &&
                         version_compare(version, constraint_version) >= 0);
        version_free(constraint_version);
        return satisfies;
    }
    
    if (strncmp(constraint, "~", 1) == 0) {
        // Tilde range - compatible within minor version
        SemanticVersion* constraint_version = version_parse(constraint + 1);
        bool satisfies = (version->major == constraint_version->major &&
                         version->minor == constraint_version->minor &&
                         version_compare(version, constraint_version) >= 0);
        version_free(constraint_version);
        return satisfies;
    }
    
    // Exact match
    SemanticVersion* constraint_version = version_parse(constraint);
    bool satisfies = (version_compare(version, constraint_version) == 0);
    version_free(constraint_version);
    return satisfies;
}

bool package_init(const char* project_name, const char* project_dir) {
    // Create project directory
    mkdir(project_dir, 0755);
    
    // Create package.json
    char manifest_path[512];
    snprintf(manifest_path, sizeof(manifest_path), "%s/package.json", project_dir);
    
    PackageManifest* manifest = package_manifest_create(project_name, "1.0.0");
    manifest->description = strdup("A Rubolt project");
    
    package_manifest_save(manifest, manifest_path);
    package_manifest_free(manifest);
    
    // Create basic project structure
    char src_dir[512];
    snprintf(src_dir, sizeof(src_dir), "%s/src", project_dir);
    mkdir(src_dir, 0755);
    
    char main_file[512];
    snprintf(main_file, sizeof(main_file), "%s/src/main.rbo", project_dir);
    
    FILE* file = fopen(main_file, "w");
    if (file) {
        fprintf(file, "// %s - A Rubolt project\n\n", project_name);
        fprintf(file, "def main() -> void {\n");
        fprintf(file, "    print(\"Hello from %s!\");\n", project_name);
        fprintf(file, "}\n\n");
        fprintf(file, "main();\n");
        fclose(file);
    }
    
    printf("Initialized Rubolt project '%s' in %s\n", project_name, project_dir);
    return true;
}