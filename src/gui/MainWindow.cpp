#include "MainWindow.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
constexpr int kInitialWidth = 1380;
constexpr int kInitialHeight = 760;

constexpr Color kBg{18, 24, 38, 255};
constexpr Color kPanel{31, 42, 59, 255};
constexpr Color kText{234, 241, 255, 255};
constexpr Color kSubText{159, 220, 255, 255};
constexpr Color kButton{52, 129, 201, 255};
constexpr Color kButtonAccent{102, 204, 255, 255};
constexpr Color kButtonDark{34, 49, 74, 255};
constexpr Color kInputBg{15, 21, 34, 255};
constexpr Color kOutline{59, 68, 88, 255};

std::vector<std::string> readNonEmptyLines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    if (!in) return lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = line;
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), trimmed.end());
        if (!trimmed.empty()) lines.push_back(trimmed);
    }
    return lines;
}

bool writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    if (!out) return false;
    out << content;
    return true;
}

std::string displayFileName(const std::string& raw_path) {
    if (raw_path.empty()) return "-";
    return std::filesystem::path(raw_path).filename().string();
}

std::string buildSearchOutputFileName(const std::string& input_path) {
    std::string base = std::filesystem::path(input_path).stem().string();
    if (base.empty()) base = "input";
    for (char& ch : base) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_')) {
            ch = '_';
        }
    }
    return "hasil-pencarian-" + base + ".txt";
}

std::string formatMs5(double ms) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(5) << ms;
    return out.str();
}

std::string formatSeconds2(double sec) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << sec;
    return out.str();
}
}  // namespace

MainWindow::MainWindow()
    : algo_index_(2),
      heuristic_index_(1),
      show_process_(true),
      show_playback_(false),
      show_log_overlay_(false),
      log_scroll_(0),
      has_map_(false),
      has_solution_(false),
      sequence_mode_(SequenceMode::Process),
      current_index_(0),
      elapsed_ms_(0.0),
      autoplay_(false),
      last_autoplay_step_(0.0),
      autoplay_step_seconds_(0.35),
      status_until_(0.0),
      warning_open_(false),
      warning_message_(""),
      exe_dir_(std::filesystem::current_path()),
      window_maximized_(false),
      ui_font_{},
      font_loaded_(false),
      run_output_text_(""),
      run_output_lines_(),
      run_output_scroll_(0),
      txt_candidates_(),
      map_pick_index_(-1),
      cost_pick_index_(-1),
      picker_target_(PickerTarget::None),
      picker_open_(false),
      picker_scroll_(0) {
    map_field_ = {{22.0f, 170.0f, 360.0f, 34.0f}, "test/input/input.txt", false};
    cost_field_ = {{22.0f, 225.0f, 360.0f, 34.0f}, "", false};
}

void MainWindow::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(kInitialWidth, kInitialHeight, "Ice Sliding Puzzle Solver - Raylib GUI");
    try {
        exe_dir_ = std::filesystem::absolute(std::filesystem::path(GetApplicationDirectory()));
    } catch (...) {
        exe_dir_ = std::filesystem::current_path();
    }
    const char* segoe_path = "C:/Windows/Fonts/segoeui.ttf";
    const char* arial_path = "C:/Windows/Fonts/arial.ttf";
    if (FileExists(segoe_path)) {
        ui_font_ = LoadFontEx(segoe_path, 32, nullptr, 0);
        font_loaded_ = true;
    } else if (FileExists(arial_path)) {
        ui_font_ = LoadFontEx(arial_path, 32, nullptr, 0);
        font_loaded_ = true;
    }
    if (font_loaded_) {
        SetTextureFilter(ui_font_.texture, TEXTURE_FILTER_BILINEAR);
    }
    board_widget_.setFont(&ui_font_, font_loaded_);
    refreshTxtCandidates();

    MaximizeWindow();
    window_maximized_ = true;
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        handleInput();
        updatePlaybackFromKeyboard();
        updateAutoplay();

        BeginDrawing();
        draw();
        EndDrawing();
    }

    if (font_loaded_) {
        UnloadFont(ui_font_);
    }
    CloseWindow();
}

void MainWindow::handleInput() {
    updateTextFieldInput(map_field_);
    updateTextFieldInput(cost_field_);

    if (!run_output_lines_.empty()) {
        const int max_scroll = std::max(0, static_cast<int>(run_output_lines_.size()) - 1);
        if (IsKeyPressed(KEY_DOWN)) run_output_scroll_ = std::min(run_output_scroll_ + 1, max_scroll);
        if (IsKeyPressed(KEY_UP)) run_output_scroll_ = std::max(run_output_scroll_ - 1, 0);
        if (IsKeyPressed(KEY_PAGE_DOWN)) run_output_scroll_ = std::min(run_output_scroll_ + 15, max_scroll);
        if (IsKeyPressed(KEY_PAGE_UP)) run_output_scroll_ = std::max(run_output_scroll_ - 15, 0);
        if (IsKeyPressed(KEY_END)) run_output_scroll_ = max_scroll;
        if (IsKeyPressed(KEY_HOME)) run_output_scroll_ = 0;
        const int wheel = static_cast<int>(GetMouseWheelMove());
        if (wheel != 0) {
            run_output_scroll_ = std::clamp(run_output_scroll_ - wheel * 3, 0, max_scroll);
        }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        picker_open_ = false;
        picker_target_ = PickerTarget::None;
        picker_scroll_ = 0;
    }
}

