/*
* Author: Nina Li
* Assignment: Lunar Lander
* Date due: 2024-07-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
*/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState {
    Entity* player;
    Entity* platforms;
    Entity* bg;
    Entity* win;
    Entity* lose;
    Entity* nofuel;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH = 640,
              WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.1922f,
                BG_BLUE = 0.549f,
                BG_GREEN = 0.9059f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
              VIEWPORT_Y = 0,
              VIEWPORT_WIDTH = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char BG_FILEPATH[] = "bg.png",
               SPRITESHEET_FILEPATH[] = "ash.png",
               PC_FILEPATH[] = "pc.png",
               WIN_FILEPATH[] = "win.png",
               SHROOM_FILEPATH[] = "shroom.png",
               LOSE_FILEPATH[] = "lose.png",
               NOFUEL_FILEPATH[] = "nofuel.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_win = false,
     g_lose = false,
     g_nofuel = false;


ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath);

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath) {
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL) {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Project 3: Lunar Lander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (context == nullptr) {
        LOG("ERROR: Could not create OpenGL context.\n");
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– BACKGROUND ––––– //
    GLuint bg_texture_id = load_texture(BG_FILEPATH);
    g_game_state.bg = new Entity();
    g_game_state.bg->m_texture_id = bg_texture_id;
    g_game_state.bg->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.bg->set_size(glm::vec3(16.89f, 9.75f, 0.0f));
    
    // ––––– WIN/LOSE/NOFUEL ––––– //
    GLuint win_texture_id = load_texture(WIN_FILEPATH);
    g_game_state.win = new Entity();
    g_game_state.win->m_texture_id = win_texture_id;
    g_game_state.win->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.win->m_model_matrix = glm::scale(g_game_state.win->m_model_matrix, glm::vec3(5.0f, 3.0f, 0.0f));
    g_game_state.win->set_entity_type(WIN);
    g_game_state.win->deactivate();
    
    GLuint lose_texture_id = load_texture(LOSE_FILEPATH);
    g_game_state.lose = new Entity();
    g_game_state.lose->m_texture_id = lose_texture_id;
    g_game_state.lose->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.lose->m_model_matrix = glm::scale(g_game_state.lose->m_model_matrix, glm::vec3(5.0f, 3.0f, 0.0f));
    g_game_state.lose->set_entity_type(LOSE);
    g_game_state.lose->deactivate();
    
    GLuint nofuel_texture_id = load_texture(NOFUEL_FILEPATH);
    g_game_state.nofuel = new Entity();
    g_game_state.nofuel->m_texture_id = nofuel_texture_id;
    g_game_state.nofuel->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.nofuel->m_model_matrix = glm::scale(g_game_state.nofuel->m_model_matrix, glm::vec3(5.39f, 1.82f, 0.0f));
    g_game_state.nofuel->set_entity_type(NOFUEL);
    g_game_state.nofuel->deactivate();
    
    // ––––– PLATFORMS ––––– //
    GLuint pc_texture_id = load_texture(PC_FILEPATH);
    GLuint shroom_texture_id = load_texture(SHROOM_FILEPATH);
    
    g_game_state.platforms = new Entity[PLATFORM_COUNT];
    
    // ––––– SHROOMS ––––– //
    g_game_state.platforms[0].m_texture_id = shroom_texture_id;
    g_game_state.platforms[0].set_position(glm::vec3(-3.5f, -1.75f, 0.0f));
    g_game_state.platforms[0].set_width(0.0f);
    g_game_state.platforms[0].set_height(0.5f);
    g_game_state.platforms[0].set_entity_type(SHROOM);
    g_game_state.platforms[0].update(0.0f, NULL, 0, g_win, g_lose, g_nofuel);
    g_game_state.platforms[0].set_size(glm::vec3(0.765f, 0.765f, 0.0f));
        
    g_game_state.platforms[1].m_texture_id = shroom_texture_id;
    g_game_state.platforms[1].set_position(glm::vec3(3.5f, -1.75f, 0.0f));
    g_game_state.platforms[1].set_width(0.0f);
    g_game_state.platforms[1].set_height(0.5f);
    g_game_state.platforms[1].set_entity_type(SHROOM);
    g_game_state.platforms[1].update(0.0f, NULL, 0, g_win, g_lose, g_nofuel);
    g_game_state.platforms[1].set_size(glm::vec3(0.765f, 0.765f, 0.0f));
    
    g_game_state.platforms[2].m_texture_id = shroom_texture_id;
    g_game_state.platforms[2].set_position(glm::vec3(0.0f, -1.75f, 0.0f));
    g_game_state.platforms[2].set_width(0.0f);
    g_game_state.platforms[2].set_height(0.5f);
    g_game_state.platforms[2].set_entity_type(SHROOM);
    g_game_state.platforms[2].update(0.0f, NULL, 0, g_win, g_lose, g_nofuel);
    g_game_state.platforms[2].set_size(glm::vec3(0.765f, 0.765f, 0.0f));
    
    // ––––– PCS ––––– //
    g_game_state.platforms[3].m_texture_id = pc_texture_id;
    g_game_state.platforms[3].set_position(glm::vec3(-1.95f, -3.15f, 0.0f));
    g_game_state.platforms[3].set_width(0.25f);
    g_game_state.platforms[3].set_height(1.0f);
    g_game_state.platforms[3].set_entity_type(PC);
    g_game_state.platforms[3].update(0.0f, NULL, 0, g_win, g_lose, g_nofuel);
    g_game_state.platforms[3].set_size(glm::vec3(1.128f, 1.114f, 0.0f));
    
    g_game_state.platforms[4].m_texture_id = pc_texture_id;
    g_game_state.platforms[4].set_position(glm::vec3(1.95f, -3.15f, 0.0f));
    g_game_state.platforms[4].set_width(0.25f);
    g_game_state.platforms[4].set_height(1.0f);
    g_game_state.platforms[4].set_entity_type(PC);
    g_game_state.platforms[4].update(0.0f, NULL, 0, g_win, g_lose, g_nofuel);
    g_game_state.platforms[4].set_size(glm::vec3(1.128f, 1.114f, 0.0f));
    
    // ————— PLAYER ————— //
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.player->set_entity_type(PLAYER);
    g_game_state.player->m_speed = 5.0f;
    g_game_state.player->set_acceleration(glm::vec3(0.0f, -3.0f, 0.0f));
    
    int player_walking_animation[4][4] = {
        { 4, 5, 6, 7 },         // left
        { 8, 9, 10, 11 },       // right
        { 0, 1, 2, 3 },         // forwards
        { 12, 13, 14, 15 }      // downwards
    };
    
    glm::vec3 acceleration = glm::vec3(0.0f, -4.905f, 0.0f);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        3.0f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        0.9f,                      // width
        0.9f                       // height
    );
    

//    // Jumping
//    g_game_state.player->set_jumping_power(3.0f);
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    g_game_state.player->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        // Jump
                        if (g_game_state.player->get_collided_bottom())
                            g_game_state.player->jump();
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT]) g_game_state.player->move_left();
    else if (key_state[SDL_SCANCODE_RIGHT]) g_game_state.player->move_right();
    
    if (glm::length(g_game_state.player->get_movement()) > 1.0f) g_game_state.player->normalise_movement();
}

void update() {
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP) {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT, g_win, g_lose, g_nofuel);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    
    if (g_win) { g_game_state.win->activate(); }
    if (g_lose) { g_game_state.lose->activate(); }
    if (g_nofuel) { g_game_state.nofuel->activate(); }
    
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_game_state.bg->render(&g_shader_program);
    g_game_state.player->render(&g_shader_program);
    
    for (int i = 0; i < PLATFORM_COUNT; i++) g_game_state.platforms[i].render(&g_shader_program);
    
    if (g_win) {
        g_game_state.win->render(&g_shader_program);
    } else if (g_lose) {
        g_game_state.lose->render(&g_shader_program);
    }
    
    if (g_nofuel) {
        g_game_state.nofuel->render(&g_shader_program);
    }
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() {
    SDL_Quit();
    
    delete [] g_game_state.platforms;
    delete g_game_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[]) {
    initialise();
    
    while (g_game_is_running) {
        process_input();
        
        if (g_win == 0 and g_lose == 0) {
            update();
        }
        render();
    }
    shutdown();
    return 0;
}
