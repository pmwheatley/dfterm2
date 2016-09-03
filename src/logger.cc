#include <wchar.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include "logger.hpp"
#include "boost/thread/thread.hpp"
#include "unicode/unistr.h"
#include "unicode/ustring.h"

using namespace dfterm;
using namespace std;
using namespace boost;
using namespace trankesbel;

/* Every message goes here. */
namespace dfterm {
SP<Logger> admin_logger;
SP<LoggerReader> admin_messages_reader;

UnicodeString log_file; /* if not empty, logs to this file */
};

void dfterm::initialize_logger()
{
    if (admin_logger) return;

    admin_logger = SP<Logger>(new Logger);
    admin_messages_reader = admin_logger->createReader();
}

/* Flush admin messages */
void dfterm::flush_messages()
{
    assert(admin_logger && admin_messages_reader);

    bool msg;
    do
    {
        UnicodeString us = admin_messages_reader->getLogMessage(&msg);
        if (!msg) break;

        ::int32_t src_len = us.length();
        UChar* us_str = us.getBuffer(src_len);
        ::int32_t dest_len = 0;
        UErrorCode uerror = U_ZERO_ERROR;
        u_strToWCS(NULL, 0, &dest_len, us_str, src_len, &uerror);
        if (U_FAILURE(uerror) && uerror != U_BUFFER_OVERFLOW_ERROR)
        {
            cout << "Failure in converting log message to wchar_t. (pre-flighting) " << u_errorName(uerror) << endl;
            us.releaseBuffer();
            continue;
        }
        uerror = U_ZERO_ERROR;

        wchar_t* dest_str = new wchar_t[dest_len+1];
        memset(dest_str, 0, sizeof(wchar_t) * (dest_len+1));
        u_strToWCS(dest_str, dest_len, &dest_len, us_str, src_len, &uerror);
        us.releaseBuffer();

        if (U_FAILURE(uerror))
        {
            cout << "Failure in converting log message to wchar_t." << u_errorName(uerror) << endl;
            delete[] dest_str;
            continue;
        }

        /* Sanizite dest_str (to some point) 
         * There are more characters that should be sanitized (see Unicode
         * control characters or Firefox blacklist (google it)) but at the
         * moment we aren't certain what encoding is actually used in the
         * terminal. */
        for (size_t i1 = 0; dest_str[i1]; ++i1)
            if (dest_str[i1] < 32) /* control characters */
                dest_str[i1] = '?';

        std::string utf8_str = TO_UTF8(us);
        for (size_t i1 = 0; i1 < utf8_str.size(); ++i1)
            if (utf8_str[i1] < 32)
                utf8_str[i1] = '?';

        /* wcout not supported by mingw, so we use wprintf, but only do it
           on Windows. */
        #ifdef _WIN32
        wprintf(L"%ls\n", dest_str);
        #else
        cout << utf8_str << "\n";
        #endif
        fflush(stdout);
        delete[] dest_str;

        if (log_file.length() > 0)
        {
            wchar_t* wc = (wchar_t*) 0;
            size_t wc_size = 0;
            TO_WCHAR_STRING(log_file, wc, &wc_size);
            if (wc_size == 0)
            {
                wprintf(L"There is something wrong with the log file string. Could not save previous line to the log.\n");
                continue;
            }
            wc = new wchar_t[wc_size+1];
            memset(wc, 0, (wc_size+1) * sizeof(wchar_t));

            TO_WCHAR_STRING(log_file, wc, &wc_size);
            errno = 0;
            #ifdef _WIN32
            FILE* f = _wfopen(wc, L"at");
            #else
            FILE* f = fopen(TO_UTF8(log_file).c_str(), "at");
            #endif
            if (!f)
            {
                int err = errno;
                wprintf(L"Could not open log file %ls, errno %d.\n", wc, err);
                delete[] wc;
                continue;
            }
            if (fprintf(f, "%s\n", TO_UTF8(us).c_str()) < 0)
            {
                int err = errno;
                wprintf(L"Error when trying to write to log file %ls with fprintf(), errno %d.\n", wc, err);
                delete[] wc;
                fclose(f);
                continue;
            }
            fclose(f);
            delete[] wc;
        }
        
    } while(msg);
}

void Logger::logMessage(const UnicodeString &message)
{
    lock_guard<recursive_mutex> lock(logmutex);
    vector<WP<LoggerReader> >::iterator i1, readers_end = readers.end();
    for (i1 = readers.begin(); i1 != readers_end; ++i1)
    {
        SP<LoggerReader> reader = (*i1).lock();
        if (!reader)
            continue;
        reader->logMessage(message);
    }
    /* Remove null readers */
    bool repeat = true;
    while (repeat)
    {
        repeat = false;
        for (i1 = readers.begin(); i1 != readers_end; ++i1)
        {
            SP<LoggerReader> reader = (*i1).lock();
            if (reader) continue;

            readers.erase(i1);
            readers_end = readers.end();
            repeat = true;
            break;
        }
    }
}

void Logger::logMessageUTF8(const string &message)
{
    logMessage(UnicodeString::fromUTF8(message));
}

void Logger::logMessageUTF8(const stringstream &message)
{
    logMessage(UnicodeString::fromUTF8(message.str()));
}

void Logger::logMessageUTF8(const char* message, size_t len)
{
    logMessage(UnicodeString::fromUTF8(string(message, len)));
};

void Logger::logMessageUTF8(const char* message)
{
    logMessage(UnicodeString::fromUTF8(string(message)));
}

SP<LoggerReader> Logger::createReader()
{
    SP<LoggerReader> lr(new LoggerReader);
    readers.push_back(lr);
    return lr;
}

void LoggerReader::logMessage(UnicodeString message)
{
    lock_guard<recursive_mutex> lock(logmutex);
    messages.push(message);
}

UnicodeString LoggerReader::getLogMessage(bool* got_message)
{
    assert(got_message);

    lock_guard<recursive_mutex> lock(logmutex);
    (*got_message) = false;
    if (messages.empty()) return UnicodeString("");
    (*got_message) = true;
    UnicodeString us = messages.front();
    messages.pop();
    return us;
};

