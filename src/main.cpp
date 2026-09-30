#include <iostream>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>
#include <cmath>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;
const int FRAMES_PER_ANIM = 60;
int ELAPSED_FRAMES = 0;
// index of the control point being dragged, -1 is none
int selectedPoint = -1;
const std::vector<sf::Color> SET_COLORS = { 
    sf::Color::Red, 
    sf::Color::Green, 
    sf::Color::Blue,
    sf::Color::Yellow, 
    sf::Color::Cyan,
};

using Point2D = sf::Vector2f;

// (Part 1) Define a function that samples a cubic Bezier curve at t in [0, 1].
Point2D getPoint(const std::vector<sf::Vector2f>& pts, float t) { 
    float u = 1.0f - t; // opposite of t, used in the weights
    float w0 = u * u * u;
    float w1 = 3 * u * u * t;
    float w2 = 3 * u * t * t;
    float w3 = t * t * t;

    // scale each control point by its weight
    Point2D term0 = pts[0] * w0;
    Point2D term1 = pts[1] * w1;
    Point2D term2 = pts[2] * w2;
    Point2D term3 = pts[3] * w3;

    // add the weighted points together to get the point on the curve
    Point2D result = term0 + term1 + term2 + term3;
    return result;
}

// (Part 2) Define a function that returns the curve's slope at t in [0, 1].
Point2D getSlope(const std::vector<sf::Vector2f>& pts, float t) {
    // returns the direction the curve is heading at time t
    float u = 1.0f - t;

    // derivative of w
    float w0 = 3 * u * u;
    float w1 = 6 * u * t;
    float w2 = 3 * t * t;

    // derivative
    Point2D slope = (pts[1] - pts[0]) * w0 + (pts[2] - pts[1]) * w1 + (pts[3] - pts[2]) * w2;
    return slope; 
}

int closestPoint(const std::vector<sf::Vector2f>& pts, Point2D mousePos) {
    // returns the index of the control point closest to mousePos
    int closestIndex = 0;
    float closestDist = std::hypot(pts[0].x - mousePos.x, pts[0].y - mousePos.y);

    // check the rest of the points, keeping whichever is closest
    for (size_t i = 1; i < pts.size(); i++) {
        float dist = std::hypot(pts[i].x - mousePos.x, pts[i].y - mousePos.y);
        if (dist < closestDist) {
            closestDist = dist;
            closestIndex = i;
        }
    }
    return closestIndex;
}

void matchSlope(std::vector<sf::Vector2f>& pts, int moved, int joint, int other) {
    // created in assistance with Claude
    // "When the user drags a handle next to a shared endpoint (e.g. point 3), 
    // how do I move the handle on the other side (point 5) so the slope through 
    // the shared point stays the same, while keeping point 5's distance from point 4 unchanged?"
    // This was helpful because it helped me to understand how this can be calculated as I struggled to understand
    // it on my own. I was able to write the code from the pseudocode.

    // moves "other" so it's in line with "moved" through "joint", keeping its distance from joint
    // direction from the moved point through the joint
    Point2D dir = pts[joint] - pts[moved];
    float dirLen = std::hypot(dir.x, dir.y);
    if (dirLen == 0) { return; } // moved point is on top of the joint, no direction

    // current distance of the other point from the joint
    Point2D otherVec = pts[other] - pts[joint];
    float otherLen = std::hypot(otherVec.x, otherVec.y);

    // place other point along dir, at its original distance
    pts[other] = pts[joint] + dir * (otherLen / dirLen);
}

sf::Color getSetColor(size_t pointIndex) {
    // original 4 points
    if (pointIndex < 4) {
        return SET_COLORS[0];
    }

    // points after the original 4
    size_t indexAfterOriginal = pointIndex - 4;

    // every 3 points is a new set
    size_t setNumber = indexAfterOriginal / 3 + 1;

    // wrap around if out of colors
    size_t colorIndex = setNumber % SET_COLORS.size();

    return SET_COLORS[colorIndex];
}

//(Part 1) Store four control points for the curve.
std::vector<sf::Vector2f> controlPoints = {{100, 400}, {200, 300}, {500, 250}, {700, 650}};

