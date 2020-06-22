// Platform Specific Functions
#pragma once
#include <R_ext/GraphicsEngine.h>
#include "drawingml_device.h"

std::string FontFamilyName(const pGEcontext gc);
void TextBoundingRect(const pGEcontext gc, const std::string &text, ML_Bounds &bounds);
