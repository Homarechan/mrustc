#pragma once

#include <string>
#include <vector>
#include <memory>
#include "path.h"

class PackageManifest;
class Repository;

struct PackageVersion
{
    unsigned major;
    unsigned minor;
    unsigned patch;

    static PackageVersion from_string(const ::std::string& s);

    PackageVersion next_breaking() const {
        if(major == 0) {
            return PackageVersion { 0, minor + 1, 0 };
        }
        else {
            return PackageVersion { major + 1, 0, 0 };
        }
    }
    PackageVersion prev_compat() const {
        if(major == 0) {
            // Before 1.0, there's no patch levels
            return *this;
        }
        else {
            // Anything from the same patch series
            return PackageVersion { major, minor, 0 };
        }
    }

    bool operator==(const PackageVersion& x) const {
        if( major != x.major )  return false;
        if( minor != x.minor )  return false;
        return patch == x.patch;
    }
    bool operator!=(const PackageVersion& x) const {
        if( major != x.major )  return true;
        if( minor != x.minor )  return true;
        return patch != x.patch;
    }
    bool operator<(const PackageVersion& x) const {
        if( major != x.major )  return major < x.major;
        if( minor != x.minor )  return minor < x.minor;
        return patch < x.patch;
    }
    bool operator>(const PackageVersion& x) const {
        if( major != x.major )  return major > x.major;
        if( minor != x.minor )  return minor > x.minor;
        return patch > x.patch;
    }

    friend ::std::ostream& operator<<(::std::ostream& os, const PackageVersion& v) {
        os << v.major << "." << v.minor << "." << v.patch;
        return os;
    }
};
struct PackageVersionSpec
{
    struct Bound
    {
        enum class Type
        {
            Compatible,
            Greater,
            Equal,
            Less,
        };

        Type    ty;
        PackageVersion  ver;
    };
    // TODO: Just upper and lower?
    ::std::vector<Bound>   m_bounds;

    // Construct from a string
    static PackageVersionSpec from_string(const ::std::string& s);

    /// Check if this spec accepts the passed version
    bool accepts(const PackageVersion& v) const;

    friend ::std::ostream& operator<<(::std::ostream& os, const PackageVersionSpec& v) {
        bool first = true;
        for(const auto& b : v.m_bounds) {
            if(!first)
                os << ",";
            first = false;
            switch(b.ty)
            {
            case Bound::Type::Compatible: os << "^";  break;
            case Bound::Type::Greater:    os << ">";  break;
            case Bound::Type::Equal:      os << "=";  break;
            case Bound::Type::Less:       os << "<";  break;
            }
            os << b.ver;
        }
        return os;
    }
};

class PackageRef
{
    friend class PackageManifest;
    ::std::string   m_name;
    PackageVersionSpec  m_version;

    bool m_optional = false;
    ::std::string   m_path;
    ::std::shared_ptr<PackageManifest> m_manifest;

    PackageRef(const ::std::string& n) :
        m_name(n)
    {
    }

public:
    const ::std::string& name() const { return m_name; }
    //const ::std::string& get_repo_name() const  { return m_repo; }
    const PackageVersionSpec& get_version() const { return m_version; }

    bool is_optional() const { return m_optional; }

    const bool has_path() const { return m_path != ""; }
    const ::std::string& path() const { return m_path; }
    const bool has_git() const { return false; }

    const PackageManifest& get_package() const {
        if(!m_manifest) throw ::std::runtime_error("Manifest not loaded for package " + m_name);
        return *m_manifest;
    }

    void load_manifest(Repository& repo, const ::helpers::path& base_path);
};

struct PackageTarget
{
    enum class Type
    {
        Lib,
        Bin,
        Test,
        Bench,
        Example,
    };

    Type    m_type;
    ::std::string   m_name;
    ::std::string   m_path;
    bool    m_enable_test = true;
    bool    m_enable_doctest = true;
    bool    m_enable_bench = true;
    bool    m_enable_doc = true;
    bool    m_is_plugin = false;
    bool    m_is_proc_macro = false;
    bool    m_is_own_harness = false;

    PackageTarget(Type ty):
        m_type(ty)
    {
        switch(ty)
        {
        case Type::Lib:
            m_path = "src/lib.rs";
            break;
        case Type::Bin:
            m_path = "src/main.rs";
            break;
        default:
            break;
        }
    }
};

class BuildScriptOutput
{
public:
    // `cargo:minicargo-pre-build=make -C bar/`
    // MiniCargo hack
    ::std::vector<::std::string>    pre_build_commands;

    // cargo:rustc-link-search=foo/bar/baz
    ::std::vector<::std::pair<const char*,::std::string>>    rustc_link_search;
    // cargo:rustc-link-lib=mysql
    ::std::vector<::std::pair<const char*,::std::string>>    rustc_link_lib;
    // cargo:rustc-cfg=foo
    ::std::vector<::std::string>    rustc_cfg;
    // cargo:rustc-flags=-l foo
    ::std::vector<::std::string>    rustc_flags;
    // cargo:rustc-env=FOO=BAR
    ::std::vector<::std::string>    rustc_env;
};

class PackageManifest
{
    ::std::string   m_manifest_path;

    ::std::string   m_name;
    PackageVersion  m_version;

    ::std::string   m_build_script;

    ::std::vector<PackageRef>   m_dependencies;

    ::std::vector<PackageTarget>    m_targets;

    BuildScriptOutput   m_build_script_output;

    PackageManifest();

public:
    static PackageManifest load_from_toml(const ::std::string& path);

    const PackageTarget& get_library() const;


    const ::std::string& manifest_path() const {
        return m_manifest_path;
    }
    const ::std::string& name() const {
        return m_name;
    }
    const ::std::string& build_script() const {
        return m_build_script;
    }
    const BuildScriptOutput& build_script_output() const {
        return m_build_script_output;
    }
    const ::std::vector<PackageRef>& dependencies() const {
        return m_dependencies;
    }

    void load_dependencies(Repository& repo);

    void load_build_script(const ::std::string& path);
};