void MainWindow::draw() {
    ClearBackground(kBg);
    drawWindowControls();

    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float margin = 12.0f;
    const float gap = 10.0f;
    const float bottom_h = 120.0f;
    const float left_w = std::max(300.0f, std::min(360.0f, sw * 0.23f));
    const float right_w = sw - (2.0f * margin + gap + left_w);
    const float right_top_h = sh - (3.0f * margin + bottom_h);
    const float left_h = sh - (2.0f * margin);

    const Rectangle left{margin, margin, left_w, left_h};
    const Rectangle right{margin + left_w + gap, margin, right_w, right_top_h};
    const float log_w = std::max(300.0f, right.width * 0.34f);
    const Rectangle board_panel{right.x, right.y, right.width - log_w - gap, right.height};
    const Rectangle log_panel{board_panel.x + board_panel.width + gap, right.y, log_w, right.height};
    const Rectangle bottom{right.x, right.y + right.height + margin, right.width, bottom_h};

    drawConfigPanel(left);
    drawBoardPanel(board_panel);
    drawIterLogPanel(log_panel);
    drawBottomPanel(bottom);
    if (warning_open_) {
        drawWarningPopup();
    }

    if (!status_message_.empty() && GetTime() <= status_until_) {
        drawUiText(status_message_, 20, sh - 26.0f, 18, YELLOW);
    }
}

void MainWindow::drawWindowControls() {
    const float sw = static_cast<float>(GetScreenWidth());
    const Rectangle minimize_btn{sw - 100.0f, 10.0f, 40.0f, 26.0f};
    const Rectangle maximize_btn{sw - 52.0f, 10.0f, 40.0f, 26.0f};

    if (button(minimize_btn, "_", kButtonDark)) {
        MinimizeWindow();
    }

    const bool is_max = IsWindowMaximized() || window_maximized_;
    if (button(maximize_btn, is_max ? "RES" : "MAX", kButtonDark)) {
        if (IsWindowMaximized()) {
            RestoreWindow();
            window_maximized_ = false;
        } else {
            MaximizeWindow();
            window_maximized_ = true;
        }
    }
}

void MainWindow::drawUiText(const std::string& text, float x, float y, int size, Color color) const {
    if (font_loaded_) {
        DrawTextEx(ui_font_, text.c_str(), {x, y}, static_cast<float>(size), 1.0f, color);
    } else {
        DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y), size, color);
    }
}

int MainWindow::measureUiText(const std::string& text, int size) const {
    if (font_loaded_) {
        return static_cast<int>(MeasureTextEx(ui_font_, text.c_str(), static_cast<float>(size), 1.0f).x);
    }
    return MeasureText(text.c_str(), size);
}

