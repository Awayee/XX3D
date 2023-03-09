#include "RHIVulkan.h"
#include "VulkanUtil.h"
#include <GLFW/glfw3.h>
#ifdef USE_VMA
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#else
#include <vulkan/vulkan.h>
#endif
#include "Core/Public/SmartPointer.h"

namespace RHI {

#define RETURN_RHI_PTR(cls, hd)\
	cls##Vk* cls##Ptr = new cls##Vk;\
	cls##Ptr->handle = hd; \
	return reinterpret_cast<cls*>(cls##Ptr)

#pragma region rhi initialzie

	TVector<const char*> RHIVulkan::GetRequiredExtensions()
	{
		uint32     glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		TVector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (m_EnableValidationLayers || m_EnableDebugUtils)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}
	void RHIVulkan::CreateInstance()
	{
		ASSERT(!m_EnableValidationLayers || CheckValidationLayerSupport(m_ValidationLayers), 
			"validation layers requested, but not available!");
		// app info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = m_AppName.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = m_AppName.c_str();
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VER;

		// create info
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo; // the appInfo is stored here

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if(m_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
			SetDebugInfo(debugCreateInfo);
			debugCreateInfo.pUserData = (void*)this;
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// create m_vulkan_context._instance
		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance), "");
	}
	void RHIVulkan::InitializeDebugMessenger()
	{
		if(m_EnableValidationLayers) {
			VkDebugUtilsMessengerCreateInfoEXT createInfo{};
			SetDebugInfo(createInfo);
			createInfo.pUserData = (void*)this;
			VK_CHECK(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger), "vk create debug messenger!");
		}
	}
	void RHIVulkan::CreateWindowSurface()
	{
		VK_CHECK(glfwCreateWindowSurface(m_Instance, reinterpret_cast<GLFWwindow*>(m_WindowHandle), nullptr, &m_Surface), "vk create window surface");
	}

	void RHIVulkan::InitializePhysicalDevice()
	{
		uint32 deviceCount = 0;
		VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr), "no avaliable physical device");
		TVector<VkPhysicalDevice> devices(deviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()), "no avaliable physical device");
		TVector<TPair<VkPhysicalDevice, SPhyicalDeviceInfo>> devicesInfo;
		for (auto& device : devices) {
			auto deviceInfo = GetPhysicalDeviceInfo(device, m_Surface, m_DeviceExtensions);
			if(deviceInfo.score > 0) {
				devicesInfo.push_back({device, std::move(deviceInfo)});
			}
		}

		ASSERT(!devicesInfo.empty(), "no avaliable GPU!");

		TSort(devicesInfo.begin(), devicesInfo.end(),
			[](const TPair<VkPhysicalDevice, SPhyicalDeviceInfo> p1, const TPair<VkPhysicalDevice, SPhyicalDeviceInfo> p2) {return p1.second.score > p2.second.score; });

		m_PhysicalDevice = devicesInfo[0].first;
		auto& deviceInfo = devicesInfo[0].second;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
		LOG("Picked GPU: %s %u", m_PhysicalDeviceProperties.deviceName, m_PhysicalDeviceProperties.deviceID);

		// record device info
		m_GraphicsIndex = static_cast<uint32>(deviceInfo.graphicsIndex);
		m_PresentIndex = static_cast<uint32>(deviceInfo.presentIndex);
		m_ComputeIndex = static_cast<uint32>(deviceInfo.computeIndex);
		m_ImageCount = deviceInfo.imageCount;
		m_SwapchainFormat = deviceInfo.swapchainFormat;
		m_SwapchainPresentMode = deviceInfo.swapchainPresentMode;
		m_SwapchainTransform = deviceInfo.swapchainTransform;
		m_SwapchainExtent.w = deviceInfo.swapchainExtent.width;
		m_SwapchainExtent.h = deviceInfo.swapchainExtent.height;
	}

	void RHIVulkan::CreateLogicalDevice()
	{
		TUnorderedSet<uint32> queueFamilies{m_GraphicsIndex, m_PresentIndex, m_ComputeIndex};
		TVector<VkDeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(queueFamilies.size());
		float queuePriority = 1.0f;
		for(uint32 queueFamily: queueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(std::move(queueCreateInfo));
		}

		VkPhysicalDeviceFeatures features{};
		features.samplerAnisotropy = VK_TRUE;
		features.fragmentStoresAndAtomics = VK_TRUE;
		features.independentBlend = VK_TRUE;
		features.geometryShader = m_EnableGeometryShader;

		VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
		deviceCreateInfo.pEnabledFeatures = &features;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32>(m_DeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
		deviceCreateInfo.enabledLayerCount = 0;

		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device), "vkCreateDevice");

		// initialize queues of this device
		vkGetDeviceQueue(m_Device, m_GraphicsIndex, 0, &m_GraphicsQueue.handle);
		vkGetDeviceQueue(m_Device, m_ComputeIndex, 0, &m_ComputeQueue.handle);
		vkGetDeviceQueue(m_Device, m_PresentIndex, 0, &m_PresentQueue.handle);

		m_DepthFormat = FindDepthFormat(m_PhysicalDevice);
	}


	void RHIVulkan::LoadDeviceFunctions()
	{
		// more efficient pointer
		const auto& device = m_Device;
		GET_DEVICE_FUNC(vkWaitForFences             );
		GET_DEVICE_FUNC(vkResetFences               );
		GET_DEVICE_FUNC(vkResetCommandPool          );
		GET_DEVICE_FUNC(vkBeginCommandBuffer        );
		GET_DEVICE_FUNC(vkEndCommandBuffer          );
		GET_DEVICE_FUNC(vkCmdBeginRenderPass        );
		GET_DEVICE_FUNC(vkCmdCopyBufferToImage      );
		GET_DEVICE_FUNC(vkCmdBlitImage              );
		GET_DEVICE_FUNC(vkCmdNextSubpass            );
		GET_DEVICE_FUNC(vkCmdEndRenderPass          );
		GET_DEVICE_FUNC(vkCmdBindPipeline           );
		GET_DEVICE_FUNC(vkCmdSetViewport            );
		GET_DEVICE_FUNC(vkCmdSetScissor             );
		GET_DEVICE_FUNC(vkCmdBindVertexBuffers      );
		GET_DEVICE_FUNC(vkCmdBindIndexBuffer        );
		GET_DEVICE_FUNC(vkCmdBindDescriptorSets     );
		GET_DEVICE_FUNC(vkCmdDrawIndexed            );
		GET_DEVICE_FUNC(vkCmdClearAttachments       );
		GET_DEVICE_FUNC(vkCmdDraw                   );
		GET_DEVICE_FUNC(vkCmdDispatch               );
		GET_DEVICE_FUNC(vkCmdCopyBuffer             );

		if (m_EnableDebugUtils) {
			GET_DEVICE_FUNC(vkCmdBeginDebugUtilsLabelEXT);
			GET_DEVICE_FUNC(vkCmdEndDebugUtilsLabelEXT);
		}

	}

	void RHIVulkan::CreateCommandPools()
	{
		// graphics command pool
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo{};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.pNext = NULL;
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			commandPoolCreateInfo.queueFamilyIndex = m_GraphicsIndex;
			VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_RHICommandPool), "VkCreateCommandPool");
		}

		// other command pool
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.pNext = NULL;
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			commandPoolCreateInfo.queueFamilyIndex = m_GraphicsIndex;
			m_CommandPools.resize(m_MaxFramesInFlight);
			for (uint32 i = 0; i < m_MaxFramesInFlight; ++i) {
				VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPools[i]), "VkCreateCommandPool");
			}
		}
	}

	void RHIVulkan::CreateDescriptorPool()
	{
		// Since DescriptorSet should be treated as asset in Vulkan, DescriptorPool
		// should be big enough, and thus we can sub-allocate DescriptorSet from
		// DescriptorPool merely as we sub-allocate Buffer/Image from DeviceMemory.

		uint32 maxVertexBlendingMeshCount{ 256 };
		uint32 maxMaterialCount{ 256 };

		uint32 poolCount = 7;
		TArray<VkDescriptorPoolSize> poolSizes(poolCount);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		poolSizes[0].descriptorCount = 3 + 2 + 2 + 2 + 1 + 1 + 3 + 3;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[1].descriptorCount = 1 + 1 + 1 * maxVertexBlendingMeshCount;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[2].descriptorCount = 1 * maxMaterialCount;
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = 3 + 5 * maxMaterialCount + 1 + 1; // ImGui_ImplVulkan_CreateDeviceObjects
		poolSizes[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSizes[4].descriptorCount = 4 + 1 + 1 + 2;
		poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[5].descriptorCount = 3;
		poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[6].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolCount;
		poolInfo.pPoolSizes = poolSizes.Data();
		poolInfo.maxSets = 1 + 1 + 1 + maxMaterialCount + maxVertexBlendingMeshCount + 1 + 1; // +skybox + axis descriptor set
		poolInfo.flags = 0U;
		VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool), "VkCreateDescriptorPool");
	}

	void RHIVulkan::CreateSyncResources()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // the fence is initialized as signaled

		m_ImageAvaliableSemaphores.resize(m_MaxFramesInFlight);
		m_PresentationFinishSemaphores.resize(m_MaxFramesInFlight);
		m_ImageAvaliableForTextureCopySemaphores.resize(m_MaxFramesInFlight);
		m_IsFrameInFlightFences.resize(m_MaxFramesInFlight);
		for(uint32 i=0; i< m_MaxFramesInFlight; ++i) {
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvaliableSemaphores[i]), "ImageAvaliableSemaphore");
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_PresentationFinishSemaphores[i]), "PresentationFinishSemaphore");
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvaliableForTextureCopySemaphores[i]), "ImageAvaliableForTextureCopySemaphore");
			VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_IsFrameInFlightFences[i]), "vkCreateFence");
		}
	}
	void RHIVulkan::CreateMemoryAllocator()
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.vulkanApiVersion = VK_API_VER;
		allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
		allocatorCreateInfo.device = m_Device;
		allocatorCreateInfo.instance = m_Instance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
		vmaCreateAllocator(&allocatorCreateInfo, &m_Vma);
	}
	void RHIVulkan::CreateSwapchain()
	{
		VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		createInfo.surface = m_Surface;
		createInfo.minImageCount = m_ImageCount;
		createInfo.imageFormat = m_SwapchainFormat.format;
		createInfo.imageColorSpace = m_SwapchainFormat.colorSpace;
		createInfo.imageExtent.width = m_SwapchainExtent.w;
		createInfo.imageExtent.height = m_SwapchainExtent.h;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		uint32 queueFamilyIndices[] = { m_GraphicsIndex, m_PresentIndex};

		if (queueFamilyIndices[0] != queueFamilyIndices[1])
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		createInfo.preTransform = m_SwapchainTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = m_SwapchainPresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		VK_CHECK(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain), "CreateSwapchain");
		// swapchain images
		uint32 image_count;
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &image_count, nullptr);
		m_SwapchainImages.resize(image_count);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &image_count, m_SwapchainImages.data());
		m_Scissor = { {0, 0,}, {m_SwapchainExtent.w, m_SwapchainExtent.h} };
		// Create image views
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		// create imageview (one for each this time) for all swapchain images
		for (uint64 i = 0; i < m_SwapchainImages.size(); i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			imageViewCreateInfo.image = m_SwapchainImages[i];
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = m_SwapchainFormat.format;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			VK_CHECK(vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &m_SwapchainImageViews[i].handle), "CreateImageView");
		}
	}
	void RHIVulkan::ClearSwapchain()
	{
		for(auto imageView: m_SwapchainImageViews) {
			vkDestroyImageView(m_Device, imageView.handle, nullptr);
		}
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		m_Swapchain = nullptr;
	}

	void RHIVulkan::Initialize(const RSInitInfo* initInfo)
	{
		if(m_Initialized) {
			return;
		}

		m_EnableValidationLayers = initInfo->enableDebug;
		m_EnableDebugUtils = initInfo->enableDebug;
		m_EnableGeometryShader = initInfo->enableGeometryShader;
		m_AppName = initInfo->applicationName;
		m_WindowHandle = initInfo->windowHandle;
		m_MaxFramesInFlight = initInfo->maxFramesInFlight;

		CreateInstance();
		InitializeDebugMessenger();
		CreateWindowSurface();
		InitializePhysicalDevice();
		CreateLogicalDevice();
		LoadDeviceFunctions();
		CreateCommandPools();
		CreateDescriptorPool();
		CreateSyncResources();
		CreateSwapchain();
		CreateMemoryAllocator();

		m_Initialized = true;

		LOG("RHI: Vulkan initialized successfully!");
	}

	void RHIVulkan::Release()
	{
		//vma
		vmaDestroyAllocator(m_Vma);
		m_Vma = nullptr;
		//sync
		for (auto smp : m_ImageAvaliableSemaphores) {
			vkDestroySemaphore(m_Device, smp, nullptr);
		}
		m_ImageAvaliableSemaphores.clear();
		for (auto smp : m_PresentationFinishSemaphores) {
			vkDestroySemaphore(m_Device, smp, nullptr);
		}
		m_PresentationFinishSemaphores.clear();
		for (auto smp : m_ImageAvaliableForTextureCopySemaphores) {
			vkDestroySemaphore(m_Device, smp, nullptr);
		}
		m_ImageAvaliableForTextureCopySemaphores.clear();
		for (auto fence : m_IsFrameInFlightFences) {
			vkDestroyFence(m_Device, fence, nullptr);
		}
		m_IsFrameInFlightFences.clear();
		for (auto pool : m_CommandPools) {
			vkDestroyCommandPool(m_Device, pool, nullptr);
		}
		m_CommandPools.clear();
		vkDestroyCommandPool(m_Device, m_RHICommandPool, nullptr);
		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		for (auto imageView : m_SwapchainImageViews) {
			vkDestroyImageView(m_Device, imageView.handle, nullptr);
		}
		m_SwapchainImageViews.clear();
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		m_Swapchain = nullptr;
		vkDestroyDevice(m_Device, nullptr);
		m_Device = nullptr;
		m_Initialized = false;
	}

	RSVkImGuiInitInfo RHIVulkan::GetImGuiInitInfo()
	{
		RSVkImGuiInitInfo info;
		info.windowHandle = m_WindowHandle;
		info.instance = m_Instance;
		info.device = m_Device;
		info.physicalDevice = m_PhysicalDevice;
		info.queueIndex = m_GraphicsIndex;
		info.queue = m_GraphicsQueue.handle;
		info.descriptorPool = m_DescriptorPool;
		return info;
	}

