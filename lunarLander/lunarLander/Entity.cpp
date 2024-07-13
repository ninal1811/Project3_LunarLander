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

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

// Default constructor
Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(0.0f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_texture_id(0), m_velocity(0.0f), m_acceleration(0.0f), m_width(0.0f), m_height(0.0f)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

// Parameterized constructor
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float jump_power, int walking[4][4], float animation_time,
    int animation_frames, int animation_index, int animation_cols,
    int animation_rows, float width, float height)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed),m_acceleration(acceleration), m_jumping_power(jump_power), m_animation_cols(animation_cols),
    m_animation_frames(animation_frames), m_animation_index(animation_index),
    m_animation_rows(animation_rows), m_animation_indices(nullptr),
    m_animation_time(animation_time), m_texture_id(texture_id), m_velocity(0.0f),
    m_width(width), m_height(height)
{
    face_right();
    set_walking(walking);
}

// Simpler constructor for partial initialization
Entity::Entity(GLuint texture_id, float speed,  float width, float height)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width), m_height(height)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

Entity::~Entity() { }

void Entity::draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index) {
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float)(index % m_animation_cols) / (float)m_animation_cols;
    float v_coord = (float)(index / m_animation_cols) / (float)m_animation_rows;

    // Step 2: Calculate its UV size
    float width = 1.0f / (float)m_animation_cols;
    float height = 1.0f / (float)m_animation_rows;

    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] = {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] = {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const {
    if (!m_is_active || !other->m_is_active) { return false; }
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}

void const Entity::check_collision_y(Entity *collidable_entities, int collidable_entity_count, bool& g_win, bool& g_lose) {
    for (int i = 0; i < collidable_entity_count; i++) {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity)) {
            if (collidable_entity->get_entity_type() == SHROOM) {
                g_lose = true;
            } else if (collidable_entity->get_entity_type() == PC) {
                g_win = true;
            }
            
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            if (m_velocity.y > 0) {
                m_position.y -= y_overlap;
                m_velocity.y = 0;
                m_collided_top = true;          // Collision!
            } else if (m_velocity.y < 0) {
                m_position.y += y_overlap;
                m_velocity.y = 0;
                m_collided_bottom = true;       // Collision!
            }
        }
    }
}

void const Entity::check_collision_x(Entity *collidable_entities, int collidable_entity_count, bool& g_win, bool& g_lose) {
    for (int i = 0; i < collidable_entity_count; i++) {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity)) {
            if (collidable_entity->get_entity_type() == SHROOM) {
                g_lose = true;
            } else if (collidable_entity->get_entity_type() == PC) {
                g_win = true;
            }
            
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0) {
                m_position.x  -= x_overlap;
                m_velocity.x = 0;
                m_collided_right = true;        // Collision!
                
            } else if (m_velocity.x < 0) {
                m_position.x += x_overlap;
                m_velocity.x = 0;
                m_collided_left = true;         // Collision!
            }
        }
    }
}

void Entity::update(float delta_time, Entity* collidable_entities, int collidable_entity_count, bool& g_win, bool& g_lose, bool& g_nofuel) {
    
    if (!m_is_active) { return; }
    
    m_collided_top = false;
    m_collided_bottom = false;
    m_collided_left = false;
    m_collided_right = false;
    
//    for (int i = 0; i < collidable_entity_count; i++) {
//        if (check_collision(&collidable_entities[i])) return;
//    }

    if (m_animation_indices != NULL) {
        if (glm::length(m_movement) != 0) {
            m_animation_time += delta_time;
            float frames_per_second = 1.0f / SECONDS_PER_FRAME;

            if (m_animation_time >= frames_per_second) {
                m_animation_time = 0.0f;
                m_animation_index++;

                if (m_animation_index >= m_animation_frames) {
                    m_animation_index = 0;
                }
            }
        }
    }
    
    if (m_fuel <= 0) {
        g_nofuel = true;
        m_acceleration.x = 0.0f;
        m_acceleration.y = 0.0f;
        return;
    }

    if (m_movement.x == 0.0f) {
        if (m_velocity.x == 0.0f) {
            m_acceleration.x = 0.0f;
        } else {
            if (m_velocity.x < 0.0f) {
                m_acceleration.x = 1.0f * m_speed;
//                m_fuel -= 0.15f;
            } else {
                m_acceleration.x = -1.0f * m_speed;
//                m_fuel -= 0.15f;
            }
        }
    } else {
        m_acceleration.x = m_movement.x * m_speed;
        m_fuel -= 0.2f;
    }
    
    if (m_movement.y == 0.0f) {
        if (m_velocity.y == -0.25f) {
            m_acceleration.y = -0.25f;
        } else {
            if (m_velocity.y < -0.25f) {
                m_acceleration.y = 1.0f * m_speed;
//                m_fuel -= 0.15f;
            } else {
                m_acceleration.y = -1.0f * m_speed;
//                m_fuel -= 0.15f;
            }
        }
    } else {
        m_acceleration.y = m_movement.y * m_speed;
        m_fuel -= 0.2f;
    }
    
    m_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // And we add the gravity next
    m_velocity += m_acceleration * delta_time;
    
    m_position.y += m_velocity.y * delta_time;
    check_collision_y(collidable_entities, collidable_entity_count, g_win, g_lose);
    
    m_position.x += m_velocity.x * delta_time;
    check_collision_x(collidable_entities, collidable_entity_count, g_win, g_lose);
  
//    if(m_is_jumping) {
//        m_is_jumping = false;
//        m_velocity.y += m_jumping_power;
//    }
    
    std::cout << "Current fuel level: " << m_fuel << std::endl;
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
//    m_model_matrix = glm::scale(m_model_matrix, m_scale);
}

void Entity::render(ShaderProgram* program) {
    if (!m_is_active) { return; }
    
    program->set_model_matrix(m_model_matrix);

    if (m_animation_indices != NULL) {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index]);
        return;
    }

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}