void MainWindow::drawConfigPanel(const Rectangle& panel) {
    DrawRectangleRounded(panel, 0.03f, 8, kPanel);
    drawUiText("ALGORITHM CONFIG", panel.x + 16, panel.y + 14, 34, kText);

    const float field_w = panel.width - 32.0f;

    const std::vector<std::string> algo_options{"UCS", "BFS", "GBFS", "A*", "IDA*"};
    const std::vector<std::string> heur_options{"H1", "H2", "H3"};
    const float small_pick_w = 66.0f;
    Rectangle algo_rect{panel.x + 16, panel.y + 90, field_w - (small_pick_w + 8.0f), 34};
    Rectangle heur_rect{panel.x + 16, panel.y + 158, field_w - (small_pick_w + 8.0f), 34};
    Rectangle algo_pick_btn{algo_rect.x + algo_rect.width + 8.0f, algo_rect.y, small_pick_w, 34};
    Rectangle heur_pick_btn{heur_rect.x + heur_rect.width + 8.0f, heur_rect.y, small_pick_w, 34};

    drawUiText("Select Algorithm (UCS/BFS/GBFS/A*/IDA*)", panel.x + 16, panel.y + 64, 20, kText);
    DrawRectangleRec(algo_rect, kButtonDark);
    DrawRectangleLinesEx(algo_rect, 1.0f, kOutline);
    drawUiText("Algorithm: " + algo_options[algo_index_], algo_rect.x + 12, algo_rect.y + 8, 18, kText);
    if (button(algo_pick_btn, "Pilih", kButtonDark)) {
        const bool same_target = (picker_target_ == PickerTarget::Algo);
        picker_target_ = PickerTarget::Algo;
        picker_open_ = same_target ? !picker_open_ : true;
    }

    drawUiText("Select Heuristic (H1/H2/H3)", panel.x + 16, panel.y + 132, 20, kText);
    if (algoText() == "UCS" || algoText() == "BFS") {
        DrawRectangleRec(heur_rect, ColorAlpha(kButtonDark, 0.5f));
        DrawRectangleLinesEx(heur_rect, 1.0f, kOutline);
        drawUiText("Heuristic nonaktif untuk UCS", panel.x + 24, panel.y + 167, 16, GRAY);
        DrawRectangleRec(heur_pick_btn, ColorAlpha(kButtonDark, 0.5f));
        DrawRectangleLinesEx(heur_pick_btn, 1.0f, kOutline);
        drawUiText("Pilih", heur_pick_btn.x + 14, heur_pick_btn.y + 8, 16, GRAY);
    } else {
        DrawRectangleRec(heur_rect, kButtonDark);
        DrawRectangleLinesEx(heur_rect, 1.0f, kOutline);
        drawUiText("Heuristic: " + heur_options[heuristic_index_], heur_rect.x + 12, heur_rect.y + 8, 18, kText);
        if (button(heur_pick_btn, "Pilih", kButtonDark)) {
            const bool same_target = (picker_target_ == PickerTarget::Heuristic);
            picker_target_ = PickerTarget::Heuristic;
            picker_open_ = same_target ? !picker_open_ : true;
        }
    }

    const float picker_btn_w = 66.0f;
    map_field_.rect = {panel.x + 16, panel.y + 226, field_w - (picker_btn_w + 8.0f), 34};
    cost_field_.rect = {panel.x + 16, panel.y + 294, field_w - (picker_btn_w + 8.0f), 34};
    drawUiText("Select Map File (.txt)", panel.x + 16, panel.y + 200, 20, kText);
    DrawRectangleRec(map_field_.rect, kButtonDark);
    DrawRectangleLinesEx(map_field_.rect, 1.0f, kOutline);
    drawUiText("Map: " + displayFileName(map_field_.value), map_field_.rect.x + 12, map_field_.rect.y + 8, 18, kText);
    drawUiText("Select Cost File (.txt) [optional]", panel.x + 16, panel.y + 268, 20, kText);
    DrawRectangleRec(cost_field_.rect, kButtonDark);
    DrawRectangleLinesEx(cost_field_.rect, 1.0f, kOutline);
    drawUiText("Cost: " + displayFileName(cost_field_.value), cost_field_.rect.x + 12, cost_field_.rect.y + 8, 18, kText);
    Rectangle map_btn{map_field_.rect.x + map_field_.rect.width + 8.0f, map_field_.rect.y, picker_btn_w, 34};
    Rectangle cost_btn{cost_field_.rect.x + cost_field_.rect.width + 8.0f, cost_field_.rect.y, picker_btn_w, 34};

    if (button(map_btn, "Pilih", kButtonDark)) {
        refreshTxtCandidates();
        const bool same_target = (picker_target_ == PickerTarget::Map);
        picker_target_ = PickerTarget::Map;
        picker_open_ = same_target ? !picker_open_ : true;
        picker_scroll_ = 0;
    }
    if (button(cost_btn, "Pilih", kButtonDark)) {
        refreshTxtCandidates();
        const bool same_target = (picker_target_ == PickerTarget::Cost);
        picker_target_ = PickerTarget::Cost;
        picker_open_ = same_target ? !picker_open_ : true;
        picker_scroll_ = 0;
    }

    if (picker_open_ && picker_target_ != PickerTarget::None) {
        const int max_rows = 8;
        const int row_h = 28;

        Rectangle anchor = map_btn;
        std::vector<std::string> options;
        if (picker_target_ == PickerTarget::Map) {
            anchor = map_field_.rect;
            options = txt_candidates_;
        } else if (picker_target_ == PickerTarget::Cost) {
            anchor = cost_field_.rect;
            options = txt_candidates_;
        } else if (picker_target_ == PickerTarget::Algo) {
            anchor = algo_rect;
            options = algo_options;
        } else if (picker_target_ == PickerTarget::Heuristic) {
            anchor = heur_rect;
            options = heur_options;
        }

        const bool is_file_picker = (picker_target_ == PickerTarget::Map || picker_target_ == PickerTarget::Cost);
        const int visible_rows = is_file_picker ? 3 : max_rows;
        const int total_rows = std::min(static_cast<int>(options.size()), visible_rows);
        const bool needs_scroll = is_file_picker && static_cast<int>(options.size()) > visible_rows;
        const int max_start = std::max(0, static_cast<int>(options.size()) - visible_rows);
        picker_scroll_ = std::clamp(picker_scroll_, 0, max_start);

        Rectangle list_rect{anchor.x, anchor.y + anchor.height + 4.0f, anchor.width, static_cast<float>(std::max(1, total_rows) * row_h + 6)};
        const float scroll_w = needs_scroll ? 24.0f : 0.0f;
        Rectangle items_rect{list_rect.x, list_rect.y, list_rect.width - scroll_w, list_rect.height};

        if (needs_scroll && CheckCollisionPointRec(GetMousePosition(), list_rect)) {
            const int wheel = static_cast<int>(GetMouseWheelMove());
            if (wheel != 0) {
                picker_scroll_ = std::clamp(picker_scroll_ - wheel, 0, max_start);
            }
        }

        DrawRectangleRec(list_rect, kInputBg);
        DrawRectangleLinesEx(list_rect, 1.0f, kOutline);

        if (options.empty()) {
            drawUiText("Tidak ada pilihan.", items_rect.x + 8, items_rect.y + 6, 16, GRAY);
        } else {
            int start_index = 0;
            if (needs_scroll) {
                start_index = picker_scroll_;
            }
            for (int i = 0; i < total_rows; ++i) {
                const int option_index = start_index + i;
                if (option_index < 0 || option_index >= static_cast<int>(options.size())) break;
                Rectangle row{items_rect.x + 3, items_rect.y + 3 + i * row_h, items_rect.width - 6, static_cast<float>(row_h - 2)};
                std::string option_label = options[option_index];
                if (picker_target_ == PickerTarget::Map || picker_target_ == PickerTarget::Cost) {
                    option_label = displayFileName(options[option_index]);
                }
                if (button(row, option_label.c_str(), ColorAlpha(kButtonDark, 0.65f), kText)) {
                    if (picker_target_ == PickerTarget::Map) {
                        map_field_.value = options[option_index];
                    } else if (picker_target_ == PickerTarget::Cost) {
                        cost_field_.value = options[option_index];
                    } else if (picker_target_ == PickerTarget::Algo) {
                        algo_index_ = option_index;
                    } else if (picker_target_ == PickerTarget::Heuristic) {
                        heuristic_index_ = option_index;
                    }
                    picker_open_ = false;
                    picker_target_ = PickerTarget::None;
                    picker_scroll_ = 0;
                }
            }

            if (needs_scroll) {
                Rectangle scroll_area{list_rect.x + list_rect.width - scroll_w, list_rect.y, scroll_w, list_rect.height};
                DrawRectangleRec(scroll_area, ColorAlpha(kButtonDark, 0.5f));
                DrawRectangleLinesEx(scroll_area, 1.0f, kOutline);

                Rectangle up_btn{scroll_area.x + 2, scroll_area.y + 2, scroll_area.width - 4, 22};
                Rectangle down_btn{scroll_area.x + 2, scroll_area.y + scroll_area.height - 24, scroll_area.width - 4, 22};
                if (button(up_btn, "^", kButtonDark, kText)) {
                    picker_scroll_ = std::max(0, picker_scroll_ - 1);
                }
                if (button(down_btn, "v", kButtonDark, kText)) {
                    picker_scroll_ = std::min(max_start, picker_scroll_ + 1);
                }

                const float track_y = up_btn.y + up_btn.height + 2.0f;
                const float track_h = down_btn.y - 2.0f - track_y;
                Rectangle track{scroll_area.x + 7, track_y, scroll_area.width - 14, track_h};
                DrawRectangleRec(track, ColorAlpha(kInputBg, 0.7f));

                const float thumb_h = std::max(12.0f, track.height * (static_cast<float>(visible_rows) / static_cast<float>(options.size())));
                const float ratio = (max_start == 0) ? 0.0f : static_cast<float>(picker_scroll_) / static_cast<float>(max_start);
                const float thumb_y = track.y + ratio * std::max(0.0f, track.height - thumb_h);
                Rectangle thumb{track.x, thumb_y, track.width, thumb_h};
                DrawRectangleRec(thumb, kButtonAccent);
            }
        }
    }

    const float viz_line_h = 20.0f;
    const float viz_title_y = cost_field_.rect.y + cost_field_.rect.height + (5.0f * viz_line_h);
    drawUiText("VISUALIZATION OPTIONS", panel.x + 16, viz_title_y, 30, kText);
    checkBox({panel.x + 16, viz_title_y + 38.0f, 20, 20}, &show_process_, "Show Process (iterations)");
    checkBox({panel.x + 16, viz_title_y + 68.0f, 20, 20}, &show_playback_, "Show Solved Playback");

    const float btn_w = (field_w - 12.0f) * 0.5f;
    const float action_y = panel.y + panel.height - 102.0f;
    if (button({panel.x + 16, action_y, btn_w, 32}, "Top Iter Log", kButtonDark)) {
        run_output_scroll_ = 0;
    }
    if (button({panel.x + 28 + btn_w, action_y, btn_w, 32}, "Save Solution", kButtonDark)) {
        saveSolutionToFile();
    }

    if (button({panel.x + 16, panel.y + panel.height - 54, btn_w, 40}, "SOLVE", kButtonAccent)) {
        runSolve();
    }
    if (button({panel.x + 28 + btn_w, panel.y + panel.height - 54, btn_w, 40}, "RESET", kButtonDark)) {
        resetState();
    }
}

