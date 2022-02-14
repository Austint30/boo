#pragma once
#if BOO_HAS_GL

#include <cstdint>

#include "boo/BooObject.hpp"
#include "boo/graphicsdev/IGraphicsCommandQueue.hpp"
#include "boo/graphicsdev/IGraphicsDataFactory.hpp"

namespace boo {
struct BaseGraphicsData;

struct GLContext {
  uint32_t m_sampleCount = 1;
  uint32_t m_anisotropy = 1;
  bool m_deepColor = false;
};

class GLDataFactory : public IGraphicsDataFactory {
public:
  class Context final : public IGraphicsDataFactory::Context {
    friend class GLDataFactoryImpl;
    GLDataFactory& m_parent;
    ObjToken<BaseGraphicsData> m_data;
    Context(GLDataFactory& parent __BooTraceArgs);
    ~Context();

  public:
    Platform platform() const override { return Platform::OpenGL; }
    const char* platformName() const override { return "OpenGL"; }

    ObjToken<IGraphicsBufferS> newStaticBuffer(BufferUse use, const void* data, size_t stride, size_t count) override;
    ObjToken<IGraphicsBufferD> newDynamicBuffer(BufferUse use, size_t stride, size_t count) override;

    ObjToken<ITextureS> newStaticTexture(size_t width, size_t height, size_t mips, TextureFormat fmt,
                                         TextureClampMode clampMode, const void* data, size_t sz) override;
    ObjToken<ITextureSA> newStaticArrayTexture(size_t width, size_t height, size_t layers, size_t mips,
                                               TextureFormat fmt, TextureClampMode clampMode, const void* data,
                                               size_t sz) override;
    ObjToken<ITextureD> newDynamicTexture(size_t width, size_t height, TextureFormat fmt,
                                          TextureClampMode clampMode) override;
    ObjToken<ITextureR> newRenderTexture(size_t width, size_t height, TextureClampMode clampMode,
                                         size_t colorBindingCount, size_t depthBindingCount) override;
    ObjToken<ITextureCubeR> newCubeRenderTexture(size_t width, size_t mips) override;

    ObjToken<IShaderStage> newShaderStage(const uint8_t* data, size_t size, PipelineStage stage) override;

    ObjToken<IShaderPipeline> newShaderPipeline(ObjToken<IShaderStage> vertex, ObjToken<IShaderStage> fragment,
                                                ObjToken<IShaderStage> geometry, ObjToken<IShaderStage> control,
                                                ObjToken<IShaderStage> evaluation, const VertexFormatInfo& vtxFmt,
                                                const AdditionalPipelineInfo& additionalInfo,
                                                bool asynchronous = true) override;

    ObjToken<IShaderDataBinding>
    newShaderDataBinding(const ObjToken<IShaderPipeline>& pipeline, const ObjToken<IGraphicsBuffer>& vbo,
                         const ObjToken<IGraphicsBuffer>& instVbo, const ObjToken<IGraphicsBuffer>& ibo,
                         size_t ubufCount, const ObjToken<IGraphicsBuffer>* ubufs, const PipelineStage* ubufStages,
                         const size_t* ubufOffs, const size_t* ubufSizes, size_t texCount,
                         const ObjToken<ITexture>* texs, const int* texBindIdx, const bool* depthBind,
                         size_t baseVert = 0, size_t baseInst = 0) override;
  };
  std::vector<std::string> openXrInstanceExtensions() override { return {}; };
};

} // namespace boo

#endif
