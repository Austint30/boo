#include "graphicsys/IGFXContext.hpp"
#include "windowsys/IWindow.hpp"

namespace boo
{

class CGraphicsContextWayland final : public IGFXContext
{
    
    EGraphicsAPI m_api;
    EPixelFormat m_pf;
    IWindow* m_parentWindow;
    
public:
    IWindowCallback* m_callback;
    
    CGraphicsContextWayland(EGraphicsAPI api, IWindow* parentWindow)
    : m_api(api),
      m_pf(PF_RGBA8),
      m_parentWindow(parentWindow)
    {}
    
    ~CGraphicsContextWayland()
    {
        
    }
    
    void _setCallback(IWindowCallback* cb)
    {
        m_callback = cb;
    }
    
    EGraphicsAPI getAPI() const
    {
        return m_api;
    }
    
    EPixelFormat getPixelFormat() const
    {
        return m_pf;
    }
    
    void setPixelFormat(EPixelFormat pf)
    {
        if (pf > PF_RGBAF32_Z24)
            return;
        m_pf = pf;
    }
    
    void initializeContext()
    {
        
    }
    
};

IGFXContext* _CGraphicsContextWaylandNew(IGFXContext::EGraphicsAPI api,
                                         IWindow* parentWindow)
{
    return new CGraphicsContextWayland(api, parentWindow);
}
    
}
