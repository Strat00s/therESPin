#include "entry.hpp"
#include <tuple>

class Menu {
    private:
        Entry *current_entry;           //current entry pointer
        std::vector<Entry *> entries;   //all entries vector
        U8G2 *u8g2;                     //u8g2 object pointer
        int entry_id_counter;           //entry counter for id increments
        int old_value;                  //old value for menu scroling
        uint64_t blink_timer;           //cursor blinking timer
        bool blink;                     //cursor blinking bool

        /** @brief helper function for entry setup
         * 
         * @param entry entry to setup
         * @param parent_id parent id
         * @param parent_name parent name to which we want add new entry (default empty string)
         * @return returns entry ID or -1 if parent wasn't found
         */
        int entrySetup(Entry *entry, int parent_id, std::string parent_name = "");

    public:
        #define ROOT 0

        /** @brief Construct a new Menu:: Menu object with new default root menu named 'Main menu'
        * 
        * @param u8g2 pointer to u8g2 object
        */
        Menu(U8G2* u8g2 = nullptr);

        /** @brief Destroy the Menu object
         * 
         */
        ~Menu();

        /** @brief start (check) menu
         * 
         * @param u8g2 u8g2 pointer
         * @return returns 1 if u8h2 was not properly setup, 0 otherwise
         */
        int begin(U8G2* u8g2 = nullptr);


        /*** @brief Change root name (mainly menu header)
         * 
         * @param name entry name
         * @param header menu header
         */
        void changeRootName(std::string name, std::string header);

        /** @brief Change root menu function
        * 
        * @param function function (nullptr if none)
        */
        void changeRootFunction(Entry *(*function)(int *, bool *, Entry *) = nullptr);


        //add entries to parents by id
        /** @brief Add copy of and already created entry
         * 
         * @param parent_id parent id
         * @param entry entry to be added
         * @return Entry id on succes, -1 on failure
         */
        int addByID(int parent_id, Entry *entry);
        
        /** @brief Create and add menu entry
         * 
         * @param parent_id parent id
         * @param name entry name
         * @param header header string
         * @param function custom function
         * @return Entry id on succes, -1 on failure 
         */
        int addByID(int parent_id, std::string name, std::string header, Entry *(*function)(int *, bool *, Entry *) = nullptr);
        
        /** @brief Create and add counter item entry
         * 
         * @param parent_id parent id
         * @param name entry name
         * @param item_type item type
         * @param min min item value
         * @param max max item value
         * @param start start index
         * @param modifier by how much should item value change
         * @return Entry id on succes, -1 on failure 
         */
        int addByID(int parent_id, std::string name, type_t item_type, int min, int max, int *start, int *modifier = nullptr);
        
        /** @brief Create and add picker/toggle item entry
         * 
         * @param parent_id parent id
         * @param name entry name
         * @param item_type item type
         * @param start start index
         * @param options vector of strings containing options
         * @return Entry id on succes, -1 on failure 
         */
        int addByID(int parent_id, std::string name, type_t item_type, int *start, std::vector<std::string> options);
        
        /** @brief Create and add custom item entry
         * 
         * @param parent_id parent id
         * @param name entry name
         * @param min min item value (default 0)
         * @param max max item value ((default 0)
         * @param data data pointer (default nullptr)
         * @param modifier by how much should item value change (default null; if null, 1 will be used)
         * @param options vector of strings containing options
         * @param function custom function
         * @return Entry id on succes, -1 on failure 
         */
        int addByID(int parent_id, std::string name, int min = 0, int max = 0, int *data = nullptr, int *modifier = nullptr, std::vector<std::string> options = {}, Entry *(*function)(int *, bool *, Entry *) = nullptr); //create custom item

        //add entries to parents by name
        /** @brief Add copy of and already created entry 
         * 
         * @param parent_name parent name
         * @param entry entry to be added
         * @return Entry id on succes, -1 on failure
         */
        int addByName(std::string parent_name, Entry *entry);
        
        /** @brief Creat and add menu entry
         * 
         * @param parent_name parent name
         * @param name entry name
         * @param header menu header
         * @param function custom function pointer
         * @return Entry id on succes, -1 on failure 
         */
        int addByName(std::string parent_name, std::string name, std::string header, Entry *(*function)(int *, bool *, Entry *) = nullptr);
        
        /** @brief Create and add counter item
         * 
         * @param parent_name parent name
         * @param name entry name
         * @param item_type item type
         * @param min min item value
         * @param max max item value
         * @param start start index
         * @param modifier by how much should item value change
         * @return Entry id on succes, -1 on failure
         */
        int addByName(std::string parent_name, std::string name, type_t item_type, int min, int max, int *start, int *modifier = nullptr);
        
        /** @brief Create and add picker/toggle item entry
         * 
         * @param parent_name parent name
         * @param name entry name
         * @param item_type item type
         * @param start start index pointer
         * @param options vector of strings containing options
         * @return Entry id on succes, -1 on failure 
         */
        int addByName(std::string parent_name, std::string name, type_t item_type, int *start, std::vector<std::string> options);
        
        /** @brief Create and add custom item entry
         * 
         * @param parent_name parent name
         * @param name entry name
         * @param min min item value (default 0)
         * @param max max item value (default 0)
         * @param data data pointer (default nullptr)
         * @param modifier by how much should item value change (default null; if null, 1 will be used)
         * @param options vector of strings containing options
         * @param function custom function
         * @return Entry id on succes, -1 on failure 
         */
        int addByName(std::string parent_name, std::string name, int min = 0, int max = 0, int *data = nullptr, int *modifier = nullptr, std::vector<std::string> options = {}, Entry *(*function)(int *, bool *, Entry *) = nullptr); //create custom item

        
        /** @brief Get the current Entry (selected menu/item) id
         * 
         * @return entry id
         */
        int getCurrentId();

        /** @brief Get the current Entry (selected menu/item)
         * 
         * @return Enttry pointer
         */
        Entry *getCurrentEntry();

        /** @brief Get entry by id
         * 
         * @param id entry id
         * @return Entry pointer
         */
        Entry *getEntryById(int id);

        /** @brief Get entry by name
         * 
         * @param name entry name
         * @return Entry pointer
         */
        Entry *getEntryByName(std::string name);


        /** @brief Render the menu and all entries or a single item/menu with custom function
         * 
         * @param position general data parameter (used for example for cursor claculations in menus)
         * @param select select value (ok, enter)
         * @param update should the menu be updated (false)
         */
        void render(int *position, bool *select, bool *update = nullptr);

        /** @brief return root index (0)
         * 
         * @return root index
         */
        int root();
};