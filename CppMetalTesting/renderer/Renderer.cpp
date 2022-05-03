//
//  Renderer.cpp
//  CppMetalTesting
//
//  Created by Johan Solbakken on 03/05/2022.
//

#include "Renderer.hpp"

#include "../math/Math.hpp"

Renderer::Renderer(MTL::Device* pDevice)
: pDevice(pDevice->retain()), angle(0.f), frame(0)
{
    pCommandQueue = pDevice->newCommandQueue();
    buildShaders();
    buildDepthStencilStates();
    buildTextures();
    buildBuffers();
    
    semaphore = dispatch_semaphore_create(kMaxFramesInFlight);
}

Renderer::~Renderer()
{
    pTexture->release();
    pShaderLibrary->release();
    pDepthStencilState->release();
    pVertexDataBuffer->release();
    
    for (int i = 0; i < kMaxFramesInFlight; i++)
    {
        pInstanceDataBuffer[i]->release();
        pCameraDataBuffer[i]->release();
    }
    
    pIndexBuffer->release();
    pPSO->release();
    pCommandQueue->release();
    pDevice->release();
}

namespace shader_types
{
    struct VertexData
    {
        simd::float3 position;
        simd::float3 normal;
        simd::float2 texcoord;
    };
    
    struct InstanceData
    {
        simd::float4x4 instanceTransform;
        simd::float3x3 instanceNormalTransform;
        simd::float4 instanceColor;
    };
    
    struct CameraData
    {
        simd::float4x4 perspectiveTransform;
        simd::float4x4 worldTransform;
        simd::float3x3 worldNormalTransform;
    };
}

void Renderer::draw(MTK::View* pView)
{
    using simd::float3;
    using simd::float4;
    using simd::float4x4;
    
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
    
    frame = (frame + 1) % kMaxFramesInFlight;
    MTL::Buffer* pInstanceDataBuffer = this->pInstanceDataBuffer[frame];
    
    MTL::CommandBuffer* pCommandBuffer = pCommandQueue->commandBuffer();
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    Renderer* pRenderer = this;
    pCommandBuffer->addCompletedHandler([pRenderer]( MTL::CommandBuffer* pCmd){
        dispatch_semaphore_signal( pRenderer->semaphore );
    });
    
    angle += 0.002f;
    
    // Update instance positions
    
    const float scl = 0.2f;
    shader_types::InstanceData* pInstanceData = reinterpret_cast<shader_types::InstanceData*>(pInstanceDataBuffer->contents());
    
    float3 objectPosition = {0.0f, 0.0f, -10.0f};
    
    float4x4 rt = math::makeTranslate(objectPosition);
    float4x4 rr1 = math::makeYRotate(-angle);
    float4x4 rr0 = math::makeXRotate(angle * 0.5f);
    float4x4 rtInv = math::makeTranslate({-objectPosition.x, -objectPosition.y, -objectPosition.z});
    float4x4 fullObjectRot = rt * rr1 * rr0 * rtInv;
    
    
    size_t ix = 0;
    size_t iy = 0;
    size_t iz = 0;
    for (size_t i = 0; i <kNumInstances; i++)
    {
        if (ix == kInstanceRows)
        {
            ix = 0;
            iy += 1;
        }
        if (iy == kInstanceRows)
        {
            iy = 0;
            iz += 1;
        }
        
        float4x4 scale = math::makeScale( (float3){ scl, scl, scl } );
        float4x4 zrot = math::makeZRotate( angle * sinf((float)ix) );
        float4x4 yrot = math::makeYRotate( angle * cosf((float)iy));
        
        float x = ((float)ix - (float)kInstanceRows/2.f) * (2.f * scl) + scl;
        float y = ((float)iy - (float)kInstanceColumns/2.f) * (2.f * scl) + scl;
        float z = ((float)iz - (float)kInstanceDepth/2.f) * (2.f * scl);
        float4x4 translate = math::makeTranslate( math::add( objectPosition, { x, y, z } ) );

        pInstanceData[ i ].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
        pInstanceData[ i ].instanceNormalTransform = math::discardTranslation( pInstanceData[ i ].instanceTransform );

        float iDivNumInstances = i / (float)kNumInstances;
        float r = iDivNumInstances;
        float g = 1.0f - r;
        float b = sinf( M_PI * 2.0f * iDivNumInstances );
        pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };

        ix += 1;
    }
    
    pInstanceDataBuffer->didModifyRange(NS::Range::Make(0, pInstanceDataBuffer->length()));
    
    // Update camera state:
    
    MTL::Buffer* pCameraDataBuffer = this->pCameraDataBuffer[frame];
    shader_types::CameraData* pCameraData = reinterpret_cast<shader_types::CameraData*>(pCameraDataBuffer->contents());
    pCameraData->perspectiveTransform = math::makePerspective(45.0f * M_PI / 180.0f, 1.0f, 0.03f, 500.0f);
    pCameraData->worldTransform = math::makeIdentity();
    pCameraData->worldNormalTransform = math::discardTranslation(pCameraData->worldTransform);
    pCameraDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::CameraData)));
    
    // Begin render pass:
    
    MTL::RenderPassDescriptor* pRenderPassDescriptor = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pCommandEncoder = pCommandBuffer->renderCommandEncoder(pRenderPassDescriptor);
    
    pCommandEncoder->setRenderPipelineState(pPSO);
    pCommandEncoder->setDepthStencilState(pDepthStencilState);
    
    pCommandEncoder->setVertexBuffer(pVertexDataBuffer, 0, 0);
    pCommandEncoder->setVertexBuffer(pInstanceDataBuffer, 0, 1);
    pCommandEncoder->setVertexBuffer(pCameraDataBuffer, 0, 2);
    
    pCommandEncoder->setFragmentTexture(pTexture, 0);
    
    pCommandEncoder->setCullMode(MTL::CullModeBack);
    pCommandEncoder->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);
    
    pCommandEncoder->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 6*6, MTL::IndexType::IndexTypeUInt16, pIndexBuffer, 0, kNumInstances);
    
    pCommandEncoder->endEncoding();
    pCommandBuffer->presentDrawable(pView->currentDrawable());
    pCommandBuffer->commit();
    
    pPool->release();
}