void MainWindow::drawBoardPanel(const Rectangle& panel) {
    DrawRectangleRounded(panel, 0.03f, 8, kPanel);
    const std::string title = "PUZZLE MAP";
    const int title_w = measureUiText(title, 34);
    drawUiText(title, panel.x + (panel.width - title_w) * 0.5f, panel.y + 14, 34, kText);

    std::string state_caption = "Active state: -";
    if (has_map_ && !active_states_.empty()) {
        if (sequence_mode_ == SequenceMode::Process) {
            state_caption = "Active state: Iteration " + std::to_string(current_index_ + 1);
        } else {
            state_caption = "Active state: Step " + std::to_string(current_index_);
        }
    }
    const int caption_w = measureUiText(state_caption, 24);
    drawUiText(state_caption, panel.x + (panel.width - caption_w) * 0.5f, panel.y + 54, 24, kSubText);

    Rectangle board_area{panel.x + 14, panel.y + 84, panel.width - 28, panel.height - 96};
    const State* state_ptr = (has_map_ && !active_states_.empty()) ? &active_states_[current_index_] : nullptr;
    board_widget_.draw(has_map_ ? &map_ : nullptr, state_ptr, board_area);
}

void MainWindow::drawIterLogPanel(const Rectangle& panel) {
    DrawRectangleRounded(panel, 0.03f, 8, kPanel);
    drawUiText("ITERATION LOG", panel.x + 14, panel.y + 12, 30, kText);
    drawUiText("Live during solve/play", panel.x + 14, panel.y + 44, 18, kSubText);
    DrawRectangle(panel.x + 10, panel.y + 66, panel.width - 20, 3, kButtonAccent);

    Rectangle body{panel.x + 10, panel.y + 78, panel.width - 20, panel.height - 88};
    DrawRectangleRec(body, kInputBg);
    DrawRectangleLinesEx(body, 1.0f, kOutline);

    if (run_output_lines_.empty()) {
        drawUiText("Belum ada output. Klik SOLVE untuk menjalankan.", body.x + 10, body.y + 10, 18, kText);
        return;
    }

    const int line_h = 20;
    const int visible_lines = std::max(1, static_cast<int>((body.height - 12.0f) / line_h));
    const int max_start = std::max(0, static_cast<int>(run_output_lines_.size()) - visible_lines);
    const int start = std::clamp(run_output_scroll_, 0, max_start);
    run_output_scroll_ = start;

    int y = static_cast<int>(body.y + 6);
    for (int i = start; i < static_cast<int>(run_output_lines_.size()) && i < start + visible_lines; ++i) {
        drawUiText(run_output_lines_[i], body.x + 8, static_cast<float>(y), 16, kText);
        y += line_h;
    }
}

