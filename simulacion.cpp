#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <random>
#include <chrono>
#include <list>

// Window class definition remains the same
class Window {
private:
    int x, y, w, h;
    std::vector<std::string> code_lines;
    int current_line;
    int scroll_offset;

public:
    Window(int x_pos, int y_pos, int width, int height, const std::vector<std::string>& lines)
        : x(x_pos), y(y_pos), w(width), h(height), code_lines(lines), current_line(0), scroll_offset(0) {}

    void draw(cv::Mat& frame) {
        // Create a semi-transparent overlay for the window
        cv::Mat overlay = frame.clone();
        cv::rectangle(overlay, cv::Rect(x, y, w, h), cv::Scalar(0,0,0), -1);
        double alpha = 0.5;
        cv::addWeighted(overlay, alpha, frame, 1.0 - alpha, 0, frame);

        // Draw borders
        cv::rectangle(frame, cv::Rect(x, y, w, 30), cv::Scalar(255,255,255), -1);
        cv::rectangle(frame, cv::Rect(x, y+30, w, h-30), cv::Scalar(255,255,255), 2);

        // Text parameters
        int line_spacing = 30;
        int left_margin = 40;
        int top_margin  = 60;
        cv::Scalar text_color(0,255,0);

        // Render visible lines
        int y_pos0 = y + top_margin - scroll_offset;
        for (int i = 0; i < current_line && i < (int)code_lines.size(); ++i) {
            std::string line = code_lines[i];
            if (!line.empty() && (line.back()=='\n' || line.back()=='\r' || line.back()=='?'))
                line.pop_back();

            int yy = y_pos0 + i*line_spacing;
            if (yy >= y + top_margin && yy < y + h - line_spacing) {
                cv::putText(frame, line, cv::Point(x + left_margin, yy),
                            cv::FONT_HERSHEY_SIMPLEX, 0.7, text_color, 2, cv::LINE_AA);
            }
        }
    }

    void update() {
        if (current_line < (int)code_lines.size())
            current_line++;

        // Scroll if overflow
        int line_spacing = 30;
        int top_margin = 60;
        if (current_line * line_spacing > h - top_margin)
            scroll_offset += line_spacing;

        int max_offset = current_line * line_spacing - (h - top_margin);
        if (scroll_offset > max_offset && max_offset > 0)
            scroll_offset = max_offset;
    }
};

Window createRandomWindow(int width, int height, const std::vector<std::string>& code_lines) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> w_dist(600, width - 100);
    std::uniform_int_distribution<> h_dist(600, height - 100);
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
    std::vector<std::string> code_lines;
    for (std::string line; std::getline(file, line); )
        code_lines.push_back(line);
    file.close();

    // Video settings
    const int width  = 3840;
    const int height = 2160;
    const int fps    = 10;
    cv::Scalar background_color(0,0,0);

    // Output file with timestamp
    std::time_t epoch_time = std::time(nullptr);
    std::string output_file = "output_" + std::to_string(epoch_time) + ".mp4";
    cv::VideoWriter video(output_file,
                          cv::VideoWriter::fourcc('m','p','4','v'),
                          fps, cv::Size(width,height));
    if (!video.isOpened()) {
        std::cerr << "Could not open the output video file for writing" << std::endl;
        return 1;
    }

    // Frame buffer
    cv::Mat frame(height, width, CV_8UC3, background_color);

    int total_frames    = 3600 * fps;
    int frames_written  = 0;
    int stats_interval  = 10 * fps;
    int last_stats_frame= 0;

    std::list<Window> active_windows;
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::mt19937 gen(std::random_device{}());

    auto start_time = std::chrono::steady_clock::now();

    // Main render loop
    while (frames_written < total_frames) {
        // Clear frame
        frame = background_color;

        // Maybe spawn a new window
        if (dis(gen) < 0.02)
            active_windows.push_back(createRandomWindow(width, height, code_lines));

        // Update and draw windows
        for (auto& window : active_windows)
            window.update();

        for (auto& window : active_windows)
            window.draw(frame);

        // Maybe close a window
        if (!active_windows.empty() && dis(gen) < 0.01) {
            auto it = active_windows.begin();
            std::advance(it, std::uniform_int_distribution<>(0, active_windows.size() - 1)(gen));
            active_windows.erase(it);
        }

        // Write and stats
        video.write(frame);
        frames_written++;
        if (frames_written - last_stats_frame >= stats_interval) {
            last_stats_frame = frames_written;
            double pct = (frames_written * 100.0) / total_frames;
            int rem = (total_frames - frames_written) / fps;

            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            int estimated_total_time = (pct > 0) ? static_cast<int>((elapsed_time * 100.0) / pct) : 0;
            int estimated_remaining_time = (pct > 0) ? estimated_total_time - elapsed_time : 0;

            std::cout << "Frames: " << frames_written
                      << " (" << pct << "%) / Remaining video seconds: "
                      << rem << "s / Estimated time remaining: "
                      << estimated_remaining_time << "s\n";
        }
    }

    video.release();
    std::cout << "Saved to " << output_file << std::endl;
    return 0;
}
