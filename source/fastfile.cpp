//https://docs.python.org/3/c-api/intro.html#include-files
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "debugger.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

#define FASTFILE_GETLINE_DISABLED     0
#define FASTFILE_GETLINE_STDGETLINE   1
#define FASTFILE_GETLINE_POSIXGETLINE 2

#if !defined(FASTFILE_GETLINE)
    #define FASTFILE_GETLINE 0
#endif

// https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
// https://stackoverflow.com/questions/17237545/preprocessor-check-if-multiple-defines-are-not-defined
#if !defined(__unix__)
    #if FASTFILE_GETLINE == FASTFILE_GETLINE_POSIXGETLINE
        #undef FASTFILE_GETLINE
        #define FASTFILE_GETLINE 0
    #endif
#endif


#define FASTFILE_UTF8CHARACTER_TRIMMING
#define FASTFILE_TRIMUFT8_DISABLED      0
#define FASTFILE_TRIMUFT8_PRINTABLEONLY 1

#if !defined(FASTFILE_TRIMUFT8)
    #define FASTFILE_TRIMUFT8 FASTFILE_TRIMUFT8_PRINTABLEONLY
#endif

#if FASTFILE_TRIMUFT8 == FASTFILE_TRIMUFT8_DISABLED
    #define FASTFILE_ISTRIM_UFT8_DISABLED(value) value
    #define FASTFILE_ISTRIM_UFT8_ENABLED(value)

#else
    #define FASTFILE_ISTRIM_UFT8_ENABLED(value) value
    #define FASTFILE_ISTRIM_UFT8_DISABLED(value)

#endif

#define FASTFILE_NEWLINETRIMMING \
    --charsread; \
    if( readline[charsread] == '\n' ) { \
        readline[charsread] = '\0'; \
    } \
    else { \
        ++charsread; \
        readline[charsread] = '\0'; \
    }

#if FASTFILE_TRIMUFT8 == FASTFILE_TRIMUFT8_PRINTABLEONLY
    #undef FASTFILE_UTF8CHARACTER_TRIMMING

    // https://stackoverflow.com/questions/56604934/how-to-remove-the-uft8-character-from-a-char-string
    #define FASTFILE_UTF8CHARACTER_TRIMMING \
        lineend = readline + charsread; \
        destination = readline; \
        for( source = readline; source != lineend; ++source ) \
        { \
            fixedchar = static_cast<unsigned int>( *source ); \
            if( 31 < fixedchar && fixedchar < 128 ) { \
                *destination = *source; \
                ++destination; \
            } \
            else { \
                --charsread; \
            } \
        }
#endif


#define FASTFILE_REGEX_DISABLED  0
#define FASTFILE_REGEX_C_ENGINE  1
#define FASTFILE_REGEX_PCRE2     2
#define FASTFILE_REGEX_RE2       3
#define FASTFILE_REGEX_HYPERSCAN 4

#if !defined(FASTFILE_REGEX) || FASTFILE_GETLINE != 2
    #undef FASTFILE_REGEX
    #define FASTFILE_REGEX 0

#else
    #define STANDARDERRORMESSAGE \
            std::cerr << "ERROR: FastFile match failed the following returncode='" \
                    << returncode << "' file='" \
                    << filepath <<  "' line='" << readline << "'!" << std::endl; \

    #define STANDARDERRORMESSAGEDETAILS \
            LOG( 1, "regexresult: %s readline %p charsread %d '%s'", returncode, readline, charsread, readline );

    #if FASTFILE_REGEX == FASTFILE_REGEX_C_ENGINE
        #include <regex.h>
        #define REGEXMATCHFUNCTION \
                ( ( returncode = regexec( &monsterregex, readline, 0, NULL, 0 ) ) != REG_NOMATCH )

        #define REGEXERRORFUNCTION \
                STANDARDERRORMESSAGEDETAILS

    #elif FASTFILE_REGEX == FASTFILE_REGEX_PCRE2
        #define PCRE2_CODE_UNIT_WIDTH 8
        #include <pcre2.h>
        #define REGEXMATCHFUNCTION \
                ( ( returncode = pcre2_match( monsterregex, reinterpret_cast<PCRE2_SPTR>( readline ), \
                        PCRE2_ZERO_TERMINATED, 0, PCRE2_NO_UTF_CHECK, unused_match_data, NULL ) ) > -1 )

        #define REGEXERRORFUNCTION \
                if( returncode < -2 ) { \
                    STANDARDERRORMESSAGE \
                } \
                STANDARDERRORMESSAGEDETAILS

    #elif FASTFILE_REGEX == FASTFILE_REGEX_RE2
        #include <re2/re2.h>
        #define REGEXMATCHFUNCTION \
                ( returncode = RE2::PartialMatch( readline, *monsterregex ) )

        #define REGEXERRORFUNCTION \
                STANDARDERRORMESSAGEDETAILS

    #elif FASTFILE_REGEX == FASTFILE_REGEX_HYPERSCAN
        #include <hs.h>
        #define REGEXMATCHFUNCTION \
                ( ( returncode = hs_scan( \
                        monsterregex, readline, charsread, 0, scratchspace, null_onEvent, NULL \
                        ) ) == HS_SCAN_TERMINATED )

        #define REGEXERRORFUNCTION \
                if( returncode < 0 ) { \
                    STANDARDERRORMESSAGE \
                } \
                STANDARDERRORMESSAGEDETAILS

        static int HS_CDECL null_onEvent(
                unsigned id,
                unsigned long long from,
                unsigned long long to,
                unsigned flags,
                void *ctxt)
        {
            return 1;
        }
    #endif
