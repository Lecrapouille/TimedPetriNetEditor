#include "Editor/Theme.hpp"


ThemeId& theme() { static ThemeId idx = ThemeId::Dark; return idx; }