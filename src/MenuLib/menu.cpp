#include "menu.hpp"

#include <iostream>

Menu::Menu(U8G2* u8g2) {
    this->u8g2 = u8g2;                                                 //save u8h2 object
    Entry *root = new Entry(this->u8g2, "root", "Main menu", nullptr); //create default root item
    entry_id_counter = 0;                                              //set id counter to zero (main menu's id)
    entries.push_back(root);                                           //push newly created default root item
    current_entry = entries[0];                                        //save pointer
    blink_timer = millis();                                            //set timer
    blink = false;                                                     //set blink status
    old_value = -1;                                                    //set old value (-1 for single load on start)
}

Menu::~Menu() {};

int Menu::begin(U8G2* u8g2) {
    if (this->u8g2 == nullptr) {
        if (u8g2 == nullptr)
            return 1;
        this->u8g2 = u8g2;
    }
    return 0;
}


void Menu::changeRootName(std::string name, std::string header) {
    entries[ROOT]->name = name;
    entries[ROOT]->header = header;
}

void Menu::changeRootFunction(Entry *(*function)(int *, bool *, Entry *)) {
    entries[ROOT]->setFunction(function);
}


int Menu::entrySetup(Entry *entry, int parent_id, std::string parent_name) {
    entry_id_counter++; //increase entry id (this is where in vector this entry will be stored)
    entries.push_back(entry);                                //push it onto the vector
    //find parent id by name
    if (parent_id == -1) {
        for (int i = 0; i < entries.size(); i++) {
            if (entries[i]->name == parent_name) {
                parent_id = i;  //set id
                break;
            }
        }
    }

    //check if parent id is correct
    if (parent_id < 0 || parent_id > entries.size() - 1)
        return -1;
 
    entries[parent_id]->addEntry(entries[entry_id_counter]); //add entry to parent
    return entry_id_counter;
}


//Add entry
int Menu::addByID(int parent_id, Entry *entry){
    return entrySetup(entry, parent_id);
}

int Menu::addByID(int parent_id, std::string name, std::string header, Entry *(*function)(int *, bool *, Entry *)){
    Entry *tmp = new Entry(this->u8g2, name, header, function); //create new entry
    return entrySetup(tmp, parent_id);
}

int Menu::addByID(int parent_id, std::string name, type_t item_type, int min, int max, int *start, int *modifier){
    Entry *tmp = new Entry(this->u8g2, name, item_type, min, max, start, modifier);
    return entrySetup(tmp, parent_id);
}

int Menu::addByID(int parent_id, std::string name, type_t item_type, int *start, std::vector<std::string> options){
    Entry *tmp = new Entry(this->u8g2, name, item_type, start, options);
    return entrySetup(tmp, parent_id);
}

int Menu::addByID(int parent_id, std::string name, int min, int max, int *data, int *modifier, std::vector<std::string> options, Entry *(*function)(int *, bool *, Entry *)){
    Entry *tmp = new Entry(this->u8g2, name, min, max, data, modifier, options, function);
    return entrySetup(tmp, parent_id);
}

int Menu::addByName(std::string parent_name, Entry *entry){
    return entrySetup(entry, -1, parent_name);
}

int Menu::addByName(std::string parent_name, std::string name, std::string header, Entry *(*function)(int *, bool *, Entry *)) {
    Entry *tmp = new Entry(this->u8g2, name, header, function); //create new entry
    return entrySetup(tmp, -1, parent_name);
}

int Menu::addByName(std::string parent_name, std::string name, type_t item_type, int min, int max, int *start, int *modifier) {
    Entry *tmp = new Entry(this->u8g2, name, item_type, min, max, start, modifier);
    return entrySetup(tmp,  -1, parent_name);
}

int Menu::addByName(std::string parent_name, std::string name, type_t item_type, int *start, std::vector<std::string> options) {
    Entry *tmp = new Entry(this->u8g2, name, item_type, start, options);
    return entrySetup(tmp, -1, parent_name);
}

int Menu::addByName(std::string parent_name, std::string name, int min, int max, int *data, int *modifier, std::vector<std::string> options, Entry *(*function)(int *, bool *, Entry *)) {
    Entry *tmp = new Entry(this->u8g2, name, min, max, data, modifier, options, function);
    return entrySetup(tmp, -1, parent_name);
}


int Menu::getCurrentId(){
    int i = 0;
    for (; i < entries.size(); i++) {
        if (current_entry == entries[i])
            break;
    }
    return i;
}

Entry *Menu::getCurrentEntry(){
    return current_entry;
}

Entry *Menu::getEntryById(int id) {
    if (id < 0 || id > entries.size() - 1)
        return nullptr;
    return entries[id];
}

Entry *Menu::getEntryByName(std::string name) {
    //not the most efficient way of searching
    for (int i = 0; i < entries.size(); i++) {
        if (entries[i]->name == name)
            return entries[i];
    }
    return nullptr;
}


void Menu::render(int *value, bool *select, bool *update) {
    //execute entry function on select, update or data change
    if ((old_value != *value) || *select || *update == true) {
        std::cout << "Old: " << old_value << "| val: " << *value << " | sel: " << *select << " | update: " << *update << std::endl;
        current_entry = current_entry->execute(value, select, current_entry);
        old_value = *value;
        *update = false;
    }

    //cursour blinking on select
    if (current_entry->type() != not_item && current_entry->type() != custom) {
        if (millis() - blink_timer > 500) {
            blink_timer = millis();
            blink = !blink;
            if (blink) {
                u8g2->setDrawColor(0);
                u8g2->drawBox(0, current_entry->position * u8g2->getMaxCharHeight() + u8g2->getMaxCharHeight() + 1, u8g2->getMaxCharWidth(), u8g2->getMaxCharHeight());
            }
            else {
                u8g2->setDrawColor(1);
                u8g2->drawStr(0, current_entry->position * u8g2->getMaxCharHeight() + u8g2->getMaxCharHeight() * 2, ">");
            }
            u8g2->sendBuffer();
        }
    }
    else if (blink)
        blink = false;
}


int Menu::root() {
    return ROOT;
}