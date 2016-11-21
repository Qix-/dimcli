// cli.h - dim cli
//
// Command line parser
//
// For documentation and examples:
// https://github.com/gknowles/dimcli

#pragma once

#include "config.h"

#include "util.h"

#include <cassert>
#include <experimental/filesystem>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Dim {

// forward declarations
class Cli;


/****************************************************************************
*
*   Exit codes
*
***/

// These mirror the program exit codes defined in <sysexits.h>
enum {
    kExitOk = 0,        // EX_OK
    kExitUsage = 64,    // EX_USAGE     - bad command line
    kExitSoftware = 70, // EX_SOFTWARE  - internal software error
};


/****************************************************************************
*
*   Cli
*
***/

class Cli {
public:
    struct Config;
    struct CommandConfig;
    struct GroupConfig;

    class OptBase;
    template <typename A, typename T> class OptShim;
    template <typename T> class Opt;
    template <typename T> class OptVec;
    struct OptIndex;

    struct ArgMatch;
    template <typename T> struct Value;
    template <typename T> struct ValueVec;

public:
    // Creates a handle to the shared command line configuration, this
    // indirection allows options to be statically registered from multiple
    // source files.
    Cli();

    //-----------------------------------------------------------------------
    // Configuration

    template <
        typename T,
        typename U,
        typename =
            typename std::enable_if<std::is_convertible<U, T>::value>::type>
    Opt<T> & opt(T * value, const std::string & keys, const U & def);

    template <typename T> Opt<T> & opt(T * value, const std::string & keys);

    template <typename T>
    OptVec<T> &
    optVec(std::vector<T> * values, const std::string & keys, int nargs = -1);

    template <typename T>
    Opt<T> & opt(const std::string & keys, const T & def = {});

    template <typename T>
    OptVec<T> & optVec(const std::string & keys, int nargs = -1);

    template <
        typename T,
        typename U,
        typename =
            typename std::enable_if<std::is_convertible<U, T>::value>::type>
    Opt<T> & opt(Opt<T> & value, const std::string & keys, const U & def);

    template <typename T>
    Opt<T> & opt(Opt<T> & value, const std::string & keys);

    template <typename T>
    OptVec<T> &
    optVec(OptVec<T> & values, const std::string & keys, int nargs = -1);

    // Add --version argument that shows "${progName.filename()} version ${ver}"
    // and exits. An empty progName defaults to argv[0].
    Opt<bool> &
    versionOpt(const std::string & ver, const std::string & progName = {});

    // Get reference to internal help option, can be used to change the
    // desciption, option group, etc.
    Opt<bool> & helpOpt();

    //-----------------------------------------------------------------------
    // An option group that shares a section in the help text. Options are
    // always added to a group, either the default group of the cli (or of the
    // selected command), or an explicitly created one.

    // Changes config context to point at the selected option group of the
    // current command, that you can then start stuffing args into.
    Cli & group(const std::string & name);

    // Heading title to display, defaults to group name. If empty there will
    // be a single blank line separating this group from the previous one.
    Cli & title(const std::string & val);

    // Option groups are sorted by key, defaults to group name
    Cli & sortKey(const std::string & key);

    const std::string & group() const { return m_group; }
    const std::string & title() const;
    const std::string & sortKey() const;

    //-----------------------------------------------------------------------
    // Changes config context to point at the default option group of the 
    // selected command.
    Cli & command(const std::string & name, const std::string & group = {});

    // Action that should be taken when the currently selected command is run.
    // Actions are executed when cli.run() is called by the application. The
    // action function should:
    //  - do something useful
    //  - return an exitCode.
    using ActionFn = int(Cli & cli, const std::string & cmd);
    Cli & action(std::function<ActionFn> fn);

    // Arbitrary text can be added to the generated help for each command,
    // this text can come before the usage (header), between the usage and
    // the arguments / options (desc), or after the options (footer). Use
    // line breaks for semantics, let the automatic line wrapping take care
    // of the rest.
    Cli & header(const std::string & val);
    Cli & desc(const std::string & val);
    Cli & footer(const std::string & val);

    const std::string & command() const { return m_command; }
    const std::string & header() const;
    const std::string & desc() const;
    const std::string & footer() const;

    //-----------------------------------------------------------------------
    // Enabled by default, reponse file expansion replaces arguments of the
    // form "@file" with the contents of the file.
    void responseFiles(bool enable = true);

    //-----------------------------------------------------------------------
    // Help

