#include "lua_configuration.hpp"
#include "types.hpp"
#include "dfterm2_limits.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <errno.h>
#include "lua.hpp"

using namespace dfterm;
using namespace std;
using namespace trankesbel;

static int getenv_unicode(lua_State* l)
{
    const char* str = luaL_checkstring(l, 1);
    #ifdef _WIN32
    wchar_t* wc = (wchar_t*) 0;
    size_t wc_size = 0;
    TO_WCHAR_STRING(string(str), wc, &wc_size);
    if (wc_size == 0)
        luaL_error(l, "Environment variable name string is corrupted.");
    wc = new wchar_t[wc_size+1];
    memset(wc, 0, sizeof(wchar_t)+1);
    TO_WCHAR_STRING(string(str), wc, &wc_size);

    wchar_t* result = _wgetenv(wc);
    delete[] wc;
    if (!result)
    {
        lua_pushnil(l);
        return 1;
    }
    string result2 = TO_UTF8(result);
    lua_pushlstring(l, result2.c_str(), result2.size());
    return 1;
    #else
    char* result = getenv(str);
    if (!result)
    {
        lua_pushnil(l);
        return 1;
    }
    lua_pushstring(l, result);
    return 1;
    #endif
}

static void readAddress(lua_State* l,
                        vector<AddressSettings32>* settings)
{
    AddressSettings32 as;

    if (!lua_istable(l, -1))
        return;

    lua_getfield(l, -1, "name");
    if (!lua_isstring(l, -1))
    {
        printf("Invalid address entry: \"name\" doesn't exist or is "
               "not a string.\n");
        lua_pop(l, 1);
        return;
    }

    as.name = string(lua_tostring(l, -1));
    lua_pop(l, 1);

    lua_getfield(l, -1, "size_address");
    if (!lua_isnumber(l, -1))
    {
        printf("Invalid address entry: \"size_address\" doesn't exist or is "
               "not a number.\n");
        lua_pop(l, 1);
        return;
    }

    as.size_address = (uint32_t) lua_tonumber(l, -1);
    lua_pop(l, 1);

    lua_getfield(l, -1, "checksum");
    if (!lua_isnumber(l, -1))
    {
        printf("Invalid address entry: \"checksum\" doesn't exist "
               "or is not a number.\n");
        lua_pop(l, 1);
        return;
    }

    as.checksum = (uint32_t) lua_tonumber(l, -1);
    lua_pop(l, 1);

    lua_getfield(l, -1, "screendata_address");
    if (!lua_isnumber(l, -1))
    {
        printf("Invalid address entry: \"screendata_address\" doesn't exist "
               "or is not a number.\n");
        lua_pop(l, 1);
        return;
    }

    as.screendata_address = (uint32_t) lua_tonumber(l, -1);
    lua_pop(l, 1);

    lua_getfield(l, -1, "method");
    if (!lua_isstring(l, -1))
    {
        printf("Invalid address entry: \"method\" doesn't exist or is "
               "not a string.\n");
        lua_pop(l, 1);
        return;
    }
    if (strcmp("PackedVarying", lua_tostring(l, -1)))
    {
        printf("Invalid address entry: \"method\" is not "
               "\"PackedVarying\".\n"); 
        lua_pop(l, 1);
        return;
    }

    as.method = PackedVarying;
    lua_pop(l, 1);

    settings->push_back(as);
}

static void readAddresses(lua_State* l,
                          vector<AddressSettings32>* settings)
{
    if (!lua_istable(l, -1))
        return;
   
    lua_pushnil(l);
    while(lua_next(l, -2) != 0)
    {
        readAddress(l, settings);
        lua_pop(l, 1);
    }
}

/* Read from lua script what to use as database and logfile */
bool dfterm::readConfigurationFile(const std::string &conffile, std::string*
        logfile, std::string* database, std::vector<AddressSettings32>*
        settings)
{
    assert(logfile);
    assert(database);

    wchar_t* wc = (wchar_t*) 0;
    size_t wc_size = 0;
    TO_WCHAR_STRING(conffile, wc, &wc_size);
    if (wc_size == 0)
    {
        cout << "Configuration file name is corrupted. Can't read configuration." << endl;
        return false;
    }
    wc = new wchar_t[wc_size+1];
    memset(wc, 0, (wc_size+1) * sizeof(wchar_t));
    TO_WCHAR_STRING(conffile, wc, &wc_size);

    #ifdef _WIN32
    FILE* f = _wfopen(wc, L"rb");
    #else
    FILE* f = fopen(conffile.c_str(), "rb");
    #endif
    if (!f)
    {
        int err = errno;
        #ifdef _WIN32
        wprintf(L"Could not open configuration file %ls, errno %d.\n", wc, err);
        #else
        cout << "Could not open configuration file " << conffile << ", errno " << err << "." << endl;
        #endif
        delete[] wc;
        return false;
    }
    delete[] wc;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    if (size > CONFIGURATION_FILE_MAX_SIZE)
    {
        cout << "Configuration file is too large. (Over " << CONFIGURATION_FILE_MAX_SIZE << " bytes)" << endl;
        return false;
    }
    fseek(f, 0, SEEK_SET);

    char* buf = new char[size];
    fread(buf, size, 1, f);
    if (ferror(f))
    {
        cout << "Error while reading configuration file, error code " << ferror(f) << endl;
        delete[] buf;
        return false;
    }

    lua_State* l = luaL_newstate();
    if (!l)
    {
        cout << "Could not create a lua state when reading configuration file." << endl;
        delete[] buf;
        return false;
    }
    luaL_openlibs(l);

    /* Replace os.getenv in lua with a version that takes unicode. */
    lua_getfield(l, LUA_GLOBALSINDEX, "os");
    lua_pushcfunction(l, getenv_unicode);
    lua_setfield(l, -2, "getenv");
    lua_pop(l, 1);

    int result = luaL_loadbuffer(l, buf, size, "conffile");
    if (result != 0)
    {
        delete[] buf;
        cout << "Could not load configuration file. ";
        if (result != LUA_ERRMEM)
            cout << lua_tostring(l, -1) << endl;
        else
            cout << "Memory error." << endl;
        return false;
    }

    delete[] buf;
    result = lua_pcall(l, 0, 0, 0);
    if (result != 0)
    {
        cout << "Could not run configuration file. ";
        if (result != LUA_ERRMEM)
            cout << lua_tostring(l, -1) << endl;
        else
            cout << "Memory error." << endl;
        return false;
    }
    cout << "Ran through configuration file." << endl;

    bool set_database = false;
    bool set_logfile = false;

    lua_getfield(l, LUA_GLOBALSINDEX, "addresses");
    if (lua_istable(l, -1))
    {
        readAddresses(l, settings);
        lua_pop(l, 1);
    }

    lua_getfield(l, LUA_GLOBALSINDEX, "database");
    if (lua_isstring(l, -1))
    {
        cout << "Database: " << lua_tostring(l, -1) << endl;
        set_database = true;
        (*database) = lua_tostring(l, -1);
        lua_pop(l, 1);
    }
    else
        cout << "Database not set." << endl;
    lua_getfield(l, LUA_GLOBALSINDEX, "logfile");
    if (lua_isstring(l, -1))
    {
        cout << "Logfile: " << lua_tostring(l, -1) << endl;
        set_logfile = true;
        (*logfile) = lua_tostring(l, -1);
        lua_pop(l, 1);
    }
    else
        cout << "Logfile not set." << endl;

    lua_close(l);

    return true;
}
