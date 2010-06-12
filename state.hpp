#ifndef state_hpp
#define state_hpp

namespace dfterm {
class State;
};

#include "types.hpp"
#include <set>
#include "sockets.hpp"
#include "logger.hpp"
#include "client.hpp"
#include "dfterm2_configuration.hpp"
#include <sstream>

using namespace std;
using namespace trankesbel;

namespace dfterm {

extern SP<Logger> admin_logger; 
extern void flush_messages();

/* Holds the state of the dfterm2 process.
 * All clients, slots etc. */
class State
{
    private:
        State();

        /* No copies */
        State(const State& s) { };
        State& operator=(const State &s) { };

        /* Self */
        WP<State> self;

        set<SP<Socket> > listening_sockets;

        /* This is the list of connected clients. */
        vector<SP<Client> > clients;
        /* And weak pointers to them. */
        vector<WP<Client> > clients_weak;
        
        /* The global chat logger */
        SP<Logger> global_chat;

        /* Database */
        SP<ConfigurationDatabase> configuration;

        /* Running slots */
        vector<SP<Slot> > slots;

        /* Current slot profiles */
        vector<SP<SlotProfile> > slotprofiles;

        bool launchSlotNoCheck(SP<SlotProfile> slot_profile);

        uint64_t ticks_per_second;

    public:
        /* Creates a new state. There can be only one, so trying to create another of this class in the same process is going to return null. */
        static SP<State> createState();
        ~State();

        /* Returns all the slots that are running. */
        vector<WP<Slot> > getSlots();
        /* Returns currently available slot profiles. */
        vector<WP<SlotProfile> > getSlotProfiles();

        /* Launches a slot. Returns bool if that can't be done (and logs to admin_logger) */
        /* If called with a slot profile, that slot profile must be in the list of slot profiles
         * this state knows about. */
        bool launchSlot(SP<SlotProfile> slot_profile);
        bool launchSlot(UnicodeString slot_profile_name);
        bool launchSlotUTF8(string slot_profile_name) { return launchSlot(UnicodeString::fromUTF8(slot_profile_name)); };

        /* Returns if a slot by given name exists and returns true if it does.
         * Also returns true for empty string. */
        bool hasSlotProfile(UnicodeString name);
        bool hasSlotProfileUTF8(string name) { return hasSlotProfile(UnicodeString::fromUTF8(name)); };

        /* Adds a new slot profile to state */
        void addSlotProfile(SP<SlotProfile> sp);

        /* Sets ticks per second. */
        void setTicksPerSecond(uint64_t ticks_per_second);

        /* Sets configuration database to use. */
        bool setDatabase(UnicodeString database_file);
        bool setDatabaseUTF8(string database_file);

        /* Add a new telnet port to listen to. */
        bool addTelnetService(SocketAddress address);

        /* Runs until admin tells it to stop. */
        void loop();
};


}; /* namespace */

#endif
