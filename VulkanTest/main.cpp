#define DEBUG

// GLFW library for window creation will automatility load the Vulkan header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Other headers
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

// Validation layers to be used
const std::vector<const char*> requestedLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

// Device extensions to be used
const std::vector<const char*> requestedDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Enable validation layers during debug
#ifdef DEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif // DEBUG

// Load create debug report callback object
VkResult CreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback) {

	std::cout << "CREATING DEBUG REPORT CALLBACK OBJECT\n" << std::endl;

	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

	if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pCallback);
	else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// Load destroy debug report callback function
void DestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT callback,
	const VkAllocationCallbacks* pAllocator) {

	std::cout << "DESTROYING DEBUG REPORT CALLBACK OBJECT\n" << std::endl;

	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) func(instance, callback, pAllocator);

}

// Indices for the queue families
struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete() {
		return (graphicsFamily >= 0 && presentFamily >= 0);
	}
};

// Details about the swap chain support
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// The program is wrapped into a class
class HelloTriangleApplication {

public:

	void run() {

		std::cout << APP_NAME << " powered by " << ENGINE_NAME << std::endl << std::endl;

		initWindow();
		initVulkan();
		mainLoop();
		cleanup();

	}

	// Vulkan objects are stored as private class members
private:

	// Application info
	const char* APP_NAME = "Hello Triangle";
	const char* ENGINE_NAME = "No Engine";

	// Window
	const int WIN_WIDTH = 800;
	const int WIN_HEIGHT = 600;
	GLFWwindow* window;

	// Vulkan instance
	VkInstance instance;

	// Debug callback handle
	VkDebugReportCallbackEXT callback;

	// Window surface
	VkSurfaceKHR surface;

	// Graphics card handle
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	// Loagical device handle
	VkDevice device;

	// Graphics queue handle
	VkQueue graphicsQueue;

	// Presentation queue handle
	VkQueue presentQueue;

	// Check if all requested validation layers are available
	bool checkValidationLayerSupport() {

		std::cout << "CHECKING VALIDATION LAYERS\n" << std::endl;

		// List available validation layers
		uint32_t availableLayerCount;
		vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(availableLayerCount);
		vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

		std::cout << "Available validation layers:" << std::endl;
		for (const auto& availableLayer : availableLayers) {
			std::cout << "\t" << availableLayer.layerName << std::endl;
		}
		std::cout << std::endl;

		// Check the requested are available
		std::cout << "Requested validation layers:" << std::endl;
		bool layerFound;
		for (const char* requestedLayer : requestedLayers) {
			std::cout << "\t" << requestedLayer;
			layerFound = false;
			for (const auto& availableLayer : availableLayers) {
				if (strcmp(requestedLayer, availableLayer.layerName) == 0) {
					layerFound = true;
					std::cout << " (Supported)" << std::endl;
					break;
				}
			}
			if (!layerFound) return false;
		}
		std::cout << std::endl;

		return true;
	}

	// Check extensions
	std::vector<const char*> getRequestedExtensions() {

		std::cout << "CHECKING EXTENSIONS\n" << std::endl;

		// List available extensions
		uint32_t availableExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

		std::cout << "Available extensions:" << std::endl;
		for (const auto& availableExtension : availableExtensions) {
			std::cout << "\t" << availableExtension.extensionName << std::endl;
		}
		std::cout << std::endl;

		// Vector to store info on required extensions
		std::vector<const char*> requestedExtensions;

		// Query GLFW for the extensions it needs
		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (unsigned int i = 0; i < glfwExtensionCount; i++) {
			requestedExtensions.push_back(glfwExtensions[i]);
		}

		// Debug reports extension for validation layers feedback
		if (enableValidationLayers) {
			requestedExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		// List requested extensions
		std::cout << "Requested extensions:" << std::endl;
		bool extensionFound;
		for (const char* requestedExtension : requestedExtensions) {
			std::cout << '\t' << requestedExtension;
			extensionFound = false;
			for (const auto& availableExtension : availableExtensions) {
				if (strcmp(requestedExtension, availableExtension.extensionName) == 0) {
					extensionFound = true;
					std::cout << " (Supported)" << std::endl;
					break;
				}
			}
			if (!extensionFound) {
				std::cout << " (Unsupported)";
				throw std::runtime_error("Extension requested but not available");
			}
		}
		std::cout << std::endl;

		return requestedExtensions;

	}

	// Debug callback function
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData) {

		std::cerr << "Validation layer: " << msg << std::endl;

		return VK_FALSE;
	}

