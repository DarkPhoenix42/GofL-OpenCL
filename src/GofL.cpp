#include <iostream>
#include <fstream>
#include <vector>
#include "../include/json.hpp"
#include "../include/argparse.hpp"
#include <SDL2/SDL.h>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.h>
using namespace std;

// Parameters
int WIDTH = 800, HEIGHT = 800, CELL_SIZE = 1;
int ROWS = HEIGHT / CELL_SIZE, COLS = WIDTH / CELL_SIZE;

// SDL Stuff
SDL_Renderer *renderer;
SDL_Window *win;
SDL_Event event;

// Timing Stuff
uint32_t draw_timer = 0;
const double DRAW_DELAY = 1000 / 60.;
uint32_t fps_timer = 0;
int frames = 0;

// OpenCL Stuff
cl_platform_id platform;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel kernel;
cl_mem grid_buffer;
cl_mem new_grid_buffer;

// Game of Life Stuff
vector<unsigned char> grid;
vector<unsigned char> new_grid;

void init_grid()
{
    for (int i = 0; i < ROWS * COLS; i++)
        grid[i] = rand() % 2;
    for (int i = 0; i < ROWS * COLS; i++)
        new_grid[i] = 0;
}

void update()
{
    clEnqueueWriteBuffer(queue, grid_buffer, CL_TRUE, 0, ROWS * COLS * sizeof(unsigned char), grid.data(), 0, NULL, NULL);
    size_t global_size[2] = {COLS, ROWS};
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, new_grid_buffer, CL_TRUE, 0, ROWS * COLS * sizeof(unsigned char), new_grid.data(), 0, NULL, NULL);
    clFinish(queue);
    grid = new_grid;
}

void draw_screen()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLS; col++)
        {
            if (grid[row * COLS + col] == 1)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_Rect rect = {col * CELL_SIZE, row * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void handle_events()
{
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            exit(0);
    }
}

void InitOpenCL()
{
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    queue = clCreateCommandQueueWithProperties(context, device, NULL, NULL);

    ifstream file("../src/gol.cl");
    string src(istreambuf_iterator<char>(file), (istreambuf_iterator<char>()));
    const char *src_c = src.c_str();
    program = clCreateProgramWithSource(context, 1, (const char **)&src_c, NULL, NULL);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    kernel = clCreateKernel(program, "update_cell", NULL);

    grid_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, ROWS * COLS * sizeof(unsigned char), NULL, NULL);
    new_grid_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, ROWS * COLS * sizeof(unsigned char), NULL, NULL);

    int err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &grid_buffer);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &new_grid_buffer);
    err |= clSetKernelArg(kernel, 2, sizeof(int), &ROWS);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &COLS);
    if (err != CL_SUCCESS)
    {
        cout << "Error setting kernel arguments!" << endl;
        exit(1);
    }
}

void InitSDL()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    win = SDL_CreateWindow("Game of Life", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

void load_args(int argc, char *argv[])
{
    argparse::ArgumentParser parser("Game of Life Simulation");
    parser.add_description("A program to simulate Conway's Game of Life using OpenCL and SDL2.");

    parser.add_argument("--width", "-w")
        .default_value(WIDTH)
        .scan<'i', int>()
        .help("Set the width of the simulation.");

    parser.add_argument("--height", "-ht")
        .default_value(HEIGHT)
        .scan<'i', int>()
        .help("Set the height of the simulation.");

    parser.add_argument("--cell-size", "-c")
        .default_value(CELL_SIZE)
        .scan<'i', int>()
        .help("Set the size of each cell in the simulation.");

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const runtime_error &err)
    {
        cout << err.what() << endl;
        cout << parser;
        exit(0);
    }

    WIDTH = parser.get<int>("--width");
    HEIGHT = parser.get<int>("--height");
    CELL_SIZE = parser.get<int>("--cell-size");
    ROWS = HEIGHT / CELL_SIZE;
    COLS = WIDTH / CELL_SIZE;
}

int main(int argc, char *argv[])
{
    cout << "Loading arguments..." << endl;
    load_args(argc, argv);
    grid.resize(ROWS * COLS);
    new_grid.resize(ROWS * COLS);

    cout << "Initializing OpenCL..." << endl;
    InitOpenCL();

    cout << "Initializing SDL..." << endl;
    InitSDL();

    cout << "Initializing grid..." << endl;
    init_grid();

    cout << "Starting simulation..." << endl;
    draw_timer = SDL_GetTicks();
    fps_timer = SDL_GetTicks();
    while (1)
    {
        frames++;
        if (SDL_GetTicks() - fps_timer > 1000)
        {
            cout << "FPS: " << frames << endl;
            frames = 0;
            fps_timer = SDL_GetTicks();
        }
        handle_events();
        if (SDL_GetTicks() - draw_timer > DRAW_DELAY)
        {
            draw_screen();
            draw_timer = SDL_GetTicks();
        }
        update();
    }
}