#pragma endregion

	RRenderPass* RHIVulkan::CreateRenderPass(uint32 attachmentCount, const RSAttachment* pAttachments, uint32 subpassCount, const RSubPassInfo* pSubpasses, uint32 dependencyCount, const RSubpassDependency* pDependencies)
	{
		uint32 i;
		TArray<VkAttachmentDescription> attachmentsVk(attachmentCount);
		for(i=0; i<attachmentCount; ++i) {
			attachmentsVk[i] = ResolveAttachmentDesc(pAttachments[i]);
		}

		TArray<VkSubpassDescription> subpassesVk(subpassCount);
		TArray<TVector<VkAttachmentReference>> inputAttachments(subpassCount);
		TArray<TVector<VkAttachmentReference>> colorAttachments(subpassCount);
		TArray<VkAttachmentReference> depthAttachments(subpassCount);

		for(i=0; i<subpassCount; ++i) {
			subpassesVk[i].flags = 0;
			subpassesVk[i].pipelineBindPoint = (VkPipelineBindPoint)pSubpasses[i].Type;
			uint32 j;
			for (j = 0; j < pSubpasses[i].InputAttachments.size(); ++j) {
				inputAttachments[i].push_back({ pSubpasses[i].InputAttachments[j].Index, (VkImageLayout)pSubpasses[i].InputAttachments[j].Layout});
			}
			subpassesVk[i].inputAttachmentCount = inputAttachments[i].size();
			subpassesVk[i].pInputAttachments = inputAttachments[i].data();
			for (j = 0; j < pSubpasses[i].ColorAttachments.size(); ++j) {
				colorAttachments[i].push_back({ pSubpasses[i].ColorAttachments[j].Index, (VkImageLayout)pSubpasses[i].ColorAttachments[j].Layout});
			}
			subpassesVk[i].colorAttachmentCount = colorAttachments[i].size();
			subpassesVk[i].pColorAttachments = colorAttachments[i].data();
			if(IMAGE_LAYOUT_UNDEFINED != pSubpasses[i].DepthStencilAttachment.Layout) {
				depthAttachments[i] = { pSubpasses[i].DepthStencilAttachment.Index, (VkImageLayout)pSubpasses[i].DepthStencilAttachment.Layout};
				subpassesVk[i].pDepthStencilAttachment = &depthAttachments[i];
			}
			else {
				subpassesVk[i].pDepthStencilAttachment = VK_NULL_HANDLE;
			}
			// empty resolve
			subpassesVk[i].preserveAttachmentCount = 0;
			subpassesVk[i].pPreserveAttachments = nullptr;
			subpassesVk[i].pResolveAttachments = nullptr;
		}

		TArray<VkSubpassDependency> dependenciesVk(dependencyCount);
		for(i=0; i<dependencyCount;++i) {
			dependenciesVk[i] = ResolveSubpassDependency(pDependencies[i]);
		}

		VkRenderPassCreateInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr };
		info.attachmentCount = attachmentCount;
		info.pAttachments = attachmentsVk.Data();
		info.subpassCount = subpassCount;
		info.pSubpasses = subpassesVk.Data();
		info.dependencyCount = dependencyCount;
		info.pDependencies = dependenciesVk.Data();
		VkRenderPass handle;
		if(VK_SUCCESS!=vkCreateRenderPass(m_Device, &info, nullptr, &handle)){
			return nullptr;
		}
		RRenderPassVk* pass = new RRenderPassVk;
		pass->handle = handle;
		pass->m_Clears.resize(attachmentCount);
		for(i=0; i< attachmentCount; ++i) {
			pass->m_Clears[i] = ResolveClearValue(pAttachments[i].Clear);
		}
		return pass;
	}

	RRenderPass* RHIVulkan::CreateRenderPass(uint32 colorAttachmentCount, const RSAttachment* pColorAttachments, const RSAttachment* depthAttachment)
	{
		uint32 i;
		uint32 attachmentCount = colorAttachmentCount + (nullptr != depthAttachment);
		TArray<VkAttachmentDescription> attachmentsVk(attachmentCount);
		TArray<VkAttachmentReference> colorAttachmentRef(colorAttachmentCount);
		for (i = 0; i < colorAttachmentCount; ++i) {
			attachmentsVk[i] = ResolveAttachmentDesc(pColorAttachments[i]);
			colorAttachmentRef[i] = { i, (VkImageLayout)pColorAttachments[i].FinalLayout };
		}
		VkAttachmentReference depthAttachmentRef;
		if(nullptr != depthAttachment) {
			depthAttachmentRef.attachment = colorAttachmentCount;
			depthAttachmentRef.layout = (VkImageLayout)depthAttachment->FinalLayout;
			attachmentsVk[colorAttachmentCount] = ResolveAttachmentDesc(*depthAttachment);
		}
		VkSubpassDescription subpass;
		subpass.flags = 0;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = colorAttachmentCount;
		subpass.pColorAttachments = colorAttachmentRef.Data();
		subpass.pResolveAttachments = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;
		subpass.pDepthStencilAttachment = (nullptr != depthAttachment) ? &depthAttachmentRef : nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr };
		info.attachmentCount = attachmentCount;
		info.pAttachments = attachmentsVk.Data();
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;

		VkRenderPass handle;
		if (VK_SUCCESS != vkCreateRenderPass(m_Device, &info, nullptr, &handle)) {
			return nullptr;
		}
		RRenderPassVk* pass = new RRenderPassVk;
		pass->handle = handle;
		return reinterpret_cast<RRenderPass*>(pass);

	}

	void RHIVulkan::DestroyRenderPass(RRenderPass* pass)
	{
		RRenderPassVk* vkPass = static_cast<RRenderPassVk*>(pass);
		vkDestroyRenderPass(m_Device, vkPass->handle, nullptr);
		delete vkPass;
	}
	RDescriptorSetLayout* RHIVulkan::CreateDescriptorSetLayout(uint32 bindingCount, const RSDescriptorSetLayoutBinding* bindings)
	{
		VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.pNext = nullptr;
		info.flags = 0;
		info.bindingCount = bindingCount;
		TArray<VkDescriptorSetLayoutBinding> bindingsVk(bindingCount);
		for(uint32 i=0; i< bindingCount; ++i) {
			bindingsVk[i].binding = i;
			bindingsVk[i].descriptorType = (VkDescriptorType)bindings[i].descriptorType;
			bindingsVk[i].descriptorCount = bindings[i].descriptorCount;
			bindingsVk[i].stageFlags = (VkShaderStageFlags)bindings[i].stageFlags;
			bindingsVk[i].pImmutableSamplers = nullptr;
		}
		info.pBindings = bindingsVk.Data();
		VkDescriptorSetLayout handle;
		if(VK_SUCCESS != vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &handle)) {
			return nullptr;
		}
		RDescriptorSetLayoutVk* descriptorSetLayout = new RDescriptorSetLayoutVk;
		descriptorSetLayout->handle = handle;
		return descriptorSetLayout;

	}

	void RHIVulkan::DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(m_Device, ((RDescriptorSetLayoutVk*)descriptorSetLayout)->handle, nullptr);
	}

	RDescriptorSet* RHIVulkan::AllocateDescriptorSet(const RDescriptorSetLayout* layout)
	{
		VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		info.pNext = nullptr;
		info.descriptorPool = m_DescriptorPool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = &((RDescriptorSetLayoutVk*)layout)->handle;
		VkDescriptorSet handle;
		if (VK_SUCCESS != vkAllocateDescriptorSets(m_Device, &info, &handle)) {
			return nullptr;
		}
		RDescriptorSetVk* descriptorSet = new RDescriptorSetVk;
		descriptorSet->handle = handle;
		descriptorSet->m_Device = m_Device;
		return descriptorSet;
	}

	void RHIVulkan::FreeDescriptorSet(RDescriptorSet* descriptorSet)
	{
		RDescriptorSetVk* descirptorSetVk = (RDescriptorSetVk*)descriptorSet;
		vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &descirptorSetVk->handle);
		delete descirptorSetVk;
	}

	/*
	void RHIVulkan::AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet* const* descriptorSets)
	{
		TArray<VkDescriptorSetLayout> layoutsVk(count);
		for(uint32 i=0; i< count; ++i) {
			layoutsVk[i] = ((RDescriptorSetLayoutVk*)layouts[i])->handle;
		}
		VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		info.pNext = nullptr;
		info.descriptorPool = m_DescriptorPool;
		info.descriptorSetCount = count;
		info.pSetLayouts = layoutsVk.Data();
		TArray<VkDescriptorSet> descriptorSetsVk(count);
		if(VK_SUCCESS != vkAllocateDescriptorSets(m_Device, &info, descriptorSetsVk.Data())) {
			return;
		}
		for(uint32 i=0; i< count; ++i) {
			RDescriptorSetVk* descriptorSetVk = (RDescriptorSetVk*)descriptorSets[i];
			descriptorSetVk->handle = descriptorSetsVk[i];
		}
	}
	void RHIVulkan::FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets)
	{
		TArray<VkDescriptorSet> descriptorSetsVk(count);
		for(uint32 i=0; i<count; ++i) {
			descriptorSetsVk[i] = ((RDescriptorSetVk*)descriptorSets[i])->handle;
			delete descriptorSets[i];
		}
		vkFreeDescriptorSets(m_Device, m_DescriptorPool, count, descriptorSetsVk.Data());
	}
	*/
	
	RPipelineLayout* RHIVulkan::CreatePipelineLayout(uint32 setLayoutCount, const RDescriptorSetLayout* const* pSetLayouts, uint32 pushConstantRangeCount, const RSPushConstantRange* pPushConstantRanges)
	{
		uint32 i;
		VkPipelineLayoutCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		info.flags = 0;
		info.pNext = nullptr;
		info.setLayoutCount = setLayoutCount;
		TArray<VkDescriptorSetLayout> setLayoutsVk(setLayoutCount);
		for (i = 0; i < setLayoutCount; ++i) {
			setLayoutsVk[i] = ((RDescriptorSetLayoutVk*)pSetLayouts[i])->handle;
		}
		info.pSetLayouts = setLayoutsVk.Data();
		
		info.pushConstantRangeCount = pushConstantRangeCount;
		if(pushConstantRangeCount > 0) {
			TArray<VkPushConstantRange> pushConstantRangesVk(pushConstantRangeCount);
			for(i=0; i< pushConstantRangeCount; ++i) {
				pushConstantRangesVk[i].offset = pPushConstantRanges[i].offset;
				pushConstantRangesVk[i].size = pPushConstantRanges[i].size;
				pushConstantRangesVk[i].stageFlags = pPushConstantRanges[i].stageFlags;
			}
			info.pPushConstantRanges = pushConstantRangesVk.Data();
		}

		VkPipelineLayout handle;
		if(VK_SUCCESS != vkCreatePipelineLayout(m_Device, &info, nullptr, &handle)) {
			return nullptr;
		}
		RPipelineLayoutVk* pipelineLayout = new RPipelineLayoutVk;
		pipelineLayout->handle = handle;
		return pipelineLayout;
	}

	void RHIVulkan::DestroyPipelineLayout(RPipelineLayout* pipelineLayout)
	{
		RPipelineLayoutVk* pipelineLayoutVk = (RPipelineLayoutVk*)pipelineLayout;
		vkDestroyPipelineLayout(m_Device, pipelineLayoutVk->handle, nullptr);
		delete pipelineLayoutVk;
	}

	RPipeline* RHIVulkan::CreateGraphicsPipeline(const RGraphicsPipelineCreateInfo& info, RPipelineLayout* layout, RRenderPass* renderPass, uint32 subpass, RPipeline* basePipeline, int32_t basePipelineIndex)
	{
		uint32 i; // iter
		// shader stages
		TArray<VkShaderModuleCreateInfo> shaderModuleInfos(info.Shaders.size());
		TArray<VkShaderModule> shaderModules(info.Shaders.size());
		TArray<VkPipelineShaderStageCreateInfo> shaderInfos(info.Shaders.size());
		for (i = 0; i < info.Shaders.size(); ++i) {
			shaderModuleInfos[i] = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0 };
			shaderModuleInfos[i].codeSize = info.Shaders[i].code.size();
			shaderModuleInfos[i].pCode = reinterpret_cast<const uint32*>(info.Shaders[i].code.data());
			vkCreateShaderModule(m_Device, &shaderModuleInfos[i], nullptr, &shaderModules[i]);

			shaderInfos[i] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0 };
			shaderInfos[i].module = shaderModules[i];
			shaderInfos[i].pName = info.Shaders[i].funcName;
			shaderInfos[i].stage = (VkShaderStageFlagBits)info.Shaders[i].stage;
		}

		// vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0 };
		TArray<VkVertexInputBindingDescription> vertexInputBindings(info.Bindings.size());
		for(i=0; i< info.Bindings.size(); ++i) {
			vertexInputBindings[i].binding = info.Bindings[i].binding;
			vertexInputBindings[i].stride = info.Bindings[i].stride;
			vertexInputBindings[i].inputRate = (VkVertexInputRate)info.Bindings[i].inputRate;
		}
		vertexInputInfo.vertexBindingDescriptionCount = info.Bindings.size();
		vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.Data();
		TArray<VkVertexInputAttributeDescription> vertexInputAttrs(info.Attributes.size());
		for(i=0; i< info.Attributes.size(); ++i) {
			vertexInputAttrs[i].location = i;
			vertexInputAttrs[i].binding = info.Attributes[i].binding;
			vertexInputAttrs[i].format = (VkFormat)info.Attributes[i].format;
			vertexInputAttrs[i].offset = info.Attributes[i].offset;
		}
		vertexInputInfo.vertexAttributeDescriptionCount = info.Attributes.size();
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttrs.Data();

		// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0 };
		inputAssemblyInfo.primitiveRestartEnable = info.PrimitiveRestartEnable;
		inputAssemblyInfo.topology = (VkPrimitiveTopology)info.Topology;

		// tessellation
		VkPipelineTessellationStateCreateInfo tessellationInfo{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, nullptr, 0 };
		tessellationInfo.patchControlPoints = info.PatchControlPoints;

		// viewport
		VkPipelineViewportStateCreateInfo viewportInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0 };
		auto& viewport = info.Viewport;
		VkViewport viewportVk{ viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth };
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewportVk;
		auto& scissor = info.Scissor;
		VkRect2D scissorVk{ {(int32)scissor.x, (int32)scissor.y}, {scissor.w, scissor.h} };
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissorVk;

		// rasterization
		VkPipelineRasterizationStateCreateInfo rasterizationInfo = TranslateVkPipelineRasterizationState(info);
		// multi sample
		VkPipelineMultisampleStateCreateInfo multisampleInfo = TranslatePipelineMultisample(info);
		// depth stencil
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo = TranslatePipelineDepthStencil(info);

		// color blend
		TArray<VkPipelineColorBlendAttachmentState> attachments(info.AttachmentStates.size());
		for(i=0; i<info.AttachmentStates.size(); ++i) {
			TranslateColorBlendAttachmentState(attachments[i], info.AttachmentStates[i]);
		}
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0 };
		colorBlendInfo.logicOpEnable = info.LogicOpEnable;
		colorBlendInfo.logicOp = (VkLogicOp)info.LogicOp;
		colorBlendInfo.attachmentCount = info.AttachmentStates.size();
		colorBlendInfo.pAttachments = attachments.Data();
		colorBlendInfo.blendConstants[0] = info.BlendConstants[0];
		colorBlendInfo.blendConstants[1] = info.BlendConstants[1];
		colorBlendInfo.blendConstants[2] = info.BlendConstants[2];
		colorBlendInfo.blendConstants[3] = info.BlendConstants[3];

		// dynamic
		VkPipelineDynamicStateCreateInfo dynamicInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0 };
		dynamicInfo.dynamicStateCount = info.DynamicStates.size();
		dynamicInfo.pDynamicStates = (const VkDynamicState*)info.DynamicStates.data();

		// pipeline
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0 };
		createInfo.stageCount = info.Shaders.size();
		createInfo.pStages = shaderInfos.Data();
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssemblyInfo;
		createInfo.pTessellationState = &tessellationInfo;
		createInfo.pViewportState = &viewportInfo;
		createInfo.pRasterizationState = &rasterizationInfo;
		createInfo.pMultisampleState = &multisampleInfo;
		createInfo.pDepthStencilState = &depthStencilInfo;
		createInfo.pColorBlendState = &colorBlendInfo;
		createInfo.pDynamicState = &dynamicInfo;
		createInfo.layout = ((RPipelineLayoutVk*)layout)->handle;
		createInfo.renderPass = ((RRenderPassVk*)renderPass)->handle;
		createInfo.subpass = subpass;
		createInfo.basePipelineHandle = nullptr == basePipeline ? VK_NULL_HANDLE : ((RPipelineVk*)basePipeline)->handle;
		createInfo.basePipelineIndex = basePipelineIndex;

		VkPipeline handle;
		VkResult res = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		for(i=0; i< info.Shaders.size(); ++i) {
			vkDestroyShaderModule(m_Device, shaderModules[i], nullptr);
		}
		if(VK_SUCCESS != res) {
			return nullptr;
		}
		RPipelineVk* pipeline = new RPipelineVk;
		pipeline->handle = handle;
		pipeline->m_Type = PIPELINE_GRAPHICS;
		return pipeline;
	}

	RPipeline* RHIVulkan::CreateComputePipeline(const RPipelineShaderInfo& shader, RPipelineLayout* layout, RPipeline* basePipeline, uint32 basePipelineIndex)
	{
		VkComputePipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };
		// shader stages
		VkShaderModuleCreateInfo shaderModuleInfo;
		VkShaderModule shaderModule;
		VkPipelineShaderStageCreateInfo& shaderInfo = createInfo.stage;
		shaderModuleInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0 };
		shaderModuleInfo.codeSize = shader.code.size();
		shaderModuleInfo.pCode = reinterpret_cast<const uint32*>(shader.code.data());
		vkCreateShaderModule(m_Device, &shaderModuleInfo, nullptr, &shaderModule);

		shaderInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0 };
		shaderInfo.module = shaderModule;
		shaderInfo.pName = shader.funcName;
		shaderInfo.stage = (VkShaderStageFlagBits)shader.stage;

		createInfo.layout = ((RPipelineLayoutVk*)layout)->handle;
		createInfo.basePipelineHandle = nullptr == basePipeline ? VK_NULL_HANDLE : ((RPipelineVk*)basePipeline)->handle;
		createInfo.basePipelineIndex = basePipelineIndex;

		VkPipeline handle;
		VkResult res = vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		vkDestroyShaderModule(m_Device, shaderModule, nullptr);
		if(VK_SUCCESS != res) {
			return nullptr;
		}
		RPipelineVk* pipeline = new RPipelineVk;
		pipeline->handle = handle;
		pipeline->m_Type = PIPELINE_COMPUTE;
		return pipeline;
	}

	void RHIVulkan::DestroyPipeline(RPipeline* pipeline)
	{
		RPipelineVk* pipelineVk = (RPipelineVk*)pipeline;
		vkDestroyPipeline(m_Device, pipelineVk->handle, nullptr);
		delete pipelineVk;
	}

	RQueue* RHIVulkan::GetGraphicsQueue()
	{
		return reinterpret_cast<RQueue*>(&m_GraphicsQueue);
	}

	void RHIVulkan::ResizeSwapchain(uint32 width, uint32 height)
	{
		if (VK_SUCCESS != _vkWaitForFences(m_Device, m_IsFrameInFlightFences.size(), m_IsFrameInFlightFences.data(), VK_TRUE, UINT64_MAX))
		{
			ERROR("_vkWaitForFences failed");
			return;
		}
		m_SwapchainExtent.w = static_cast<uint32>(width);
		m_SwapchainExtent.h = static_cast<uint32>(height);
		ClearSwapchain();
		CreateSwapchain();
	}

	void RHIVulkan::QueueSubmit(RQueue* queue,
		uint32 cmdCount, RCommandBuffer* cmds,
		uint32 waitSemaphoreCount, RSemaphore* waitSemaphores, RPipelineStageFlags* waitStageFlags,
		uint32 signalSemaphoreCount, RSemaphore* signalSemaphores,
		RFence* fence) {
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext = nullptr;
		uint32 i;

		// wait semaphores
		TArray<VkSemaphore> vkWaitSmps(waitSemaphoreCount);
		for (i = 0; i < waitSemaphoreCount; ++i) {
			vkWaitSmps[i] = reinterpret_cast<RSemaphoreVk*>(waitSemaphores + i)->handle;
		}
		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = vkWaitSmps.Data();
		submitInfo.pWaitDstStageMask = (VkPipelineStageFlags*)waitStageFlags;

		// commands
		TArray<VkCommandBuffer> vkCmds(cmdCount);
		for (i = 0; i < cmdCount; ++i) {
			vkCmds[i] = reinterpret_cast<RCommandBufferVk*>(cmds + i)->handle;
		}
		submitInfo.commandBufferCount = cmdCount;
		submitInfo.pCommandBuffers = vkCmds.Data();

		// signal semaphores
		TArray<VkSemaphore> vkSignalSmps(signalSemaphoreCount);
		for(i=0; i< signalSemaphoreCount; ++i) {
			vkSignalSmps[i] = reinterpret_cast<RSemaphoreVk*>(signalSemaphores + i)->handle;
		}
		submitInfo.signalSemaphoreCount = signalSemaphoreCount;
		submitInfo.pSignalSemaphores = vkSignalSmps.Data();

		// fence
		VkFence vkFence = (nullptr == fence) ? VK_NULL_HANDLE : reinterpret_cast<RFenceVk*>(fence)->handle;

		VK_CHECK(vkQueueSubmit(reinterpret_cast<RQueueVk*>(queue)->handle, 1, &submitInfo, vkFence), "vkQueueSubmit");
	}
	void RHIVulkan::QueueWaitIdle(RQueue* queue)
	{
		VK_CHECK(vkQueueWaitIdle(reinterpret_cast<RQueueVk*>(queue)->handle), "vkQueueWaitIdle");
	}

	void RHIVulkan::WaitGraphicsQueue()
	{
		VK_CHECK(vkQueueWaitIdle(m_GraphicsQueue.handle), "vkQueueWaitIdle");
	}

	RImageView* RHIVulkan::GetSwapchainImageView(uint8 i)
	{
		ASSERT(i < m_SwapchainImageViews.size(), "i < m_MaxFramesInFlight");
		return reinterpret_cast<RImageView*>(&m_SwapchainImageViews[i]);
	}
	uint32 RHIVulkan::GetSwapchainMaxImageCount()
	{
		return (uint32)m_SwapchainImages.size();
	}
	RFramebuffer* RHIVulkan::CreateFrameBuffer(RRenderPass* pass, uint32 attachmentCount, const RImageView* const* pAttachments, uint32 width, uint32 height, uint32 layers)
	{
		VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = reinterpret_cast<RRenderPassVk*>(pass)->handle;
		TArray<VkImageView> vkImageViews(attachmentCount);
		for(uint32 i=0; i< attachmentCount; ++i) {
			const RImageViewVk* imageViewVk = (const RImageViewVk*)(pAttachments[i]);
			vkImageViews[i] = imageViewVk->handle;
		}
		framebufferInfo.attachmentCount = attachmentCount;
		framebufferInfo.pAttachments = vkImageViews.Data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = layers;

		VkFramebuffer handle;
		if(VK_SUCCESS != vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &handle)) {
			return nullptr;
		}

		RFramebufferVk* framebuffer = new RFramebufferVk;
		framebuffer->handle = handle;
		framebuffer->m_Width = width;
		framebuffer->m_Height = height;
		return framebuffer;
	}

	void RHIVulkan::DestroyFramebuffer(RFramebuffer* framebuffer)
	{
		RFramebufferVk* framebufferVk = (RFramebufferVk*)framebuffer;
		vkDestroyFramebuffer(m_Device, framebufferVk->handle, nullptr);
		delete framebufferVk;
	}
	RCommandBuffer* RHIVulkan::AllocateCommandBuffer(RCommandBufferLevel level)
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_RHICommandPool;
		allocateInfo.level = (VkCommandBufferLevel)level;
		allocateInfo.commandBufferCount = 1;
		VkCommandBuffer handle;
		if(VK_SUCCESS != vkAllocateCommandBuffers(m_Device, &allocateInfo, &handle)){
			return nullptr;
		}
		RCommandBufferVk* cmd = new RCommandBufferVk;
		cmd->handle = handle;
		cmd->m_Pool = allocateInfo.commandPool;
		return cmd;
	}

	void RHIVulkan::FreeCommandBuffer(RCommandBuffer* cmd)
	{
		RCommandBufferVk* vkCmd = reinterpret_cast<RCommandBufferVk*>(cmd);
		vkFreeCommandBuffers(m_Device, vkCmd->m_Pool, 1, &vkCmd->handle);
		delete vkCmd;
	}

	void RHIVulkan::ImmediateCommit(const CommandBufferFunc& func)
	{
		RCommandBufferVk cmd;

		// allocate
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_RHICommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocateInfo, &cmd.handle), "Failed to allocate command buffers!");
		cmd.m_Pool = m_RHICommandPool;

		// begin
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;
		VK_CHECK(_vkBeginCommandBuffer(cmd.handle, &beginInfo), "Failed to begin command buffer!");

		// run
		func(&cmd);

		// end
		_vkEndCommandBuffer(cmd.handle);

		// submit
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd.handle;
		vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue.handle);

		// free
		vkFreeCommandBuffers(m_Device, cmd.m_Pool, 1, &cmd.handle);
	}

	int RHIVulkan::PreparePresent(uint8 frameIndex)
	{
		_vkWaitForFences(m_Device, 1, &m_IsFrameInFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
		VkResult res = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvaliableSemaphores[frameIndex], VK_NULL_HANDLE, &m_CurrentSwapchainImageIndex);
		if(VK_ERROR_OUT_OF_DATE_KHR == res){
			return -1;
		}
		else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			ERROR("failed to acquire swap chain image!");
		}


		VK_CHECK(_vkResetCommandPool(m_Device, m_CommandPools[frameIndex], 0), "Failed to reset command pool!");
		return static_cast<int>(m_CurrentSwapchainImageIndex);
	}
	int RHIVulkan::QueueSubmitPresent(RCommandBuffer* cmd, uint8 frameIndex)
	{
		VK_CHECK(_vkResetFences(m_Device, 1, &m_IsFrameInFlightFences[frameIndex]));

		// submit command buffer
		//VkSemaphore semaphores[2] = { m_ImageAvaliableForTextureCopySemaphores[m_CurrentFrame], m_PresentationFinishSemaphores[m_CurrentFrame] };
		VkSemaphore semaphores[1] = { m_PresentationFinishSemaphores[frameIndex] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkCommandBuffer vkCmd = reinterpret_cast<RCommandBufferVk*>(cmd)->handle;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_ImageAvaliableSemaphores[frameIndex];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vkCmd;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = semaphores;
		VK_CHECK(vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, m_IsFrameInFlightFences[frameIndex]), "Failed to submit graphics queue!");

		// present
		VkPresentInfoKHR presentQueue = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentQueue.waitSemaphoreCount = 1;
		presentQueue.pWaitSemaphores = &m_PresentationFinishSemaphores[frameIndex];
		presentQueue.swapchainCount = 1;
		presentQueue.pSwapchains = &m_Swapchain;
		presentQueue.pImageIndices = &m_CurrentSwapchainImageIndex;

		VkResult presentResult = vkQueuePresentKHR(m_PresentQueue.handle, &presentQueue);
		if (VK_ERROR_OUT_OF_DATE_KHR == presentResult || VK_SUBOPTIMAL_KHR == presentResult) {
			return -1;
		}
		return 0;
	}
	void RHIVulkan::FreeMemory(RMemory* memory)
	{
		vmaFreeMemory(m_Vma, reinterpret_cast<RMemoryVma*>(memory)->handle);
	}

	RBuffer* RHIVulkan::CreateBuffer(uint64 size, RBufferUsageFlags usage)
	{
		VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // not sharing among queue families

		VkBuffer handle;
		if (VK_SUCCESS != vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &handle)) {
			return nullptr;
		}
		RBufferVk* buffer = new RBufferVk;
		buffer->handle = handle;
		return buffer;
	}

	RMemory* RHIVulkan::CreateBufferMemory(RBuffer* buffer, RMemoryPropertyFlags memoryProperty, uint64 dataSize, void* pData)
	{
		VmaAllocationCreateInfo info;
		info.flags = 0;
		info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		info.requiredFlags = (VkMemoryPropertyFlags)memoryProperty;
		info.preferredFlags = (VkMemoryPropertyFlags)memoryProperty;
		info.memoryTypeBits = 0;
		info.pool = nullptr;
		info.priority = 1.0f;
		VmaAllocation allocHandle; 
		if(VK_SUCCESS != vmaAllocateMemoryForBuffer(m_Vma, ((RBufferVk*)buffer)->handle, &info, &allocHandle, nullptr)) {
			return nullptr;
		}
		if (0 != dataSize) {
			void* p;
			vmaMapMemory(m_Vma, allocHandle, &p);
			memcpy(p, pData, dataSize);
			vmaUnmapMemory(m_Vma, allocHandle);
		}
		RMemoryVma* memory = new RMemoryVma;
		memory->handle = allocHandle;
		return memory;
	}

	void RHIVulkan::CreateBufferWithMemory(uint64 size, RBufferUsageFlags usage, RMemoryPropertyFlags memoryFlags, RBuffer*& pBuffer, RMemory*& pMemory, uint64 dataSize, void* pData)
	{
		VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // not sharing among queue families

		VmaAllocationCreateInfo allocInfo;
		allocInfo.flags = 0;
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = (VkMemoryPropertyFlags)memoryFlags;
		allocInfo.preferredFlags = (VkMemoryPropertyFlags)memoryFlags;
		allocInfo.memoryTypeBits = 0;
		allocInfo.pool = nullptr;
		allocInfo.priority = 1.0f;
		VkBuffer bufferHandle;
		VmaAllocation allocHandle;
		if(VK_SUCCESS != vmaCreateBuffer(m_Vma, &bufferCreateInfo, &allocInfo, &bufferHandle, &allocHandle, nullptr)) {
			return;
		}

		if(nullptr != pData) {
			void* p;
			vmaMapMemory(m_Vma, allocHandle, &p);
			memcpy(p, pData, dataSize);
			vmaUnmapMemory(m_Vma, allocHandle);
		}

		RBufferVk* bufferVk = new RBufferVk;
		bufferVk->handle = bufferHandle;
		bufferVk->m_Size = size;
		RMemoryVma* memoryVk = new RMemoryVma;
		memoryVk->handle = allocHandle;

		pBuffer = bufferVk;
		pMemory = memoryVk;
	}

	void RHIVulkan::DestroyBuffer(RBuffer* buffer)
	{
		RBufferVk* bufferVk = (RBufferVk*)buffer;
		vkDestroyBuffer(m_Device, bufferVk->handle, nullptr);
		delete bufferVk;
	}

	void RHIVulkan::MapMemory(RMemory* memory, void** pData)
	{
		vmaMapMemory(m_Vma, ((RMemoryVma*)memory)->handle, pData);
	}

	void RHIVulkan::UnmapMemory(RMemory* memory)
	{
		vmaUnmapMemory(m_Vma, ((RMemoryVma*)memory)->handle);
	}

	RImageVk* RHIVulkan::CreateImage(RImageType type, RFormat format, USize3D&& extent, uint32 mipLevels, uint32 arrayLayers, RSampleCountFlags samples, RImageTiling tiling, RImageUsageFlags usage)
	{
		VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.pNext = nullptr;
		imageInfo.flags = 0;
		imageInfo.imageType = (VkImageType)type;
		imageInfo.format = (VkFormat)format;
		imageInfo.extent.width = extent.w; imageInfo.extent.height = extent.h; imageInfo.extent.depth = extent.d;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = arrayLayers;
		imageInfo.samples = (VkSampleCountFlagBits)samples;
		imageInfo.tiling = (VkImageTiling)tiling;
		imageInfo.usage = (VkImageUsageFlags)usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkImage handle;
		if(VK_SUCCESS != vkCreateImage(m_Device, &imageInfo, nullptr, &handle)) {
			return nullptr;
		}
		RImageVk* image = new RImageVk();
		image->handle = handle;
		image->m_Type = type;
		image->m_Extent = extent;
		image->m_Format = format;
		image->m_Layout = IMAGE_LAYOUT_UNDEFINED;
		return image;
	}

	RImage* RHIVulkan::CreateImage2D(RFormat format, uint32 width, uint32 height, uint32 mipLevels, RSampleCountFlagBits samples, RImageTiling tiling, RImageUsageFlags usage)
	{
		return CreateImage(IMAGE_TYPE_2D, format, {width, height, 1}, mipLevels, 1, samples, tiling, usage);
	}

	RMemory* RHIVulkan::CreateImageMemory(RImage* image, RMemoryPropertyFlags memoryProperty, void* pData)
	{
		VkImage imageVk = reinterpret_cast<RImageVk*>(image)->handle;
		VmaAllocationCreateInfo info;
		info.flags = 0;
		info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		info.requiredFlags = (VkMemoryPropertyFlags)memoryProperty;
		info.preferredFlags = (VkMemoryPropertyFlags)memoryProperty;
		info.memoryTypeBits = 0;
		info.pool = nullptr;
		info.pUserData = pData;
		info.priority = 1.0f;
		VmaAllocation handle;
		if (VK_SUCCESS != vmaAllocateMemoryForImage(m_Vma, imageVk, &info, &handle, nullptr) ||
			VK_SUCCESS != vmaBindImageMemory(m_Vma, handle, imageVk)) {
			return nullptr;
		}
		RMemoryVma* memory = new RMemoryVma;
		memory->handle = handle;
		return reinterpret_cast<RMemory*>(memory);
	}

	void RHIVulkan::DestroyImage(RImage* image)
	{
		RImageVk* imageVk = (RImageVk*)image;
		vkDestroyImage(m_Device, imageVk->handle, nullptr);
		delete imageVk;
	}

	RImageView* RHIVulkan::CreateImageView(RImage* image, RImageViewType viewType, RImageAspectFlags aspectMask,
		uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount)
	{
		RImageVk* imageVk = (RImageVk*)image;
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = imageVk->handle;
		imageViewCreateInfo.viewType = (VkImageViewType)viewType;
		imageViewCreateInfo.format = (VkFormat)imageVk->m_Format;
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
		imageViewCreateInfo.subresourceRange.levelCount = levelCount;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = baseLayer;
		imageViewCreateInfo.subresourceRange.layerCount = layerCount;
		VkImageView handle;
		if (VK_SUCCESS != vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &handle)) {
			return nullptr;
		}
		RDescriptorInfo desInfo;

		RImageViewVk* imageView = new RImageViewVk;
		imageView->handle = handle;
		imageView->m_Layout = image->GetLayout();
		return imageView;
	}

	void RHIVulkan::DestroyImageView(RImageView* imageView)
	{
		RImageViewVk* imageViewVk = (RImageViewVk*)imageView;
		vkDestroyImageView(m_Device, imageViewVk->handle, nullptr);
		delete imageViewVk;
	}

	RSampler* RHIVulkan::CreateSampler(const RSSamplerInfo& samplerInfo)
	{
		VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		info.pNext = nullptr;
		info.flags = 0;
		info.magFilter = (VkFilter)samplerInfo.magFilter;
		info.minFilter = (VkFilter)samplerInfo.minFilter;
		info.addressModeU = (VkSamplerAddressMode)samplerInfo.addressModeU;
		info.addressModeV = (VkSamplerAddressMode)samplerInfo.addressModeV;
		info.addressModeW = (VkSamplerAddressMode)samplerInfo.addressModeW;
		info.mipLodBias = samplerInfo.minLodBias;
		info.anisotropyEnable = samplerInfo.anisotropyEnable;
		info.maxAnisotropy = samplerInfo.maxAnisotropy;
		info.compareEnable = samplerInfo.compareEnable;
		info.compareOp = (VkCompareOp)samplerInfo.compreOp;
		info.minLod = samplerInfo.minLod;
		info.maxLod = samplerInfo.maxLod;
		info.borderColor = (VkBorderColor)samplerInfo.borderColor;
		info.unnormalizedCoordinates = samplerInfo.unnormalizedCoordinates;

		VkSampler handle;
		if(VK_SUCCESS != vkCreateSampler(m_Device, &info, nullptr, &handle)) {
			return nullptr;
		}
		RETURN_RHI_PTR(RSampler, handle);
	}

	void RHIVulkan::DestroySampler(RSampler* sampler)
	{
		vkDestroySampler(m_Device, ((RSamplerVk*)sampler)->handle, nullptr);
	}

} // namespace RHI
