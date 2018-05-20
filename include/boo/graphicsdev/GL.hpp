#ifndef GDEV_GL_HPP
#define GDEV_GL_HPP
#if BOO_HAS_GL

#include "IGraphicsDataFactory.hpp"
#include "IGraphicsCommandQueue.hpp"
#include "boo/IGraphicsContext.hpp"
#include "GLSLMacros.hpp"

namespace boo
{
struct BaseGraphicsData;

struct GLContext
{
    uint32_t m_sampleCount = 1;
    uint32_t m_anisotropy = 1;
    bool m_deepColor = false;
};

class GLDataFactory : public IGraphicsDataFactory
{
public:
    class Context : public IGraphicsDataFactory::Context
    {
        friend class GLDataFactoryImpl;
        GLDataFactory& m_parent;
        ObjToken<BaseGraphicsData> m_data;
        Context(GLDataFactory& parent __BooTraceArgs);
        ~Context();
    public:
        Platform platform() const { return Platform::OpenGL; }
        const SystemChar* platformName() const { return _S("OpenGL"); }

        ObjToken<IGraphicsBufferS> newStaticBuffer(BufferUse use, const void* data, size_t stride, size_t count);
        ObjToken<IGraphicsBufferD> newDynamicBuffer(BufferUse use, size_t stride, size_t count);

        ObjToken<ITextureS> newStaticTexture(size_t width, size_t height, size_t mips, TextureFormat fmt,
                                             TextureClampMode clampMode, const void* data, size_t sz);
        ObjToken<ITextureSA> newStaticArrayTexture(size_t width, size_t height, size_t layers, size_t mips,
                                                   TextureFormat fmt, TextureClampMode clampMode, const void* data, size_t sz);
        ObjToken<ITextureD> newDynamicTexture(size_t width, size_t height, TextureFormat fmt, TextureClampMode clampMode);
        ObjToken<ITextureR> newRenderTexture(size_t width, size_t height, TextureClampMode clampMode,
                                             size_t colorBindingCount, size_t depthBindingCount);

        bool bindingNeedsVertexFormat() const { return true; }
        ObjToken<IVertexFormat> newVertexFormat(size_t elementCount, const VertexElementDescriptor* elements,
                                                size_t baseVert = 0, size_t baseInst = 0);

        ObjToken<IShaderPipeline> newShaderPipeline(const char* vertSource, const char* fragSource,
                                                    size_t texCount, const char** texNames,
                                                    size_t uniformBlockCount, const char** uniformBlockNames,
                                                    BlendFactor srcFac, BlendFactor dstFac, Primitive prim,
                                                    ZTest depthTest, bool depthWrite, bool colorWrite,
                                                    bool alphaWrite, CullMode culling, bool overwriteAlpha = true);

        ObjToken<IShaderDataBinding>
        newShaderDataBinding(const ObjToken<IShaderPipeline>& pipeline,
                             const ObjToken<IVertexFormat>& vtxFormat,
                             const ObjToken<IGraphicsBuffer>& vbo,
                             const ObjToken<IGraphicsBuffer>& instVbo,
                             const ObjToken<IGraphicsBuffer>& ibo,
                             size_t ubufCount, const ObjToken<IGraphicsBuffer>* ubufs, const PipelineStage* ubufStages,
                             const size_t* ubufOffs, const size_t* ubufSizes,
                             size_t texCount, const ObjToken<ITexture>* texs,
                             const int* texBindIdx, const bool* depthBind,
                             size_t baseVert = 0, size_t baseInst = 0);
    };
};

}

#endif
#endif // GDEV_GL_HPP
