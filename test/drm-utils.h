#pragma once

#include "mpp_frame.h"

int drm_init(int drm, int *crtc_id, int *plane_id);
void drm_display_frame(MppFrame frame, int drmfd, int crtc_id, int plane_id);
void drm_rga_display_frame(MppFrame frame, int drm, int crtc_id, int plane_id);
