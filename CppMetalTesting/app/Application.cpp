//
//  Application.cpp
//  CppMetalTesting
//
//  Created by Johan Solbakken on 03/05/2022.
//

#include "Application.hpp"

Application::Application()
{
    
}

void Application::Run()
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
    
    MyAppDelegate del;
    
    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate(&del);
    pSharedApplication->run();
    
    pPool->release();
    
}
