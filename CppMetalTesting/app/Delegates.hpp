//
//  Delegates.hpp
//  CppMetalTesting
//
//  Created by Johan Solbakken on 03/05/2022.
//

#ifndef Delegates_hpp
#define Delegates_hpp

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "../renderer/Renderer.hpp"

class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
    MyMTKViewDelegate(MTL::Device* pDevice);
    ~MyMTKViewDelegate() override;
    void drawInMTKView(MTK::View* pView) override;
    
private:
    Renderer* pRenderer;
    
};

class MyAppDelegate : public NS::ApplicationDelegate
{
public:
    ~MyAppDelegate();
    
    NS::Menu* createMenuBar();
    
    void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
    void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
    bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;
    
private:
    NS::Window* pWindow;
    MTK::View* pMtkView;
    MTL::Device* pDevice;
    MyMTKViewDelegate* pViewDelegate = nullptr;
    
};

#endif /* Delegates_hpp */