void MainWindow::drawBottomPanel(const Rectangle& panel) {
    DrawRectangleRounded(panel, 0.03f, 8, kPanel);
    drawUiText("SOLUTION PLAYBACK & METRICS", panel.x + 16, panel.y + 8, 30, kText);
    DrawRectangle(panel.x + 8, panel.y + 44, panel.width - 16, 4, kButtonAccent);

    const float y = panel.y + 66;
    const float controls_group_w = 5.0f * 44.0f + 4.0f * 6.0f;
    const float controls_x = panel.x + panel.width * 0.53f - controls_group_w * 0.5f;

    if (button({controls_x + 0.0f, y, 44, 30}, "|<<", kButton)) {
        autoplay_ = false;
        current_index_ = 0;
    }
    if (button({controls_x + 50.0f, y, 44, 30}, "<", kButton)) {
        autoplay_ = false;
        current_index_ = std::max(0, current_index_ - 1);
    }
    if (button({controls_x + 100.0f, y, 44, 30}, autoplay_ ? "||" : ">", kButtonAccent)) {
        if (!active_states_.empty()) autoplay_ = !autoplay_;
    }
    if (button({controls_x + 150.0f, y, 44, 30}, ">", kButton)) {
        autoplay_ = false;
        current_index_ = std::min(current_index_ + 1, std::max(0, static_cast<int>(active_states_.size()) - 1));
    }
    if (button({controls_x + 200.0f, y, 44, 30}, ">>|", kButton)) {
        autoplay_ = false;
        current_index_ = std::max(0, static_cast<int>(active_states_.size()) - 1);
    }

    if (button({controls_x + 252.0f, y, 30, 30}, "-", kButtonDark)) {
        autoplay_step_seconds_ = std::max(0.05, autoplay_step_seconds_ - 0.05);
    }
    if (button({controls_x + 288.0f, y, 30, 30}, "+", kButtonDark)) {
        autoplay_step_seconds_ = std::min(2.00, autoplay_step_seconds_ + 0.05);
    }
    drawUiText(
        "Speed: " + formatSeconds2(autoplay_step_seconds_) + " s/step",
        controls_x + 324.0f,
        y + 6.0f,
        16,
        kText
    );
    clampCurrentIndex();

    Rectangle slider{panel.x + panel.width - 280.0f, y + 12, 248, 6};
    DrawRectangleRec(slider, kInputBg);
    DrawRectangleLinesEx(slider, 1.0f, kOutline);
    if (!active_states_.empty()) {
        const int max_idx = std::max(1, static_cast<int>(active_states_.size()) - 1);
        const float ratio = static_cast<float>(current_index_) / static_cast<float>(max_idx);
        const float knob_x = slider.x + ratio * slider.width;
        DrawCircle(static_cast<int>(knob_x), static_cast<int>(slider.y + slider.height / 2.0f), 8.0f, kButtonAccent);

        if (CheckCollisionPointRec(GetMousePosition(), {slider.x - 6, slider.y - 10, slider.width + 12, 24}) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float t = (GetMousePosition().x - slider.x) / slider.width;
            t = std::clamp(t, 0.0f, 1.0f);
            current_index_ = static_cast<int>(std::round(t * max_idx));
            autoplay_ = false;
        }
    }

    std::string step_txt = "Step 0 / 0";
    if (!active_states_.empty()) {
        step_txt = "Step " + std::to_string(current_index_) + " / " + std::to_string(active_states_.size() - 1);
    }
    drawUiText(step_txt, slider.x - 94.0f, y + 5, 18, kText);

    std::string cost = has_solution_ ? std::to_string(solution_node_.g) : "-";
    std::string moves = has_solution_ ? solution_node_.path : "-";
    std::string metrics1 =
        "Solution Cost: " + cost + "   Execution Time: " + formatMs5(elapsed_ms_) + " ms   Total Iterations: "
        + std::to_string(iteration_logs_.size());
    bool needs_h = (algoText() == "GBFS" || algoText() == "A*" || algoText() == "IDA*");
    std::string metrics2 = "Selected Algo: " + algoText() + " (" + (needs_h ? heuristicText() : "-") + ")   Found Moves: " + moves;
    drawUiText(metrics1, panel.x + 16, panel.y + 74, 18, kText);
    drawUiText(metrics2, panel.x + 16, panel.y + 94, 18, kText);

}

void MainWindow::drawLogOverlay() {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    DrawRectangle(0, 0, static_cast<int>(sw), static_cast<int>(sh), ColorAlpha(BLACK, 0.65f));
    Rectangle panel{120, 80, sw - 240.0f, sh - 160.0f};
    DrawRectangleRounded(panel, 0.02f, 8, Color{16, 24, 36, 250});
    drawUiText("RUN OUTPUT (UP/DOWN scroll, ESC close)", panel.x + 16, panel.y + 14, 24, kText);
    if (button({panel.x + panel.width - 106, panel.y + 12, 90, 30}, "Close", kButtonDark)) {
        show_log_overlay_ = false;
        return;
    }

    if (run_output_lines_.empty()) {
        drawUiText("Belum ada output run. Jalankan SOLVE dahulu.", panel.x + 20, panel.y + 58, 20, kText);
        return;
    }

    const int visible_lines = 22;
    const int start = std::clamp(run_output_scroll_, 0, std::max(0, static_cast<int>(run_output_lines_.size()) - visible_lines));
    int y = static_cast<int>(panel.y + 56);
    for (int i = start; i < static_cast<int>(run_output_lines_.size()) && i < start + visible_lines; ++i) {
        drawUiText(run_output_lines_[i], panel.x + 16, static_cast<float>(y), 18, kText);
        y += 22;
    }
}

void MainWindow::drawWarningPopup() {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    DrawRectangle(0, 0, static_cast<int>(sw), static_cast<int>(sh), ColorAlpha(BLACK, 0.55f));

    Rectangle panel{sw * 0.5f - 260.0f, sh * 0.5f - 100.0f, 520.0f, 200.0f};
    DrawRectangleRounded(panel, 0.03f, 8, Color{44, 32, 38, 245});
    DrawRectangleLinesEx(panel, 1.5f, Color{255, 143, 143, 255});
    drawUiText("WARNING", panel.x + 18.0f, panel.y + 14.0f, 30, Color{255, 196, 196, 255});
    drawUiText(warning_message_, panel.x + 18.0f, panel.y + 62.0f, 19, kText);

    Rectangle close_btn{panel.x + panel.width - 110.0f, panel.y + panel.height - 46.0f, 92.0f, 30.0f};
    if (button(close_btn, "Close", Color{140, 65, 65, 255})) {
        warning_open_ = false;
    }
}

bool MainWindow::button(const Rectangle& rect, const char* label, Color bg, Color fg) const {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    const Color draw = hover ? ColorAlpha(bg, 0.85f) : bg;
    DrawRectangleRec(rect, draw);
    DrawRectangleLinesEx(rect, 1.0f, kOutline);
    const int fs = 18;
    const int tw = measureUiText(label, fs);
    drawUiText(label, rect.x + (rect.width - tw) / 2.0f, rect.y + 6, fs, fg);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool MainWindow::textField(TextField& field, const char* label) {
    drawUiText(label, field.rect.x, field.rect.y - 22, 18, kText);

    if (CheckCollisionPointRec(GetMousePosition(), field.rect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        map_field_.active = false;
        cost_field_.active = false;
        field.active = true;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), field.rect)) {
        field.active = false;
    }

    DrawRectangleRec(field.rect, kInputBg);
    DrawRectangleLinesEx(field.rect, 1.0f, field.active ? kButtonAccent : kOutline);
    drawUiText(field.value, field.rect.x + 8, field.rect.y + 8, 18, field.value.empty() ? GRAY : kText);
    return field.active;
}

