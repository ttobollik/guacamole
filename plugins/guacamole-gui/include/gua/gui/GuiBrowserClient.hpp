
#ifndef GUA_CEF_CLIENT_HPP
#define GUA_CEF_CLIENT_HPP

#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <gua/gui/GLSurface.inl>


namespace gua{

class GuiBrowserClient : public CefClient
{
public:
    GuiBrowserClient( CefRefPtr<GLSurface> renderHandler)
        : m_renderHandler(renderHandler)
    {std::cout << "Its-a-me-Client!" << std::endl;}

    virtual CefRefPtr<GLSurface> getRenderHandler() {
        std::cout << "get Render" << std::endl;
        return m_renderHandler;
    }

    CefRefPtr<GLSurface> m_renderHandler;

    IMPLEMENT_REFCOUNTING(GuiBrowserClient);
};

}

#endif //GUA_CEF_CLIENT_HPP

