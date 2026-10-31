// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef FILE_DIALOG_HELPER_HPP
#  define FILE_DIALOG_HELPER_HPP

#  include <string>
#  include <functional>
#  include "ImGuiFileDialog.h"
#  if defined(__EMSCRIPTEN__) && !defined(EXAEQUOS)
#    include <emscripten.h>
#  endif

// *****************************************************************************
//! \brief Helper class for ImGuiFileDialog to reduce repetitive code.
//! Wraps the Open/Display/Close pattern with a callback for success handling.
// *****************************************************************************
class FileDialogHelper
{
public:

    //--------------------------------------------------------------------------
    //! \brief Folder shown when a dialog opens. The web version only has the
    //! nets of the examples (its files are downloaded when saved).
    //--------------------------------------------------------------------------
    static const char* startFolder()
    {
#  if defined(__EMSCRIPTEN__) && !defined(EXAEQUOS)
        return "/examples";
#  else
        return ".";
#  endif
    }

    //--------------------------------------------------------------------------
    //! \brief Open a file dialog for loading files.
    //! \param[in] key Unique key for the dialog instance.
    //! \param[in] title Window title.
    //! \param[in] extensions File extensions filter (e.g., ".json,.xml").
    //--------------------------------------------------------------------------
    static void openLoad(const char* key, const char* title, const char* extensions)
    {
        IGFD::FileDialogConfig config;
        config.path = startFolder();
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog(key, title, extensions, config);
    }

    //--------------------------------------------------------------------------
    //! \brief Open a file dialog for saving files (with overwrite confirmation).
    //! \param[in] key Unique key for the dialog instance.
    //! \param[in] title Window title.
    //! \param[in] extensions File extensions filter.
    //--------------------------------------------------------------------------
    static void openSave(const char* key, const char* title, const char* extensions)
    {
        IGFD::FileDialogConfig config;
        config.path = startFolder();
        config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
        ImGuiFileDialog::Instance()->OpenDialog(key, title, extensions, config);
    }

    //--------------------------------------------------------------------------
    //! \brief Display the dialog and handle user interaction.
    //! \param[in] key The dialog key used in openLoad/openSave.
    //! \param[in] on_ok Callback invoked when user clicks OK with the file path.
    //! \return true if the dialog was closed (OK or Cancel), false if still open.
    //--------------------------------------------------------------------------
    static bool display(const char* key, std::function<void(std::string const&)> on_ok)
    {
        // Without a size, the dialog opens at its smallest: give it 70% of
        // the window the first time, centered, and never less than 40%.
        ImGuiViewport const* viewport = ImGui::GetMainViewport();
        ImVec2 const area = viewport->WorkSize;
        ImGui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(area.x * 0.7f, area.y * 0.7f), ImGuiCond_FirstUseEver);
        ImVec2 const min_size(area.x * 0.4f, area.y * 0.4f);
        if (ImGuiFileDialog::Instance()->Display(key, ImGuiWindowFlags_NoCollapse, min_size))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                on_ok(ImGuiFileDialog::Instance()->GetFilePathName());
            }
            ImGuiFileDialog::Instance()->Close();
            return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Web version: a file written lives in the memory of the page, so
    //! the browser downloads it to the computer. Nothing to do elsewhere.
    //! \param[in] path The file just written.
    //--------------------------------------------------------------------------
    static void download(std::string const& path)
    {
#  if defined(__EMSCRIPTEN__) && !defined(EXAEQUOS)
        // $0 is how EM_ASM names its first argument, not a C++ identifier.
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
        EM_ASM({
            var path = UTF8ToString($0);
            var blob = new Blob([FS.readFile(path)], { type: 'application/octet-stream' });
            var link = document.createElement('a');
            link.href = URL.createObjectURL(blob);
            link.download = path.split('/').pop();
            document.body.appendChild(link);
            link.click();
            link.remove();
            setTimeout(function() { URL.revokeObjectURL(link.href); }, 1000);
        }, path.c_str());
#    pragma clang diagnostic pop
#  else
        (void) path;
#  endif
    }
};

#endif // FILE_DIALOG_HELPER_HPP