    // writeHelp & writeUsage return the current exitCode()
    int writeHelp(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}) const;
    int writeUsage(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}) const;

    void
    writePositionals(std::ostream & os, const std::string & cmd = {}) const;
    void writeOptions(std::ostream & os, const std::string & cmd = {}) const;
    void writeCommands(std::ostream & os) const;

    //-----------------------------------------------------------------------
    // Parsing

    bool parse(size_t argc, char * argv[]);
    bool parse(std::ostream & os, size_t argc, char * argv[]);

    // "args" is non-const so response files can be expanded in place
    bool parse(std::vector<std::string> & args);
    bool parse(std::ostream & os, std::vector<std::string> & args);

    // Parse cmdline into vector of args, using the default conventions
    // (Gnu or Windows) of the platform.
    static std::vector<std::string> toArgv(const std::string & cmdline);
    // Copy array of pointers into vector of args
    static std::vector<std::string> toArgv(size_t argc, char * argv[]);
    // Copy array of wchar_t pointers into vector of utf-8 encoded args
    static std::vector<std::string> toArgv(size_t argc, wchar_t * argv[]);
    // Create vector of pointers suitable for use with argc/argv APIs,
    // includes trailing null, so use "size() - 1" for argc. The return
    // values point into the source string vector and are only valid until
    // that vector is resized or destroyed.
    static std::vector<const char *>
    toPtrArgv(const std::vector<std::string> & args);

    // Parse according to glib conventions, based on the UNIX98 shell spec
    static std::vector<std::string> toGlibArgv(const std::string & cmdline);
    // Parse using GNU conventions, same rules as buildargv()
    static std::vector<std::string> toGnuArgv(const std::string & cmdline);
    // Parse using Windows rules
    static std::vector<std::string> toWindowsArgv(const std::string & cmdline);

    void resetValues();

    // Intended for use from return statements in action callbacks. Sets
    // exit code (to EX_USAGE) and error msg, then returns false.
    bool badUsage(const std::string & msg) { return fail(kExitUsage, msg); }
    // Calls badUsage(msg) with msg set to, depending on if its a subcommand,
    // one of the following:
    //  "<prefix>: <value>"
    //  "<prefix> for '<command>' command: <value>"
    bool badUsage(const std::string & prefix, const std::string & value);

    //-----------------------------------------------------------------------
    // After parsing

    int exitCode() const;
    const std::string & errMsg() const;

    // Program name received in argv[0]
    const std::string & progName() const;

    // Command selected by argv, empty string if there are no commands
    // defined or none were selected.
    const std::string & runCommand() const;

    // Runs the action of the selected command and returns its exit code;
    // which is also used to set cli.exitCode(). If no command was selected
    // it runs the action of the empty "" command, which can be set via
    // cli.action() just like any other command.
    int run();

protected:
    Cli(std::shared_ptr<Config> cfg);

private:
    bool defaultAction(OptBase & opt, const std::string & val);

    void addOpt(std::unique_ptr<OptBase> opt);

    template <typename A> A & addOpt(std::unique_ptr<A> ptr);

    template <typename Opt, typename Value, typename T>
    std::shared_ptr<Value> getProxy(T * ptr);

    // find an option (from any subcommand) that updates the value
    OptBase * findOpt(const void * value);

    CommandConfig & cmdCfg();
    const CommandConfig & cmdCfg() const;
    GroupConfig & grpCfg();
    const GroupConfig & grpCfg() const;

    void
    index(OptIndex & ndx, const std::string & cmd, bool requireVisible) const;
    std::string nameList(const OptBase & opt, const OptIndex & ndx) const;
    std::string nameList(
        const OptBase & opt,
        const OptIndex & ndx,
        bool disableOptions) const;

    bool parseAction(
        OptBase & out,
        const std::string & name,
        int pos,
        const char src[]);
    bool fail(int code, const std::string & msg);

    std::shared_ptr<Config> m_cfg;
    std::string m_group;
    std::string m_command;
};