bool MainWindow::checkBox(const Rectangle& rect, bool* value, const char* label) {
    DrawRectangleRec(rect, *value ? kButtonAccent : kInputBg);
    DrawRectangleLinesEx(rect, 1.0f, kOutline);
    drawUiText(label, rect.x + rect.width + 10, rect.y - 1, 18, kText);
    if (CheckCollisionPointRec(GetMousePosition(), {rect.x, rect.y, rect.width + 260, rect.height})
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        *value = !(*value);
        refreshActiveSequence();
        return true;
    }
    return false;
}

void MainWindow::updateTextFieldInput(TextField& field) {
    if (!field.active) return;

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            field.value.push_back(static_cast<char>(ch));
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !field.value.empty()) {
        field.value.pop_back();
    }
}

void MainWindow::setStatus(const std::string& msg, double duration_sec) {
    status_message_ = msg;
    status_until_ = GetTime() + duration_sec;
}

void MainWindow::clampCurrentIndex() {
    if (active_states_.empty()) {
        current_index_ = 0;
    } else {
        current_index_ = std::clamp(current_index_, 0, static_cast<int>(active_states_.size()) - 1);
    }
}

void MainWindow::updatePlaybackFromKeyboard() {
    if (active_states_.empty()) return;
    if (IsKeyPressed(KEY_RIGHT)) {
        autoplay_ = false;
        current_index_ = std::min(current_index_ + 1, static_cast<int>(active_states_.size()) - 1);
    }
    if (IsKeyPressed(KEY_LEFT)) {
        autoplay_ = false;
        current_index_ = std::max(current_index_ - 1, 0);
    }
}

void MainWindow::updateAutoplay() {
    if (!autoplay_ || active_states_.empty()) return;
    double now = GetTime();
    if (now - last_autoplay_step_ < autoplay_step_seconds_) return;
    last_autoplay_step_ = now;
    if (current_index_ >= static_cast<int>(active_states_.size()) - 1) {
        autoplay_ = false;
        return;
    }
    ++current_index_;
}

std::filesystem::path MainWindow::resolvePathForRead(const std::string& raw) const {
    if (raw.empty()) return {};

    std::filesystem::path p(raw);
    if (p.is_absolute()) return p;

    const std::filesystem::path cwd_candidate = std::filesystem::current_path() / p;
    if (std::filesystem::exists(cwd_candidate)) return cwd_candidate;

    const std::filesystem::path exe_candidate = exe_dir_ / p;
    if (std::filesystem::exists(exe_candidate)) return exe_candidate;

    // fallback: relative to executable directory
    return exe_candidate;
}

std::filesystem::path MainWindow::resolvePathForWrite(const std::string& filename) const {
    std::filesystem::path out_name(filename);
    if (out_name.is_absolute()) return out_name;

    // New structure: prefer test/output for generated files.
    const std::filesystem::path cwd_out_dir = std::filesystem::current_path() / "test" / "output";
    std::error_code ec;
    if (std::filesystem::exists(cwd_out_dir, ec) && std::filesystem::is_directory(cwd_out_dir, ec)) {
        return cwd_out_dir / out_name;
    }

    const std::filesystem::path exe_out_dir = exe_dir_ / "test" / "output";
    ec.clear();
    if (std::filesystem::exists(exe_out_dir, ec) && std::filesystem::is_directory(exe_out_dir, ec)) {
        return exe_out_dir / out_name;
    }

    // If folder doesn't exist yet, create it beside executable.
    ec.clear();
    std::filesystem::create_directories(exe_out_dir, ec);
    if (!ec) {
        return exe_out_dir / out_name;
    }

    // Final fallback: beside executable.
    return exe_dir_ / out_name;
}

bool MainWindow::mergeSplitInputToCombined(
    const std::string& map_path,
    const std::string& cost_path,
    std::string& combined_output,
    std::string& error
) {
    const std::vector<std::string> map_lines = readNonEmptyLines(map_path);
    const std::vector<std::string> cost_lines = readNonEmptyLines(cost_path);
    if (map_lines.empty()) {
        error = "File map kosong.";
        return false;
    }
    if (cost_lines.empty()) {
        error = "File cost kosong.";
        return false;
    }

    std::istringstream size_stream(map_lines.front());
    int rows = 0;
    int cols = 0;
    if (!(size_stream >> rows >> cols) || rows <= 0 || cols <= 0) {
        error = "Baris pertama map harus berisi N M yang valid.";
        return false;
    }
    if (static_cast<int>(map_lines.size()) < rows + 1) {
        error = "Baris board pada file map tidak lengkap.";
        return false;
    }

    int cost_start = 0;
    {
        std::istringstream first_cost(cost_lines.front());
        int r = 0;
        int c = 0;
        if ((first_cost >> r >> c) && r == rows && c == cols) {
            cost_start = 1;
        }
    }

    std::ostringstream merged;
    merged << map_lines.front() << "\n";
    for (int i = 1; i <= rows; ++i) {
        merged << map_lines[i] << "\n";
    }
    for (int i = cost_start; i < static_cast<int>(cost_lines.size()); ++i) {
        merged << cost_lines[i] << "\n";
    }
    combined_output = merged.str();
    return true;
}

