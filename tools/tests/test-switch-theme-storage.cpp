// Regression checks for a first-run/legacy Switch UI theme library.
// Compile with: c++ -std=c++20 -Iheadless tools/tests/test-switch-theme-storage.cpp -o /tmp/theme-storage-test
#include "prosperoeden/pe/ui/switch_bootstrap.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
namespace library = pe::ui::switch_bootstrap;

void check(bool result, const char *why) {
    if (!result) throw std::runtime_error(why);
}

int main() {
    const fs::path scratch = fs::temp_directory_path() /
        ("pe-switch-theme-check-" + std::to_string(::getpid()));
    fs::create_directories(scratch / "data");
    try {
        const fs::path root = scratch / "data" / "themes";
        check(library::ensure(root.string()), "clean theme root must initialize");
        check(library::directory_without_links(root), "root must be a real folder");
        check(library::directory_without_links(root / "installed"), "installed/ must be created");
        check(!fs::exists(root / "active"), "legacy active/ is unnecessary for new installs");
        check(library::regular_without_links(root / "current.txt"), "current.txt must be created");
        check(library::selected(root.string()) == "switch-dark", "clean theme selection must be Switch Dark");
        check(library::ensure(root.string()), "initializer must be idempotent");

        // Existing configuration and legacy imports must be preserved.
        fs::create_directory(root / "active");
        { std::ofstream(root / "current.txt", std::ios::trunc) << "T123456\n"; }
        check(library::ensure(root.string()), "legacy/existing theme root must still initialize");
        check(fs::exists(root / "active"), "legacy active/ must be preserved");
        check(library::selected(root.string()) == "T123456", "existing selection must not be overwritten");

        // Reject links to avoid saving packs outside the theme library.
        const fs::path redirected = scratch / "redirected";
        fs::create_directory_symlink(root, redirected);
        check(!library::directory_without_links(redirected), "symbolic-link root must be rejected");
        check(!library::ensure(redirected.string()), "initializer must reject symbolic-link root");

        const fs::path linked = scratch / "linked";
        fs::create_directory(linked);
        fs::create_directory_symlink(root / "installed", linked / "installed");
        check(!library::ensure(linked.string()), "initializer must reject symbolic-link installed folder");

        const fs::path current_link = scratch / "current-link";
        fs::create_symlink(root / "current.txt", current_link);
        check(!library::regular_without_links(current_link), "selection file symlink must be rejected");
        fs::remove_all(scratch);
        std::cout << "theme storage: fresh install, idempotency, migration and symlink guards passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "theme storage: " << e.what() << "\n";
        fs::remove_all(scratch);
        return 1;
    }
}
