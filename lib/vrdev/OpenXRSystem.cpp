//
// Created by austin on 1/30/22.
// Derived from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/openxr_program.cpp
//

#include "OpenXRSystem.hpp"

boo::OpenXRSystem::OpenXRSystem(const std::shared_ptr<Options>& options) : m_options(options) {

}

void boo::OpenXRSystem::CreateInstance() {
  LogLayersAndExtensions();

//  CHECK(m_instance == XR_NULL_HANDLE);
//
//  // Create union of extensions required by platform and graphics plugins.
//  std::vector<const char*> extensions;
//
//  // Transform platform and graphics extension std::strings to C strings.
//  const std::vector<std::string> platformExtensions = m_platformPlugin->GetInstanceExtensions();
//  std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(extensions),
//                 [](const std::string& ext) { return ext.c_str(); });
//  const std::vector<std::string> graphicsExtensions = m_graphicsPlugin->GetInstanceExtensions();
//  std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
//                 [](const std::string& ext) { return ext.c_str(); });
//
//  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
//  createInfo.next = m_platformPlugin->GetInstanceCreateExtension();
//  createInfo.enabledExtensionCount = (uint32_t)extensions.size();
//  createInfo.enabledExtensionNames = extensions.data();
//
//  strcpy(createInfo.applicationInfo.applicationName, "HelloXR");
//  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
//
//  CHECK_XRCMD(xrCreateInstance(&createInfo, &m_instance));

  LogInstanceInfo();
}
