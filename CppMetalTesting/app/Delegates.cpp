//
//  Delegates.cpp
//  CppMetalTesting
//
//  Created by Johan Solbakken on 03/05/2022.
//

#include "Delegates.hpp"

#pragma mark - ViewDelegate
#pragma region ViewDelegate {

MyMTKViewDelegate::MyMTKViewDelegate(MTL::Device* pDevice)
: MTK::ViewDelegate(), pRenderer(new Renderer(pDevice))
{
    
}

MyMTKViewDelegate::~MyMTKViewDelegate()
{
    delete pRenderer;
}

void MyMTKViewDelegate::drawInMTKView(MTK::View *pView)
{
    pRenderer->draw(pView);
}

#pragma endregion ViewDelegate }

#pragma mark - AppDelegate
#pragma region AppDelegate {

MyAppDelegate::~MyAppDelegate()
{
    pMtkView->release();
    pWindow->release();
    pDevice->release();
    delete pViewDelegate;
}

NS::Menu* MyAppDelegate::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;
    
    NS::Menu* pMainMenu = NS::Menu::alloc()->init();
    
    // SUBMENU: App
    NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pAppMenu = NS::Menu::alloc()->init(NS::String::string("Appname", UTF8StringEncoding));
    // Creating quit item string
    NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quitItemName = NS::String::string("Quit ", UTF8StringEncoding)->stringByAppendingString(appName);
    SEL quitCallback = NS::MenuItem::registerActionCallback("appQuit", [](void*, SEL, const NS::Object* pSender){
        auto pApp = NS::Application::sharedApplication();
        pApp->terminate(pSender);
    });
    NS::MenuItem* pAppQuitItem = pAppMenu->addItem(quitItemName, quitCallback, NS::String::string("q", UTF8StringEncoding));
    pAppQuitItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    pAppMenuItem->setSubmenu(pAppMenu);
    
    // SUBMENU: Window
    NS::MenuItem* pWindowMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pWindowMenu = NS::Menu::alloc()->init(NS::String::string("Window", UTF8StringEncoding));
    SEL closeWindowCallback = NS::MenuItem::registerActionCallback("windowClose", [](void*, SEL, const NS::Object*){
        auto pApp = NS::Application::sharedApplication();
        pApp->windows()->object<NS::Window>(0)->close();
    });
    NS::MenuItem* pCloseWindowItem = pWindowMenu->addItem( NS::String::string( "Close Window", UTF8StringEncoding ), closeWindowCallback, NS::String::string( "w", UTF8StringEncoding ) );
    pCloseWindowItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
    pWindowMenuItem->setSubmenu( pWindowMenu );
    
    // Add submenus to main menu
    pMainMenu->addItem( pAppMenuItem );
    pMainMenu->addItem( pWindowMenuItem );
    
    pAppMenuItem->release();
    pWindowMenuItem->release();
    pAppMenu->release();
    pWindowMenu->release();
    
    return pMainMenu->autorelease();
}

void MyAppDelegate::applicationWillFinishLaunching(NS::Notification *pNotification)
{
    NS::Menu* pMenu = createMenuBar();
    NS::Application* pApp = reinterpret_cast<NS::Application*>(pNotification->object());
    pApp->setMainMenu(pMenu);
    pApp->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
}

void MyAppDelegate::applicationDidFinishLaunching(NS::Notification *pNotification)
{
    CGRect frame = (CGRect){ {100.0, 100.0}, {1024.0, 1024.0} };
    
    pWindow = NS::Window::alloc()->init(frame,
                                        NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled,
                                        NS::BackingStoreBuffered,
                                        false);
    
    pDevice = MTL::CreateSystemDefaultDevice();
    
    pMtkView = MTK::View::alloc()->init(frame, pDevice);
    pMtkView->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    pMtkView->setClearColor(MTL::ClearColor::Make(0.1, 0.1,0.1, 1.0));
    pMtkView->setDepthStencilPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);
    pMtkView->setClearDepth(1.0f);
    
    pViewDelegate = new MyMTKViewDelegate(pDevice);
    pMtkView->setDelegate(pViewDelegate);
    
    pWindow->setContentView(pMtkView);
    pWindow->setTitle(NS::String::string("Min tittel på dette vindu", NS::StringEncoding::UTF8StringEncoding));
    
    pWindow->makeKeyAndOrderFront(nullptr);
    
    NS::Application* pApp = reinterpret_cast<NS::Application*>(pNotification->object());
    pApp->activateIgnoringOtherApps(true);
}

bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application *pSender)
{
    return true;
}

#pragma endregion AppDelegate }
