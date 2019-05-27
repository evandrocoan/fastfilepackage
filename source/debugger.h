/*********************** Licensing *******************************************************
*
*   Copyright 2019 @ Evandro Coan, https://github.com/evandrocoan
*
*  This program is free software; you can redistribute it and/or modify it
*  under the terms of the GNU Lesser General Public License as published by the
*  Free Software Foundation; either version 2.1 of the License, or ( at
*  your option ) any later version.
*
*  This program is distributed in the hope that it will be useful, but
*  WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************************
*/


/* Deprecated the usage of `#pragma once` due:

> #pragma once does have one drawback (other than being non-standard) and that is if you have the
> same file in different locations (we have this because our build system copies files around) then
> the compiler will think these are different files.
>
> Is #pragma once a safe include guard?
> https://stackoverflow.com/a/1946730/4934640
*/
#ifndef FASTFILE_APP_DEBUGGER_INT_DEBUG_LEVEL_H
#define FASTFILE_APP_DEBUGGER_INT_DEBUG_LEVEL_H

// C like printf support on C++
// https://github.com/c42f/tinyformat
#include "tinyformat.h"


/**
 * This is to view internal program data while execution. Default value: 0
 *
 * A value like 127 binary(111111) enables all masked debugging levels.
 *
 *  0     - Disables this feature.
 *  1     - Error or very important messages.
 *  4     - Comment messages inside functions calls
 *  8     - High called functions, i.e., create very big massive text output
 *  1024  - Disable debugging with time stamp.
 */
#if !defined(FASTFILE_DEBUGGER_INT_DEBUG_LEVEL)
  #define FASTFILE_DEBUGGER_INT_DEBUG_LEVEL 0
#endif


#define DEBUG_LEVEL_DISABLED_DEBUG     0
#define DEBUG_LEVEL_WITHOUT_TIME_STAMP 1024

/**
 * Control all program debugging.
 */
