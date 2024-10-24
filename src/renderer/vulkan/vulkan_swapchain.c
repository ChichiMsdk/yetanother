#include "vulkan_swapchain.h"
#include "core/ymemory.h"
#include "core/logger.h"

#include <vulkan/vk_enum_string_helper.h>

/* TODO: Resize  !!!!!!!!!!! */

void 
vkImageViewCreate(VkContext* pContext, VkFormat format, VulkanImage* pImage, VkImageAspectFlags aspectFlags)
{
	VkResult errcode = 0;
    VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewCreateInfo.image = pImage->handle;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // TODO: Make configurable.
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    // TODO: Make configurable
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    errcode = 
		vkCreateImageView(pContext->device.logicalDev, &viewCreateInfo, pContext->pAllocator, &pImage->view);
	if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); }
}

/*
 * TODO: This function sucks, enhance it
 * - Errors
 * - Params
 */
void 
vkImageCreate(VkContext* pContext, VkImageType imageType, uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, b32 bCreateView,
		VkImageAspectFlags viewAspectFlags,
		VulkanImage* pOutImage)
{
	(void)imageType;
	VkResult errcode = 0;
    // Copy params
    pOutImage->width = width;
    pOutImage->height = height;

    // Creation info.
    VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,  // TODO: Support configurable depth.
		.mipLevels = 4,     // TODO: Support mip mapping
		.arrayLayers = 1,   // TODO: Support number of layers in the image.
		.format = format,
		.tiling = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.samples = VK_SAMPLE_COUNT_1_BIT,          // TODO: Configurable sample count.
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE  // TODO: Configurable sharing mode.
	};

    errcode = vkCreateImage(pContext->device.logicalDev, &imageCreateInfo, pContext->pAllocator, &pOutImage->handle);

    // Query memory requirements.
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(pContext->device.logicalDev, pOutImage->handle, &memoryRequirements);

    int32_t memoryType = pContext->MemoryFindIndex(memoryRequirements.memoryTypeBits, memoryFlags);
    if (memoryType == -1)
        YERROR("Required memory type not found. Image not valid.");

    VkMemoryAllocateInfo memAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryType
	};
	VK_ASSERT(vkAllocateMemory(pContext->device.logicalDev, &memAllocInfo, pContext->pAllocator, &pOutImage->memory));

    // Bind the memory
	// TODO: configurable memory offset.
    VK_ASSERT(vkBindImageMemory(pContext->device.logicalDev, pOutImage->handle, pOutImage->memory, 0));

    // Create view
    if (bCreateView)
	{
        pOutImage->view = VK_NULL_HANDLE;
        vkImageViewCreate(pContext, format, pOutImage, viewAspectFlags);
    }
}

b8
vkDeviceDetectDepthFormat(VulkanDevice *pDevice)
{
	const uint64_t candidateCount = 3;
	VkFormat pCandidates[3] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT};

	uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (uint64_t i = 0; i < candidateCount; i++)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(pDevice->physicalDev, pCandidates[i], &properties);
		if ((properties.linearTilingFeatures & flags) == flags )
		{
			pDevice->depthFormat = pCandidates[i];
			return TRUE;
		}
		else if ((properties.optimalTilingFeatures & flags) == flags)
		{
			pDevice->depthFormat = pCandidates[i];
			return TRUE;
		}
	}
	return FALSE;
}

void
vkDeviceQuerySwapchainSupport(VkPhysicalDevice physDevice, VkSurfaceKHR surface,
		VkSwapchainSupportInfo* pOutSupportInfo)
{
	VkResult errcode = 0;
    // Surface capabilities
    errcode = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physDevice, surface, &pOutSupportInfo->capabilities);
	if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); }

    // Surface formats
    errcode = vkGetPhysicalDeviceSurfaceFormatsKHR( physDevice, surface, &pOutSupportInfo->formatCount, 0);
	if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); }

    if (pOutSupportInfo->formatCount != 0)
	{
        if (!pOutSupportInfo->pFormats)
		{
            pOutSupportInfo->pFormats =
				yAlloc(sizeof(VkSurfaceFormatKHR) * pOutSupportInfo->formatCount, MEMORY_TAG_RENDERER);
        }
        errcode =
			vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &pOutSupportInfo->formatCount, pOutSupportInfo->pFormats);
		if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); }
    }

    // Present modes
    errcode = vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &pOutSupportInfo->presentModeCount, 0);
    if (pOutSupportInfo->presentModeCount != 0)
	{
        if (!pOutSupportInfo->pPresentModes)
		{
            pOutSupportInfo->pPresentModes = 
				yAlloc(sizeof(VkPresentModeKHR) * pOutSupportInfo->presentModeCount, MEMORY_TAG_RENDERER);
        }
        errcode = vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &pOutSupportInfo->presentModeCount,
				pOutSupportInfo->pPresentModes);
		if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); }
    }
}