	// Create Vulkan instance
	void createInstance() {

		// Set up validation layers
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("Validation layer requested but not available");
		}

		// Vulkan application information
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = APP_NAME;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = ENGINE_NAME;
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		std::cout << "ENABLING VALIDATION LAYERS\n" << std::endl;

		// Validation layers to be used
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(requestedLayers.size());
			createInfo.ppEnabledLayerNames = requestedLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// Query for required extensions
		auto extensions = getRequestedExtensions();

		std::cout << "ENABLING EXTENSIONS\n" << std::endl;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		std::cout << "CREATING VULKAN INSTANCE\n" << std::endl;

		// Create the instance and check for errors
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create instance!");
		}

	}

	// Setup debug callback (with reference to its handle)
	void setupDebugCallback() {

		if (!enableValidationLayers) return;

		std::cout << "DEBUG CALLBACK SETUP\n" << std::endl;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		// Which type of messages to receive
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		// Pointer to the callback function itself
		createInfo.pfnCallback = debugCallback;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up debug callback!");
		}

	}

	// Queue family lookup
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice) {

		QueueFamilyIndices indices;

		// Retrieve the list of queue families
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		VkBool32 presentSupport;
		int i = 0;

		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;
			presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) indices.presentFamily = i;
			if (indices.isComplete()) break;
			i++;
		}

		return indices;
	}

	// Checks if a GPU is suitable based on its properties and features
	/*bool isDeviceSuitable(
	VkPhysicalDeviceProperties deviceProperties,
	VkPhysicalDeviceFeatures deviceFeatures) {

	// Physical device must be a discrete GPU
	VkPhysicalDeviceType requestedDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

	// Physical device must support the requested features
	bool deviceSupportFeatures = true;

	std::cout << "Feature check: ";

	std::cout << "Geometry shader";
	if (deviceFeatures.geometryShader) std::cout << " (Supported), ";
	else {
	std::cout << " (Unsupported), ";
	deviceSupportFeatures = false;
	}

	std::cout << "Logic operations";
	if (deviceFeatures.logicOp) std::cout << " (Supported)";
	else {
	std::cout << " (Unsupported)";
	deviceSupportFeatures = false;
	}

	return (deviceProperties.deviceType == requestedDeviceType &&
	deviceSupportFeatures);

	}*/

	// Check device extension support
	bool checkPhysicalDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requestedExtenstions(requestedDeviceExtensions.begin(), requestedDeviceExtensions.end());

		for (const auto& availableExtension : availableExtensions) requestedExtenstions.erase(availableExtension.extensionName);

		return requestedExtenstions.empty();
	}

	// Query details of swap chain support
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice) {

		SwapChainSupportDetails details;

		// Query supported surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		// Query supported surface formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
		}

		// Query supported presentation modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	// Rate a physical device
	int ratePhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		int score = 0;

		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

		QueueFamilyIndices queueFamilyIndices;
		queueFamilyIndices = findQueueFamilies(physicalDevice);

		// Discrete GPUs have a significant performance advantage
		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;

		// Maximum possible size of textures affects graphics quality
		score += physicalDeviceProperties.limits.maxImageDimension2D;

		// Device must support requested features
		if (!physicalDeviceFeatures.geometryShader) return 0;

		// Device must be able to process some commands 
		if (!queueFamilyIndices.isComplete()) return 0;

		// Device must support some extensions
		if (!checkPhysicalDeviceExtensionSupport(physicalDevice)) return 0;

		// Device must support at least one image format and one presentation mode
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
		if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return 0;

		return score;
	}

	// Selects GPUs
	void pickPhysicalDevice() {

		std::multimap<int, VkPhysicalDevice> physicalDevicesAvailable;

		std::cout << "CHECKING PHYSICAL DEVICES\n" << std::endl;
		// Detect GPUs
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		if (physicalDeviceCount == 0) throw std::runtime_error("Failed to detect GPUs with Vulkan support.");
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

		// List GPUs
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		QueueFamilyIndices queueFamilyIndices;
		int physicalDeviceScore = 0;
		std::cout << "Available physical devices:" << std::endl;
		std::cout << "ID\tType\tName\t\tVersion" << std::endl;
		for (const auto& physicalDevice : physicalDevices) {

			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
			vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
			queueFamilyIndices = findQueueFamilies(physicalDevice);

			physicalDeviceScore = ratePhysicalDevice(physicalDevice);
			physicalDevicesAvailable.insert(std::make_pair(physicalDeviceScore, physicalDevice));
			std::cout << physicalDeviceProperties.deviceID << '\t' << physicalDeviceProperties.deviceType << '\t' << physicalDeviceProperties.deviceName << '\t' << physicalDeviceProperties.driverVersion << '\t';
			std::cout << "Physical device score: " << physicalDeviceScore << std::endl;
		}
		std::cout << std::endl;

		if (physicalDevicesAvailable.rbegin()->first > 0) {
			physicalDevice = physicalDevicesAvailable.rbegin()->second;
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
			std::cout << "Selected physical device ID: " << physicalDeviceProperties.deviceID << "\n\n";
		}
		else throw std::runtime_error("Failed to find a suitable GPU");

	}

	// Create logical interface to physical device
	void createLogicalDevice() {

		std::cout << "CREATING LOGICAL DEVICE\n" << std::endl;

		// Specify queues to be created
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;

		for (int queueFamily : uniqueQueueFamilies) {

			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Specify the set of device features to be used
		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

		// Logical device creation
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &physicalDeviceFeatures;

		// Device specific extensions to be used
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();

		// Device specific validation layers
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(requestedLayers.size());
			createInfo.ppEnabledLayerNames = requestedLayers.data();
		}
		else createInfo.enabledLayerCount = 0;

		// Instantiate logical device
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) throw std::runtime_error("Failed to create logical device");

		// Get graphics queue from the logical device
		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	}

	// Create window surface
	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) throw std::runtime_error("Failed to create window surface.");
	}

	// Choose swap chain surface format
	VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats) {

		// Preferred format
		VkFormat bestFormat = VK_FORMAT_B8G8R8_UNORM;
		VkColorSpaceKHR bestColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		// If the surface has no preferred format
		if (availableSurfaceFormats.size() == 1 && availableSurfaceFormats[0].format == VK_FORMAT_UNDEFINED) return { bestFormat, bestColorSpace };

		// Check our preferred format is available
		for (const auto& availableSurfaceFormat : availableSurfaceFormats) {
			if (availableSurfaceFormat.format == bestFormat && availableSurfaceFormat.colorSpace == bestColorSpace) {
				return availableSurfaceFormat;
			}
		}

		// Or else, take the first format available
		return availableSurfaceFormats[0];

	}

	// Choose swap chain presentation mode
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {

		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return availablePresentMode;
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) bestMode = availablePresentMode;
		}

		return bestMode;
	}

	// Chosse swap extent
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = { WIN_WIDTH, WIN_HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	// Create window
	void initWindow() {

		std::cout << "INITIALIZING GLFW\n" << std::endl;

		// Initialize the GLFW library
		glfwInit();

		std::cout << "CREATING WINDOW\n" << std::endl;

		// Tell GLFW not to use OpenGL
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// No resizable windows for now
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		// Create window
		window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, APP_NAME, nullptr, nullptr);
	}

	// Initiate all Vulkan objects
	void initVulkan() {

		// Create Vulkan instance
		createInstance();
		// Setup debug callback
		setupDebugCallback();
		// Create window surface
		createSurface();
		// Selects GPUs
		pickPhysicalDevice();
		// Create logical interface to physical device
		createLogicalDevice();

	}

	// Main loop iterates until window is closed
	void mainLoop() {

		std::cout << "STARTING MAIN LOOP\n" << std::endl;

		while (!glfwWindowShouldClose(window)) {
			// Check for events (errors, inputs, ...)
			glfwPollEvents();
		}
	}

	// Deallocate the resources
	void cleanup() {
		std::cout << "DEALLOCATING RESOURCES\n" << std::endl;
		vkDestroyDevice(device, nullptr);
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

};

int main() {

	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	system("pause");

	return EXIT_SUCCESS;
}