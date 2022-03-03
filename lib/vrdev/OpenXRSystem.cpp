//
// Created by austin on 1/30/22.
// Derived from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/openxr_program.cpp
//

#include "boo/vrdev/OpenXRSystem.hpp"

namespace boo {

inline XrFormFactor GetXrFormFactor(const std::string& formFactorStr) {
  if (EqualsIgnoreCase(formFactorStr, "Handheld")) {
    return XR_FORM_FACTOR_HANDHELD_DISPLAY;
  }
  if (!EqualsIgnoreCase(formFactorStr, "Hmd")) {
    Log.report(logvisor::Fatal, FMT_STRING("Unknown form factor '{}'"), formFactorStr.c_str());
  }
  return XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
}

inline XrViewConfigurationType GetXrViewConfigurationType(const std::string& viewConfigurationStr) {
  if (EqualsIgnoreCase(viewConfigurationStr, "Mono")) {
    return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
  }
  if (!EqualsIgnoreCase(viewConfigurationStr, "Stereo")) {
    Log.report(logvisor::Fatal, FMT_STRING("Unknown view configuration '{}'"), viewConfigurationStr.c_str());
  }

  return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
}

inline XrEnvironmentBlendMode GetXrEnvironmentBlendMode(const std::string& environmentBlendModeStr) {
    if (EqualsIgnoreCase(environmentBlendModeStr, "Additive")) {
    return XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
  }
  if (EqualsIgnoreCase(environmentBlendModeStr, "AlphaBlend")) {
    return XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
  }
  if (!EqualsIgnoreCase(environmentBlendModeStr, "Opaque")) {
    Log.report(logvisor::Fatal, FMT_STRING("Unknown environment blend mode '{}'"), environmentBlendModeStr.c_str());
  }

  return XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
}

OpenXRSystem::OpenXRSystem(const OpenXROptions options)
: m_options(options) {}

void OpenXRSystem::createInstance(std::vector<std::string> graphicsExtensions) {
  LogLayersAndExtensions();

  CHECK(m_instance == XR_NULL_HANDLE);

  // Create union of extensions required by platform and graphics plugins.
  std::vector<const char*> extensions;

  // Transform platform and graphics extension std::strings to C strings.
  std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                 [](const std::string& ext) { return ext.c_str(); });

  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  //  createInfo.next = m_application->GetInstanceCreateExtension();
  createInfo.enabledExtensionCount = (uint32_t)extensions.size();
  createInfo.enabledExtensionNames = extensions.data();

  strcpy(createInfo.applicationInfo.applicationName, m_options.AppName.c_str());
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  CHECK_XRCMD(xrCreateInstance(&createInfo, &m_instance));

  LogInstanceInfo();
}

void OpenXRSystem::initializeSystem() {
  CHECK(m_instance != XR_NULL_HANDLE);
  CHECK(m_systemId == XR_NULL_SYSTEM_ID);

  m_formFactor = GetXrFormFactor(m_options.FormFactor);
  m_viewConfigType = GetXrViewConfigurationType(m_options.ViewConfiguration);
  m_environmentBlendMode = GetXrEnvironmentBlendMode(m_options.EnvironmentBlendMode);

  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = m_formFactor;
  CHECK_XRCMD(xrGetSystem(m_instance, &systemInfo, &m_systemId));

  Log.report(logvisor::Info, FMT_STRING("Using system {} for form factor {}"), m_systemId, to_string(m_formFactor));
  CHECK(m_instance != XR_NULL_HANDLE);
  CHECK(m_systemId != XR_NULL_SYSTEM_ID);

  LogViewConfigurations();

  // The graphics API can initialize the graphics device now that the systemId and instance
  // handle are available.
//  m_graphicsPlugin->InitializeDevice(m_instance, m_systemId);
}

void OpenXRSystem::initializeSession(XrBaseInStructure* graphicsBinding) {
  CHECK(m_instance != XR_NULL_HANDLE);
  CHECK(m_session == XR_NULL_HANDLE);

  {
    Log.report(logvisor::Info, FMT_STRING("Creating session..."));

    XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = graphicsBinding;
    createInfo.systemId = m_systemId;
    CHECK_XRCMD(xrCreateSession(m_instance, &createInfo, &m_session));
  }
//  InitializeActions();
}

} // boo