YND VkResult 
vkSwapchainCreate(VkContext* pContext, uint32_t width, uint32_t height, VkSwapchain* pSwapchain)
{
	VkResult errcode = 0;
    VkExtent2D swapchainExtent = {width, height};
    pSwapchain->maxFrameInFlight = 2;

    // Choose a swap surface format.
    b8 bFound = FALSE;
    for (uint32_t i = 0; i < pContext->device.swapchainSupport.formatCount; ++i)
	{
        VkSurfaceFormatKHR format = pContext->device.swapchainSupport.pFormats[i];
        // Preferred formats
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
            pSwapchain->imageFormat = format;
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
        pSwapchain->imageFormat = pContext->device.swapchainSupport.pFormats[0];
    /* VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; */
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (uint32_t i = 0; i < pContext->device.swapchainSupport.presentModeCount; ++i)
	{
        VkPresentModeKHR mode = pContext->device.swapchainSupport.pPresentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
            present_mode = mode;
            break;
        }
    }
    // Requery swapchain support.
    vkDeviceQuerySwapchainSupport( pContext->device.physicalDev, pContext->surface, &pContext->device.swapchainSupport);

    // Swapchain extent
    if (pContext->device.swapchainSupport.capabilities.currentExtent.width != UINT32_MAX)
        swapchainExtent = pContext->device.swapchainSupport.capabilities.currentExtent;

    // Clamp to the value allowed by the GPU.
    VkExtent2D min = pContext->device.swapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = pContext->device.swapchainSupport.capabilities.maxImageExtent;
    swapchainExtent.width = YCLAMP(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = YCLAMP(swapchainExtent.height, min.height, max.height);

    uint32_t imageCount = pContext->device.swapchainSupport.capabilities.minImageCount + 1;
    if (pContext->device.swapchainSupport.capabilities.maxImageCount > 0 
			&& imageCount > pContext->device.swapchainSupport.capabilities.maxImageCount)
	{
        imageCount = pContext->device.swapchainSupport.capabilities.maxImageCount;
    }

    // Swapchain create info
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.surface = pContext->surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = pSwapchain->imageFormat.format;
    swapchainCreateInfo.imageColorSpace = pSwapchain->imageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	/* swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT */
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Setup the queue family indices
	uint32_t queueFamilyIndices[] = {
		(uint32_t)pContext->device.graphicsQueueIndex,
		(uint32_t)pContext->device.presentQueueIndex};
	if (pContext->device.graphicsQueueIndex != pContext->device.presentQueueIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = 0;
	}

    swapchainCreateInfo.preTransform = pContext->device.swapchainSupport.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = present_mode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = 0;

    errcode = vkCreateSwapchainKHR(pContext->device.logicalDev, &swapchainCreateInfo, pContext->pAllocator,
			&pSwapchain->handle);
	if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); return errcode;}

    // Start with a zero frame index.
    pContext->currentFrame = 0;

    // Images
    pSwapchain->imageCount = 0;
    errcode = vkGetSwapchainImagesKHR(pContext->device.logicalDev, pSwapchain->handle, &pSwapchain->imageCount, 0);
    if (!pSwapchain->pImages)
	{
        pSwapchain->pImages = yAlloc(sizeof(VkImage) * pSwapchain->imageCount, MEMORY_TAG_RENDERER);
    }
    if (!pSwapchain->pViews)
	{
        pSwapchain->pViews = yAlloc(sizeof(VkImageView) * pSwapchain->imageCount, MEMORY_TAG_RENDERER);
    }
    errcode = vkGetSwapchainImagesKHR(pContext->device.logicalDev, pSwapchain->handle, &pSwapchain->imageCount,
			pSwapchain->pImages);
	if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); return errcode;}

    // Views
    for (uint32_t i = 0; i < pSwapchain->imageCount; ++i)
	{
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = pSwapchain->pImages[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = pSwapchain->imageFormat.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        errcode = vkCreateImageView(pContext->device.logicalDev, &view_info, pContext->pAllocator, 
				&pSwapchain->pViews[i]);
		if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); return errcode;}
    }

    // Depth resources
    if (!vkDeviceDetectDepthFormat(&pContext->device))
	{
        pContext->device.depthFormat = VK_FORMAT_UNDEFINED;
        YFATAL("Failed to find a supported format!");
		return VK_ERROR_UNKNOWN;
    }

    // Create depth image and its view.
    vkImageCreate(pContext, VK_IMAGE_TYPE_2D, swapchainExtent.width, swapchainExtent.height,
        pContext->device.depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        TRUE,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &pSwapchain->depthAttachment);

    YINFO("Swapchain created successfully.");
	return errcode;
}