//===========================================================================
template <typename T, typename U, typename>
inline Cli::Opt<T> &
Cli::opt(T * value, const std::string & keys, const U & def) {
    auto proxy = getProxy<Opt<T>, Value<T>>(value);
    auto ptr = std::make_unique<Opt<T>>(proxy, keys);
    ptr->defaultValue(def);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T>
inline Cli::Opt<T> & Cli::opt(T * value, const std::string & keys) {
    return opt(value, keys, T{});
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> &
Cli::optVec(std::vector<T> * values, const std::string & keys, int nargs) {
    auto proxy = getProxy<OptVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<OptVec<T>>(proxy, keys, nargs);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T, typename U, typename>
inline Cli::Opt<T> &
Cli::opt(Opt<T> & alias, const std::string & keys, const U & def) {
    return opt(&*alias, keys, def);
}

//===========================================================================
template <typename T>
inline Cli::Opt<T> & Cli::opt(Opt<T> & alias, const std::string & keys) {
    return opt(&*alias, keys, T{});
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> &
Cli::optVec(OptVec<T> & alias, const std::string & keys, int nargs) {
    return optVec(&*alias, keys, nargs);
}

//===========================================================================
template <typename T>
inline Cli::Opt<T> & Cli::opt(const std::string & keys, const T & def) {
    return opt<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> & Cli::optVec(const std::string & keys, int nargs) {
    return optVec<T>(nullptr, keys, nargs);
}

//===========================================================================
template <typename A> inline A & Cli::addOpt(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.action(&Cli::defaultAction).command(command()).group(group());
    addOpt(std::unique_ptr<OptBase>(ptr.release()));
    return opt;
}

//===========================================================================
template <typename A, typename V, typename T>
inline std::shared_ptr<V> Cli::getProxy(T * ptr) {
    if (OptBase * opt = findOpt(ptr)) {
        A * dopt = static_cast<A *>(opt);
        return dopt->m_proxy;
    }

    // Since there was no pre-existing proxy to raw value, create new proxy.
    return std::make_shared<V>(ptr);
}


/****************************************************************************
*
*   CliLocal
*
*   Stand-alone cli instance independent of the shared configuration. Mainly
*   for testing.
*
***/

class CliLocal : public Cli {
public:
    CliLocal();
};


/****************************************************************************
*
*   Cli::OptBase
*
*   Common base class for all options, has no information about the derived
*   options value type, and handles all services required by the parser.
*
***/

class Cli::OptBase {
public:
    OptBase(const std::string & keys, bool boolean);
    virtual ~OptBase() {}

    // Name of the last argument to populated the value, or an empty string if
    // it wasn't populated. For vectors, it's what populated the last value.
    virtual const std::string & from() const = 0;

    // Absolute position in argv[] of last the argument that populated the
    // value, or an empty string if it wasn't populated. For vectors, it's what
    // populated the last value.
    virtual int pos() const = 0;

    // set to passed in default
    virtual void reset() = 0;

    // parses the string into the value, returns false on error
    virtual bool parseValue(const std::string & value) = 0;

    // set to (or add to vec) value for missing optional
    virtual void unspecifiedValue() = 0;

    // number of values, non-vecs are always 1
    virtual size_t size() const = 0;

protected:
    virtual bool parseAction(Cli & cli, const std::string & value) = 0;
    virtual void set(const std::string & name, int pos) = 0;

    // Allows the type unaware layer to determine if a new option is pointing
    // at the same value as an existing option -- with RTTI disabled
    virtual bool sameValue(const void * value) = 0;

    template <typename T> void setValueName();

    void index(OptIndex & ndx);
    void indexName(OptIndex & ndx, const std::string & name);
    void indexLongName(
        OptIndex & ndx,
        const std::string & name,
        bool invert,
        bool optional);

    std::string m_command;
    std::string m_group;
    std::string m_desc;
    std::string m_valueDesc;
    bool m_visible{true};

    // Are multiple values are allowed, and how many there can be (-1 for
    // unlimited).
    bool m_multiple{false};
    int m_nargs{1};

    // the value is a bool on the command line (no separate value)?
    bool m_bool{false};

    bool m_flagValue{false};
    bool m_flagDefault{false};

private:
    friend class Cli;
    std::string m_names;
};

//===========================================================================
template <typename T> inline void Cli::OptBase::setValueName() {
    if (std::is_integral<T>::value) {
        m_valueDesc = "NUM";
    } else if (std::is_convertible<T, std::string>::value) {
        m_valueDesc = "STRING";
    } else {
        m_valueDesc = "VALUE";
    }
}

//===========================================================================
template <>
inline void Cli::OptBase::setValueName<std::experimental::filesystem::path>() {
    m_valueDesc = "FILE";
}


/****************************************************************************
*
*   Cli::OptShim
*
*   Common base for the more derived simple and vector option types. Has
*   knowledge of the value type but no knowledge about it.
*
***/

template <typename A, typename T> class Cli::OptShim : public OptBase {
public:
    OptShim(const std::string & keys, bool boolean);
    OptShim(const OptShim &) = delete;
    OptShim & operator=(const OptShim &) = delete;

    //-----------------------------------------------------------------------
    // Configuration

    // Set subcommand for which this is an option.
    A & command(const std::string & val);

    // Set group under which this argument will show up in the help text.
    A & group(const std::string & val);

    // Set desciption to associate with the argument in writeHelp()
    A & desc(const std::string & val);

    // Set name of meta-variable in writeHelp. For example, in "--count NUM"
    // this is used to change "NUM" to something else.
    A & valueDesc(const std::string & val);

    // Allows the default to be changed after the opt has been created.
    A & defaultValue(const T & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without a value.
    A & implicitValue(const T & val);

    // Turns the argument into a feature switch, there are normally multiple
    // switches pointed at a single external value, one of which should be
    // flagged as the default. If none (or many) are set marked as the default
    // a default will be choosen for you.
    A & flagValue(bool isDefault = false);

    // Change the action to take when parsing this argument. The function
    // should:
    //  - parse the src string and use the result to set the value (or
    //    push_back the new value for vectors).
    //  - call cli.badUsage() with an error message if there's a problem
    //  - return false if the program should stop, otherwise true. This
    //    could be due to error or just to early out like "--version" and
    //    "--help".
    //
    // You can use opt.from() and opt.pos() to get the argument name that the
    // value was attached to on the command line and its position in argv[].
    // For bool arguments the source value string will always be either "0"
    // or "1".
    //
    // If you just need support for a new type you can provide a std::istream
    // extraction (>>) or assignment from std::string operator and the
    // default action will pick it up.
    using ActionFn = bool(Cli & cli, A & arg, const std::string & src);
    A & action(std::function<ActionFn> fn);

    // Controls whether or not the option appears in help pages.
    A & show(bool visible = true);

    //-----------------------------------------------------------------------
    // Queries
    const T & implicitValue() const { return m_implicitValue; }
    const T & defaultValue() const { return m_defValue; }

protected:
    bool parseAction(Cli & cli, const std::string & value) final;

    std::function<ActionFn> m_action;
    T m_implicitValue{};
    T m_defValue{};
};

//===========================================================================
template <typename A, typename T>
inline Cli::OptShim<A, T>::OptShim(const std::string & keys, bool boolean)
    : OptBase(keys, boolean) {
    setValueName<T>();
}

//===========================================================================
template <typename A, typename T>
inline bool
Cli::OptShim<A, T>::parseAction(Cli & cli, const std::string & val) {
    auto self = static_cast<A *>(this);
    return m_action(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::command(const std::string & val) {
    m_command = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::group(const std::string & val) {
    m_group = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::desc(const std::string & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::valueDesc(const std::string & val) {
    m_valueDesc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::defaultValue(const T & val) {
    m_defValue = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::implicitValue(const T & val) {
    if (m_bool) {
        // they don't have separate values, just their presence/absence
        assert(!m_bool && "bool argument values are never implicit");
    } else {
        m_implicitValue = val;
    }
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::flagValue(bool isDefault) {
    auto self = static_cast<A *>(this);
    m_flagValue = true;
    if (!self->m_proxy->m_defFlagOpt) {
        self->m_proxy->m_defFlagOpt = self;
        m_flagDefault = true;
    } else if (isDefault) {
        self->m_proxy->m_defFlagOpt->m_flagDefault = false;
        self->m_proxy->m_defFlagOpt = self;
        m_flagDefault = true;
    } else {
        m_flagDefault = false;
    }
    m_bool = true;
    return *self;
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::action(std::function<ActionFn> fn) {
    m_action = fn;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::show(bool visible) {
    m_visible = visible;
    return static_cast<A &>(*this);
}


/****************************************************************************
*
*   Cli::ArgMatch
*
*   Reference to the commandline argument that was used to populate a value
*
***/

struct Cli::ArgMatch {
    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string name;

    // member of argv[] that populated the value or 0 if it wasn't.
    int pos{0};
};


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T> struct Cli::Value {
    // where the value came from
    ArgMatch m_match;

    // the value was explicitly set
    bool m_explicit{false};

    // points to the opt with the default flag value
    Opt<T> * m_defFlagOpt{nullptr};

    T * m_value{nullptr};
    T m_internal;

    Value(T * value)
        : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::Opt
*
***/

template <typename T> class Cli::Opt : public OptShim<Opt<T>, T> {
public:
    Opt(std::shared_ptr<Value<T>> value, const std::string & keys);

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // True if the value was populated from the command line and while the
    // value may be the same as the default it wasn't simply left that way.
    explicit operator bool() const { return m_proxy->m_explicit; }

    using OptShim<Opt, T>::defaultValue;
    using OptShim<Opt, T>::implicitValue;

    // OptBase
    const std::string & from() const final { return m_proxy->m_match.name; }
    int pos() const final { return m_proxy->m_match.pos; }
    void reset() final;
    bool parseValue(const std::string & value) final;
    void unspecifiedValue() final;
    size_t size() const final;

private:
    friend class Cli;
    void set(const std::string & name, int pos) final;
    bool sameValue(const void * value) final {
        return value == m_proxy->m_value;
    }

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
inline Cli::Opt<T>::Opt(
    std::shared_ptr<Value<T>> value,
    const std::string & keys)
    : OptShim<Opt, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value} {}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::set(const std::string & name, int pos) {
    m_proxy->m_match.name = name;
    m_proxy->m_match.pos = pos;
    m_proxy->m_explicit = true;
}

//===========================================================================
template <typename T> inline void Cli::Opt<T>::reset() {
    if (!OptBase::m_flagValue || OptBase::m_flagDefault)
        *m_proxy->m_value = defaultValue();
    m_proxy->m_match.name.clear();
    m_proxy->m_match.pos = 0;
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::parseValue(const std::string & value) {
    if (OptBase::m_flagValue) {
        bool flagged;
        if (!stringTo(flagged, value))
            return false;
        if (flagged)
            *m_proxy->m_value = defaultValue();
        return true;
    }

    return stringTo(*m_proxy->m_value, value);
}

//===========================================================================
template <typename T> inline void Cli::Opt<T>::unspecifiedValue() {
    *m_proxy->m_value = implicitValue();
}

//===========================================================================
template <typename T> inline size_t Cli::Opt<T>::size() const {
    return 1;
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T> struct Cli::ValueVec {
    // where the values came from
    std::vector<ArgMatch> m_matches;

    // points to the opt with the default flag value
    OptVec<T> * m_defFlagOpt{nullptr};

    std::vector<T> * m_values{nullptr};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::OptVec
*
***/

template <typename T> class Cli::OptVec : public OptShim<OptVec<T>, T> {
public:
    OptVec(
        std::shared_ptr<ValueVec<T>> values,
        const std::string & keys,
        int nargs);

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    // True if values where added from the command line
    explicit operator bool() const { return !m_proxy->m_values->empty(); }

    using OptShim<OptVec, T>::defaultValue;
    using OptShim<OptVec, T>::implicitValue;

    // OptBase
    const std::string & from() const final { return from(size() - 1); }
    int pos() const final { return pos(size() - 1); }
    void reset() final;
    bool parseValue(const std::string & value) final;
    void unspecifiedValue() final;
    size_t size() const final;

    // Information about a specific member of the vector of values
    const std::string & from(size_t index) const;
    int pos(size_t index) const;

private:
    friend class Cli;
    void set(const std::string & name, int pos) final;
    bool sameValue(const void * value) final {
        return value == m_proxy->m_values;
    }

    std::shared_ptr<ValueVec<T>> m_proxy;
    std::string m_empty;
};

//===========================================================================
template <typename T>
inline Cli::OptVec<T>::OptVec(
    std::shared_ptr<ValueVec<T>> values,
    const std::string & keys,
    int nargs)
    : OptShim<OptVec, T>{keys, std::is_same<T, bool>::value}
    , m_proxy(values) {
    OptBase::m_multiple = true;
    OptBase::m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::set(const std::string & name, int pos) {
    ArgMatch match;
    match.name = name;
    match.pos = pos;
    m_proxy->m_matches.push_back(match);
}

//===========================================================================
template <typename T>
inline const std::string & Cli::OptVec<T>::from(size_t index) const {
    return index >= size() ? m_empty : m_proxy->m_matches[index].name;
}

//===========================================================================
template <typename T> inline int Cli::OptVec<T>::pos(size_t index) const {
    return index >= size() ? 0 : m_proxy->m_matches[index].pos;
}

//===========================================================================
template <typename T> inline void Cli::OptVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_matches.clear();
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::parseValue(const std::string & value) {
    if (OptBase::m_flagValue) {
        bool flagged;
        if (!stringTo(flagged, value))
            return false;
        if (flagged)
            m_proxy->m_values->push_back(defaultValue());
        return true;
    }

    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_proxy->m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void Cli::OptVec<T>::unspecifiedValue() {
    m_proxy->m_values->push_back(implicitValue());
}

//===========================================================================
template <typename T> inline size_t Cli::OptVec<T>::size() const {
    return m_proxy->m_values->size();
}

} // namespace