bool MainWindow::loadMapFromInputs(std::string& error) {
    if (map_field_.value.empty()) {
        error = "Isi path file map terlebih dahulu.";
        return false;
    }

    const std::filesystem::path map_path = resolvePathForRead(map_field_.value);
    const std::filesystem::path cost_path = resolvePathForRead(cost_field_.value);
    if (!std::filesystem::exists(map_path)) {
        error = "File map tidak ditemukan: " + map_path.string();
        return false;
    }

    bool ok = false;
    if (cost_field_.value.empty()) {
        ok = map_.load(map_path.string());
    } else {
        std::string merged_content;
        if (!std::filesystem::exists(cost_path)) {
            error = "File cost tidak ditemukan: " + cost_path.string();
            return false;
        }
        if (!mergeSplitInputToCombined(map_path.string(), cost_path.string(), merged_content, error)) {
            return false;
        }
        const std::filesystem::path temp = std::filesystem::temp_directory_path() / "ice_sliding_gui_combined_input.txt";
        if (!writeTextFile(temp.string(), merged_content)) {
            error = "Gagal membuat file sementara input gabungan.";
            return false;
        }
        ok = map_.load(temp.string());
    }

    if (!ok) {
        error = map_.last_error.empty() ? "Gagal memuat map." : map_.last_error;
        return false;
    }

    has_map_ = true;
    return true;
}

void MainWindow::runSolve() {
    autoplay_ = false;
    std::string err;
    if (!loadMapFromInputs(err)) {
        warning_message_ = err;
        warning_open_ = true;
        setStatus("Error: " + err, 4.0);
        return;
    }
    warning_open_ = false;
    warning_message_.clear();

    int iterations = 0;
    iteration_logs_.clear();
    const auto start = std::chrono::high_resolution_clock::now();
    bool needs_h = (algoText() == "GBFS" || algoText() == "A*" || algoText() == "IDA*");
    solution_node_ = solve(map_, algoText(), (needs_h ? heuristicText() : "H1"), iterations, &iteration_logs_);
    const auto finish = std::chrono::high_resolution_clock::now();
    elapsed_ms_ = std::chrono::duration<double, std::milli>(finish - start).count();

    has_solution_ = !(solution_node_.path.empty() && (solution_node_.state.r == -1 || solution_node_.g == -1));
    buildSolutionStates();
    refreshActiveSequence();
    clampCurrentIndex();
    run_output_text_ = buildCliStyleOutput();
    rebuildRunOutputLines();
    run_output_scroll_ = 0;

    if (has_solution_) {
        setStatus("Solusi ditemukan.");
    } else {
        setStatus("Solusi tidak ditemukan.");
    }
}

void MainWindow::resetState() {
    has_map_ = false;
    has_solution_ = false;
    iteration_logs_.clear();
    solution_states_.clear();
    active_states_.clear();
    current_index_ = 0;
    elapsed_ms_ = 0.0;
    autoplay_ = false;
    show_log_overlay_ = false;
    run_output_text_.clear();
    run_output_lines_.clear();
    run_output_scroll_ = 0;
    setStatus("State GUI di-reset.");
}

void MainWindow::buildSolutionStates() {
    solution_states_.clear();
    State start{map_.start.r, map_.start.c, 0};
    solution_states_.push_back(start);
    if (!has_solution_) return;

    State curr = start;
    for (char move : solution_node_.path) {
        SlideResult res = performSlide(map_, curr, move);
        if (!res.valid) break;
        curr = res.final_state;
        solution_states_.push_back(curr);
    }
}

void MainWindow::refreshActiveSequence() {
    if (!has_map_) {
        active_states_.clear();
        current_index_ = 0;
        return;
    }

    if (show_process_ && !iteration_logs_.empty()) {
        sequence_mode_ = SequenceMode::Process;
        active_states_.clear();
        for (const IterationLog& log : iteration_logs_) active_states_.push_back(log.state);
    } else if (show_playback_ && !solution_states_.empty()) {
        sequence_mode_ = SequenceMode::Playback;
        active_states_ = solution_states_;
    } else if (!solution_states_.empty()) {
        sequence_mode_ = SequenceMode::Playback;
        active_states_ = solution_states_;
    } else if (!iteration_logs_.empty()) {
        sequence_mode_ = SequenceMode::Process;
        active_states_ = {iteration_logs_.front().state};
    } else {
        sequence_mode_ = SequenceMode::Process;
        active_states_ = {State{map_.start.r, map_.start.c, 0}};
    }
    current_index_ = 0;
}

void MainWindow::saveSolutionToFile() {
    if (!has_map_) {
        setStatus("Belum ada data untuk disimpan.");
        return;
    }

    const std::string out_name = buildSearchOutputFileName(map_field_.value);
    const std::filesystem::path out_path = resolvePathForWrite(out_name);
    std::ofstream out(out_path);
    if (!out) {
        setStatus("Gagal menulis " + out_name);
        return;
    }

    // Samakan isi file output dengan panel ITERATION LOG.
    if (run_output_text_.empty()) {
        out << buildCliStyleOutput();
    } else {
        out << run_output_text_;
    }
    setStatus("Solusi disimpan: " + out_path.string(), 4.0);

    // Hapus file log iterasi lama agar output hanya satu file.
    std::error_code ec;
    std::filesystem::remove(resolvePathForWrite("iterasi_pencarian_gui.txt"), ec);
}

void MainWindow::saveIterationLogToFile() {
    const std::filesystem::path out_path = resolvePathForWrite("iterasi_pencarian_gui.txt");
    std::ofstream out(out_path);
    if (!out) {
        setStatus("Gagal menulis iterasi_pencarian_gui.txt");
        return;
    }
    out << "Algoritma: " << algoText() << "\n";
    if (algoText() == "GBFS" || algoText() == "A*" || algoText() == "IDA*") out << "Heuristic: " << heuristicText() << "\n";
    out << "Total iterasi: " << iteration_logs_.size() << "\n\n";

    if (iteration_logs_.empty()) {
        out << "Tidak ada data iterasi.\n";
        setStatus("Log iterasi disimpan: " + out_path.string(), 4.0);
        return;
    }

    // Simpan seluruh jejak iterasi yang ditinjau algoritma (awal sampai akhir).
    for (const IterationLog& log : iteration_logs_) {
        out << "Iterasi " << log.iteration << "\n";
        out << "Posisi: (" << log.state.r << ", " << log.state.c << ")\n";
        out << "Mask waypoint: " << log.state.mask << "\n";
        out << "Path: " << log.path << "\n";
        out << "g: " << log.g << ", h: " << log.h << "\n";
        out << renderMapState(log.state);
        out << "\n";
    }
    setStatus("Log iterasi disimpan: " + out_path.string(), 4.0);
}

