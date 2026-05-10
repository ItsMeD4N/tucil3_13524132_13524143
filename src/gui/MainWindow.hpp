#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "raylib.h"
#include "../core/Map.hpp"
#include "../core/Solver.hpp"
#include "BoardWidget.hpp"

class MainWindow {
public:
    MainWindow();
    void run();

private:
    struct TextField {
        Rectangle rect;
        std::string value;
        bool active;
    };

    enum class SequenceMode { Process, Playback };

    // UI state
    TextField map_field_;
    TextField cost_field_;
    int algo_index_;
    int heuristic_index_;
    bool show_process_;
    bool show_playback_;
    bool show_log_overlay_;
    int log_scroll_;

    // Runtime data
    Map map_;
    bool has_map_;
    bool has_solution_;
    Node solution_node_;
    std::vector<IterationLog> iteration_logs_;
    std::vector<State> solution_states_;
    std::vector<State> active_states_;
    SequenceMode sequence_mode_;
    int current_index_;
    double elapsed_ms_;
    bool autoplay_;
    double last_autoplay_step_;
    double autoplay_step_seconds_;
    std::string status_message_;
    double status_until_;
    bool warning_open_;
    std::string warning_message_;
    std::filesystem::path exe_dir_;
    bool window_maximized_;
    Font ui_font_;
    bool font_loaded_;
    std::string run_output_text_;
    std::vector<std::string> run_output_lines_;
    int run_output_scroll_;
    std::vector<std::string> txt_candidates_;
    int map_pick_index_;
    int cost_pick_index_;
    enum class PickerTarget { None, Map, Cost, Algo, Heuristic };
    PickerTarget picker_target_;
    bool picker_open_;
    int picker_scroll_;

    BoardWidget board_widget_;

    // render/input
    void handleInput();
    void draw();
    void drawConfigPanel(const Rectangle& panel);
    void drawBoardPanel(const Rectangle& panel);
    void drawIterLogPanel(const Rectangle& panel);
    void drawBottomPanel(const Rectangle& panel);
    void drawLogOverlay();
    void drawWarningPopup();

    // helpers
    bool button(const Rectangle& rect, const char* label, Color bg, Color fg = WHITE) const;
    bool textField(TextField& field, const char* label);
    bool checkBox(const Rectangle& rect, bool* value, const char* label);
    void updateTextFieldInput(TextField& field);
    void setStatus(const std::string& msg, double duration_sec = 2.5);
    void clampCurrentIndex();
    void updatePlaybackFromKeyboard();
    void updateAutoplay();
    void drawWindowControls();
    void drawUiText(const std::string& text, float x, float y, int size, Color color) const;
    int measureUiText(const std::string& text, int size) const;

    // solve/data
    bool loadMapFromInputs(std::string& error);
    bool mergeSplitInputToCombined(const std::string& map_path, const std::string& cost_path, std::string& combined_output, std::string& error);
    std::filesystem::path resolvePathForRead(const std::string& raw) const;
    std::filesystem::path resolvePathForWrite(const std::string& filename) const;
    void runSolve();
    void resetState();
    void buildSolutionStates();
    void refreshActiveSequence();
    void saveSolutionToFile();
    void saveIterationLogToFile();
    std::string renderMapState(const State& state) const;
    std::string buildCliStyleOutput() const;
    void rebuildRunOutputLines();
    void refreshTxtCandidates();
    std::string pickNextTxt(int& idx, bool allow_empty = false) const;
    std::filesystem::path resolveTestDir() const;

    // text helpers
    std::string algoText() const;
    std::string heuristicText() const;
};
