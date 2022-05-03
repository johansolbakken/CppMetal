//
//  Renderer.hpp
//  CppMetalTesting
//
//  Created by Johan Solbakken on 03/05/2022.
//

#ifndef Renderer_hpp
#define Renderer_hpp

#include <stdio.h>

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

static constexpr size_t kMaxFramesInFlight = 3;

static constexpr size_t kInstanceRows = 10;
static constexpr size_t kInstanceColumns = 10;
static constexpr size_t kInstanceDepth = 10;

static constexpr size_t kNumInstances = (kInstanceRows * kInstanceColumns * kInstanceDepth);



class Renderer
{
public:
    Renderer(MTL::Device* pDevice);
    ~Renderer();
    void draw(MTK::View* pView);
    void buildShaders();
    void buildBuffers();
    void buildTextures();
    void buildDepthStencilStates();
    
private:
    MTL::Device* pDevice;
    MTL::CommandQueue* pCommandQueue;
    MTL::Library* pShaderLibrary;
    MTL::RenderPipelineState* pPSO;
    
    MTL::DepthStencilState* pDepthStencilState;
    MTL::Buffer* pCameraDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* pIndexBuffer;
    MTL::Buffer* pInstanceDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* pVertexDataBuffer;
    MTL::Texture* pTexture;

    float angle;
    int frame;
    dispatch_semaphore_t semaphore;
};

#endif /* Renderer_hpp */
