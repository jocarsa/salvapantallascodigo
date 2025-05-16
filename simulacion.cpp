#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <random>
#include <chrono>
#include <list>
#include <memory>
#include <iomanip>

// Window class with optimizations
class Window {
private:
    int x, y, w, h;
    std::shared_ptr<std::vector<std::string>> code_lines; // Shared code lines
    int current_line;
    int scroll_offset;
    cv::Mat cached_background; // Cache for the background
    bool needs_redraw;         // Flag to track if redraw is needed

public:
    Window(int x_pos, int y_pos, int width, int height, std::shared_ptr<std::vector<std::string>> lines)
        : x(x_pos), y(y_pos), w(width), h(height), code_lines(lines), current_line(0), scroll_offset(0), needs_redraw(true) {
        // Pre-create the window background
        cached_background = cv::Mat(height, width, CV_8UC3, cv::Scalar(0,0,0));
        cv::rectangle(cached_background, cv::Rect(0, 0, width, 30), cv::Scalar(255,255,255), -1);
        cv::rectangle(cached_background, cv::Rect(0, 30, width, height-30), cv::Scalar(255,255,255), 2);
    }

    void draw(cv::Mat& frame) {
        // Add semi-transparent window to frame
        cv::Mat roi = frame(cv::Rect(x, y, w, h));
        double alpha = 0.5;

        // Only apply the cached background
        cv::Mat overlay = roi.clone();
        cached_background.copyTo(overlay);
        cv::addWeighted(overlay, alpha, roi, 1.0 - alpha, 0, roi);

        // Text parameters
        int line_spacing = 30;
        int left_margin = 40;
        int top_margin = 60;
        cv::Scalar text_color(0,255,0);

        // Get font metrics to calculate text width
        int font_face = cv::FONT_HERSHEY_SIMPLEX;
        double font_scale = 0.7;
        int font_thickness = 2;
        int right_margin = 20; // Margin from the right edge of the window

        // Only render visible lines - with bounds checking
        int visible_lines = (h - top_margin) / line_spacing;
        int start_line = scroll_offset / line_spacing;
        int end_line = std::min(current_line, start_line + visible_lines + 1);

        // Calculate maximum available width for text
        int max_text_width = w - left_margin - right_margin;

        // Further optimization - only render lines that are in the visible area
        for (int i = start_line; i < end_line && i < static_cast<int>(code_lines->size()); ++i) {
            std::string line = (*code_lines)[i];
            if (!line.empty() && (line.back()=='\n' || line.back()=='\r' || line.back()=='?'))
                line.pop_back();

            int yy = y + top_margin + (i - start_line) * line_spacing;
            if (yy >= y + top_margin && yy < y + h - 5) {
                // Crop text that would overflow horizontally
                std::string display_text = line;
                int baseline = 0;
                cv::Size text_size = cv::getTextSize(display_text, font_face, font_scale, font_thickness, &baseline);

                // If text is too wide, crop it
                if (text_size.width > max_text_width) {
                    // Binary search to find the maximum number of characters that fit
                    int left = 0;
                    int right = display_text.length();
                    std::string ellipsis = "...";

                    while (left < right) {
                        int mid = left + (right - left) / 2;
                        std::string test_text = display_text.substr(0, mid) + ellipsis;
                        cv::Size test_size = cv::getTextSize(test_text, font_face, font_scale, font_thickness, &baseline);

                        if (test_size.width <= max_text_width) {
                            left = mid + 1;
                        } else {
                            right = mid;
                        }
                    }

                    // Ensure we have at least some characters plus ellipsis
                    int max_chars = std::max(1, left - 1);
                    display_text = display_text.substr(0, max_chars) + ellipsis;
                }

                cv::putText(frame, display_text, cv::Point(x + left_margin, yy),
                            font_face, font_scale, text_color, font_thickness, cv::LINE_AA);
            }
        }

        needs_redraw = false;
    }

    bool update() {
        if (current_line < static_cast<int>(code_lines->size())) {
            current_line++;
            needs_redraw = true;

            // Scroll if overflow
            int line_spacing = 30;
            int top_margin = 60;
            if (current_line * line_spacing > h - top_margin) {
                scroll_offset += line_spacing;

                int max_offset = current_line * line_spacing - (h - top_margin);
                if (scroll_offset > max_offset && max_offset > 0)
                    scroll_offset = max_offset;
            }

            return true; // Window is still active
        }

        // Return false when window has displayed all content
        return current_line < static_cast<int>(code_lines->size());
    }