#endif


#if FASTFILE_TRIMUFT8 < 0 || FASTFILE_TRIMUFT8 > 1
    #error The FASTFILE_TRIMUFT8 define must to be between 0 and 1!
    #undef FASTFILE_TRIMUFT8
    #define FASTFILE_TRIMUFT8 0
#endif

#if FASTFILE_GETLINE < 0 || FASTFILE_GETLINE > 2
    #error The FASTFILE_GETLINE define must to be between 0 and 2!
    #undef FASTFILE_GETLINE
    #define FASTFILE_GETLINE 0
#endif

#if FASTFILE_REGEX < 0 || FASTFILE_REGEX > 4
    #error The FASTFILE_REGEX define must to be between 0 and 4!
    #undef FASTFILE_REGEX
    #define FASTFILE_REGEX 0
#endif


struct FastFile {
    const char* filepath;

    PyObject* emtpycacheobject;
    std::deque<PyObject*> linecache;

    bool hasclosed;
    bool hasfinished;

    char* readline;
    size_t linebuffersize;

#if FASTFILE_GETLINE == FASTFILE_GETLINE_DISABLED
    PyObject* iomodule;
    PyObject* openfile;
    PyObject* fileiterator;
#else

    #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
        bool getnewline;
        bool hasinitializedmonsterregex;

        #if FASTFILE_REGEX == FASTFILE_REGEX_C_ENGINE
            regex_t monsterregex;

        #elif FASTFILE_REGEX == FASTFILE_REGEX_PCRE2
            pcre2_code* monsterregex;
            pcre2_match_data* unused_match_data;

        #elif FASTFILE_REGEX == FASTFILE_REGEX_RE2
            RE2* monsterregex;
            RE2::Options myglobaloptions;

        #elif FASTFILE_REGEX == FASTFILE_REGEX_HYPERSCAN
            hs_scratch_t *scratchspace = NULL;
            hs_database_t *monsterregex;
        #endif
    #endif

    #if FASTFILE_GETLINE == FASTFILE_GETLINE_POSIXGETLINE
        FILE* cfilestream;

    #elif FASTFILE_GETLINE == FASTFILE_GETLINE_STDGETLINE
        std::ifstream fileifstream;
    #endif
#endif

    long long int linecount;
    long long int currentline;

    // https://stackoverflow.com/questions/25167543/how-can-i-get-exception-information-after-a-call-to-pyrun-string-returns-nu
    FastFile(const char* filepath, const char* rawregex) :
                filepath(filepath),
                hasclosed(false),
                hasfinished(false),

