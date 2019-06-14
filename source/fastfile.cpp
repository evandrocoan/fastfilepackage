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

// https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
// https://stackoverflow.com/questions/17237545/preprocessor-check-if-multiple-defines-are-not-defined
#if defined(FASTFILE_GETLINE)

    #if FASTFILE_GETLINE == 0
        #undef FASTFILE_GETLINE

    #else
        #if defined(__unix__)
            #define FASTFILE_USE_POSIX_GETLINE

            #if FASTFILE_GETLINE == 1
                #undef FASTFILE_USE_POSIX_GETLINE
            #endif

        #elif defined(FASTFILE_USE_POSIX_GETLINE)
            #undef FASTFILE_USE_POSIX_GETLINE
        #endif
    #endif
#endif


#if !defined(FASTFILE_REGEX) || !defined(FASTFILE_GETLINE) || !defined(FASTFILE_USE_POSIX_GETLINE) || FASTFILE_GETLINE != 2
    #undef FASTFILE_REGEX
    #define FASTFILE_REGEX 0

#else
    #define STANDARDERRORMESSAGE \
            std::cerr << "ERROR: FastFile match failed the following returncode='" \
                    << returncode << "' file='" \
                    << filepath <<  "' line='" << readline << "'!" << std::endl; \

    #define STANDARDERRORMESSAGEDETAILS \
            LOG( 1, "regexresult: %s readline %p charsread %d '%s'", returncode, readline, charsread, readline );

    #if FASTFILE_REGEX == 1
        #include <regex.h>
        #define REGEXMATCHFUNCTION \
                ( ( returncode = regexec( &monsterregex, readline, 0, NULL, 0 ) ) != REG_NOMATCH )

        #define REGEXERRORFUNCTION \
                STANDARDERRORMESSAGEDETAILS

    #elif FASTFILE_REGEX == 2
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

    #elif FASTFILE_REGEX == 3
        #include <re2/re2.h>
        #define REGEXMATCHFUNCTION \
                ( returncode = RE2::PartialMatch( readline, *monsterregex ) )

        #define REGEXERRORFUNCTION \
                STANDARDERRORMESSAGEDETAILS

    #elif FASTFILE_REGEX == 4
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


struct FastFile {
    const char* filepath;

    PyObject* emtpycacheobject;
    std::deque<PyObject*> linecache;

    bool hasclosed;
    bool hasfinished;

#if defined(FASTFILE_GETLINE)
    char* readline;
    size_t linebuffersize;

    #if FASTFILE_REGEX != 0
        bool getnewline;
        bool hasinitializedmonsterregex;

        #if FASTFILE_REGEX == 1
            regex_t monsterregex;

        #elif FASTFILE_REGEX == 2
            pcre2_code* monsterregex;
            pcre2_match_data* unused_match_data;

        #elif FASTFILE_REGEX == 3
            RE2* monsterregex;
            RE2::Options myglobaloptions;

        #elif FASTFILE_REGEX == 4
            hs_scratch_t *scratchspace = NULL;
            hs_database_t *monsterregex;
        #endif
    #endif

    #ifdef FASTFILE_USE_POSIX_GETLINE
        FILE* cfilestream;

    #else
        std::ifstream fileifstream;
    #endif
#else
    PyObject* iomodule;
    PyObject* openfile;
    PyObject* fileiterator;
#endif

    long long int linecount;
    long long int currentline;

    // https://stackoverflow.com/questions/25167543/how-can-i-get-exception-information-after-a-call-to-pyrun-string-returns-nu
    FastFile(const char* filepath, const char* rawregex) :
                filepath(filepath),
                hasclosed(false),
                hasfinished(false),

            #if FASTFILE_REGEX != 0
                getnewline(false),
                hasinitializedmonsterregex(false),
            #endif
                linecount(0),
                currentline(-1)
    {
        LOG( 1, "Constructor with:\nFASTFILE_GETLINE=%s\nFASTFILE_REGEX=%s\nfilepath=%s\nrawregex=%s",
                FASTFILE_GETLINE, FASTFILE_REGEX, filepath, rawregex );

        emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "replace" );
        if( emtpycacheobject == NULL ) {
            std::cerr << "ERROR: FastFile failed to create the empty string object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }

#if defined(FASTFILE_GETLINE)
        linebuffersize = 131072;
        readline = (char*) malloc( linebuffersize );

        if( readline == NULL ) {
            std::cerr << "ERROR: FastFile failed to alocate internal line buffer for '"
                    << filepath << "'!" << std::endl;
            return;
        }

    #ifdef FASTFILE_USE_POSIX_GETLINE

        #if FASTFILE_REGEX == 1
            int rawresultregex = regcomp( &monsterregex, rawregex, REG_NOSUB | REG_EXTENDED );

            if( rawresultregex ) {
                std::cerr << "ERROR: FastFile failed to compile rawregex for '"
                        << filepath << " & " << rawregex << ", error==" << rawresultregex << "'!" << std::endl;
                return;
            }
            else {
                hasinitializedmonsterregex = true;
            }

        #elif FASTFILE_REGEX == 2
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
                    std::cerr << ", error==" << errorcode << ", " << readline;
                }

