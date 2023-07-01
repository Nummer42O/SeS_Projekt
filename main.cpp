#include "ThisThread.h"
#include "mbed.h"
#include "LCD_DISCO_F429ZI.h"
#include "TS_DISCO_F429ZI.h"
#include "stm32f429i_discovery.h"
#include "fonts.h"
#include "stm32f429i_discovery_ts.h"

#include <iostream>
#include <chrono>
using namespace std::chrono_literals;

#include "helpers.hpp"


#define SWIPE_SCALING 4



// const globals
const Vec3d vertices[8] = {
    //left                 right
    //x      y      z        x      y      z
    {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, //lower rear
    {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, //lower front
    {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, //upper rear
    {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, //upper front
};
const uint16_t lines[12][2] = {
    {0, 1}, {1, 5}, {5, 4}, {4, 0}, //rear
    {0, 2}, {1, 3}, {4, 6}, {5, 7}, //middle
    {2, 3}, {3, 7}, {7, 6}, {6, 2}, //front
};
const int16_t angles[NR_PRECOMPUTED_TRIG_VALS] = {
    -180, -170, -160, -150, -140, -130, 
    -120, -110, -100, -90, -80, -70, 
    -60, -50, -40, -30, -20, -10, 
    0, 10, 20, 30, 40, 50, 
    60, 70, 80, 90, 100, 110, 
    120, 130, 140, 150, 160, 170, 
};


// non const globals
enum ProgramState {
    BROKEN,
    BUSY,
    READY
} prog_state;
LCD_DISCO_F429ZI LCD;
TS_DISCO_F429ZI TS;


// function declarations
void preparePoints(Vec3d* altered_vertices, int16_t x_angle_idx, int16_t y_angle_idx);
void drawLine(Vec3d a, Vec3d b);
void setProgState(ProgramState new_state);


// main() runs in its own thread in the OS
int main()
{
    // LEDs
    BSP_LED_Init(GREEN_LED);
    BSP_LED_Init(RED_LED);
    
    // Display
    LCD.Init();

    LCD.SetFont(&Font24);
    LCD.SetTextColor(LCD_COLOR_BLACK);

    // touch screen
    TS.Init(SCREEN_WIDTH, SCREEN_HEIGHT);

    TS_StateTypeDef current_ts_state, prev_ts_state;
    TS.GetState(&prev_ts_state);

    // miscalenious
    int16_t x_angle_idx = 18,
            y_angle_idx = 18;
    Vec3d altered_vertices[8];
    Timer timer;
    char angle_buffer[8];

    while (true) {
        // start timer
        timer.reset();
        timer.start();

        // indicate to work
        setProgState(BUSY);

        // get if user swiped over the screen and adjust the angle
        TS.GetState(&current_ts_state);
        if (current_ts_state.TouchDetected && prev_ts_state.TouchDetected) {
            int16_t diff = (prev_ts_state.X - current_ts_state.X) / SWIPE_SCALING;
            x_angle_idx = (x_angle_idx + diff) % NR_PRECOMPUTED_TRIG_VALS;
            if (x_angle_idx < 0) { //TODO: why is this necessary? Why doesn't the modulo operation work?
                x_angle_idx = NR_PRECOMPUTED_TRIG_VALS + x_angle_idx;
            }

            diff = (prev_ts_state.Y - current_ts_state.Y) / SWIPE_SCALING;
            y_angle_idx = (y_angle_idx + diff) % NR_PRECOMPUTED_TRIG_VALS;
            if (y_angle_idx < 0) { //TODO: why is this necessary? Why doesn't the modulo operation work?
                y_angle_idx = NR_PRECOMPUTED_TRIG_VALS + y_angle_idx;
            }
        }
        prev_ts_state = current_ts_state;

        // clear screen before redrawing
        LCD.Clear(LCD_COLOR_WHITE);

        // rotate, move and draw points
        preparePoints(altered_vertices, x_angle_idx, y_angle_idx);
        for (uint16_t i = 0ul; i < 12ul; i++) {
            const uint16_t* vertex_idcs = lines[i];

            drawLine(altered_vertices[vertex_idcs[0]], altered_vertices[vertex_idcs[1]]);
        }

        // write current angle to screen
        sprintf(angle_buffer, "H:% 3d", angles[x_angle_idx]);
        LCD.DisplayStringAt(0u, 0u,  (uint8_t*)angle_buffer, CENTER_MODE);
        sprintf(angle_buffer, "V:% 3d", angles[y_angle_idx]);
        LCD.DisplayStringAt(0u, 24u, (uint8_t*)angle_buffer, CENTER_MODE);

        // indicate idle/ready phase
        setProgState(READY);

        // stop timer
        timer.stop();

        // sleep the rest of the time
        // time target are ~67ms to achive 15fps
        std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds>(timer.elapsed_time());
        std::chrono::milliseconds diff = 67ms - time;
        // printf("elapsed time: %lldms\tremaining time: %lldms\n", time.count(), diff.count());
        ThisThread::sleep_for(diff);
    }
}

void preparePoints(Vec3d* altered_vertices, int16_t x_angle_idx, int16_t y_angle_idx) {
    for (size_t i = 0ul; i < 8ul; i++) {
        Vec3d vertex = vertices[i];

        vertex = rotateXCenter(vertex, y_angle_idx);
        vertex = rotateYCenter(vertex, x_angle_idx);

        vertex = move(vertex, 0, 0, -4);

        altered_vertices[i] = vertex;
    }
}

void drawLine(Vec3d a, Vec3d b) {
    Vec2d a_2d = getScreenCoordsFrom3d(a),
          b_2d = getScreenCoordsFrom3d(b);

    if (!checkInBounds(a_2d) || !checkInBounds(b_2d))
    {
        //TODO: not optimal, the visible rest of the line could be interoplated
        return;
    }

    LCD.DrawLine(a_2d.x, a_2d.y, b_2d.x, b_2d.y);
}

void setProgState(ProgramState new_state) {
    prog_state = new_state;
    
    switch (prog_state) {
        case BROKEN:
            BSP_LED_Off(GREEN_LED);
            BSP_LED_On(RED_LED);
            break;
        case BUSY:
            BSP_LED_On(GREEN_LED);
            BSP_LED_On(RED_LED);
            break;
        case READY:
            BSP_LED_Off(RED_LED);
            BSP_LED_On(GREEN_LED);
    }

    while (prog_state == BROKEN) {
        ThisThread::sleep_for(1s);
    } //block if broken
}