void Renderer::buildShaders()
{
    using NS::StringEncoding::UTF8StringEncoding;
    
    NS::Error* pError = nullptr;
    pShaderLibrary = pDevice->newDefaultLibrary();
    if (!pShaderLibrary)
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    
    MTL::Function* pVertexFn = pShaderLibrary->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
    MTL::Function* pFragFn = pShaderLibrary->newFunction(NS::String::string("fragmentMain", UTF8StringEncoding));
    
    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(pVertexFn);
    pDesc->setFragmentFunction(pFragFn);
    pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);
    
    pPSO = pDevice->newRenderPipelineState(pDesc, &pError);
    if (!pPSO)
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    
    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
}

void Renderer::buildDepthStencilStates()
{
    MTL::DepthStencilDescriptor* pDepthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDepthStencilDesc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    pDepthStencilDesc->setDepthWriteEnabled(true);
    pDepthStencilState = pDevice->newDepthStencilState(pDepthStencilDesc);
    pDepthStencilDesc->release();
}

void Renderer::buildBuffers()
{
    using simd::float3;
    
    const float s = 0.5f;
    
    shader_types::VertexData verts[] = {
        //                                         Texture
        //   Positions           Normals         Coordinates
        { { -s, -s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
        { { +s, -s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
        { { +s, +s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
        { { -s, +s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },

        { { +s, -s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { +s, -s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { +s, +s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { +s, +s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },

        { { +s, -s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
        { { -s, -s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
        { { -s, +s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
        { { +s, +s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },

        { { -s, -s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { -s, -s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { -s, +s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { -s, +s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },

        { { -s, +s, +s }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },
        { { +s, +s, +s }, {  0.f,  1.f,  0.f }, { 1.f, 1.f } },
        { { +s, +s, -s }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
        { { -s, +s, -s }, {  0.f,  1.f,  0.f }, { 0.f, 0.f } },

        { { -s, -s, -s }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } },
        { { +s, -s, -s }, {  0.f, -1.f,  0.f }, { 1.f, 1.f } },
        { { +s, -s, +s }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
        { { -s, -s, +s }, {  0.f, -1.f,  0.f }, { 0.f, 0.f } }
    };

    uint16_t indices[] = {
        0,  1,  2,  2,  3,  0, /* front */
        4,  5,  6,  6,  7,  4, /* right */
        8,  9, 10, 10, 11,  8, /* back */
       12, 13, 14, 14, 15, 12, /* left */
       16, 17, 18, 18, 19, 16, /* top */
       20, 21, 22, 22, 23, 20, /* bottom */
    };
    
    const size_t vertexDataSize = sizeof(verts);
    const size_t indexDataSize = sizeof(indices);
    
    pVertexDataBuffer = pDevice->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);
    pIndexBuffer = pDevice->newBuffer(indexDataSize, MTL::ResourceStorageModeManaged);

    memcpy(pVertexDataBuffer->contents(), verts, vertexDataSize);
    memcpy(pIndexBuffer->contents(), indices, indexDataSize);
    
    pVertexDataBuffer->didModifyRange(NS::Range::Make(0,pVertexDataBuffer->length()));
    pIndexBuffer->didModifyRange(NS::Range::Make(0, pIndexBuffer->length()));
    
    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof(shader_types::InstanceData);
    for (size_t i = 0; i < kMaxFramesInFlight; i++)
    {
        pInstanceDataBuffer[i] = pDevice->newBuffer(instanceDataSize, MTL::ResourceStorageModeManaged);
    }
    
    const size_t cameraDataSize = kMaxFramesInFlight * sizeof(shader_types::CameraData);
    for (size_t i = 0; i < kMaxFramesInFlight; i++)
    {
        pCameraDataBuffer[i] = pDevice->newBuffer(cameraDataSize, MTL::ResourceStorageModeManaged);
    }
}

void Renderer::buildTextures()
{
    const uint32_t tw = 128;
    const uint32_t th = 128;
    
    MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pTextureDesc->setWidth(tw);
    pTextureDesc->setHeight(th);
    pTextureDesc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    pTextureDesc->setTextureType(MTL::TextureType2D);
    pTextureDesc->setStorageMode(MTL::StorageModeManaged);
    pTextureDesc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead);
    
    pTexture = pDevice->newTexture(pTextureDesc);
    
    uint8_t* pTextureData = (uint8_t*)alloca(tw*th*4);
    for ( size_t y = 0; y < th; ++y )
    {
        for ( size_t x = 0; x < tw; ++x )
        {
            bool isWhite = (x^y) & 0b1000000;
            uint8_t c = isWhite ? 0xFF : 0xA;

            size_t i = y * tw + x;

            pTextureData[ i * 4 + 0 ] = c;
            pTextureData[ i * 4 + 1 ] = c;
            pTextureData[ i * 4 + 2 ] = c;
            pTextureData[ i * 4 + 3 ] = 0xFF;
        }
    }
    
    pTexture->replaceRegion( MTL::Region( 0, 0, 0, tw, th, 1 ), 0, pTextureData, tw * 4 );
    
    pTextureDesc->release();
}