std::string MainWindow::renderMapState(const State& state) const {
    std::ostringstream out;
    for (int r = 0; r < map_.rows; ++r) {
        for (int c = 0; c < map_.cols; ++c) {
            if (r == state.r && c == state.c) {
                out << 'Z';
                continue;
            }
            char ch = map_.grid[r][c];
            if (ch >= '0' && ch <= '9' && (ch - '0') < state.mask) {
                out << '*';
            } else {
                out << ch;
            }
        }
        out << "\n";
    }
    return out.str();
}

std::string MainWindow::buildCliStyleOutput() const {
    std::ostringstream out;
    auto appendIterationSection = [&]() {
        out << "\n===== KONFIGURASI / ITERASI PENCARIAN =====\n";
        out << "Total iterasi: " << iteration_logs_.size() << "\n\n";
        if (iteration_logs_.empty()) {
            out << "Tidak ada data iterasi.\n";
            return;
        }

        for (const IterationLog& log : iteration_logs_) {
            out << "Iterasi " << log.iteration << "\n";
            out << "Posisi: (" << log.state.r << ", " << log.state.c << ")\n";
            out << "Mask waypoint: " << log.state.mask << "\n";
            out << "Path: " << log.path << "\n";
            out << "g: " << log.g << ", h: " << log.h << "\n";
            out << renderMapState(log.state) << "\n";
        }
    };

    out << ">> Masukan file input :\n";
    out << "   " << map_field_.value << "\n";
    out << ">> Algoritma apa yang anda pilih? (UCS/BFS/GBFS/A*/IDA*)\n";
    out << "   " << algoText() << "\n";
    if (algoText() == "GBFS" || algoText() == "A*" || algoText() == "IDA*") {
        out << ">> Heuristic apa yang anda pilih? (H1/H2/H3)\n";
        out << "   " << heuristicText() << "\n";
    }
    appendIterationSection();

    if (!has_solution_) {
        out << "Solusi tidak ditemukan!\n";
        out << ">> Waktu eksekusi: " << formatMs5(elapsed_ms_) << " ms\n";
        out << ">> Banyak iterasi yang dilakukan: " << iteration_logs_.size() << " iterasi\n";
        return out.str();
    }

    out << "Solusi Yang Ditemukan : " << solution_node_.path << "\n";
    out << "Cost dari Solusi : " << solution_node_.g << "\n";

    out << ">> Waktu eksekusi: " << formatMs5(elapsed_ms_) << " ms\n";
    out << ">> Banyak iterasi yang dilakukan: " << iteration_logs_.size() << " iterasi\n";
    out << ">> Apakah Anda ingin melakukan playback? (Ya/Tidak) : \n";
    out << "   " << (show_playback_ ? "Ya" : "Tidak") << "\n";
    if (show_playback_) {
        out << ">> Pada step berapa anda ingin melakukan playback : \n";
        out << "   " << current_index_ << "\n";
    }
    out << ">> Apakah Anda ingin menyimpan solusi? (Ya/Tidak) :\n";
    out << "   Ya\n";
    return out.str();
}

void MainWindow::rebuildRunOutputLines() {
    run_output_lines_.clear();
    std::stringstream ss(run_output_text_);
    std::string line;
    while (std::getline(ss, line)) {
        run_output_lines_.push_back(line);
    }
}

void MainWindow::refreshTxtCandidates() {
    txt_candidates_.clear();
    const std::filesystem::path test_dir = resolveTestDir();
    if (!test_dir.empty() && std::filesystem::exists(test_dir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(test_dir)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".txt") continue;
            std::error_code ec;
            const auto rel = std::filesystem::relative(entry.path(), std::filesystem::current_path(), ec);
            const std::string path_str = ec ? entry.path().generic_string() : rel.generic_string();
            txt_candidates_.push_back(path_str);
        }
    }
    std::sort(txt_candidates_.begin(), txt_candidates_.end());
}

std::string MainWindow::pickNextTxt(int& idx, bool allow_empty) const {
    if (txt_candidates_.empty()) {
        return allow_empty ? "" : "test/input/input.txt";
    }

    if (allow_empty && idx == -1) {
        idx = 0;
        return "";
    }

    idx = (idx < 0) ? 0 : (idx + 1);

    if (allow_empty) {
        const int count = static_cast<int>(txt_candidates_.size()) + 1;
        idx %= count;
        if (idx == 0) return "";
        return txt_candidates_[idx - 1];
    }

    idx %= static_cast<int>(txt_candidates_.size());
    return txt_candidates_[idx];
}

std::filesystem::path MainWindow::resolveTestDir() const {
    // New structure: test/input for source files.
    const std::filesystem::path cwd_test_input = std::filesystem::current_path() / "test" / "input";
    if (std::filesystem::exists(cwd_test_input) && std::filesystem::is_directory(cwd_test_input)) {
        return cwd_test_input;
    }

    const std::filesystem::path exe_test_input = exe_dir_ / "test" / "input";
    if (std::filesystem::exists(exe_test_input) && std::filesystem::is_directory(exe_test_input)) {
        return exe_test_input;
    }

    // Backward compatibility (old structure).
    const std::filesystem::path cwd_test = std::filesystem::current_path() / "test";
    if (std::filesystem::exists(cwd_test) && std::filesystem::is_directory(cwd_test)) {
        return cwd_test;
    }

    const std::filesystem::path exe_test = exe_dir_ / "test";
    if (std::filesystem::exists(exe_test) && std::filesystem::is_directory(exe_test)) {
        return exe_test;
    }
    return {};
}

std::string MainWindow::algoText() const {
    static const std::vector<std::string> values{"UCS", "BFS", "GBFS", "A*", "IDA*"};
    return values[algo_index_ % static_cast<int>(values.size())];
}

std::string MainWindow::heuristicText() const {
    static const std::vector<std::string> values{"H1", "H2", "H3"};
    return values[heuristic_index_ % static_cast<int>(values.size())];
}
