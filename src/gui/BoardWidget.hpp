#pragma once

#include "raylib.h"
#include "../core/Map.hpp"

class BoardWidget {
public:
    BoardWidget();
    void setFont(const Font* font, bool loaded);
    void draw(const Map* map, const State* state, const Rectangle& area) const;

private:
    const Font* font_;
    bool font_loaded_;
};
