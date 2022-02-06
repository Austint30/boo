//
// Created by austin on 1/30/22.
// Derived from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/openxr_program.h
//

#include <memory>
#include "Options.h"
#include "check.h"
#include <openxr/openxr.h>
#include <vector>
#include <logvisor/logvisor.hpp>

namespace boo {

class OpenXRSystem {
private:
  const std::shared_ptr<Options> m_options;
  XrInstance m_instance{XR_NULL_HANDLE};
  XrSession m_session{XR_NULL_HANDLE};
  XrSpace m_appSpace{XR_NULL_HANDLE};

public:
  explicit OpenXRSystem(const std::shared_ptr<Options>& options);
  virtual ~OpenXRSystem() = default;

  // Create an Instance and other basic instance-level initialization.
  virtual void CreateInstance() = 0;

  // Select a System for the view configuration specified in the Options and initialize the graphics device for the
  // selected system.
  virtual void InitializeSystem() = 0;

  // Create a Session and other basic session-level initialization.
  virtual void InitializeSession() = 0;

  // Create a Swapchain which requires coordinating with the graphics plugin to select the format, getting the system
  // graphics properties, getting the view configuration and grabbing the resulting swapchain images.
  virtual void CreateSwapchains() = 0;

  // Process any events in the event queue.
  virtual void PollEvents(bool* exitRenderLoop, bool* requestRestart) = 0;

  // Manage session lifecycle to track if RenderFrame should be called.
  virtual bool IsSessionRunning() const = 0;

  // Manage session state to track if input should be processed.
  virtual bool IsSessionFocused() const = 0;

  // Sample input actions and generate haptic feedback.
  virtual void PollActions() = 0;

  // Create and submit a frame.
  virtual void RenderFrame() = 0;

  static void LogLayersAndExtensions() {
    // Write out extension properties for a given layer.
    const auto logExtensions = [](const char* layerName, int indent = 0) {
      uint32_t instanceExtensionCount;
      CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));

      std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
      for (XrExtensionProperties& extension : extensions) {
        extension.type = XR_TYPE_EXTENSION_PROPERTIES;
      }

      CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensions.size(),
                                                         &instanceExtensionCount, extensions.data()));

      const std::string indentStr(indent, ' ');

      Log.report(logvisor::Info, FMT_STRING("{}Available Extensions: {}"), indentStr.c_str(), instanceExtensionCount);
      for (const XrExtensionProperties& extension : extensions) {
        Log.report(logvisor::Info, FMT_STRING("{}  Name={} SpecVersion={}"), indentStr.c_str(), extension.extensionName,
                   extension.extensionVersion);
      }
    };

    // Log non-layer extensions (layerName==nullptr).
    logExtensions(nullptr);

    // Log layers and any of their extensions.
    {
      uint32_t layerCount;
      CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));

      std::vector<XrApiLayerProperties> layers(layerCount);
      for (XrApiLayerProperties& layer : layers) {
        layer.type = XR_TYPE_API_LAYER_PROPERTIES;
      }

      CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

      Log.report(logvisor::Info, FMT_STRING("Available Layers: ({})"), layerCount);
      for (const XrApiLayerProperties& layer : layers) {
        Log.report(logvisor::Info, FMT_STRING("  Name={} SpecVersion={} LayerVersion={} Description={}"), layer.layerName,
                   GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description);
        logExtensions(layer.layerName, 4);
      }
    }
  }

  static inline std::string GetXrVersionString(XrVersion ver) {
    return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
  }

  void LogInstanceInfo() {
    if (m_instance == XR_NULL_HANDLE){
      Log.report(logvisor::Error, FMT_STRING("XRInstance is null. Cannot log instance info."));
      return;
    }

    XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
    CHECK_XRCMD(xrGetInstanceProperties(m_instance, &instanceProperties));

    Log.report(logvisor::Info, FMT_STRING("Instance RuntimeName={} RuntimeVersion={}"), instanceProperties.runtimeName,
                                     GetXrVersionString(instanceProperties.runtimeVersion).c_str());
  }
};

} // namespace boo