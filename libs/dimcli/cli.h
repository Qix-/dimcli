// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// cli.h - dimcli
//
// Command line parser toolkit
//
// Instead of trying to figure it all out from just this header please take
// a moment and look at the documentation and examples:
// https://github.com/gknowles/dimcli

#pragma once


/****************************************************************************
*
*   Configuration
*
***/

// The dimcli_userconfig.h include is defined by the application and may be
// used to configure the standard library before the headers get included.
// This includes macros such as _ITERATOR_DEBUG_LEVEL (Microsoft),
// _LIBCPP_DEBUG (libc++), etc.
#ifdef __has_include
#if __has_include("dimcli_userconfig.h")
#include "dimcli_userconfig.h"
#elif __has_include("cppconf/dimcli_userconfig.h")
#include "cppconf/dimcli_userconfig.h"
#endif
#endif

//---------------------------------------------------------------------------
// Configuration of this installation, these are options that must be the
// same when building the app as when building the library.

// DIMCLI_LIB_DYN_LINK: Forces all libraries that have separate source, to be
// linked as dll's rather than static libraries on Microsoft Windows (this
// macro is used to turn on __declspec(dllimport) modifiers, so that the
// compiler knows which symbols to look for in a dll rather than in a static
// library). Note that there may be some libraries that can only be linked in
// one way (statically or dynamically), in these cases this macro has no
// effect.
//#define DIMCLI_LIB_DYN_LINK

// DIMCLI_LIB_WINAPI_FAMILY_APP: Removes all functions that rely on windows
// WINAPI_FAMILY_DESKTOP mode, such as the console and environment
// variables.
//#define DIMCLI_LIB_WINAPI_FAMILY_APP


//---------------------------------------------------------------------------
// Configuration of the application. These options, if desired, are set by the
// application before including the library headers.

// DIMCLI_LIB_KEEP_MACROS: By default the DIMCLI_LIB_* macros defined
// internally (including in this file) are undef'd so they don't leak out to
// application code. Setting this macro leaves them available for the
// application to use. Also included are other platform specific adjustments,
// such as suppression of specific compiler warnings.

// DIMCLI_LIB_NO_FILESYSTEM: Prevents the <filesystem> header from being
// included and as a side-effect disables support for response files. You can
// try this if you are working with an older compiler with incompatible or 
// broken filesystem support.


//===========================================================================
// Internal
//===========================================================================

#if defined(DIMCLI_LIB_SOURCE) && !defined(DIMCLI_LIB_KEEP_MACROS)
#define DIMCLI_LIB_KEEP_MACROS
#endif

#ifdef DIMCLI_LIB_WINAPI_FAMILY_APP
#define DIMCLI_LIB_NO_ENV
#define DIMCLI_LIB_NO_CONSOLE
#endif

#ifdef _MSC_VER
#ifndef DIMCLI_LIB_KEEP_MACROS
#pragma warning(push)
#endif
// attribute 'identifier' is not recognized
#pragma warning(disable : 5030)
#endif

#ifdef DIMCLI_LIB_DYN_LINK
#if defined(_MSC_VER)
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif

#if defined(_WIN32)
#ifdef DIMCLI_LIB_SOURCE
#define DIMCLI_LIB_DECL __declspec(dllexport)
#else
#define DIMCLI_LIB_DECL __declspec(dllimport)
#endif
#endif
#else
#define DIMCLI_LIB_DECL
#endif

#if !defined(_CPPUNWIND) && !defined(_HAS_EXCEPTIONS)
#define _HAS_EXCEPTIONS 0
#endif


/****************************************************************************
*
*   Includes
*
***/

#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef DIMCLI_LIB_NO_FILESYSTEM
#if defined(_MSC_VER) && _MSC_VER < 1914
#include <experimental/filesystem>
#define DIMCLI_LIB_FILESYSTEM std::experimental::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::experimental::filesystem::path
#elif defined(__has_include)
#if __has_include(<filesystem>)
#include <filesystem>
#define DIMCLI_LIB_FILESYSTEM std::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::filesystem::path
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#define DIMCLI_LIB_FILESYSTEM std::experimental::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::experimental::filesystem::path
#endif
#endif
#endif