    bool needsRedraw() const {
        return needs_redraw;
    }

    bool isFinished() const {
        return current_line >= static_cast<int>(code_lines->size());
    }
};

Window createRandomWindow(int width, int height, std::shared_ptr<std::vector<std::string>> code_lines) {
    static thread_local std::mt19937 gen{std::random_device{}()};

    // Constrain window sizes to be more reasonable
    std::uniform_int_distribution<> w_dist(400, std::min(800, width - 100));
    std::uniform_int_distribution<> h_dist(300, std::min(600, height - 100));
    int w = w_dist(gen), h = h_dist(gen);

    std::uniform_int_distribution<> x_dist(0, width - w - 1);
    std::uniform_int_distribution<> y_dist(0, height - h - 1);
    int x = x_dist(gen), y = y_dist(gen);

    return Window(x, y, w, h, code_lines);
}

int main() {
    // Read code from file
    std::string file_path = "codigo.cpp";
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << file_path << std::endl;
        return 1;
    }

    // Store code lines in a shared pointer to avoid copies
    auto code_lines = std::make_shared<std::vector<std::string>>();
    for (std::string line; std::getline(file, line); )
        code_lines->push_back(line);
    file.close();

    // Video settings
    const int width = 3840;
    const int height = 2160;
    const int fps = 10;
    cv::Scalar background_color(0,0,0);

    // Output file with timestamp
    std::time_t epoch_time = std::time(nullptr);
    std::string output_file = "output_" + std::to_string(epoch_time) + ".mp4";
    cv::VideoWriter video(output_file,
                         cv::VideoWriter::fourcc('m','p','4','v'),
                         fps, cv::Size(width, height));
    if (!video.isOpened()) {
        std::cerr << "Could not open the output video file for writing" << std::endl;
        return 1;
    }

    // Frame buffer
    cv::Mat frame(height, width, CV_8UC3, background_color);

    int total_frames = 10 * 60 * 60 * fps;  // 10 hours of video
    int frames_written = 0;
    int stats_interval = 10 * fps;  // Show stats every 10 seconds
    int last_stats_frame = 0;

    // Window management with maximum limit
    std::list<Window> active_windows;
    const size_t MAX_WINDOWS = 10;  // Limit maximum number of windows

    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::mt19937 gen(std::random_device{}());

    auto start_time = std::chrono::steady_clock::now();

    // Main render loop
    while (frames_written < total_frames) {
        // Clear frame
        frame = background_color;

        // Maybe spawn a new window, but only if under the limit
        if (active_windows.size() < MAX_WINDOWS && dis(gen) < 0.02) {
            active_windows.push_back(createRandomWindow(width, height, code_lines));
            // When adding a window, immediately update it to ensure it displays something
            active_windows.back().update();
        }

        // Update windows (returns false when window has finished)
        for (auto it = active_windows.begin(); it != active_windows.end(); ) {
            if (!it->update() || (dis(gen) < 0.005 && it->isFinished())) {
                // Remove finished windows or randomly close completed ones
                it = active_windows.erase(it);
            } else {
                ++it;
            }
        }

        // Draw only windows that need redrawing
        for (auto& window : active_windows) {
            if (window.needsRedraw()) {
                window.draw(frame);
            }
        }

        // Write frame and update stats
        video.write(frame);
        frames_written++;

        if (frames_written - last_stats_frame >= stats_interval) {
            last_stats_frame = frames_written;
            double pct = (frames_written * 100.0) / total_frames;
            int rem = (total_frames - frames_written) / fps;

            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            double fps_actual = elapsed_time > 0 ? frames_written / static_cast<double>(elapsed_time) : 0;
            int estimated_total_time = (pct > 0) ? static_cast<int>((elapsed_time * 100.0) / pct) : 0;
            int estimated_remaining_time = (pct > 0) ? estimated_total_time - elapsed_time : 0;

            // Convert remaining seconds to hours, minutes, and seconds
            int hours = rem / 3600;
            int minutes = (rem % 3600) / 60;
            int seconds = rem % 60;

            // Use carriage return to update the same line
            std::cout << "\rFrames: " << frames_written
                      << " (" << std::fixed << std::setprecision(2) << pct << "%) / Remaining video time: "
                      << hours << "h " << minutes << "m " << seconds << "s"
                      << " / Active windows: " << active_windows.size()
                      << " / Actual FPS: " << fps_actual << std::flush;
        }
    }

    video.release();
    std::cout << "\nSaved to " << output_file << std::endl;
    return 0;
}

