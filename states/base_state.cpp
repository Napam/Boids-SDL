#include "../include/base_state.hpp"
#include <iostream>
#include <eigen3/Eigen/Dense>
#include <nlohmann/json.hpp>

using namespace Eigen;
using json = nlohmann::json;

Clock::Clock(Uint8 targetFps)
    : targetFps(targetFps), targetFrameTime(1000 / targetFps), dt(targetFrameTime),
      prevTime(SDL_GetTicks()) {std::cout << "targetFrameTime: " << targetFrameTime << std::endl;}

void Clock::fpsControll()
{
    dt = SDL_GetTicks() - prevTime;
    if (dt < targetFrameTime)
        SDL_Delay(targetFrameTime - dt);
    frameTime = SDL_GetTicks() - prevTime;
    prevTime = SDL_GetTicks();
}

BaseState::BaseState(SDL_Window *window, SDL_Renderer *renderer, SDL_Event *event, json &config)
    : window(window), renderer(renderer), event(event), active(false), eventHappened(false),
      inputEventListeners(new std::vector<StateEventListener *>), keystates(SDL_GetKeyboardState(NULL)),
      config(config), clock(Clock(config["targetFps"].get<int>()))
{
    SDL_GetWindowSize(window, &pixelSize[0], &pixelSize[1]);
    inputEventListeners = new std::vector<StateEventListener *>;
    std::cout << "(" << pixelSize[0] << "," << pixelSize[1] << ")\n";
}

BaseState::~BaseState()
{
    delete inputEventListeners;
}

void BaseState::addInputEventListener(StateEventListener *listener)
{
    inputEventListeners->emplace_back(listener);
}

void BaseState::handleUserInput()
{
    // Polling events updates keystates, VERY IMPORTANT!!!
    while (SDL_PollEvent(event) > 0)
    {
        switch (event->type)
        {
        case SDL_QUIT:
            active = false;
            break;

        case SDL_KEYDOWN:
            keyDown = (*event).key.keysym.sym;
            switch (keyDown)
            {
            case SDLK_F4:
                if ((*event).key.keysym.mod == SDLK_LALT)
                    active = false;
                break;
            }

            // Key presses are broadcasted to listeners
            for (StateEventListener *listener : *inputEventListeners)
            {
                listener->onKeyDown(keyDown);
            }
            break;

        case SDL_KEYUP:
            keyUp = (*event).key.keysym.sym;
            // Key presses are broadcasted to listeners
            for (StateEventListener *listener : *inputEventListeners)
            {
                listener->onKeyUp(keyUp);
            }
            break;
        
        case SDL_MOUSEBUTTONDOWN:
            mouseDown = (*event).button.button;
            for (StateEventListener *listener : *inputEventListeners)
            {
                listener->onMouseDown(mouseDown);
            }
            break;
        
        case SDL_MOUSEBUTTONUP:
            mouseUp = (*event).button.button;
            for (StateEventListener *listener : *inputEventListeners)
            {
                listener->onMouseUp(mouseUp);
            }
            break;
        }
    }
}

void BaseState::clearfill(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);
}

void BaseState::activate()
{
    active = true;
}

void BaseState::run()
{
    activate();
    while (active)
    {
        handleUserInput();
        update();
        clock.fpsControll();
        worldDt = clock.frameTime / clock.targetFrameTime;
        // std::cout << "worldDt: " << worldDt << " ("<< clock.frameTime << "ms)"<< std::endl;
        // SDL_Delay(20);
    }
}

BaseWorldObject::BaseWorldObject(BaseState *state, float x, float y, int w, int h)
    : state(state)
{
    rect.w = w;
    rect.h = h;
    updateWorldPosition(w, h);
}



BaseWorldObject::BaseWorldObject(BaseState *state, int x, int y, int w, int h)
    : state(state)
{
    rect.w = w;
    rect.h = h;
    updatePixelPosition(x, y);
}


BaseWorldObject::BaseWorldObject(BaseState *state)
    : state(state)
{
    rect.w = 0;
    rect.h = 0;
    updateWorldPosition(0,0);
}


void BaseWorldObject::drawRect()
{
    SDL_SetRenderDrawColor(state->renderer, 20, 255, 10, 255);
    SDL_Rect temp = {rect.x - rect.w/2, rect.y-rect.h/2, rect.w, rect.h};
    SDL_RenderDrawRect(state->renderer, &temp);
}


void BaseWorldObject::drawCircle(Sint16 rad)
{
    filledCircleRGBA(state->renderer, rect.x, rect.y, rad, 20, 255, 10, 255);
}


int BaseWorldObject::worldToPixel(float unit, int dim)
{
    return static_cast<int>((unit / state->worldSize[dim]) * state->pixelSize[dim]);
}


Eigen::Array2i BaseWorldObject::worldToPixel(Eigen::Array2f units)
{
    // (...).template cast<type> is C++ syntax for calling member functions of template objects
    // ^ Artifact from old code, but I leave it here since it may be useful info

    // Must explicitly cast stuff, or eigen will complain
    return ((units / state->worldSize) * state->pixelSize.cast<float>()).cast<int>();
}


float BaseWorldObject::pixelToWorld(int pixel, int dim)
{
    return (static_cast<float>(pixel) / state->pixelSize[dim]) * state->worldSize[dim];
}


Eigen::Array2f BaseWorldObject::pixelToWorld(Eigen::Array2i pixels)
{
    // Must explicitly cast stuff, or eigen will complain
    return ((pixels.cast<float>() / state->pixelSize.cast<float>()) * state->worldSize).cast<float>();
}


void BaseWorldObject::updateWorldPosition()
{    
    pixelPosition = worldToPixel(worldPosition);

    rect.x = pixelPosition[0];
    rect.y = pixelPosition[1];
}


void BaseWorldObject::updateWorldPosition(float x, float y)
{
    worldPosition[0] = x;
    worldPosition[1] = y;
    updateWorldPosition();
}


void BaseWorldObject::updateWorldPosition(Eigen::Array2f units)
{
    worldPosition = units;
    updateWorldPosition();
}


void BaseWorldObject::updateWorldPositionX(float x)
{
    worldPosition[0] = x;
    updateWorldPosition();
}


void BaseWorldObject::updateWorldPositionY(float y)
{
    worldPosition[1] = y;
    updateWorldPosition();
}


void BaseWorldObject::updatePixelPosition()
{
    worldPosition = pixelToWorld(pixelPosition);

    rect.x = pixelPosition[0];
    rect.y = pixelPosition[1];
}


void BaseWorldObject::updatePixelPosition(int x, int y)
{
    pixelPosition[0] = x;
    pixelPosition[1] = y;
    updatePixelPosition();
}


void BaseWorldObject::updatePixelPosition(Eigen::Array2i pixels)
{
    pixelPosition = pixels;
    updatePixelPosition();
}


void BaseWorldObject::updatePixelPositionX(int x)
{
    pixelPosition[0] = x;
    updatePixelPosition();
}


void BaseWorldObject::updatePixelPositionY(int y)
{
    pixelPosition[1] = y;
    updatePixelPosition();
}