            #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
                getnewline(false),
                hasinitializedmonsterregex(false),
            #endif
                linecount(0),
                currentline(-1)
    {
        LOG( 1, "Constructor with:\nFASTFILE_GETLINE=%s\nFASTFILE_REGEX=%s\nFASTFILE_TRIMUFT8=%s\nfilepath=%s\nrawregex=%s",
                FASTFILE_GETLINE, FASTFILE_REGEX, FASTFILE_TRIMUFT8, filepath, rawregex );

        emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "ignore" );
        if( emtpycacheobject == NULL ) {
            std::cerr << "ERROR: FastFile failed to create the empty string object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }

        linebuffersize = 131072;
        readline = (char*) malloc( linebuffersize );

        if( readline == NULL ) {
            std::cerr << "ERROR: FastFile failed to alocate internal line buffer for readline '"
                    << filepath << "'!" << std::endl;
            return;
        }

    #if FASTFILE_GETLINE == FASTFILE_GETLINE_DISABLED
        // https://stackoverflow.com/questions/47054623/using-python3-c-api-to-add-to-builtins
        iomodule = PyImport_ImportModule( "builtins" );

        if( iomodule == NULL ) {
            std::cerr << "ERROR: FastFile failed to import the io module (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* openfunction = PyObject_GetAttrString( iomodule, "open" );

        if( openfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module open function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        // https://stackoverflow.com/questions/56482802/c-python-api-extensions-is-ignoring-openerrors-ignore-and-keeps-throwing-the
        openfile = PyObject_CallFunction( openfunction, "ssiss", filepath, "r", -1, "UTF8", "ignore" );

        if( openfile == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file'"
                    << filepath << "'!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* iterfunction = PyObject_GetAttrString( openfile, "__iter__" );
        Py_DECREF( openfunction );

        if( iterfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* openiteratorobject = PyObject_CallObject( iterfunction, NULL );
        Py_DECREF( iterfunction );

        if( openiteratorobject == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        fileiterator = PyObject_GetAttrString( openfile, "__next__" );
        Py_DECREF( openiteratorobject );

        if( fileiterator == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
    #else

        #if FASTFILE_GETLINE == FASTFILE_GETLINE_POSIXGETLINE
            cfilestream = fopen( filepath, "r" );
            if( cfilestream == NULL ) {
                std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
                return;
            }

            #if FASTFILE_REGEX == FASTFILE_REGEX_C_ENGINE
                int rawresultregex = regcomp( &monsterregex, rawregex, REG_NOSUB | REG_EXTENDED );

                if( rawresultregex ) {
                    std::cerr << "ERROR: FastFile failed to compile rawregex for '"
                            << filepath << " & " << rawregex << ", error==" << rawresultregex << "'!" << std::endl;
                    return;
                }
                else {
                    hasinitializedmonsterregex = true;
                }

            #elif FASTFILE_REGEX == FASTFILE_REGEX_PCRE2
                int errorcode;
                PCRE2_SIZE erroffset;

                unused_match_data = pcre2_match_data_create( 1, NULL );
                monsterregex = pcre2_compile( reinterpret_cast<PCRE2_SPTR>( rawregex ),
                        PCRE2_ZERO_TERMINATED, PCRE2_UTF, &errorcode, &erroffset, NULL );

                if( monsterregex == NULL ) {
                    hasinitializedmonsterregex = true;
                    PCRE2_UCHAR8 errorbuffer[1024];
                    int errormessageresult = pcre2_get_error_message( errorcode, errorbuffer, sizeof( errorbuffer ) );

                    std::cerr << "ERROR: FastFile failed to compile rawregex for '"
                            << filepath << " & " << rawregex;

                    if( errormessageresult < 1 ) {
                            std::cerr << ", error==" << errorcode;
                    }
                    else {
                        std::cerr << ", error==" << errorcode << ", " << errorbuffer;
                    }

                    std::cerr << ", on position==" << erroffset << "'!" << std::endl;
                    return;
                }

            #elif FASTFILE_REGEX == FASTFILE_REGEX_RE2
                // myglobaloptions.set_posix_syntax(true);
                monsterregex = new RE2(rawregex, myglobaloptions);

                if( monsterregex->ok() ) {
                    hasinitializedmonsterregex = true;
                }
                else {
                    std::cerr << "ERROR: FastFile failed to compile rawregex for '"
                            << filepath << " & " << rawregex
                            << ", error==" << monsterregex->error() << "'!" << std::endl;
                    return;
                }

            #elif FASTFILE_REGEX == FASTFILE_REGEX_HYPERSCAN
                hs_compile_error_t *compile_err;

                if( hs_compile( rawregex, HS_FLAG_SINGLEMATCH | HS_FLAG_DOTALL, HS_MODE_BLOCK, NULL, &monsterregex,
                               &compile_err ) != HS_SUCCESS )
                {
                    std::cerr << "ERROR: FastFile failed to compile rawregex for '"
                            << filepath << " & " << rawregex
                            << ", error==" << compile_err->message << "'!" << std::endl;
                    hs_free_compile_error( compile_err );
                    return;
                }

                if( hs_alloc_scratch( monsterregex, &scratchspace ) != HS_SUCCESS ) {
                    std::cerr << "ERROR: FastFile failed to allocate scratch space for '"
                            << filepath << " & " << rawregex << "'!" << std::endl;
                    return;
                }

                hasinitializedmonsterregex = true;
            #endif

        #elif FASTFILE_GETLINE == FASTFILE_GETLINE_STDGETLINE
            fileifstream.open( filepath );

            if( fileifstream.fail() ) {
                std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
                return;
            }
        #endif
    #endif
    }

    ~FastFile() {
        LOG( 1, "Destructor linecount %llu currentline %llu hasclosed %d", linecount, currentline, hasclosed );
        if( hasclosed ) {
            return;
        }
        this->close();
    }

    void close() {
        LOG( 1, "linecount %llu currentline %llu hasclosed %d", linecount, currentline, hasclosed );
        if( hasclosed ) {
            return;
        }

        hasclosed = true;
        Py_XDECREF( emtpycacheobject );

        for( PyObject* pyobject : linecache ) {
            Py_DECREF( pyobject );
        }

        if( readline ) {
            free( readline );
            readline = NULL;
        }

    #if FASTFILE_GETLINE == FASTFILE_GETLINE_DISABLED
        PyObject* closefunction = PyObject_GetAttrString( openfile, "close" );

        if( closefunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the close file function for '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }

        PyObject* closefileresult = PyObject_CallObject( closefunction, NULL );
        Py_DECREF( closefunction );

        if( closefileresult == NULL ) {
            std::cerr << "ERROR: FastFile failed close open file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        Py_DECREF( closefileresult );

        Py_XDECREF( iomodule );
        Py_XDECREF( openfile );
        Py_XDECREF( fileiterator );

    #else

        #if FASTFILE_GETLINE == FASTFILE_GETLINE_POSIXGETLINE
            if( cfilestream != NULL ) {
                fclose( cfilestream );
                cfilestream = NULL;
            }
        #elif FASTFILE_GETLINE == FASTFILE_GETLINE_STDGETLINE
            if( fileifstream.is_open() ) {
                fileifstream.close();
            }
        #endif

        #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
            if( hasinitializedmonsterregex ) {
                hasinitializedmonsterregex = false;

            #if FASTFILE_REGEX == FASTFILE_REGEX_C_ENGINE
                regfree( &monsterregex );

            #elif FASTFILE_REGEX == FASTFILE_REGEX_PCRE2
                pcre2_code_free( monsterregex );

            #elif FASTFILE_REGEX == FASTFILE_REGEX_RE2
                delete monsterregex;

            #elif FASTFILE_REGEX == FASTFILE_REGEX_HYPERSCAN
                hs_free_scratch( scratchspace );
                hs_free_database( monsterregex );
            #endif
            }
        #endif
    #endif
    }

    void resetlines(int linetoreset=0) {
        currentline = linetoreset;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;

        if( linestoget ) {
            Py_ssize_t linesize;
            const char* cppline;
            unsigned int current = 1;

            for( PyObject* linepy : linecache ) {
                ++current;
                cppline = PyUnicode_AsUTF8AndSize( linepy, &linesize );
                stream << std::string{cppline};

                if( linestoget < current ) {
                    if( cppline[linesize-1] == '\n' ) {
                        stream.seekp( -1, std::ios_base::end );
                        stream << " ";
                    }
                    break;
                }
            // lines comming will not have a ending new line character
            #if FASTFILE_GETLINE == FASTFILE_GETLINE_DISABLED
                else {
                    stream << '\n';
                }
            #endif
            }
        }
        return stream.str();
    }

    // https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
    bool _getline() {
        // Fix StopIteration being raised multiple times because _getlines is called multiple times
        if( hasfinished ) { return false; }
        ssize_t charsread;

    #if FASTFILE_TRIMUFT8 == FASTFILE_TRIMUFT8_PRINTABLEONLY
        char* source;
        char* lineend;
        char* destination;
        unsigned int fixedchar;
    #endif

    #if FASTFILE_GETLINE == FASTFILE_GETLINE_POSIXGETLINE

        #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
            int returncode;
        #endif

        while( true ) {
            if( ( charsread = getline( &readline, &linebuffersize, cfilestream ) ) != -1 )
            {
                FASTFILE_UTF8CHARACTER_TRIMMING
                FASTFILE_ISTRIM_UFT8_ENABLED( FASTFILE_NEWLINETRIMMING )

            #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
                if( !getnewline || REGEXMATCHFUNCTION )
                {
                    getnewline = false;
            #endif
                    FASTFILE_ISTRIM_UFT8_DISABLED( FASTFILE_NEWLINETRIMMING )
                    ++linecount;

                    PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, charsread, "ignore" );
                    linecache.push_back( pythonobject );

                    // Py_XINCREF( emtpycacheobject );
                    // linecache.push_back( emtpycacheobject );
                    LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, pythonobject, readline );
                    return true;

            #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
                }

                REGEXERRORFUNCTION
            #endif
            }
            else {
                break;
            }
        }
    #elif FASTFILE_GETLINE == FASTFILE_GETLINE_STDGETLINE
        if( !fileifstream.eof() )
        {
            linecount += 1;
            fileifstream.getline( readline, linebuffersize );
            charsread = fileifstream.gcount();

            FASTFILE_UTF8CHARACTER_TRIMMING
            readline[charsread] = '\0';

            PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, charsread, "ignore" );
            linecache.push_back( pythonobject );

            // Py_XINCREF( emtpycacheobject );
            // linecache.push_back( emtpycacheobject );
            LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, pythonobject, readline );
            return true;
        }
    #elif FASTFILE_GETLINE == FASTFILE_GETLINE_DISABLED
        PyObject* readpyline = PyObject_CallObject( fileiterator, NULL );

        if( readpyline != NULL ) {
            linecount += 1;

            // we cannot modify a python string! Then, to remove a trailling new line, we must to
            // create a new python string without the trailling new line!
        #if FASTFILE_TRIMUFT8 == FASTFILE_TRIMUFT8_PRINTABLEONLY
            char* cppreadline = PyUnicode_AsUTF8AndSize( readpyline, &charsread );

            if( charsread > static_cast<long long int>( linebuffersize ) ) {
                charsread = linebuffersize - 1;
            }
            lineend = cppreadline + charsread;
            destination = readline;
            for( source = cppreadline; source != lineend; ++source )
            {
                fixedchar = static_cast<unsigned int>( *source );
                if( 31 < fixedchar && fixedchar < 128 ) {
                    *destination = *source;
                    ++destination;
                }
                else {
                    --charsread;
                }
            }
        #else
            char* readline = PyUnicode_AsUTF8AndSize( readpyline, &charsread );
        #endif
            --charsread;
            if( readline[charsread] == '\n' ) {
            }
            else {
                ++charsread;
            }
            PyObject* newpyline = PyUnicode_DecodeUTF8( readline, charsread, "ignore" );

            LOG( 1, "linecount %llu currentline %llu newpyline '%p' '%s'",
                    linecount, currentline, readpyline, PyUnicode_AsUTF8( newpyline ) );

            Py_DECREF( readpyline );
            linecache.push_back( newpyline );
            return true;
        }
        // PyErr_PrintEx(100); // uncomment this to see why this function is stopping
        PyErr_Clear();
    #endif

        hasfinished = true;
        return false;
    }

    bool next() {
        currentline = -1;

    #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
        getnewline = true;
    #endif
        if( linecache.size() ) {
            Py_DECREF( linecache[0] );
            linecache.pop_front();
            return true;
        }
        bool hasnextline = _getline();

        LOG( 1, "hasnextline: %d linecount %llu currentline %llu", hasnextline, linecount, currentline );
        return hasnextline;
    }

    PyObject* call()
    {
        currentline += 1;
        LOG( 1, "linecache.size %zd linecount %llu currentline %llu", linecache.size(), linecount, currentline );

    #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
        if( getnewline ) {
            Py_ssize_t charsread;
            long int popfrontcount = 0;

            int returncode;
            const char* readline;

            for( PyObject* pyobject : linecache ) {
                readline = PyUnicode_AsUTF8AndSize( pyobject, &charsread );

                if( REGEXMATCHFUNCTION ) {
                    break;
                }

                REGEXERRORFUNCTION
                ++popfrontcount;
            }

            while( popfrontcount-- ) {
                Py_DECREF( linecache[0] );
                linecache.pop_front();
            }
        }
    #endif

        if( currentline < static_cast<long long int>( linecache.size() ) )
        {
        #if FASTFILE_REGEX != FASTFILE_REGEX_DISABLED
            getnewline = false;
        #endif
            return linecache[currentline];
        }
        else
        {
            if( !_getline() )
            {
                LOG( 1, "Raising StopIteration" );
                return emtpycacheobject;
            }
        }
        LOGCD( 1, std::ostringstream contents; for( auto value : linecache ) contents << PyUnicode_AsUTF8( value ); LOG( 1, "contents %s**\n**linecache.size %zd linecount %llu currentline %llu (%p)", contents.str().c_str(), linecache.size(), linecount, currentline, linecache[currentline] ) );
        return linecache[currentline];
    }
};
