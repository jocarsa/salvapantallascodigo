import cv2
import numpy as np
import time
import random

# File path
file_path = 'codigo.cpp'

# Read the C++ code from the file
with open(file_path, 'r') as file:
    code_lines = file.readlines()

# Video settings
width, height = 1920, 1080
fps = 10
font = cv2.FONT_HERSHEY_SIMPLEX
font_scale = 0.7
font_thickness = 2
text_color = (0, 255, 0)  # Green
background_color = (0, 0, 0)  # Black
line_spacing = 30
left_margin = 40  # Increased left margin for padding
top_margin = 60  # Increased top margin for padding and top bar

# Create a video writer object with epoch timestamp
epoch_time = int(time.time())
output_file = f'output_{epoch_time}.mp4'
fourcc = cv2.VideoWriter_fourcc(*'mp4v')
video = cv2.VideoWriter(output_file, fourcc, fps, (width, height))

# Initialize a frame
frame = np.zeros((height, width, 3), dtype=np.uint8)
frame[:] = background_color

# Max time for animation in seconds
max_time = 3600
start_time = time.time()

# List to keep track of active windows
active_windows = []

class Window:
    def __init__(self, x, y, w, h):
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.code_lines = code_lines.copy()
        self.current_line = 0
        self.scroll_offset = 0

    def draw(self, frame):
        # Create a semi-transparent overlay for the window
        overlay = frame.copy()
        cv2.rectangle(overlay, (self.x, self.y), (self.x + self.w, self.y + self.h), (0, 0, 0), -1)
        cv2.addWeighted(overlay, 0.5, frame, 0.5, 0, frame)

        # Draw the rectangle border with a thicker top bar
        cv2.rectangle(frame, (self.x, self.y), (self.x + self.w, self.y + 30), (255, 255, 255), -1)  # Top bar
        cv2.rectangle(frame, (self.x, self.y + 30), (self.x + self.w, self.y + self.h), (255, 255, 255), 2)  # Rest of the border

        # Draw the code lines with scrolling and padding
        y = self.y + top_margin - self.scroll_offset
        for i, line in enumerate(self.code_lines[:self.current_line]):
            line = line.rstrip('\n').rstrip('\r').rstrip('?')
            if self.y + top_margin <= y + i * line_spacing < self.y + self.h - line_spacing:
                cv2.putText(frame, line, (self.x + left_margin, y + i * line_spacing), font, font_scale, text_color, font_thickness, cv2.LINE_AA)

    def update(self):
        if self.current_line < len(self.code_lines):
            self.current_line += 1

        # Check if scrolling is needed
        if (self.current_line * line_spacing) > (self.h - top_margin):
            self.scroll_offset += line_spacing

        # Ensure scroll_offset does not exceed the maximum possible offset
        max_offset = self.current_line * line_spacing - (self.h - top_margin)
        if self.scroll_offset > max_offset:
            self.scroll_offset = max_offset

def create_random_window():
    w = random.randint(600, width - 100)
    h = random.randint(600, height - 100)
    x = random.randint(0, width - w - 1)
    y = random.randint(0, height - h - 1)
    return Window(x, y, w, h)

# Main loop
while time.time() - start_time < max_time:
    # Randomly create new windows with reduced probability
    if random.random() < 0.02:  # Reduced probability
        active_windows.append(create_random_window())

    # Update and draw active windows
    for window in active_windows:
        window.update()
        window.draw(frame)

    # Randomly close windows
    if active_windows and random.random() < 0.01:  # Reduced probability
        active_windows.pop(random.randint(0, len(active_windows) - 1))

    # Write the frame to the video
    video.write(frame.copy())

    # Display the frame buffer as it is being calculated
    cv2.imshow('Frame Buffer', frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release the video writer
video.release()

# Close the frame buffer window
cv2.destroyAllWindows()

print(f"Video saved as {output_file}")
