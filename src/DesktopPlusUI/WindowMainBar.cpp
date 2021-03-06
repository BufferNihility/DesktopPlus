#include "WindowMainBar.h"

#include "ImGuiExt.h"
#include "TextureManager.h"
#include "InterprocessMessaging.h"
#include "WindowSettings.h"
#include "Util.h"
#include "UIManager.h"

static const ImVec4 Col_ButtonActiveDesktop(0.180f, 0.349f, 0.580f, 0.404f);

void WindowMainBar::DisplayTooltipIfHovered(const char* text)
{
    if (ImGui::IsItemHovered())
    {
        ImVec2 pos = ImGui::GetItemRectMin();
        float button_width = ImGui::GetItemRectSize().x;

        //Default tooltips are not suited for this as there's too much trouble with resize flickering and stuff
        ImGui::Begin(text, NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text(text);

        ImVec2 window_size = ImGui::GetWindowSize();

        //32 is the default size returned by ImGui on the first frame
        //I'm not sure if there isn't a better way to make the tooltip go away before we can position it correctly (ImGui::IsWindowAppearing() doesn't help)
        //Well, it works
        if (window_size.y == 32.0f)
        {
            pos.y = -1000.0f;
        }
        else
        {
            pos.x += (button_width / 2.0f) - (window_size.x / 2.0f);
            pos.y -= window_size.y + ImGui::GetStyle().WindowPadding.y;

            //Clamp x so the tooltip does not get cut off
            pos.x = clamp(pos.x, 0.0f, ImGui::GetIO().DisplaySize.x - window_size.x);
        }
        ImGui::SetWindowPos(pos);

        ImGui::End();
    }
}

void WindowMainBar::UpdateDesktopButtons()
{
    ImGuiIO& io = ImGui::GetIO();

    ImVec2 b_size, b_uv_min, b_uv_max;
    int current_desktop = ConfigManager::Get().GetConfigInt(configid_int_overlay_desktop_id);

    if (ConfigManager::Get().GetConfigBool(configid_bool_interface_mainbar_desktop_include_all))
    {
        if (current_desktop == -1)
            ImGui::PushStyleColor(ImGuiCol_Button, Col_ButtonActiveDesktop);

        ImGui::PushID(tmtex_icon_desktop_all);
        TextureManager::Get().GetTextureInfo(tmtex_icon_desktop_all, b_size, b_uv_min, b_uv_max);
        if (ImGui::ImageButton(io.Fonts->TexID, b_size, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
        {
            //Don't change to same value to avoid flicker from mirror reset
            if (current_desktop != -1)
            {
                IPCManager::Get().PostMessageToDashboardApp(ipcmsg_set_config, ConfigManager::GetWParamForConfigID(configid_int_overlay_desktop_id), -1);
                ConfigManager::Get().SetConfigInt(configid_int_overlay_desktop_id, -1);
            }
        }
        DisplayTooltipIfHovered("Combined Desktop");
        ImGui::PopID();
        ImGui::SameLine();

        if (current_desktop == -1)
            ImGui::PopStyleColor();
    }

    int monitor_count = ::GetSystemMetrics(SM_CMONITORS);

    if (ConfigManager::Get().GetConfigInt(configid_int_interface_wmr_ignore_vscreens_selection) == 1)
    {
        monitor_count = std::max(1, monitor_count - 3); //If the 3 screen assumption doesn't hold up, at least have one button
    }

    switch (ConfigManager::Get().GetConfigInt(configid_int_interface_mainbar_desktop_listing))
    {
        case mainbar_desktop_listing_individual:
        {
            ImGui::PushID("DesktopButtons");

            char tooltip_str[16];
            for (int i = 0; i < monitor_count; ++i)
            {
                ImGui::PushID(tmtex_icon_desktop_1 + i);

                //We have icons for up to 6 desktops, the rest (if it even exists) gets blank ones)
                //Numbering on the icons starts with 1. 
                //While there is no guarantee this corresponds to the same display as in the Windows settings, it is more familiar than the 0-based ID used internally
                //Why icon bitmaps? No need to create/load an icon font, cleaner rendered number and easily customizable icon per desktop for the end-user.
                if (tmtex_icon_desktop_1 + i < tmtex_icon_desktop_6)
                    TextureManager::Get().GetTextureInfo((TMNGRTexID)(tmtex_icon_desktop_1 + i), b_size, b_uv_min, b_uv_max);
                else
                    TextureManager::Get().GetTextureInfo(tmtex_icon_desktop, b_size, b_uv_min, b_uv_max);

                

                if (i == current_desktop)
                    ImGui::PushStyleColor(ImGuiCol_Button, Col_ButtonActiveDesktop);

                if (ImGui::ImageButton(io.Fonts->TexID, b_size, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
                {
                    //Don't change to same value to avoid flicker from mirror reset
                    if (i != current_desktop)
                    {
                        IPCManager::Get().PostMessageToDashboardApp(ipcmsg_set_config, ConfigManager::GetWParamForConfigID(configid_int_overlay_desktop_id), i);
                        ConfigManager::Get().SetConfigInt(configid_int_overlay_desktop_id, i);
                    }
                }

                if (i == current_desktop)
                    ImGui::PopStyleColor();

                snprintf(tooltip_str, 16, "Desktop %d", i + 1);
                DisplayTooltipIfHovered(tooltip_str);

                ImGui::PopID();
                ImGui::SameLine();
            }

            ImGui::PopID();
            break;
        }
        case mainbar_desktop_listing_cycle:
        {
            ImGui::PushID(tmtex_icon_desktop_prev);
            TextureManager::Get().GetTextureInfo(tmtex_icon_desktop_prev, b_size, b_uv_min, b_uv_max);
            if (ImGui::ImageButton(io.Fonts->TexID, b_size, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
            {
                current_desktop--;

                if (current_desktop == -1)
                    current_desktop = monitor_count - 1;

                IPCManager::Get().PostMessageToDashboardApp(ipcmsg_set_config, ConfigManager::GetWParamForConfigID(configid_int_overlay_desktop_id), current_desktop);
                ConfigManager::Get().SetConfigInt(configid_int_overlay_desktop_id, current_desktop);
            }
            DisplayTooltipIfHovered("Previous Desktop");
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(tmtex_icon_desktop_next);
            TextureManager::Get().GetTextureInfo(tmtex_icon_desktop_next, b_size, b_uv_min, b_uv_max);
            if (ImGui::ImageButton(io.Fonts->TexID, b_size, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
            {
                current_desktop++;

                if (current_desktop == monitor_count)
                    current_desktop = 0;

                IPCManager::Get().PostMessageToDashboardApp(ipcmsg_set_config, ConfigManager::GetWParamForConfigID(configid_int_overlay_desktop_id), current_desktop);
                ConfigManager::Get().SetConfigInt(configid_int_overlay_desktop_id, current_desktop);
            }
            DisplayTooltipIfHovered("Next Desktop");
            ImGui::PopID();
            ImGui::SameLine();
            break;
        }
    }

    /*ImGui::SameLine();
    ImGui::PushClipRect(ImVec2(0, 0), ImVec2(OVERLAY_WIDTH, OVERLAY_HEIGHT), false);
    ImGui::GetWindowDrawList()->AddText(ImVec2(ImGui::GetCursorScreenPos().x - 96, ImGui::GetCursorScreenPos().y + (96 - ImGui::GetFont()->FontSize) / 2),
                                        ImGui::GetColorU32(ImGuiCol_Text), "1");
    ImGui::PopClipRect();*/
}

void WindowMainBar::UpdateActionButtons()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 b_size, b_uv_min, b_uv_max;
    TextureManager::Get().GetTextureInfo(tmtex_icon_settings, b_size, b_uv_min, b_uv_max);
    //Default button size for custom actions to be the same as the settings icon so the user is able to provide oversized images without messing up the layout
    //as well as still providing a way to change the size of text buttons by editing the settings icon's dimensions
    ImVec2 b_size_default = b_size;

    auto& custom_actions = ConfigManager::Get().GetCustomActions();
    auto& action_order = ConfigManager::Get().GetActionMainBarOrder();
    int list_id = 0;
    for (auto& order_data : action_order)
    {
        if (order_data.visible)
        {
            ImGui::PushID(list_id);

            if (order_data.action_id < action_built_in_MAX) //Built-in action
            {
                bool has_icon = true;

                //Button action is always the same but we want to use icons if available
                switch (order_data.action_id)
                {
                    case action_show_keyboard: TextureManager::Get().GetTextureInfo(tmtex_icon_keyboard, b_size, b_uv_min, b_uv_max); break;                     
                    default:                   has_icon = false;
                }

                if (has_icon)
                {
                    if (ImGui::ImageButton(io.Fonts->TexID, b_size_default, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
                    {
                        IPCManager::Get().PostMessageToDashboardApp(ipcmsg_action, ipcact_action_do, order_data.action_id);
                    }
                    DisplayTooltipIfHovered(ActionManager::Get().GetActionName(order_data.action_id));
                }
                else
                {
                    if (ImGui::ButtonWithWrappedLabel(ActionManager::Get().GetActionName(order_data.action_id), b_size_default))
                    {
                        IPCManager::Get().PostMessageToDashboardApp(ipcmsg_action, ipcact_action_do, order_data.action_id);
                    }
                }
            }
            else if (order_data.action_id >= action_custom) //Custom action
            {
                const CustomAction& custom_action = custom_actions[order_data.action_id - action_custom];
                if (custom_action.IconImGuiRectID != -1)
                {
                    TextureManager::Get().GetTextureInfo(custom_action, b_size, b_uv_min, b_uv_max);
                    if (ImGui::ImageButton(io.Fonts->TexID, b_size_default, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
                    {
                        if (custom_action.FunctionType != caction_press_keys)   //Press and release of action keys is handled below instead
                        {
                            IPCManager::Get().PostMessageToDashboardApp(ipcmsg_action, ipcact_action_do, order_data.action_id);
                        }
                    }
                }
                else
                {
                    if (ImGui::ButtonWithWrappedLabel(ActionManager::Get().GetActionName(order_data.action_id), b_size_default))
                    {
                        if (custom_action.FunctionType != caction_press_keys)
                        {
                            IPCManager::Get().PostMessageToDashboardApp(ipcmsg_action, ipcact_action_do, order_data.action_id);
                        }
                    }
                }

                //Enable press and release of action keys based on button press
                if (custom_action.FunctionType == caction_press_keys)
                {
                    if (ImGui::IsItemActivated())
                    {
                        IPCManager::Get().PostMessageToDashboardApp(ipcmsg_action, ipcact_action_start, order_data.action_id);
                    }

                    if (ImGui::IsItemDeactivated())
                    {
                        IPCManager::Get().PostMessageToDashboardApp(ipcmsg_action, ipcact_action_stop, order_data.action_id);
                    }
                }

                //Only display tooltip if the label isn't already on the button (is here since we need ImGui::IsItem*() to work on the button)
                if (custom_action.IconImGuiRectID != -1)
                {
                    DisplayTooltipIfHovered(ActionManager::Get().GetActionName(order_data.action_id));
                }
            }
            ImGui::SameLine();

            ImGui::PopID();
        }
        list_id++;
    }
}

WindowMainBar::WindowMainBar(WindowSettings* settings_window) : m_WndSettingsPtr(settings_window)
{

}

void WindowMainBar::Update()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2.0f, io.DisplaySize.y), 0, ImVec2(0.5f, 1.0f));  //Center window at bottom of the overlay
    ImGui::Begin("WindowMainBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    UpdateDesktopButtons();

    UpdateActionButtons();


    ImVec2 b_size, b_uv_min, b_uv_max;

    //Settings Button
    bool settings_shown = m_WndSettingsPtr->IsShown();
    if (settings_shown)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

    ImGui::PushID(tmtex_icon_settings);
    TextureManager::Get().GetTextureInfo(tmtex_icon_settings, b_size, b_uv_min, b_uv_max);
    if (ImGui::ImageButton(io.Fonts->TexID, b_size, b_uv_min, b_uv_max, -1, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)))
    {
        if (!m_WndSettingsPtr->IsShown())
        {
            m_WndSettingsPtr->Show();
        }
        else
        {
            m_WndSettingsPtr->Hide();
        }
    }

    if (settings_shown)
        ImGui::PopStyleColor(); //ImGuiCol_Button

    DisplayTooltipIfHovered("Settings");
        
    ImGui::PopID();
    //

    ImGui::PopStyleColor(); //ImGuiCol_Button
    ImGui::PopStyleVar();   //ImGuiStyleVar_FrameRounding

    m_Pos  = ImGui::GetWindowPos();
    m_Size = ImGui::GetWindowSize();

    ImGui::End();
}

const ImVec2 & WindowMainBar::GetPos() const
{
    return m_Pos;
}

const ImVec2 & WindowMainBar::GetSize() const
{
    return m_Size;
}