void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            shouldQuit = true;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            // (Part 3) On left-click, select the closest control point
            // using mouse->position and start dragging it.
            if (mouse->button == sf::Mouse::Button::Left) {
                Point2D mousePos = sf::Vector2f(mouse->position);
                selectedPoint = closestPoint(controlPoints, mousePos);
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
            // (Part 3) On left-button release, stop dragging.
            if (mouse->button == sf::Mouse::Button::Left) {
                selectedPoint = -1;
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            // created in assistance with Claude
            // (coming from the matchSlope prompt)
            // "How do we apply this to the action of actually moving the mouse"
            // "Can you give me pseudocode for this?"
            // helped me understand and trace the joint and handle

            // (Part 3) Move the selected control point to mouse->position.
            // (Part 4) Maintain matching slopes at shared endpoints.
            // When moving point 3, move point 5 without changing its distance
            // from point 4 (point numbers here start at 1).
            if (selectedPoint != -1) {
                controlPoints[selectedPoint] = sf::Vector2f(mouse->position);
            }
            int count = static_cast<int>(controlPoints.size());
            if (selectedPoint % 3 == 2 && selectedPoint + 2 < count) {
                matchSlope(controlPoints, selectedPoint, selectedPoint + 1, selectedPoint + 2);
            }
            else if (selectedPoint % 3 == 1 && selectedPoint >= 4) {
                matchSlope(controlPoints, selectedPoint, selectedPoint - 1, selectedPoint - 2);
            }
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            // (Part 4) '+' adds three control points; '-' removes three,
            // keeping at least four points.
            if (key->code == sf::Keyboard::Key::Add || key->code == sf::Keyboard::Key::Equal) {
                // add three control points
                controlPoints.push_back(Point2D(400, 600)); 
                controlPoints.push_back(Point2D(200, 350)); 
                controlPoints.push_back(Point2D(150, 500)); 
            } else if (key->code == sf::Keyboard::Key::Hyphen || key->code == sf::Keyboard::Key::Subtract) {
                // remove three control points, keeping at least four
                if (controlPoints.size() > 4) {
                    controlPoints.pop_back();
                    controlPoints.pop_back();
                    controlPoints.pop_back();
                }
            }
        }
    }
}

void drawLine(sf::RenderWindow& window, Point2D from, Point2D to, float width, sf::Color c) {
    // calculate length and angle (in degrees) from "from" to "to"
    float length = std::hypot(to.x - from.x, to.y - from.y);
    float angle = std::atan2(to.y - from.y, to.x - from.x) * 180.0f / 3.14159265f;

    sf::RectangleShape line({length, width});

    sf::Color myColor(c.r, c.g, c.b);
    line.setFillColor(myColor);

    // place at "from", and rotate towards "to"
    line.setOrigin({0.0f, width / 2.0f});
    line.setPosition(from);
    line.setRotation(sf::degrees(angle));

    window.draw(line);
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    // ====== ====== ======
    // (Part 1) Sample GetPoint over t in [0, 1] and connect samples using the line-drawing
    // code from your project. Draw all four control points as circles after drawing the curve.
    // ====== ====== ======

    const int SAMPLES = 100; // lines per segment

    // draw every segment
    // (Part 4) Draw all connected cubic Bezier segments and their handles.
    for (size_t s = 0; s + 3 < controlPoints.size(); s += 3) {
        std::vector<sf::Vector2f> seg = {
            controlPoints[s], controlPoints[s + 1], controlPoints[s + 2], controlPoints[s + 3]
        };

        // (Part 3) Draw control handles from point 1 to 2 and point 3 to 4.
        drawLine(window, seg[0], seg[1], 3, sf::Color(100, 100, 100));
        drawLine(window, seg[2], seg[3], 3, sf::Color(100, 100, 100));

        // curve
        Point2D prev = getPoint(seg, 0);
        for (int i = 1; i <= SAMPLES; i++) {
            float t = static_cast<float>(i) / SAMPLES;
            Point2D curr = getPoint(seg, t);
            drawLine(window, prev, curr, 4, sf::Color::White);
            prev = curr;
        }
    }

    // draw control points after the curve so they sit on top
    for (size_t i = 0; i < controlPoints.size(); i++) {
        sf::CircleShape circle(6);
        circle.setOrigin({6, 6}); // center the circle on the point
        circle.setPosition(controlPoints[i]);
        circle.setFillColor(getSetColor(i));
        window.draw(circle);
    }

    // ====== ====== ======
    // (Part 2) Draw a small square moving repeatedly along the curve.
    // Use GetSlope to orient it to the curve at each time step.
    // ====== ====== ======
    float animTime = static_cast<float>(ELAPSED_FRAMES % FRAMES_PER_ANIM) / FRAMES_PER_ANIM;
    Point2D position = getPoint(controlPoints, animTime);

    // make square
    sf::RectangleShape square({26, 26});
    square.setOrigin({13, 13}); // center the square on the point
    square.setPosition(position);
    square.setFillColor(sf::Color::Magenta);

    // rotate square to face the direction the curve is heading
    Point2D slope = getSlope(controlPoints, animTime);
    float angle = std::atan2(slope.y, slope.x) * 180.0f / 3.14159265f;
    square.setRotation(sf::degrees(angle));
    window.draw(square);

    ++ELAPSED_FRAMES;
    window.display();
}

int main() {
    sf::RenderWindow window;

    try {
        // Initialize window
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Bezier Curve Editor");
        window.setFramerateLimit(FPS_LIMIT);
        // Prevent key repeats.
        window.setKeyRepeatEnabled(false);

        bool shouldQuit = false;
        // Main game loop
        while (window.isOpen()) {
            handleInput(window, shouldQuit);
            if (shouldQuit) {
                break;
            }
            render(window);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