#if FASTFILE_DEBUGGER_INT_DEBUG_LEVEL > DEBUG_LEVEL_DISABLED_DEBUG
  #define DEBUG
  #include <iostream>
  #include <chrono>
  #include <ctime>

  // C++ -> Utilities library -> Date and time utilities -> C-style date and time utilities -> std:clock
  // http://en.cppreference.com/w/cpp/chrono/c/clock
  //
  // Measure time in Linux - time vs clock vs getrusage vs clock_gettime vs gettimeofday vs timespec_get?
  // https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof
  extern std::clock_t _debugger_current_saved_c_time;
  extern std::chrono::time_point< std::chrono::high_resolution_clock > _debugger_current_saved_chrono_time;

  #if FASTFILE_DEBUGGER_INT_DEBUG_LEVEL & DEBUG_LEVEL_WITHOUT_TIME_STAMP
    #define _DEBUGGER_TIME_STAMP_HEADER
  #else
    #define _DEBUGGER_TIME_STAMP_HEADER \
      auto duration = chrono_clock_now.time_since_epoch(); \
      /* typedef std::chrono::duration< int, std::ratio_multiply< std::chrono::hours::period, std::ratio< 21 > >::type > Days; */ \
      /* Days days = std::chrono::duration_cast< Days >( duration ); */ \
      /* duration -= days; */ \
      auto hours = std::chrono::duration_cast< std::chrono::hours >( duration ); \
      duration -= hours; \
      auto minutes = std::chrono::duration_cast< std::chrono::minutes >( duration ); \
      duration -= minutes; \
      auto seconds = std::chrono::duration_cast< std::chrono::seconds >( duration ); \
      duration -= seconds; \
      auto milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( duration ); \
      duration -= milliseconds; \
      auto microseconds = std::chrono::duration_cast< std::chrono::microseconds >( duration ); \
      /* duration -= microseconds; */ \
      /* auto nanoseconds = std::chrono::duration_cast< std::chrono::nanoseconds >( duration ); */ \
      time_t theTime = time(NULL); \
      struct tm* aTime = localtime(&theTime); \
      std::cout << tfm::format( "%02d:%02d:%02d:%03d:%03d %.3e ", /* %.3e */ \
          aTime->tm_hour, minutes.count(), seconds.count(), milliseconds.count(), microseconds.count(), /* nanoseconds.count(), */ \
          std::chrono::duration<double, std::milli>(chrono_clock_now-_debugger_current_saved_chrono_time).count() \
          /* (1000.0 * (ctime_clock_now - _debugger_current_saved_c_time)) / CLOCKS_PER_SEC */ \
      );
  #endif

  // https://stackoverflow.com/questions/1706346/file-macro-manipulation-handling-at-compile-time/
  constexpr const char * const fastfile_debugger_strend(const char * const str) {
      return *str ? fastfile_debugger_strend(str + 1) : str;
  }

  constexpr const char * const fastfile_debugger_fromlastslash(const char * const start, const char * const end) {
      return (end >= start && *end != '/' && *end != '\\') ? fastfile_debugger_fromlastslash(start, end - 1) : (end + 1);
  }

  constexpr const char * const fastfile_debugger_pathlast(const char * const path) {
      return fastfile_debugger_fromlastslash(path, fastfile_debugger_strend(path));
  }

  #define _DEBUGGER_TIME_FILE_PATH_HEADER \
    std::cout << tfm::format( "%s|%s:%s ", \
        fastfile_debugger_pathlast( __FILE__ ), __FUNCTION__, __LINE__ );

  /**
   * Print like function for logging putting a new line at the end of string. See the variables
   * 'FASTFILE_DEBUGGER_INT_DEBUG_LEVEL' for the available levels.
   *
   * On this function only, a time stamp on scientific notation as `d.dde+ddd d.ddde+ddd` will be
   * used. These values mean the `CPU time used` in milliseconds and the `Wall clock time passed`
   * respectively.
   *
   * @param level     the debugging desired level to be printed.
   * @param ...       variable number os formating arguments parameters.
   */
  #define LOG( level, ... ) \
  do \
  { \
    if( level & FASTFILE_DEBUGGER_INT_DEBUG_LEVEL ) \
    { \
      /* std::clock_t ctime_clock_now = std::clock(); */ \
      auto chrono_clock_now = std::chrono::high_resolution_clock::now(); \
      _DEBUGGER_TIME_STAMP_HEADER \
      _DEBUGGER_TIME_FILE_PATH_HEADER \
      std::cout << tfm::format( __VA_ARGS__ ) << std::endl; \
      /* _debugger_current_saved_c_time = ctime_clock_now; */ \
      _debugger_current_saved_chrono_time = chrono_clock_now; \
    } \
  } \
  while( 0 )

  /**
   * The same as LOG(...) just above, but do not put automatically a new line.
   */
  #define LOGLN( level, ... ) \
  do \
  { \
    if( level & FASTFILE_DEBUGGER_INT_DEBUG_LEVEL ) \
    { \
      /* std::clock_t ctime_clock_now = std::clock(); */ \
      auto chrono_clock_now = std::chrono::high_resolution_clock::now(); \
      _DEBUGGER_TIME_STAMP_HEADER \
      _DEBUGGER_TIME_FILE_PATH_HEADER \
      std::cout << tfm::format( __VA_ARGS__ ); \
      /* _debugger_current_saved_c_time = ctime_clock_now; */ \
      _debugger_current_saved_chrono_time = chrono_clock_now; \
    } \
  } \
  while( 0 )

  /**
   * The same as LOGLC(...) just above, but do not put automatically a new line, neither time stamp.
   */
  #define LOGLC( level, ... ) \
  do \
  { \
    if( level & FASTFILE_DEBUGGER_INT_DEBUG_LEVEL ) \
    { \
      std::cout << tfm::format( __VA_ARGS__ ) << std::flush; \
    } \
  } \
  while( 0 )

  /**
   * The same a LOG, but used to hide a whole code block behind the debug level.
   */
  #define LOGCD( level, code ) \
  do \
  { \
    if( level & FASTFILE_DEBUGGER_INT_DEBUG_LEVEL ) \
    { \
      code; \
    } \
  } \
  while( 0 )

  /**
   * The same as LOG(...), but it is for standard program output.
   */
  #define PRINT( level, ... ) \
  do \
  { \
    if( level & FASTFILE_DEBUGGER_INT_DEBUG_LEVEL ) \
    { \
      std::cout << tfm::format( __VA_ARGS__ ) << std::endl; \
    } \
  } \
  while( 0 )

  /**
   * The same as LOGLN(...), but it is for standard program output.
   */
  #define PRINTLN( level, ... ) \
  do \
  { \
    if( level & FASTFILE_DEBUGGER_INT_DEBUG_LEVEL ) \
    { \
      std::cout << tfm::format( __VA_ARGS__ ) << std::flush; \
    } \
  } \
  while( 0 )


#else

  #define LOG( level, ... )
  #define LOGLN( level, ... )
  #define LOGCD( level, ... )

  /**
   * The same as LOG(...), but it is for standard program output when the debugging is disabled.
   */
  #define PRINT( level, ... ) \
  do \
  { \
    std::cout << tfm::format( __VA_ARGS__ ) << std::endl; \
  } \
  while( 0 )

  /**
   * The same as LOGLN(...), but it is for standard program output when the debugging is disabled.
   */
  #define PRINTLN( level, ... ) \
  do \
  { \
    std::cout << tfm::format( __VA_ARGS__ ) << std::flush; \
  } \
  while( 0 )


#endif // #if FASTFILE_DEBUGGER_INT_DEBUG_LEVEL > DEBUG_LEVEL_DISABLED_DEBUG


#endif // FASTFILE_APP_DEBUGGER_INT_DEBUG_LEVEL_H


