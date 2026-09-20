#pragma once

// Phase 3: auto-detects installed Java runtimes so the user does not have
// to hunt down and type a java.exe path by hand in AddServerDialog.
//
// Scope, deliberately kept simple: this scans JAVA_HOME, every directory
// on PATH, and the handful of well-known install roots used by the major
// JDK vendors (Oracle, Eclipse Temurin/Adoptium, Azul Zulu, Amazon
// Corretto, Microsoft Build of OpenJDK). It does NOT read the Windows
// registry (every vendor uses a different registry layout, and the
// filesystem scan already covers the overwhelming majority of real
// installs) and does NOT probe each candidate with `java -version` (that
// would mean spawning N processes just to populate a combo box) - so no
// version number is reported, only the discovered path.

#include <filesystem>
#include <vector>

namespace javamanager
{

struct JavaInstallation
{
    std::filesystem::path path; // full path to a java.exe
};

std::vector<JavaInstallation> DetectInstallations();

} // namespace javamanager
