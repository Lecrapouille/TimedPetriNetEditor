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
#  include "ImGuiFileDialog/ImGuiFileDialog.h"

// *****************************************************************************
//! \brief Helper class for ImGuiFileDialog to reduce repetitive code.
//! Wraps the Open/Display/Close pattern with a callback for success handling.
// *****************************************************************************
class FileDialogHelper
{
public:

    //--------------------------------------------------------------------------
    //! \brief Open a file dialog for loading files.
    //! \param[in] key Unique key for the dialog instance.
    //! \param[in] title Window title.
    //! \param[in] extensions File extensions filter (e.g., ".json,.xml").
    //--------------------------------------------------------------------------
    static void openLoad(const char* key, const char* title, const char* extensions)
    {
        IGFD::FileDialogConfig config;
        config.path = ".";
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
        config.path = ".";
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
        if (ImGuiFileDialog::Instance()->Display(key))
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
};

#endif // FILE_DIALOG_HELPER_HPP
