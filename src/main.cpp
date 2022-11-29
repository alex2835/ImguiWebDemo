#include <imgui.h>
#include "imgui_impl_sdl.h"

#include <SDL.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArrays glBindVertexArraysAPPLE
#define glGenVertexArray glGenVertexArrayAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#else
#include <SDL_opengles2.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#endif

#include <math.h>
#include <iostream>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

bool g_done = false;
SDL_Window* g_window;
SDL_GLContext g_glcontext;

enum class ShapeType : int
{
   Line,
   Rect,
   Circle,
   Triangle
};

struct Shape
{
   ShapeType mType;
   ImVec4 mColor;
   union{
      struct{
         ImVec2 mBegin;
         ImVec2 mEnd;
      };
      struct{
         ImVec2 mCenter;
         float mRadius;
      };
   };
};
using Shapes = std::vector<Shape>;

void DrawShape( Shape shape )
{
   ImDrawList* draw_list = ImGui::GetWindowDrawList();
   const ImU32 col32 = ImColor(shape.mColor);
   float thickness = 2.0f;
   switch (shape.mType)
   {
      case ShapeType::Line:
         draw_list->AddLine(shape.mBegin, shape.mEnd, col32, thickness);
         break;
      case ShapeType::Rect:
         draw_list->AddRect(shape.mBegin, shape.mEnd, col32, 0.0f, ImDrawCornerFlags_All, thickness);
         break;
      case ShapeType::Circle:
         draw_list->AddCircle(shape.mCenter, shape.mRadius, col32, 32, thickness);
         break;
      case ShapeType::Triangle:
         draw_list->AddTriangle(ImVec2{shape.mCenter.x-shape.mRadius, shape.mCenter.y},
                                ImVec2{shape.mCenter.x+shape.mRadius, shape.mCenter.y},
                                ImVec2{shape.mCenter.x, shape.mCenter.y-shape.mRadius},
                                col32,
                                thickness);
         break;
   }            
}


void main_loop()
{
   // Update events
   SDL_Event event;
   while (SDL_PollEvent(&event))
   {
       ImGui_ImplSdl_ProcessEvent(&event);
       if (event.type == SDL_QUIT)
           g_done = true;
   }
   ImGui_ImplSdl_NewFrame(g_window);

   // Draw windows
   int w, h;
   SDL_GL_GetDrawableSize(g_window, &w, &h);

   // State
   static bool is_drawing = false;
   static Shape current_shape{ ShapeType::Line, ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, ImVec2{0, 0}, ImVec2{300, 300} };
   static Shapes shapes;

   ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize );
   static glm::vec3 bgcolor(0.2f);
   ImGui::ColorEdit3("Background", glm::value_ptr(bgcolor));
   ImGui::Combo( "Shape", (int*)&current_shape.mType, "Line\0Rect\0Circle\0Triangle" );
   ImGui::ColorEdit4("Shape color", (float*)&current_shape.mColor);
   if( ImGui::Button( "Undo", {50, 20} ) )
   {
      if( !shapes.empty() )
         shapes.erase( shapes.end()-- );
   }
   ImGui::SameLine();
   if( ImGui::Button( "Clear", {50, 20} ) )
      shapes.clear();
   ImGui::End();

   ImGui::SetNextWindowPos(ImVec2(0,0));
   ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus );
   ImGui::InvisibleButton("canvas", ImVec2(w, h));
   if (ImGui::IsItemHovered())
   {
      ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
      if (ImGui::IsMouseClicked(0))
      {
         if (is_drawing)
            shapes.push_back(current_shape);
         is_drawing = !is_drawing;
         current_shape.mBegin = mouse_pos_in_canvas;
      }
      if( current_shape.mType == ShapeType::Line || current_shape.mType == ShapeType::Rect )
         current_shape.mEnd = mouse_pos_in_canvas;
      else
         current_shape.mRadius = std::sqrt( std::pow( current_shape.mCenter.x - mouse_pos_in_canvas.x, 2 ) +
                                            std::pow( current_shape.mCenter.y - mouse_pos_in_canvas.y, 2 ) );
   }
   if (is_drawing)
      DrawShape(current_shape);
   for( auto shape : shapes )
      DrawShape(shape);
   ImGui::End();

    
   glClearColor(bgcolor.x, bgcolor.y, bgcolor.z, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
   glViewport(0, 0, w, h);
   ImGui::Render();
   SDL_GL_SwapWindow(g_window);
}

bool initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "Error: %s\n" << SDL_GetError() << '\n';
        return false;
    }
    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    g_window = SDL_CreateWindow(
        "ImGUI / WASM / WebGL demo", // title
        SDL_WINDOWPOS_CENTERED, // x
        SDL_WINDOWPOS_CENTERED, // y
        1280, 720, // width, height
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI // flags
    );
    g_glcontext = SDL_GL_CreateContext(g_window);
    ImGui_ImplSdl_Init(g_window);
    return true;
}

void destroySDL()
{
    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(g_glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void runMainLoop()
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!g_done)
        main_loop();
#endif
}

int main(int, char**)
{
    if(!initSDL())
        return EXIT_FAILURE;
    runMainLoop();
    destroySDL();
    return EXIT_SUCCESS;
}
