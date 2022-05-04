#include "entry.hpp"

//Constructors
Entry::Entry(U8G2 *u8g2, std::string name, std::string header, Entry *(*function)(int *, bool *, Entry *)) {
    this->u8g2     = u8g2;
    this->name     = name;
    this->header   = header;
    entry_type     = not_item;
    this->function = function;
}

Entry::Entry(U8G2 *u8g2, std::string name, type_t item_type, int min, int max, int *start, int *modifier) {
    this->u8g2 = u8g2;
    this->name = name;
    item.min   = min;
    item.max   = max;
    if (*start > item.max)
        *start = item.max;
    if (*start < item.min)
        *start = item.min;
    item.type     = item_type;
    item.value    = start;
    item.modifier = modifier;
    entry_type    = item_type;
}

Entry::Entry(U8G2 *u8g2, std::string name, type_t item_type, int *start, std::vector<std::string> options) {
    this->u8g2 = u8g2;
    this->name = name;
    item.min   = 0;
    item.max   = options.size() - 1;
    if (*start > item.max)
        *start = item.max;
    if (*start < item.min)
        *start = item.min;
    item.options  = options;
    item.type     = item_type;
    item.value    = start;
    item.modifier = nullptr;
    entry_type    = item_type;
}

Entry::Entry(U8G2 *u8g2, std::string name, int min, int max, int *data, int *modifier, std::vector<std::string> options, Entry *(*function)(int *, bool *, Entry *)) {
    this->u8g2     = u8g2;
    this->name     = name;
    item.type      = custom;
    item.value     = data;
    item.modifier  = modifier;
    item.min       = min;
    item.max       = max;
    item.options   = options;
    entry_type     = custom;
    this->function = function;
}

Entry::~Entry() {}


//Add entry
void Entry::addEntry(std::string name, type_t item_type, int min, int max, int *start) {
    Entry *tmp = new Entry(u8g2, name, item_type, min, max, start); //create new entry
    tmp->addParent(this);                                           //add parent
    entries.push_back(tmp);                                         //save it to a stack
    entry_cnt++;                                                    //increase entry counter
}

void Entry::addEntry(std::string name, type_t item_type, int *start, std::vector<std::string> options) {
    Entry *tmp = new Entry(u8g2, name, item_type, start, options);
    tmp->addParent(this);
    entries.push_back(tmp);
    entry_cnt++;
}

void Entry::addEntry(std::string name, std::string header) {
    Entry *tmp = new Entry(u8g2, name, header);
    tmp->addParent(this);
    entries.push_back(tmp);
    entry_cnt++;
}

void Entry::addEntry(Entry *entry) {
    entry->addParent(this);
    entries.push_back(entry);
    entry_cnt++;
}

void Entry::addParent(Entry *parent) {
    if (!has_parent) {
        has_parent = true;
        entries.push_front(parent);
        entry_cnt++;
    }
}


//Draw functions
void Entry::drawCenteredString(std::string text, int y) {
    int width = u8g2->getMaxCharWidth() * text.size();
    width = u8g2->getDisplayWidth()/2 - width/2;
    u8g2->drawStr(width, y, text.c_str());
}

void Entry::drawItemData(item_t item, int y) {
    std::string tmp;
    switch (item.type) {
        case picker:
        case toggle:  tmp = item.options[*item.value]; break;
        case counter: tmp = std::to_string(*item.value); break;
        default: break;
    }
    u8g2->setCursor((charsInRow() - tmp.length()) * this->u8g2->getMaxCharWidth(), y);
    u8g2->printf("%s", tmp.c_str());
}

