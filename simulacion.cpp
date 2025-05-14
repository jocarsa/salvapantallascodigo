#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <random>
#include <chrono>
#include <thread>

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
        cv::rectangle(overlay, cv::Rect(x, y, w, h), cv::Scalar(0, 0, 0), -1);

        // Apply the overlay with transparency
        double alpha = 0.5;
        cv::addWeighted(overlay, alpha, frame, 1.0 - alpha, 0, frame);

        // Draw the rectangle border with a thicker top bar
        cv::rectangle(frame, cv::Rect(x, y, w, 30), cv::Scalar(255, 255, 255), -1); // Top bar
        cv::rectangle(frame, cv::Rect(x, y + 30, w, h - 30), cv::Scalar(255, 255, 255), 2); // Rest of the border

        // Constants for text rendering
        int line_spacing = 30;
        int left_margin = 40;
        int top_margin = 60;
        cv::Scalar text_color(0, 255, 0); // Green

        // Draw the code lines with scrolling and padding
        int y_pos = y + top_margin - scroll_offset;
        for (int i = 0; i < current_line && i < code_lines.size(); ++i) {
            std::string line = code_lines[i];
            // Remove trailing newlines, carriage returns, and question marks
            if (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == '?')) {
                line.pop_back();
            }

            if (y + top_margin <= y_pos + i * line_spacing && y_pos + i * line_spacing < y + h - line_spacing) {
                cv::putText(frame, line, cv::Point(x + left_margin, y_pos + i * line_spacing),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, text_color, 2, cv::LINE_AA);
            }
        }
    }

    void update() {
        if (current_line < code_lines.size()) {
            current_line++;
        }

        // Check if scrolling is needed
        int line_spacing = 30;
        int top_margin = 60;
        if ((current_line * line_spacing) > (h - top_margin)) {
            scroll_offset += line_spacing;
        }

        // Ensure scroll_offset does not exceed the maximum possible offset
        int max_offset = current_line * line_spacing - (h - top_margin);
        if (scroll_offset > max_offset && max_offset > 0) {
            scroll_offset = max_offset;
        }
    }
};

Window createRandomWindow(int width, int height, const std::vector<std::string>& code_lines) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> w_dist(600, width - 100);
    std::uniform_int_distribution<> h_dist(600, height - 100);

    int w = w_dist(gen);
    int h = h_dist(gen);

    std::uniform_int_distribution<> x_dist(0, width - w - 1);
    std::uniform_int_distribution<> y_dist(0, height - h - 1);

    int x = x_dist(gen);
    int y = y_dist(gen);

    return Window(x, y, w, h, code_lines);
}

int main() {
    // File path
    std::string file_path = "codigo.cpp";

    // Read the C++ code from the file
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << file_path << std::endl;
        return 1;
    }

    std::vector<std::string> code_lines;
    std::string line;
    while (std::getline(file, line)) {
        code_lines.push_back(line);
    }
    file.close();

    // Video settings
    int width = 3840;  // 4K width
    int height = 2160; // 4K height
    int fps = 10;
    cv::Scalar background_color(0, 0, 0); // Black

    // Create a video writer object with epoch timestamp
    std::time_t epoch_time = std::time(nullptr);
    std::string output_file = "output_" + std::to_string(epoch_time) + ".mp4";

    cv::VideoWriter video(output_file, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, cv::Size(width, height));
    if (!video.isOpened()) {
        std::cerr << "Could not open the output video file for writing" << std::endl;
        return 1;
    }

    // Initialize a frame
    cv::Mat frame(height, width, CV_8UC3, background_color);

    // Max time for animation in seconds
    int max_time = 3600; // 1 hour
    auto start_time = std::chrono::steady_clock::now();

    // List to keep track of active windows
    std::vector<Window> active_windows;

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Statistics print interval in seconds
    int stats_interval = 10; // Print statistics every 10 seconds
    int last_stats_time = 0;

    // Main loop
    while (true) {
        // Calculate elapsed time
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();

        if (elapsed >= max_time) {
            break;  // Exit after max_time seconds
        }

        // Reset frame to background color
        frame = cv::Scalar(background_color);

        // Randomly create new windows with reduced probability
        if (dis(gen) < 0.02) {  // Reduced probability
            active_windows.push_back(createRandomWindow(width, height, code_lines));
        }

        // Update and draw active windows
        for (auto& window : active_windows) {
            window.update();
            window.draw(frame);
        }

        // Randomly close windows
        if (!active_windows.empty() && dis(gen) < 0.01) {  // Reduced probability
            std::uniform_int_distribution<> wind_dist(0, active_windows.size() - 1);
            active_windows.erase(active_windows.begin() + wind_dist(gen));
        }

        // Write the frame to the video
        video.write(frame);

        // Print statistics every X seconds
        if (elapsed - last_stats_time >= stats_interval) {
            last_stats_time = elapsed;
            int percentage_complete = (elapsed * 100 / max_time);
            int remaining_time = max_time - elapsed;
            std::cout << "Time Passed: " << elapsed << " seconds / "
                      << "Percentage Complete: " << percentage_complete << "% / "
                      << "Estimated Time to Finish: " << remaining_time << " seconds" << std::endl;
        }

        // Control the frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
    }

    // Release the video writer
    video.release();

    std::cout << "Video saved as " << output_file << std::endl;
    return 0;
}

