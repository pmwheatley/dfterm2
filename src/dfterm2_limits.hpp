#ifndef dfterm2_limits_hpp
#define dfterm2_limits_hpp

#include "types.hpp"

namespace dfterm
{
/* Various limits on stuff. */
/* At the moment they are rather low, because dfterm2
   is not in use on any large servers */

/* How many slots can be running at a time, at max. 
   It will not be possible to configure dfterm2 to have
   larger value than this. */
const trankesbel::ui32 MAX_SLOTS = 100;

/* Maximum number of slot profiles */
const trankesbel::ui32 MAX_SLOT_PROFILES = 300;

/* Maximum number of registered users */
const trankesbel::ui32 MAX_REGISTERED_USERS = 500;

/* Maximum number of (telnet) connections at a time */
const trankesbel::ui32 MAX_CONNECTIONS = 500;

/* Maximum number of server-to-server links at a time. */
const trankesbel::ui32 MAX_SERVER_TO_SERVER_LINKS = 30;

/* Maximum number of HTTP connections at a time. */
const trankesbel::ui32 MAX_HTTP_CONNECTIONS = 100;

/* Maximum size for a file served through HTTP. */
const trankesbel::ui64 MAX_HTTP_FILE_SIZE = 1000000;

/* Maximum size of configuration file. */
const trankesbel::ui64 CONFIGURATION_FILE_MAX_SIZE = 100000;

};

#endif
