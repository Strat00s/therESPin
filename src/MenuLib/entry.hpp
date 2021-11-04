#include <Arduino.h>
#include <U8g2lib.h>
#include <string>
#include <deque>
#include <vector>

//TODO move macros???
#define FONT_WIDTH  this->u8g2->getMaxCharWidth()
#define FONT_HEIGHT this->u8g2->getMaxCharHeight()
#define LCD_WIDTH   this->u8g2->getDisplayWidth()
#define LCD_HEIGHT  this->u8g2->getDisplayHeight()
#define CHARS_IN_ROW LCD_WIDTH / FONT_WIDTH
#define CHARS_IN_COL LCD_HEIGHT / FONT_HEIGHT
#define LINE(x) ((x) * FONT_HEIGHT)

typedef enum {
    not_item,
    counter,
    picker,
    toggle,
    custom
} type_t;


typedef struct {
    type_t type;
    int min;
    int max;
    int *value;
    std::vector<std::string> options;
    bool blink;
} item_t;


class Entry {
    private:
        /*----(Variables)----*/
        U8G2 *u8g2 = nullptr;                                 //pointer to u8g2 object for LCD control
        std::deque<Entry *> entries;                          //sub-entries for this entry
        int entry_cnt = 0;                                    //how many entries does this entry contain
        Entry *(*function)(int *, bool *, Entry *) = nullptr; //pointer to custom entry function
        type_t entry_type = custom;                           //type of this entry (item, menu, custom)
        bool has_parent = false;                              //boolean for parent registering
        int scroll_offset = 0;                                //offset (in items) for "scrolling"


        /*----(Helper functions)----*/
        /** @brief Draw x axis centered string
         * 
         * @param text string text
         * @param y screen coordinate
         */
        void drawCenteredString(std::string text, int y);

        /** @brief Draw item data at the right side of screen
         * 
         * @param entry entry pointer
         * @param y screen coordinate
         */
        void drawItemData(item_t item, int y);

        /** @brief Draw menu and individual entries
         * 
         * @param position encoder position/general value for cursor position calculations
         * @param select confirm
         * @return Entry* 
         */
        Entry *drawMenu(int *position, bool* select);   //draw menu

        /** @brief Draw current item (when changing value)
         * 
         * @param position encoder position/general value for editing item data
         * @param select confirm
         * @return Entry* 
         */
        Entry *drawItem(int *position, bool* select);   //redraw selected item with new data

    public:
        std::string name; //entry name which will be rendered as individual menu entries
        
        //menu specific data
        std::string header; //menu header
        int position = 0;   //used for cursor position when returning to menu and as current item position for redrawing; also used for blinking cursor position

        //item specific data
        item_t item; //item data struct


        /** @brief Construct a new menu Entry object
         * 
         * @param u8g2 pointer to u8g2 object
         * @param name entry name
         * @param header menu header
         * @param function custom function pointer
         */
        Entry(U8G2 *u8g2, std::string name, std::string header, Entry *(*function)(int *, bool *, Entry *) = nullptr);          //menu

        /** @brief Construct a new counter item Entry object
         * 
         * @param u8g2 pointer to u8g2 object
         * @param name entry name
         * @param item_type item type
         * @param min min item value
         * @param max max item value
         * @param start start index pointer
         */
        Entry(U8G2 *u8g2, std::string name, type_t item_type, int min, int max, int *start);                                    //counter item

        /** @brief Construct a new picker/toggle item Entry object
         * 
         * @param u8g2 pointer to u8g2 object
         * @param name entry name
         * @param item_type item type
         * @param start start index pointer
         * @param options vector of strings containing options
         */
        Entry(U8G2 *u8g2, std::string name, type_t item_type, int *start, std::vector<std::string> options);                    //picker and toggle item

        /** @brief Construct a new custom item Entry object
         * 
         * @param u8g2 pointer to u8g2 object
         * @param name entry name
         * @param min min item value
         * @param max max item value
         * @param data data pointer
         * @param options vector of strings containing options
         * @param function custom function pointer
         */
        Entry(U8G2 *u8g2, std::string name, int min = 0, int max = 0, int *data = nullptr, std::vector<std::string> options = {}, Entry *(*function)(int *, bool *, Entry *) = nullptr); //custom item

        /** @brief Destroy the Entry object
         * 
         */
        ~Entry();

        /** @brief Set custom function
         * 
         * @param function custom function pointer
         */
        void setFunction(Entry *(*function)(int *, bool *, Entry *) = nullptr);


        /** @brief Create and add counter item Entry
         * 
         * @param name entry name
         * @param item_type item type
         * @param min min item value
         * @param max max item value
         * @param start start index pinter
         */
        void addEntry(std::string name, type_t item_type, int min, int max, int *start); //add counter entry

        /** @brief Create and add picker/toggle item entry
         * 
         * @param name entry name
         * @param item_type item type
         * @param start start index pointer
         * @param options vector of strings containing options
         */
        void addEntry(std::string name, type_t item_type, int *start, std::vector<std::string> options); //add picker/toggle entry

        /** @brief Create and add menu Entry
         * 
         * @param name entry name
         * @param header menu header
         */
        void addEntry(std::string name, std::string header); //add name entry

        /** @brief Add already created entry to current entry
         * 
         * @param entry entry pointer
         */
        void addEntry(Entry *entry);


        /** @brief Add parent to current entry
         * 
         * @param parent parent pointer
         */
        void addParent(Entry *parent);

        /** @brief Return parent Entry pointer */
        Entry *parent();


        /** @brief Reset position back to parent (for custom function)
         * 
         * @param position position/general value pointer
         */
        void resetToParent(int *position);


        /** @brief Return entry type */
        type_t type();


        /** @brief Execute the entry function
         * 
         * @param position position/general value pointer
         * @param select select boolean pointer
         * @param entry entry pointer (for custom function)
         * @return next Entry pointer
         */
        Entry *execute(int *position, bool *select, Entry *entry = nullptr);
};