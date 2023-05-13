#pragma once

#include "types.h"

typedef struct vbe_info_t {
	char signature[4];
	uint16_t version;
	uint32_t oem;
	uint32_t capabilities;
	uint32_t video_modes;
	uint16_t video_memory;
	uint16_t software_rev;
	uint32_t vendor;
	uint32_t product_name;
	uint32_t product_rev;
	char reserved[222];
	char oem_data[256];
} __attribute__ ((packed)) vbe_info_t;