Entry *Entry::drawMenu(int *position, bool *select) {
    u8g2->clearBuffer();
    int header_offset = 0;
    
    //draw header when needed
    if (header != "") {
        drawCenteredString(header, getLine(1));
        header_offset = 1;
    }

    if (*position > entry_cnt - 1)
        *position = entry_cnt - 1;
    if (*position < 0)
        *position = 0;

    //when at the end of the screen, scroll items
    if (*position - scroll_offset > charsInCol() - 1 - header_offset)   //charsInCol() starts at 1, *position on 0 -> -1
        scroll_offset++;
    if (*position - scroll_offset < 0)
        scroll_offset--;

    //draw all entries
    for (int i = scroll_offset; i < entry_cnt; i++) {
        u8g2->setCursor(0, getLine(i - scroll_offset + 1 + header_offset));    //"zero" i; i starts at 0, lines start at 1 -> +1; add header offset

        //print pointer
        if (i == *position) {
            u8g2->printf("> ");
        }
        else
            u8g2->printf("  ");
        
        //print back or name
        if (i == 0 && has_parent)
            u8g2->print("back");
        else
            u8g2->print(entries[i]->name.c_str());
        
        //print item data
        if (entries[i]->type() != not_item)
            drawItemData(entries[i]->item, getLine(i - scroll_offset + 1 + header_offset));

        //exit when there are more entries than available screen space
        if (i - scroll_offset == charsInCol() - 1 - header_offset)
            break;
    }

    //draw entry count
    std::string tmp = "["+ std::to_string(*position + 1) + "/" + std::to_string(entry_cnt) + "]";
    u8g2->setCursor((charsInRow() - tmp.length()) * u8g2->getMaxCharWidth(), getLine(1));
    u8g2->printf("%s", tmp.c_str());

    u8g2->sendBuffer(); //send buffer to draw the screen

    if (*select) {
        *select = false;
        
        this->position = *position;                                         //save current position when returning back
        //if it is a menu
        if (entries[*position]->type() == not_item) {
            *position = entries[this->position]->position;           //set "new" position to the one saved in the next menu
        }
        else {
            entries[this->position]->position = *position;          //save current item position on the menu;
            *position = *entries[this->position]->item.value;
        }
        return entries[this->position]->execute(position, select, entries[this->position]);  //render next menu
    }
    return this;
}

Entry *Entry::drawItem(int *position, bool *select) {
    //clear current line for redraw
    int x = u8g2->getMaxCharWidth() * 2 + name.length() * u8g2->getMaxCharWidth(); //skip '> '
    int y = getLine(this->position + 1);                    //start right under the above text
    int w = u8g2->getDisplayWidth() - x;
    int h = u8g2->getMaxCharHeight() + 1;

    u8g2->setDrawColor(0);
    u8g2->drawBox(x, y, w , h);
    u8g2->setDrawColor(1);

    //write to now cleared line
    int line = getLine(this->position +2);
    u8g2->setCursor(x, line);

    //toggle logic
    if (item.type == toggle) {
        (*item.value)++;
        if (*item.value > item.max)
            *item.value = item.min;
        *select = true; //return immediately
    }
    else {
        int modifier = (item.modifier == nullptr ? 1 : *item.modifier);
        if (*item.value < *position)
            *item.value += modifier;
        else if (*item.value > *position)
            *item.value -= modifier;
        if (*item.value > item.max)
            *item.value = item.max;
        if (*item.value < item.min)
            *item.value = item.min;
        *position = *item.value;
    }
    
    drawItemData(item, line);

    u8g2->sendBuffer();

    if (*select) {
        *select = false;

        u8g2->drawStr(0, line, ">");  //redraw pointer just in case
        u8g2->sendBuffer();

        resetToParent(position);    //set position for inside parent
        return this->entries[0];
    }
    return this;
}


//Miscellaneous
Entry *Entry::parent() {
    if (has_parent)
        return this->entries[0];
    return nullptr;
}

type_t Entry::type() {
    return this->entry_type;
}

void Entry::resetToParent(int *position) {
    *position = entries[0]->position;
}

void Entry::setFunction(Entry *(*function)(int *, bool *, Entry *)) {
    this->function = function;
}


//Helper functions
int Entry::charsInRow(U8G2 *u8g2) {
    if (u8g2 == nullptr)
        u8g2 = this->u8g2;
    return u8g2->getDisplayWidth() / u8g2->getMaxCharWidth();
}
int Entry::charsInCol(U8G2 *u8g2) {
    if (u8g2 == nullptr)
        u8g2 = this->u8g2;
    return u8g2->getDisplayHeight() / u8g2->getMaxCharHeight();
}
int Entry::getLine(int row, U8G2 *u8g2) {
    if (u8g2 == nullptr)
        u8g2 = this->u8g2;
    return row * u8g2->getMaxCharHeight();
}


//Main
Entry *Entry::execute(int *position, bool *select, Entry *entry) {
    //menu
    if (this->function == nullptr) {
        if (this->entry_type == not_item)
            return drawMenu(position, select);
        else
            return drawItem(position, select);
    }
    else
        return function(position, select, entry);
}