void
vkDestroyVulkanImage(VkContext *pContext, VulkanImage *pImage)
{
	if (pImage->view)
	{
		vkDestroyImageView(pContext->device.logicalDev, pImage->view, pContext->pAllocator);
		pImage->view = 0;
	}
	if (pImage->memory)
	{
		vkFreeMemory(pContext->device.logicalDev, pImage->memory, pContext->pAllocator);
		pImage->memory = 0;
	}
	if (pImage->handle)
	{
		vkDestroyImage(pContext->device.logicalDev, pImage->handle, pContext->pAllocator);
		pImage->handle = 0;
	}
}

YND VkResult
vkSwapchainDestroy(VkContext* pCtx, VkSwapchain* pSwapchain)
{
	VkDevice device = pCtx->device.logicalDev;
	VK_CHECK(vkDeviceWaitIdle(device));
	vkDestroyVulkanImage(pCtx, &pSwapchain->depthAttachment);
	for (uint32_t i = 0; i < pCtx->swapchain.imageCount; i++)
		vkDestroyImageView(device, pCtx->swapchain.pViews[i], pCtx->pAllocator);
	vkDestroySwapchainKHR(device, pCtx->swapchain.handle, pCtx->pAllocator);
	yFree(pCtx->swapchain.pImages, pCtx->swapchain.imageCount, MEMORY_TAG_RENDERER);
	yFree(pCtx->swapchain.pViews, pCtx->swapchain.imageCount, MEMORY_TAG_RENDERER);
	return VK_SUCCESS;
}

YND VkResult
vkSwapchainRecreate(VkContext *pCtx, uint32_t width, uint32_t height, VkSwapchain *pSwapchain)
{
	VK_RESULT(vkSwapchainDestroy(pCtx, pSwapchain));
	VK_RESULT(vkSwapchainCreate(pCtx, width, height, pSwapchain));
	return VK_SUCCESS;
}

YND VkResult 
vkSwapchainAcquireNextImageIndex(VkContext* pCtx, VkSwapchain* pSwapchain, uint64_t timeoutNS,
		VkSemaphore semaphoreImageAvailable, VkFence fence, uint32_t* pOutImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(
        pCtx->device.logicalDev,
        pSwapchain->handle,
        timeoutNS,
        semaphoreImageAvailable,
        fence,
        pOutImageIndex);
/*
 * NOTE: on AMD you can resize window and like you say “just wait” it never throw VK_ERROR_OUT_OF_DATE_KHR, 
 * but on Nvidia its always VK_ERROR_OUT_OF_DATE_KHR no matter what
 * (and crash if you dont process this destroying/freeing/recreating your allocated data). 
 * (VK_SUBOPTIMAL_KHR happens only on X11/XCB or/and xWayland)
 * https://community.khronos.org/t/vk-suboptimal-khr-is-it-safe-to-use-it-as-window-resize-detection/107848/5
 */
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// Trigger pSwapchain recreation, then boot out of the render loop.
		VK_RESULT(vkSwapchainRecreate(pCtx, pCtx->framebufferWidth, pCtx->framebufferHeight, pSwapchain));
	}
	/* WARN: Is this fatal ?! */
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		YFATAL("Failed to acquire pSwapchain image!");
		return result;
	}
    return VK_SUCCESS;
}

YND VkResult
vkSwapchainPresent(VkContext* pCtx, VkSwapchain* pSwapchain, YMB VkQueue gfxQueue, VkQueue presentQueue,
		VkSemaphore semaphoreRenderComplete, uint32_t presentImageIndex)
{
	// Return the image to the swapchain for presentation.
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &semaphoreRenderComplete,
		.swapchainCount = 1,
		.pSwapchains = &pSwapchain->handle,
		.pImageIndices = &presentImageIndex,
		.pResults = 0,
	};

	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
		VK_ASSERT(vkSwapchainRecreate(pCtx, pCtx->framebufferWidth, pCtx->framebufferHeight, pSwapchain));
	}
	else if (result != VK_SUCCESS)
	{
		YFATAL("Failed to present swap chain image!");
		return result;
	}

	/* Increment (and loop) the index. */
	pCtx->currentFrame = (pCtx->currentFrame + 1) % pSwapchain->maxFrameInFlight;
	pCtx->nbFrames++;
	return VK_SUCCESS;
}
