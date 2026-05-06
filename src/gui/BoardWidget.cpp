#include "BoardWidget.hpp"
#include <algorithm>

namespace {
constexpr Color kGridBg{32, 37, 50, 255};
constexpr Color kTileNormal{42, 49, 64, 255};
constexpr Color kTileWall{243, 245, 247, 255};
constexpr Color kTileLava{255, 91, 58, 255};
constexpr Color kTileGoal{38, 230, 95, 255};
constexpr Color kTileWaypoint{245, 176, 77, 255};
constexpr Color kTileGrid{59, 68, 88, 255};
constexpr Color kActor{45, 107, 255, 255};
constexpr Color kActorOutline{136, 169, 255, 255};
constexpr Color kTextLight{234, 241, 255, 255};

Color tileColor(char ch) {
    if (ch == 'X') return kTileWall;
    if (ch == 'L') return kTileLava;
    if (ch == 'O') return kTileGoal;
    if (ch >= '0' && ch <= '9') return kTileWaypoint;
    return kTileNormal;
}
}  // namespace

BoardWidget::BoardWidget() : font_(nullptr), font_loaded_(false) {}

void BoardWidget::setFont(const Font* font, bool loaded) {
    font_ = font;
    font_loaded_ = loaded;
}

void BoardWidget::draw(const Map* map, const State* state, const Rectangle& area) const {
    DrawRectangleRec(area, kGridBg);

    if (map == nullptr || state == nullptr) {
        if (font_loaded_ && font_ != nullptr) {
            DrawTextEx(*font_, "Load map lalu tekan SOLVE", {area.x + 20, area.y + area.height / 2.0f}, 24.0f, 1.0f, kTextLight);
        } else {
            DrawText("Load map lalu tekan SOLVE", static_cast<int>(area.x + 20), static_cast<int>(area.y + area.height / 2.0f), 24, kTextLight);
        }
        return;
    }

    const float margin = 20.0f;
    const float usable_w = area.width - 2.0f * margin;
    const float usable_h = area.height - 2.0f * margin;
    const float cell = (map->cols == 0 || map->rows == 0)
                           ? 1.0f
                           : std::min(usable_w / static_cast<float>(map->cols), usable_h / static_cast<float>(map->rows));

    const float board_w = cell * static_cast<float>(map->cols);
    const float board_h = cell * static_cast<float>(map->rows);
    const float ox = area.x + (area.width - board_w) * 0.5f;
    const float oy = area.y + (area.height - board_h) * 0.5f;

    for (int r = 0; r < map->rows; ++r) {
        for (int c = 0; c < map->cols; ++c) {
            char ch = map->grid[r][c];
            if (ch >= '0' && ch <= '9' && (ch - '0') < state->mask) {
                ch = '*';
            }

            Rectangle tile{ox + c * cell, oy + r * cell, cell, cell};
            bool is_outer_ring = (r == 0 || r == map->rows - 1 || c == 0 || c == map->cols - 1);
            Color fill = tileColor(ch);

            // For border walls, use dark tiles and represent the boundary
            // as one thick white outline around the board.
            if (ch == 'X' && is_outer_ring) {
                fill = kTileNormal;
            }

            DrawRectangleRec(tile, fill);
            DrawRectangleLinesEx(tile, 1.0f, kTileGrid);

            if (ch >= '0' && ch <= '9') {
                char txt[2] = {ch, '\0'};
                const int font_size = static_cast<int>(cell * 0.33f);
                int tw = 0;
                if (font_loaded_ && font_ != nullptr) {
                    tw = static_cast<int>(MeasureTextEx(*font_, txt, static_cast<float>(font_size), 1.0f).x);
                    DrawTextEx(*font_, txt, {tile.x + (tile.width - tw) * 0.5f, tile.y + tile.height * 0.22f}, static_cast<float>(font_size), 1.0f, BLACK);
                } else {
                    tw = MeasureText(txt, font_size);
                    DrawText(txt, static_cast<int>(tile.x + (tile.width - tw) * 0.5f), static_cast<int>(tile.y + tile.height * 0.22f), font_size, BLACK);
                }
            }
        }
    }

    Rectangle actor{
        ox + state->c * cell + 4.0f,
        oy + state->r * cell + 4.0f,
        cell - 8.0f,
        cell - 8.0f
    };
    DrawRectangleRec(actor, kActor);
    DrawRectangleLinesEx(actor, 1.0f, kActorOutline);
    const int fs = static_cast<int>(cell * 0.30f);
    if (font_loaded_ && font_ != nullptr) {
        const int zw = static_cast<int>(MeasureTextEx(*font_, "Z", static_cast<float>(fs), 1.0f).x);
        DrawTextEx(*font_, "Z", {actor.x + (actor.width - zw) * 0.5f, actor.y + actor.height * 0.22f}, static_cast<float>(fs), 1.0f, WHITE);
    } else {
        const int zw = MeasureText("Z", fs);
        DrawText("Z", static_cast<int>(actor.x + (actor.width - zw) * 0.5f), static_cast<int>(actor.y + actor.height * 0.22f), fs, WHITE);
    }

    Rectangle board_rect{ox, oy, board_w, board_h};
    float border_thickness = std::max(4.0f, cell * 0.16f);
    DrawRectangleLinesEx(board_rect, border_thickness, kTileWall);
}
