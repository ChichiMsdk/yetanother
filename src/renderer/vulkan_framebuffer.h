#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

#include "yvulkan.h"

void vkFrameBufferCreate(
		VkContext*							pCtx, 
		VulkanRenderPass*					pRenderpass,
		uint32_t							width,
		uint32_t							height,
		uint32_t							attachmentCount,
		VkImageView*						pAttachments,
		VulkanFramebuffer*					pOutFramebuffer);

void vkFramebufferDestroy(
		VkContext*							pCtx,
		VulkanFramebuffer*					pFramebuffer);

#endif // VULKAN_FRAMEBUFFER_H