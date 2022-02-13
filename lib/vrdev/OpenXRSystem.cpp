//
// Created by austin on 1/30/22.
// Derived from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/openxr_program.cpp
//

#include "boo/vrdev/OpenXRSystem.hpp"

namespace boo {

  OpenXRSystem::OpenXRSystem(const std::shared_ptr<Options>& options,
                                  const std::shared_ptr<IGraphicsDataFactory>& graphicsFactory)
  : m_options(options), m_graphicsFactory(graphicsFactory) {}

  void OpenXRSystem::CreateInstance() {
    LogLayersAndExtensions();

    CHECK(m_instance == XR_NULL_HANDLE);

    // Create union of extensions required by platform and graphics plugins.
    std::vector<const char*> extensions;

    // Transform platform and graphics extension std::strings to C strings.
    const std::vector<std::string> graphicsExtensions = m_graphicsFactory->openXrInstanceExtensions();
    std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                   [](const std::string& ext) { return ext.c_str(); });

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    //  createInfo.next = m_application->GetInstanceCreateExtension();
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.enabledExtensionNames = extensions.data();

    std::string appName{m_options->AppName};
    strcpy(createInfo.applicationInfo.applicationName, appName.c_str());
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    CHECK_XRCMD(xrCreateInstance(&createInfo, &m_instance));

    LogInstanceInfo();
  }

}