                std::cerr << ", on position==" << erroffset << "'!" << std::endl;
                return;
            }

        #elif FASTFILE_REGEX == 3
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

        #elif FASTFILE_REGEX == 4
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

        cfilestream = fopen( filepath, "r" );
        if( cfilestream == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
            return;
        }
    #else
        fileifstream.open( filepath );

        if( fileifstream.fail() ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
            return;
        }
    #endif
#else
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

#if defined(FASTFILE_GETLINE)
        if( readline ) {
            free( readline );
            readline = NULL;
        }

    #if FASTFILE_REGEX != 0
        if( hasinitializedmonsterregex ) {
            hasinitializedmonsterregex = false;

        #if FASTFILE_REGEX == 1
            regfree( &monsterregex );

        #elif FASTFILE_REGEX == 2
            pcre2_code_free( monsterregex );

        #elif FASTFILE_REGEX == 3
            delete monsterregex;

        #elif FASTFILE_REGEX == 4
            hs_free_scratch( scratchspace );
            hs_free_database( monsterregex );
        #endif
        }
    #endif

        #ifdef FASTFILE_USE_POSIX_GETLINE
            if( cfilestream != NULL ) {
                fclose( cfilestream );
                cfilestream = NULL;
            }
        #else
            if( fileifstream.is_open() ) {
                fileifstream.close();
            }
        #endif
#else
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
#endif
    }

    void resetlines(int linetoreset=0) {
        currentline = linetoreset;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;

        if( linestoget ) {
        #if defined(FASTFILE_GETLINE)
            const char* cppline;
            unsigned int current = 1;

            for( PyObject* linepy : linecache ) {
                ++current;
                cppline = PyUnicode_AsUTF8( linepy );
                stream << std::string{cppline};

                if( linestoget < current ) {
                    break;
                }
                else {
                    stream << '\n';
                }
            }
        #else
            char* cpplinenonconst;
            const char* cppline;
            Py_ssize_t linesize;
            unsigned int current = 1;

            for( PyObject* linepy : linecache ) {
                ++current;
                cppline = PyUnicode_AsUTF8AndSize( linepy, &linesize );

                if( cppline[linesize-1] == '\n' ) {
                    cpplinenonconst = const_cast<char *>( cppline );
                    cpplinenonconst[linesize-1] = '\0';
                    stream << std::string{cpplinenonconst};
                }
                else {
                    stream << std::string{cppline};
                }

                if( linestoget < current ) {
                    break;
                }
            }
        #endif
        }
        return stream.str();
    }

    // https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
    bool _getline() {
        // Fix StopIteration being raised multiple times because _getlines is called multiple times
        if( hasfinished ) { return false; }

#if defined(FASTFILE_GETLINE)
    #ifdef FASTFILE_USE_POSIX_GETLINE

        #if FASTFILE_REGEX != 0
            int returncode;
        #endif
        ssize_t charsread;

        while( true )
        {
            if( ( charsread = getline( &readline, &linebuffersize, cfilestream ) ) != -1 )
            {
            #if FASTFILE_REGEX != 0
                if( !getnewline || REGEXMATCHFUNCTION )
                {
                    getnewline = false;
            #endif
                    --charsread;
                    linecount += 1;

                    if( readline[charsread] == '\n' ) {
                        readline[charsread] = '\0';
                    }
                    else {
                        ++charsread;
                    }

                    PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, charsread, "replace" );
                    linecache.push_back( pythonobject );

                    // Py_XINCREF( emtpycacheobject );
                    // linecache.push_back( emtpycacheobject );
                    LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, pythonobject, readline );
                    return true;

            #if FASTFILE_REGEX != 0
                }

                REGEXERRORFUNCTION
            #endif
            }
            else {
                break;
            }
        }
    #else
        if( !fileifstream.eof() )
        {
            linecount += 1;
            fileifstream.getline( readline, linebuffersize );

            PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, fileifstream.gcount(), "replace" );
            linecache.push_back( pythonobject );

            // Py_XINCREF( emtpycacheobject );
            // linecache.push_back( emtpycacheobject );
            LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, pythonobject, readline );
            return true;
        }
    #endif
#else
        PyObject* readline = PyObject_CallObject( fileiterator, NULL );

        if( readline != NULL ) {
            linecount += 1;
            LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, readline, PyUnicode_AsUTF8( readline ) );

            linecache.push_back( readline );
            return true;
        }
#endif

        // PyErr_PrintEx(100); // uncomment this to see why this function is stopping
        PyErr_Clear();
        hasfinished = true;
        return false;
    }

    bool next() {
        currentline = -1;

    #if FASTFILE_REGEX != 0
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

    #if FASTFILE_REGEX != 0
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
        #if FASTFILE_REGEX != 0
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