namespace Dim {

// forward declarations
class Cli;

#ifdef DIMCLI_LIB_BUILD_COVERAGE
void assertHandler(char const expr[], unsigned line);

#undef assert
#define assert(expr) \
    (void) ( (!!(expr)) || (Dim::assertHandler(#expr, unsigned(__LINE__)), 0) )
#endif


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

class DIMCLI_LIB_DECL Cli {
public:
    struct Config;

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
    Cli(Cli const & from);
    Cli & operator=(Cli const & from);
    Cli & operator=(Cli && from);

    //-----------------------------------------------------------------------
    // Configuration

    template <typename T, typename U, typename =
        typename std::enable_if<std::is_convertible<U, T>::value>::type
    >
    Opt<T> & opt(T * value, std::string const & keys, U const & def);

    template <typename T> Opt<T> & opt(T * value, std::string const & keys);

    template <typename T>
    OptVec<T> & optVec(
        std::vector<T> * values,
        std::string const & keys,
        int nargs = -1
    );

    template <typename T>
    Opt<T> & opt(std::string const & keys, T const & def = {});

    template <typename T>
    OptVec<T> & optVec(std::string const & keys, int nargs = -1);

    template <typename T, typename U, typename =
        typename std::enable_if<std::is_convertible<U, T>::value>::type
    >
    Opt<T> & opt(Opt<T> & value, std::string const & keys, U const & def);

    template <typename T>
    Opt<T> & opt(Opt<T> & value, std::string const & keys);

    template <typename T>
    OptVec<T> & optVec(
        OptVec<T> & values,
        std::string const & keys,
        int nargs = -1
    );

    // Add -y, --yes option that exits early when false and has an "are you
    // sure?" style prompt when it's not present.
    Opt<bool> & confirmOpt(std::string const & prompt = {});

    // Get reference to internal help option, can be used to change the
    // description, option group, etc. To completely replace it, add another
    // option that responds to --help.
    Opt<bool> & helpOpt();

    // Add --password option and prompts for a password if it's not given
    // on the command line. If confirm is true and it's not on the command
    // line you have to enter it twice.
    Opt<std::string> & passwordOpt(bool confirm = false);

    // Add --version option that shows "${progName.filename()} version ${ver}"
    // and exits. An empty progName defaults to argv[0].
    Opt<bool> & versionOpt(
        std::string const & ver,
        std::string const & progName = {}
    );

    //-----------------------------------------------------------------------
    // A group collects options into sections in the help text. Options are
    // always added to a group, either the default group of the cli (or of the
    // selected command), or an explicitly created one.

    // Changes config context to point at the selected option group of the
    // current command, that you can then start stuffing things into.
    Cli & group(std::string const & name);

    // Heading title to display, defaults to group name. If empty there will
    // be a single blank line separating this group from the previous one.
    Cli & title(std::string const & val);

    // Option groups are sorted by key, defaults to group name.
    Cli & sortKey(std::string const & key);

    std::string const & group() const { return m_group; }
    std::string const & title() const;
    std::string const & sortKey() const;

    //-----------------------------------------------------------------------
    // Changes config context to reference the options of the selected
    // command. Use "" to specify the top level context. If a new command is
    // selected it will be created in the command group of the current context.
    Cli & command(std::string const & name, std::string const & group = {});

    // Function signature of actions that are tied to commands.
    using ActionFn = bool(Cli & cli);

    // Action that should be taken when the currently selected command is run.
    // Actions are executed when cli.exec() is called by the application. The
    // command's action function should:
    //  - do something useful
    //  - return false on errors (and use fail() to set exitCode, et al)
    Cli & action(std::function<ActionFn> fn);

    // Arbitrary text can be added to the generated help for each command,
    // this text can come before the usage (header), between the usage and
    // the arguments / options (desc), or after the options (footer). Use
    // line breaks for semantics, let the automatic line wrapping take care
    // of the rest.
    //
    // Unless individually overridden commands default to using the header and
    // footer (but not the desc) specified at the top level.
    Cli & header(std::string const & val);
    Cli & desc(std::string const & val);
    Cli & footer(std::string const & val);

    std::string const & command() const { return m_command; }
    std::string const & header() const;
    std::string const & desc() const;
    std::string const & footer() const;

    // Add "help" command that shows the help text for other commands. Allows
    // users to run the more natural "prog help command" instead of the more
    // annoying "prog command --help".
    Cli & helpCmd();

    // Adds before action that replaces empty command lines with --help.
    Cli & helpNoArgs();

    //-----------------------------------------------------------------------
    // A command group collects commands into sections in the help text, in the
    // same way that option groups do with options.

    // Changes the command group of the current command. Because new commands
    // start out in the same group as the current command, it can be convenient
    // to create all the commands of one group before moving to the next.
    //
    // Setting the command group at the top level (the "" command) only serves
    // to set the initial command group for new commands created while in the
    // top level context.
    Cli & cmdGroup(std::string const & name);

    // Heading title to display, defaults to group name. If empty there will be
    // a single blank line separating this group from the previous one.
    Cli & cmdTitle(std::string const & val);

    // Command groups are sorted by key, defaults to group name.
    Cli & cmdSortKey(std::string const & key);

    std::string const & cmdGroup() const;
    std::string const & cmdTitle() const;
    std::string const & cmdSortKey() const;

    //-----------------------------------------------------------------------
    // Enabled by default, response file expansion replaces arguments of the
    // form "@file" with the contents of the file.
    void responseFiles(bool enable = true);

#if !defined(DIMCLI_LIB_NO_ENV)
    // Environment variable to get initial options from. Defaults to the empty
    // string, but when set the content of the named variable is parsed into
    // args which are then inserted into the argument list right after arg0.
    void envOpts(std::string const & envVar);
#endif

    // Change the locale used when parsing values via iostream. Defaults to
    // the user's preferred locale (aka locale("")).
    std::locale imbue(std::locale const & loc);

    // Function signature of actions that run before options are populated.
    using BeforeFn = bool(Cli & cli, std::vector<std::string> & args);

    // Actions taken after environment variable and response file expansion
    // but before any individual arguments are parsed. The before action
    // function should:
    //  - inspect and possibly modify the raw arguments coming in
    //  - return false if parsing should stop, via badUsage() for errors
    Cli & before(std::function<BeforeFn> fn);

    // Changes the streams used for prompting, printing help messages, etc.
    // Mainly intended for testing. Setting to null restores the defaults
    // which are cin and cout respectively.
    Cli & iostreams(std::istream * in, std::ostream * out);
    std::istream & conin();
    std::ostream & conout();

    //-----------------------------------------------------------------------
    // Rendering help text

    // printHelp & printUsage return the current exitCode()
    int printHelp(
        std::ostream & os,
        std::string const & progName = {},
        std::string const & cmd = {}
    );
    int printUsage(
        std::ostream & os,
        std::string const & progName = {},
        std::string const & cmd = {}
    );
    // Same as printUsage(), except individually lists all non-default options
    // instead of the [OPTIONS] catchall.
    int printUsageEx(
        std::ostream & os,
        std::string const & progName = {},
        std::string const & cmd = {}
    );

    void printPositionals(
        std::ostream & os,
        std::string const & cmd = {}
    );
    void printOptions(std::ostream & os, std::string const & cmd = {});
    void printCommands(std::ostream & os);

    // If !exitCode() prints the errMsg and errDetail (if present), but does
    // nothing if exitCode() is EX_OK. Returns exitCode(). Only makes sense
    // after parsing has completed.
    int printError(std::ostream & oerr);

    //-----------------------------------------------------------------------
    // Parsing

    // Parse the command line, populate the options, and set the error and
    // other miscellaneous state. Returns true if processing should continue.
    //
    // The ostream& argument is only used to print the error message, if any,
    // via printError(). Error information can also be extracted after parse()
    // completes, see errMsg() and friends.
    [[nodiscard]] bool parse(size_t argc, char * argv[]);
    [[nodiscard]] bool parse(std::ostream & oerr, size_t argc, char * argv[]);

    // "args" is non-const so response files can be expanded in place.
    [[nodiscard]] bool parse(std::vector<std::string> & args);
    [[nodiscard]] bool parse(
        std::ostream & oerr,
        std::vector<std::string> & args
    );

    // Sets all options to their defaults, called internally when parsing
    // starts.
    Cli & resetValues();

    // Parse cmdline into vector of args, using the default conventions
    // (Gnu or Windows) of the platform.
    static std::vector<std::string> toArgv(std::string const & cmdline);
    // Copy array of pointers into vector of args.
    static std::vector<std::string> toArgv(size_t argc, char * argv[]);
    // Copy array of wchar_t pointers into vector of UTF-8 encoded args.
    static std::vector<std::string> toArgv(size_t argc, wchar_t * argv[]);
    // Create vector of pointers suitable for use with argc/argv APIs, has a
    // trailing null that is not included in size(). The return values point
    // into the source string vector and are only valid until that vector is
    // resized or destroyed.
    static std::vector<char const *> toPtrArgv(
        std::vector<std::string> const & args
    );

    // Parse according to glib conventions, based on the UNIX98 shell spec.
    static std::vector<std::string> toGlibArgv(std::string const & cmdline);
    // Parse using GNU conventions, same rules as buildargv()
    static std::vector<std::string> toGnuArgv(std::string const & cmdline);
    // Parse using Windows rules
    static std::vector<std::string> toWindowsArgv(std::string const & cmdline);

    // Join args into a single command line, escaping as needed, that parses
    // back into those same args. Uses the default conventions (Gnu or Windows)
    // of the platform.
    static std::string toCmdline(std::vector<std::string> const & args);
    // Join array of pointers into command line, escaping as needed.
    static std::string toCmdline(size_t argc, char * argv[]);
    static std::string toCmdline(size_t argc, char const * argv[]);

    // Join according to glib conventions, based on the UNIX98 shell spec.
    static std::string toGlibCmdline(size_t argc, char * argv[]);
    // Join using GNU conventions, same rules as buildargv()
    static std::string toGnuCmdline(size_t argc, char * argv[]);
    // Join using Windows rules
    static std::string toWindowsCmdline(size_t argc, char * argv[]);

    template <typename T>
    [[nodiscard]] bool fromString(T & out, std::string const & src) const;
    template <typename T>
    [[nodiscard]] bool toString(std::string & out, T const & src) const;

    //-----------------------------------------------------------------------
    // Support functions for use from parsing actions

    // Intended for use from return statements in action callbacks. Sets
    // exit code (to EX_USAGE) and error msg, then returns false. If it's a
    // subcommand the msg is prefixed with:
    //  "Command: '<command>': "
    bool badUsage(std::string const & msg);

    // Calls badUsage(msg) with msg set to:
    //  "<prefix>: <value>"
    bool badUsage(std::string const & prefix, std::string const & value);

    // Calls badUsage(prefix, value) with prefix set to:
    //  "Invalid '" + opt.from() + "' value"
    bool badUsage(OptBase const & opt, std::string const & value);

    // Used to populate an option with an arbitrary input string through the
    // standard parsing logic. Since it causes the parse and check actions to
    // be called care must be taken to avoid infinite recursion if used from
    // those actions.
    [[nodiscard]] bool parseValue(
        OptBase & out,
        std::string const & name,
        size_t pos,
        char const src[]
    );

    // Prompt sends a prompt message to cout and read a response from cin
    // (unless cli.iostreams() changed the streams to use), the response is
    // then passed to cli.parseValue() to set the value and run any actions.
    enum {
        fPromptHide = 1,      // Hide user input as they type
        fPromptConfirm = 2,   // Make the user enter it twice
        fPromptNoDefault = 4, // Don't include default value in prompt
    };
    [[nodiscard]] bool prompt(
        OptBase & opt,
        std::string const & msg,
        int flags
    );

    //-----------------------------------------------------------------------
    // After parsing

    int exitCode() const;
    std::string const & errMsg() const;

    // Additional information that may help the user correct their mistake,
    // may be empty.
    std::string const & errDetail() const;

    // Program name received in argv[0]
    std::string const & progName() const;

    // Command to run, as selected by argv, empty string if there are no
    // commands defined or none were selected.
    std::string const & runCommand() const;

    // Executes the action of the selected command; returns true if it worked.
    // On failure it's expected to have set exitCode, errMsg, and optionally
    // errDetail via fail(). If no command was selected it runs the action of
    // the empty "" command, which defaults to failing with "No command given."
    // but can be set via cli.action() just like any other command.
    [[nodiscard]] bool exec();
    [[nodiscard]] bool exec(std::ostream & oerr);

    // Helpers to parse and, if successful, execute.
    [[nodiscard]] bool exec(size_t argc, char * argv[]);
    [[nodiscard]] bool exec(std::ostream & oerr, size_t argc, char * argv[]);
    [[nodiscard]] bool exec(std::vector<std::string> & args);
    [[nodiscard]] bool exec(
        std::ostream & oerr,
        std::vector<std::string> & args
    );

    // Sets exitCode(), errMsg(), and errDetail(), intended to be called from
    // command actions, parsing related failures should use badUsage().
    bool fail(
        int code,
        std::string const & msg,
        std::string const & detail = {}
    );

    // Returns true if the named command has been defined, used by the help
    // command implementation. Not reliable before cli.parse() has been called
    // and had a chance to update the internal data structures.
    bool commandExists(std::string const & name) const;

protected:
    Cli(std::shared_ptr<Config> cfg);

private:
    static void consoleEnableEcho(bool enable = true);

    bool defParseAction(OptBase & opt, std::string const & val);
    bool requireAction(OptBase & opt);

    void addOpt(std::unique_ptr<OptBase> opt);
    template <typename A> A & addOpt(std::unique_ptr<A> ptr);

    template <typename Opt, typename Value, typename T>
    std::shared_ptr<Value> getProxy(T * ptr);

    // Find an option (from any subcommand) that targets the value.
    OptBase * findOpt(void const * value);

    std::string descStr(Cli::OptBase const & opt) const;
    int writeUsageImpl(
        std::ostream & os,
        std::string const & arg0,
        std::string const & cmd,
        bool expandedOptions
    );

    template <typename T>
    auto fromString_impl(T & out, std::string const & src, int, int) const
        -> decltype(out = src, bool());
    template <typename T>
    auto fromString_impl(T & out, std::string const & src, int, long) const
        -> decltype(std::declval<std::istream &>() >> out, bool());
    template <typename T>
    bool fromString_impl(T & out, std::string const & src, long, long) const;

    template <typename T>
    auto toString_impl(std::string & out, T const & src, int) const
        -> decltype(std::declval<std::ostream &>() << src, bool());
    template <typename T>
    bool toString_impl(std::string & out, T const & src, long) const;

    std::shared_ptr<Config> m_cfg;
    std::string m_group;
    std::string m_command;
    mutable std::stringstream m_interpreter;
};

//===========================================================================
template <typename T, typename U, typename>
Cli::Opt<T> & Cli::opt(
    T * value,
    std::string const & keys,
    U const & def
) {
    auto proxy = getProxy<Opt<T>, Value<T>>(value);
    auto ptr = std::make_unique<Opt<T>>(proxy, keys);
    ptr->defaultValue(def);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(T * value, std::string const & keys) {
    return opt(value, keys, T{});
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(
    std::vector<T> * values,
    std::string const & keys,
    int nargs
) {
    auto proxy = getProxy<OptVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<OptVec<T>>(proxy, keys, nargs);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T, typename U, typename>
Cli::Opt<T> & Cli::opt(
    Opt<T> & alias,
    std::string const & keys,
   U  const & def
) {
    return opt(&*alias, keys, def);
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(Opt<T> & alias, std::string const & keys) {
    return opt(&*alias, keys, T{});
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(
    OptVec<T> & alias,
    std::string const & keys,
    int nargs
) {
    return optVec(&*alias, keys, nargs);
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(std::string const & keys, T const & def) {
    return opt<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(std::string const & keys, int nargs) {
    return optVec<T>(nullptr, keys, nargs);
}

//===========================================================================
template <typename A>
A & Cli::addOpt(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.parse(&Cli::defParseAction).command(command()).group(group());
    addOpt(std::unique_ptr<OptBase>(ptr.release()));
    return opt;
}

//===========================================================================
template <typename A, typename V, typename T>
std::shared_ptr<V> Cli::getProxy(T * ptr) {
    if (OptBase * opt = findOpt(ptr)) {
        auto dopt = static_cast<A *>(opt);
        return dopt->m_proxy;
    }

    // Since there was no existing proxy to the raw value, create one.
    return std::make_shared<V>(ptr);
}

//===========================================================================
// fromString - converts from string to T
//===========================================================================
template <typename T>
bool Cli::fromString(T & out, std::string const & src) const {
    // Versions of fromString_impl taking ints as extra parameters are
    // preferred (better conversion from 0), if they don't exist for T
    // (because no out=src assignment operator exists) then the versions
    // taking longs are considered.
    return fromString_impl(out, src, 0, 0);
}

//===========================================================================
template <typename T>
auto Cli::fromString_impl(T & out, std::string const & src, int, int) const
    -> decltype(out = src, bool())
{
    out = src;
    return true;
}

//===========================================================================
template <typename T>
auto Cli::fromString_impl(T & out, std::string const & src, int, long) const
    -> decltype(std::declval<std::istream &>() >> out, bool())
{
    m_interpreter.clear();
    m_interpreter.str(src);
    if (!(m_interpreter >> out) || !(m_interpreter >> std::ws).eof()) {
        out = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T>
bool Cli::fromString_impl(
    T &, // out
    std::string const &, // src
    long,
    long
) const {
    // In order to parse an argument one of the following must exist:
    //  - assignment operator for std::string to T
    //  - std::istream extraction operator for T
    //  - specialization of Cli::fromString template for T, something like:
    //      template<> bool Cli::fromString<MyType>::(
    //          MyType & out, std::string const & src) const { ... }
    //  - parse action attached to the Opt<T> instance that doesn't call
    //    opt.parseValue(), such as opt.choice().
    assert(!"no assignment from string or stream extraction operator");
    return false;
}

//===========================================================================
// toString - converts to string from T, sets to empty string and returns
// false if conversion fails or no conversion available.
//===========================================================================
template <typename T>
bool Cli::toString(std::string & out, T const & src) const {
    return toString_impl(out, src, 0);
}

//===========================================================================
template <typename T>
auto Cli::toString_impl(std::string & out, T const & src, int) const
    -> decltype(std::declval<std::ostream &>() << src, bool())
{
    m_interpreter.clear();
    m_interpreter.str("");
    if (!(m_interpreter << src)) {
        out.clear();
        return false;
    }
    out = m_interpreter.str();
    return true;
}

//===========================================================================
template <typename T>
bool Cli::toString_impl(std::string & out, T const &, long) const {
    out.clear();
    return false;
}


/****************************************************************************
*
*   CliLocal
*
*   Stand-alone cli instance independent of the shared configuration. Mainly
*   for testing.
*
***/

class DIMCLI_LIB_DECL CliLocal : public Cli {
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

class DIMCLI_LIB_DECL Cli::OptBase {
public:
    struct ChoiceDesc {
        std::string desc;
        std::string sortKey;
        size_t pos{};
        bool def{};
    };

public:
    OptBase(std::string const & keys, bool boolean);
    virtual ~OptBase() {}

    //-----------------------------------------------------------------------
    // Queries

    // True if the value was populated from the command line, whether the
    // resulting value is the same as the default is immaterial.
    explicit operator bool() const { return assigned(); }

    // Name of the last argument to populated the value, or an empty string if
    // it wasn't populated. For vectors, it's what populated the last value.
    virtual std::string const & from() const = 0;

    // Absolute position in argv[] of last the argument that populated the
    // value. For vectors, it refers to where the value on the back came from.
    // If pos() is 0 the value wasn't populated from the command line or
    // wasn't populated at all, check from() to tell the difference.
    //
    // It's possible for a value to come from prompt() or some other action
    // (which should set the position to 0) instead of the command args.
    virtual int pos() const = 0;

    // Number of values, non-vectors are always 1.
    virtual size_t size() const = 0;

    // Defaults to use when populating the option from an action that's not
    // tied to a command line argument.
    std::string const & defaultFrom() const { return m_fromName; }
    std::string defaultPrompt() const;

    // Command and group this option belongs to.
    std::string const & command() const { return m_command; }
    std::string const & group() const { return m_group; }

    //-----------------------------------------------------------------------
    // Update value

    // Set option to its default value.
    virtual void reset() = 0;

    // Parse the string into the value, return false on error.
    [[nodiscard]] virtual bool parseValue(std::string const & value);

    // Set option (or add to option vector) to value for missing optionals.
    virtual void unspecifiedValue() = 0;

protected:
    virtual bool fromString(Cli & cli, std::string const & value) = 0;
    virtual bool defaultValueToString(
        std::string & out,
        Cli const & cli
    ) const = 0;

    virtual bool parseAction(Cli & cli, std::string const & value) = 0;
    virtual bool checkAction(Cli & cli, std::string const & value) = 0;
    virtual bool afterActions(Cli & cli) = 0;
    virtual void assign(std::string const & name, size_t pos) = 0;
    virtual bool assigned() const = 0;

    // True for flags (bool on command line) that default to true.
    virtual bool inverted() const = 0;

    // Allows the type unaware layer to determine if a new option is pointing
    // at the same value as an existing option -- with RTTI disabled
    virtual bool sameValue(void const * value) const = 0;

    template <typename T> void setValueDesc();

    void setNameIfEmpty(std::string const & name);

    std::string m_command;
    std::string m_group;

    bool m_visible{true};
    std::string m_desc;
    std::string m_valueDesc;

    // empty() to use default, size 1 and *data == '\0' to suppress
    std::string m_defaultDesc;

    std::unordered_map<std::string, ChoiceDesc> m_choiceDescs;

    // Whether this option has one value or a vector of values.
    bool m_vector{};
    // IGNORED. Maximum allowed values, only when a vector is used (-1 for
    // unlimited).
    int m_nargs{1};

    // Whether the value is a bool on the command line (no separate value).
    bool m_bool{};

    bool m_flagValue{};
    bool m_flagDefault{};

private:
    friend class Cli;
    std::string m_names;
    std::string m_fromName;
};

//===========================================================================
template <typename T>
void Cli::OptBase::setValueDesc() {
    if (std::is_integral<T>::value) {
        m_valueDesc = "NUM";
    } else if (std::is_convertible<T, std::string>::value) {
        m_valueDesc = "STRING";
    } else {
        m_valueDesc = "VALUE";
    }
}

#ifdef DIMCLI_LIB_FILESYSTEM
//===========================================================================
template <>
inline void Cli::OptBase::setValueDesc<DIMCLI_LIB_FILESYSTEM_PATH>() {
    m_valueDesc = "FILE";
}
#endif


/****************************************************************************
*
*   Cli::OptShim
*
*   Common base for the more derived simple and vector option types. Has
*   knowledge of the value type but no knowledge about it.
*
***/

template <typename A, typename T>
class Cli::OptShim : public OptBase {
public:
    OptShim(std::string const & keys, bool boolean);
    OptShim(OptShim const &) = delete;
    OptShim & operator=(OptShim const &) = delete;

    //-----------------------------------------------------------------------
    // Configuration

    // Set subcommand for which this is an option.
    A & command(std::string const & val);

    // Set group under which this argument will show up in the help text.
    A & group(std::string const & val);

    // Controls whether or not the option appears in help pages.
    A & show(bool visible = true);

    // Set description to associate with the argument in help text.
    A & desc(std::string const & val);

    // Set name of meta-variable in help text. For example, in "--count NUM"
    // this is used to change "NUM" to something else.
    A & valueDesc(std::string const & val);

    // Set text to appear in the default clause of this options the help text.
    // Can change the "0" in "(default: 0)" to something else, or use an empty
    // string to suppress the entire clause.
    A & defaultDesc(std::string const & val);

    // Allows the default to be changed after the opt has been created.
    A & defaultValue(T const & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without a value.
    A & implicitValue(T const & val);

    // Turns the argument into a feature switch, there are normally multiple
    // switches pointed at a single external value, one of which should be
    // flagged as the default. If none (or many) are set marked as the default
    // one will be chosen for you.
    A & flagValue(bool isDefault = false);

    // Adds a choice, when choices have been added only values that match one
    // of the choices are allowed. Useful for things like enums where there is
    // a controlled set of possible values.
    //
    // In help text choices are sorted first by sortKey and then by the order
    // they were added.
    A & choice(
        T const & val,
        std::string const & key,
        std::string const & desc = {},
        std::string const & sortKey = {}
    );

    // Forces the value to be within the range, if it's less than the low
    // it's set to the low, if higher than high it's made merely high.
    A & clamp(T const & low, T const & high);

    // Fail if the value given for this option is not in within the range
    // (inclusive) of low to high.
    A & range(T const & low, T const & high);

    // Causes a check whether the option value was set during parsing, and
    // reports badUsage() if it wasn't.
    A & require();

    // Enables prompting. When the option hasn't been provided on the command
    // line the user will be prompted for it. Use Cli::fPrompt* flags to
    // adjust behavior.
    A & prompt(int flags = 0);
    A & prompt(
        std::string const & msg, // custom prompt message
        int flags = 0            // Cli::fPrompt* flags
    );

    // Function signature of actions that are tied to options.
    using ActionFn = bool(Cli & cli, A & opt, std::string const & val);

    // Change the action to take when parsing this argument. The function
    // should:
    //  - Parse the src string and use the result to set the value (or
    //    push_back the new value for vectors).
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Return false if the program should stop, otherwise true. This
    //    could be due to error or just to early out like "--version" and
    //    "--help".
    //
    // You can use opt.from() and opt.pos() to get the argument name that the
    // value was attached to on the command line and its position in argv[].
    // For bool arguments the source value string will always be either "0"
    // or "1".
    //
    // If you just need support for a new type you can provide a istream
    // extraction (>>) or assignment from string operator and the default
    // parse action will pick it up.
    A & parse(std::function<ActionFn> fn);

    // Action to take immediately after each value is parsed, unlike parsing
    // itself where there can only be one action, any number of check actions
    // can be added. They will be called in the order they were added and if
    // any of them return false it stops processing. As an example,
    // opt.clamp() and opt.range() both do their job by adding check actions.
    //
    // The function should:
    //  - Check the options new value, possibly in relation to other options.
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Return false if the program should stop, otherwise true to let
    //    processing continue.
    //
    // The opt is fully populated so *opt, opt.from(), etc are all available.
    A & check(std::function<ActionFn> fn);

    // Action to run after all arguments have been parsed, any number of
    // after actions can be added and will, for each option, be called in the
    // order they're added. When using subcommands, the after actions bound
    // to unselected subcommands are not executed. The function should:
    //  - Do something interesting.
    //  - Call cli.badUsage() and return false on error.
    //  - Return true if processing should continue.
    A & after(std::function<ActionFn> fn);

    //-----------------------------------------------------------------------
    // Queries
    T const & implicitValue() const { return m_implicitValue; }
    T const & defaultValue() const { return m_defValue; }

protected:
    bool parseAction(Cli & cli, std::string const & value) final;
    bool checkAction(Cli & cli, std::string const & value) final;
    bool afterActions(Cli & cli) final;
    bool inverted() const final;
    bool exec(
        Cli & cli,
        std::string const & value,
        std::vector<std::function<ActionFn>> const & actions
    );

    std::function<ActionFn> m_parse;
    std::vector<std::function<ActionFn>> m_checks;
    std::vector<std::function<ActionFn>> m_afters;

    T m_implicitValue{};
    T m_defValue{};
    std::vector<T> m_choices;
};

//===========================================================================
template <typename A, typename T>
Cli::OptShim<A, T>::OptShim(std::string const & keys, bool boolean)
    : OptBase(keys, boolean)
{
    setValueDesc<T>();
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::parseAction(
    Cli & cli,
    std::string const & val
) {
    auto self = static_cast<A *>(this);
    return m_parse(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::checkAction(
    Cli & cli,
    std::string const & val
) {
    return exec(cli, val, m_checks);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::afterActions(Cli & cli) {
    return exec(cli, {}, m_afters);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::inverted() const {
    return this->m_bool && this->m_flagValue && this->m_flagDefault;
}

//===========================================================================
template <>
inline bool Cli::OptShim<Cli::Opt<bool>, bool>::inverted() const {
    // bool options are always marked as bool
    assert(this->m_bool && "internal dimcli error");
    if (this->m_flagValue)
        return this->m_flagDefault;
    return this->defaultValue();
}

//===========================================================================
template <typename A, typename T>
bool Cli::OptShim<A, T>::exec(
    Cli & cli,
    std::string const & val,
    std::vector<std::function<ActionFn>> const & actions
) {
    auto self = static_cast<A *>(this);
    for (auto && fn : actions) {
        if (!fn(cli, *self, val))
            return false;
    }
    return true;
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::command(std::string const & val) {
    m_command = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::group(std::string const & val) {
    m_group = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::show(bool visible) {
    m_visible = visible;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::desc(std::string const & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::valueDesc(std::string const & val) {
    m_valueDesc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::defaultDesc(std::string const & val) {
    m_defaultDesc = val;
    if (val.empty())
        m_defaultDesc.push_back('\0');
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::defaultValue(T const & val) {
    m_defValue = val;
    for (auto && cd : m_choiceDescs)
        cd.second.def = !this->m_vector && val == m_choices[cd.second.pos];
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::implicitValue(T const & val) {
    if (m_bool) {
        // bools don't have separate values, just their presence/absence
        assert(!"bool argument values can't be implicit");
    } else {
        m_implicitValue = val;
    }
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::flagValue(bool isDefault) {
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
A & Cli::OptShim<A, T>::choice(
    T const & val,
    std::string const & key,
    std::string const & desc,
    std::string const & sortKey
) {
    // The empty string isn't a valid choice because it can't be specified on
    // the command line, where unspecified picks the default instead.
    assert(!key.empty() && "an empty string can't be a choice");
    auto & cd = m_choiceDescs[key];
    cd.pos = m_choices.size();
    cd.desc = desc;
    cd.sortKey = sortKey;
    cd.def = (!this->m_vector && val == this->defaultValue());
    m_choices.push_back(val);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::parse(std::function<ActionFn> fn) {
    this->m_parse = move(fn);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::check(std::function<ActionFn> fn) {
    this->m_checks.push_back(move(fn));
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::after(std::function<ActionFn> fn) {
    this->m_afters.push_back(move(fn));
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::require() {
    return after([](auto & cli, auto & opt, auto & /* val */) {
        return cli.requireAction(opt);
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::range(T const & low, T const & high) {
    assert(!(high < low) && "bad range, low greater than high");
    return check([low, high](auto & cli, auto & opt, auto & val) {
        if (*opt < low || high < *opt) {
            std::ostringstream os;
            os << "Out of range '" << opt.from() << "' value [" << low << " - "
               << high << "]";
            return cli.badUsage(os.str(), val);
        }
        return true;
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::clamp(T const & low, T const & high) {
    assert(!(high < low) && "bad clamp, low greater than high");
    return check([low, high](auto & /* cli */, auto & opt, auto & /* val */) {
        if (*opt < low) {
            *opt = low;
        } else if (high < *opt) {
            *opt = high;
        }
        return true;
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(int flags) {
    return prompt({}, flags);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(std::string const & msg, int flags) {
    return after([=](auto & cli, auto & opt, auto & /* val */) {
        return cli.prompt(opt, msg, flags);
    });
}


/****************************************************************************
*
*   Cli::ArgMatch
*
*   Reference to the command line argument that was used to populate a value
*
***/

struct Cli::ArgMatch {
    // Name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string name;

    // Member of argv[] that populated the value or 0 if it wasn't.
    int pos{};
};


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T>
struct Cli::Value {
    // Where the value came from.
    ArgMatch m_match;

    // Whether the value was explicitly set.
    bool m_explicit{};

    // Points to the opt with the default flag value.
    Opt<T> * m_defFlagOpt{};

    T * m_value{};
    T m_internal{};

    Value(T * value) : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::Opt
*
***/

template <typename T>
class Cli::Opt : public OptShim<Opt<T>, T> {
public:
    Opt(std::shared_ptr<Value<T>> value, std::string const & keys);

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // OptBase
    std::string const & from() const final { return m_proxy->m_match.name; }
    int pos() const final { return m_proxy->m_match.pos; }
    size_t size() const final { return 1; }
    void reset() final;
    void unspecifiedValue() final;

private:
    friend class Cli;
    bool fromString(Cli & cli, std::string const & value) final;
    bool defaultValueToString(std::string & out, Cli const & cli) const final;
    void assign(std::string const & name, size_t pos) final;
    bool assigned() const final { return m_proxy->m_explicit; }
    bool sameValue(void const * value) const final {
        return value == m_proxy->m_value;
    }

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
Cli::Opt<T>::Opt(
    std::shared_ptr<Value<T>> value,
    std::string const & keys
)
    : OptShim<Opt, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value}
{}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::fromString(Cli & cli, std::string const & value) {
    auto & tmp = *m_proxy->m_value;
    if (this->m_flagValue) {
        // Value passed for flagValue (just like bools) is generated
        // internally and will be 0 or 1.
        if (value == "1") {
            tmp = this->defaultValue();
        } else {
            assert(value == "0" && "internal dimcli error");
        }
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        tmp = this->m_choices[i->second.pos];
        return true;
    }
    return cli.fromString(tmp, value);
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::defaultValueToString(
    std::string & out,
    Cli const & cli
) const {
    return cli.toString(out, this->defaultValue());
}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::assign(std::string const & name, size_t pos) {
    m_proxy->m_match.name = name;
    m_proxy->m_match.pos = (int)pos;
    m_proxy->m_explicit = true;
}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::reset() {
    if (!this->m_flagValue || this->m_flagDefault)
        *m_proxy->m_value = this->defaultValue();
    m_proxy->m_match.name.clear();
    m_proxy->m_match.pos = 0;
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::unspecifiedValue() {
    *m_proxy->m_value = this->implicitValue();
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T>
struct Cli::ValueVec {
    // Where the values came from.
    std::vector<ArgMatch> m_matches;

    // Points to the opt with the default flag value.
    OptVec<T> * m_defFlagOpt{};

    std::vector<T> * m_values{};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal)
    {}
};


/****************************************************************************
*
*   Cli::OptVec
*
***/

template <typename T>
class Cli::OptVec : public OptShim<OptVec<T>, T> {
public:
    OptVec(
        std::shared_ptr<ValueVec<T>> values,
        std::string const & keys,
        int nargs
    );

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    T & operator[](size_t index) { return (*m_proxy->m_values)[index]; }

    // OptBase
    std::string const & from() const final { return from(size() - 1); }
    int pos() const final { return pos(size() - 1); }
    size_t size() const final { return m_proxy->m_values->size(); }
    void reset() final;
    void unspecifiedValue() final;

    // Information about a specific member of the vector of values at the
    // time it was parsed. If the value vector has been changed (sort, erase,
    // insert, etc) by the app these will no longer correspond.
    std::string const & from(size_t index) const;
    int pos(size_t index) const;

private:
    friend class Cli;
    bool fromString(Cli & cli, std::string const & value) final;
    bool defaultValueToString(std::string & out, Cli const & cli) const final;
    void assign(std::string const & name, size_t pos) final;
    bool assigned() const final { return !m_proxy->m_values->empty(); }
    bool sameValue(void const * value) const final {
        return value == m_proxy->m_values;
    }

    std::shared_ptr<ValueVec<T>> m_proxy;
    std::string m_empty;
};

//===========================================================================
template <typename T>
Cli::OptVec<T>::OptVec(
    std::shared_ptr<ValueVec<T>> values,
    std::string const & keys,
    int nargs
)
    : OptShim<OptVec, T>{keys, std::is_same<T, bool>::value}
    , m_proxy(values)
{
    this->m_vector = true;
    this->m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::fromString(Cli & cli, std::string const & value) {
    m_proxy->m_values->resize(m_proxy->m_values->size() + 1);
    auto & tmp = m_proxy->m_values->back();
    if (this->m_flagValue) {
        // Value passed for flagValue (just like bools) is generated
        // internally and will be 0 or 1.
        if (value == "1") {
            tmp = this->defaultValue();
        } else {
            assert(value == "0" && "internal dimcli error");
            m_proxy->m_values->pop_back();
            m_proxy->m_matches.pop_back();
        }
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        tmp = this->m_choices[i->second.pos];
        return true;
    }

    return cli.fromString(tmp, value);
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::defaultValueToString(
    std::string & out,
    Cli const &
) const {
    out.clear();
    return false;
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::assign(std::string const & name, size_t pos) {
    ArgMatch match;
    match.name = name;
    match.pos = (int)pos;
    m_proxy->m_matches.push_back(match);
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_matches.clear();
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::unspecifiedValue() {
    m_proxy->m_values->push_back(this->implicitValue());
}

//===========================================================================
template <typename T>
std::string const & Cli::OptVec<T>::from(size_t index) const {
    return index >= size() ? m_empty : m_proxy->m_matches[index].name;
}

//===========================================================================
template <typename T>
int Cli::OptVec<T>::pos(size_t index) const {
    return index >= size() ? 0 : m_proxy->m_matches[index].pos;
}

} // namespace


/****************************************************************************
*
*   Restore settings
*
***/

// Restore as many compiler settings as possible so they don't leak into the
// application.
#ifndef DIMCLI_LIB_KEEP_MACROS

// clear all dim header macros so they don't leak into the application
#undef DIMCLI_LIB_DYN_LINK
#undef DIMCLI_LIB_KEEP_MACROS
#undef DIMCLI_LIB_STANDALONE
#undef DIMCLI_LIB_WINAPI_FAMILY_APP

#undef DIMCLI_LIB_DECL
#undef DIMCLI_LIB_NO_ENV
#undef DIMCLI_LIB_NO_CONSOLE
#undef DIMCLI_LIB_SOURCE

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef DIMCLI_LIB_FILESYSTEM
#undef DIMCLI_LIB_FILESYSTEM_PATH

#endif
