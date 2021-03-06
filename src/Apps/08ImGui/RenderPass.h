#pragma once

#include <memory>
#include <AppBox/InputEvents.h>
#include <AppBox/WindowEvents.h>

class RenderCommandList;

class IPass
    : public WindowEvents
{
public:
    virtual ~IPass() = default;
    virtual void OnUpdate() {}
    virtual void OnRender(RenderCommandList& command_list)= 0;
    virtual void OnResize(int width, int height) {}
};
