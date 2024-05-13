#include "Editor/DearImGui/Theme.hpp"


ThemeId& theme() { static ThemeId idx = ThemeId::Dark; return idx; }