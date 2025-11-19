#ifndef RUBOLT_PACKAGE_MANAGER_H
#define RUBOLT_PACKAGE_MANAGER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char* name;
    char* version;
    char* description;
    char* author;
    char* license;
    char* repository;
    char** dependencies;
    size_t dependency_count;
    char** dev_dependencies;
    size_t dev_dependency_count;
} PackageManifest;

typedef struct {
    char* name;
    char* version;
    char* resolved_version;
    char* path;
    bool is_dev_dependency;
} PackageDependency;

typedef struct {
    char* name;
    char* version;
    char* download_url;
    char* checksum;
    size_t size;
} PackageInfo;

typedef struct {
    PackageInfo* packages;
    size_t package_count;
    char* registry_url;
    char* cache_dir;
} PackageRegistry;

typedef struct {
    PackageDependency* dependencies;
    size_t dependency_count;
    char* lock_file_path;
} PackageLock;

typedef struct {
    PackageRegistry registry;
    PackageLock lock;
    char* project_root;
    char* packages_dir;
} PackageManager;

// Package manifest operations
PackageManifest* package_manifest_load(const char* path);
void package_manifest_save(PackageManifest* manifest, const char* path);
void package_manifest_free(PackageManifest* manifest);
PackageManifest* package_manifest_create(const char* name, const char* version);

// Package registry operations
void package_registry_init(PackageRegistry* registry, const char* registry_url);
void package_registry_free(PackageRegistry* registry);
PackageInfo* package_registry_search(PackageRegistry* registry, const char* name);
PackageInfo* package_registry_get_info(PackageRegistry* registry, const char* name, const char* version);
bool package_registry_download(PackageRegistry* registry, PackageInfo* package, const char* dest_path);

// Package lock operations
PackageLock* package_lock_load(const char* path);
void package_lock_save(PackageLock* lock, const char* path);
void package_lock_free(PackageLock* lock);

// Package manager operations
void package_manager_init(PackageManager* pm, const char* project_root);
void package_manager_free(PackageManager* pm);
bool package_manager_install(PackageManager* pm, const char* package_spec);
bool package_manager_uninstall(PackageManager* pm, const char* package_name);
bool package_manager_update(PackageManager* pm, const char* package_name);
bool package_manager_install_all(PackageManager* pm);

// Dependency resolution
typedef struct DependencyNode {
    char* name;
    char* version;
    struct DependencyNode** dependencies;
    size_t dependency_count;
    struct DependencyNode* next;
} DependencyNode;

DependencyNode* resolve_dependencies(PackageManager* pm, PackageManifest* manifest);
void dependency_node_free(DependencyNode* node);
bool check_dependency_conflicts(DependencyNode* root);

// Version operations
typedef struct {
    int major;
    int minor;
    int patch;
    char* prerelease;
} SemanticVersion;

SemanticVersion* version_parse(const char* version_str);
void version_free(SemanticVersion* version);
int version_compare(SemanticVersion* a, SemanticVersion* b);
bool version_satisfies(SemanticVersion* version, const char* constraint);

// Package commands
bool package_init(const char* project_name, const char* project_dir);
bool package_add(PackageManager* pm, const char* package_spec);
bool package_remove(PackageManager* pm, const char* package_name);
bool package_list(PackageManager* pm);
bool package_info(PackageManager* pm, const char* package_name);
bool package_publish(PackageManager* pm, const char* registry_url);

#endif