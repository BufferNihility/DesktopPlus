#include "FloatingUI.h"

#include "UIManager.h"
#include "OverlayManager.h"
#include "Util.h"

void FloatingUI::UpdateUITargetState()
{
    UIManager& ui_manager = *UIManager::Get();
    vr::VROverlayHandle_t ovrl_handle_floating_ui = ui_manager.GetOverlayHandleFloatingUI();

    //Find which overlay is being hovered
    vr::VROverlayHandle_t ovrl_handle_hover_target = (vr::VROverlay()->IsHoverTargetOverlay(ovrl_handle_floating_ui)) ? ovrl_handle_floating_ui : vr::k_ulOverlayHandleInvalid;
    unsigned int ovrl_id_hover_target = k_ulOverlayID_Dashboard;  //Dashboard isn't a valid target, but it doesn't matter as it's the fallback either way

    //If previous target overlay is no longer visible
    if ( (m_OvrlHandleCurrentUITarget != vr::k_ulOverlayHandleInvalid) && (!vr::VROverlay()->IsOverlayVisible(m_OvrlHandleCurrentUITarget)) )
    {
        m_OvrlHandleCurrentUITarget = vr::k_ulOverlayHandleInvalid;
        ovrl_handle_hover_target = vr::k_ulOverlayHandleInvalid;
        m_Visible = false;
        m_FadeOutDelayCount = 20;
    }

    if ( (ovrl_handle_hover_target == vr::k_ulOverlayHandleInvalid) )
    {
        for (unsigned int i = 1; i < OverlayManager::Get().GetOverlayCount(); ++i)
        {
            vr::VROverlayHandle_t ovrl_handle = OverlayManager::Get().FindOverlayHandle(i);

            if ( (ovrl_handle != vr::k_ulOverlayHandleInvalid) && (vr::VROverlay()->IsHoverTargetOverlay(ovrl_handle)) )
            {
                ovrl_handle_hover_target = ovrl_handle;
                ovrl_id_hover_target = i;
                break;
            }
        }
    }

    //Check if we're switching from another active overlay hover target, in which case we want to fade out completely first
    if ( (m_OvrlHandleCurrentUITarget != vr::k_ulOverlayHandleInvalid) && (ovrl_handle_hover_target != vr::k_ulOverlayHandleInvalid) && (ovrl_handle_hover_target != ovrl_handle_floating_ui) && (ovrl_handle_hover_target != m_OvrlHandleCurrentUITarget) )
    {
        m_IsSwitchingTarget = true;
        m_Visible = false;
        return;
    }
    else if ( (ovrl_handle_hover_target != ovrl_handle_floating_ui) && (ovrl_handle_hover_target != vr::k_ulOverlayHandleInvalid) )
    {
        m_OvrlHandleCurrentUITarget = ovrl_handle_hover_target;
        m_OvrlIDCurrentUITarget = ovrl_id_hover_target;
        m_Visible = true;
    }

    //If there is an active hover target overlay, position the floating UI
    if (m_OvrlHandleCurrentUITarget != vr::k_ulOverlayHandleInvalid)
    {
        //Okay, this is a load of poop to be honest and took a while to get right
        //The gist is that GetTransformForOverlayCoordinates() is fundamentally broken if the overlay has non-default properties
        //UV min/max not 0.0/1.0? You still get coordinates as if they were
        //Pixel aspect not 1.0? That function doesn't care
        //Also doesn't care about curvature, but that's not a huge issue

        vr::HmdMatrix34_t matrix;
        vr::TrackingUniverseOrigin origin = vr::TrackingUniverseStanding;
        vr::VRTextureBounds_t bounds;

        vr::VROverlay()->GetOverlayTextureBounds(m_OvrlHandleCurrentUITarget, &bounds);


        int ovrl_pixel_width, ovrl_pixel_height;
        ui_manager.GetOverlayPixelSize(ovrl_pixel_width, ovrl_pixel_height);

        //Get 3D height factor
        const OverlayConfigData& data = OverlayManager::Get().GetConfigData(m_OvrlIDCurrentUITarget);

        float height_factor_3d = 1.0f;

        if ( (data.ConfigInt[configid_int_overlay_3D_mode] == ovrl_3Dmode_sbs) || (data.ConfigInt[configid_int_overlay_3D_mode] == ovrl_3Dmode_ou) )
        {
            height_factor_3d = 2.0f;
        }

        //Attempt to calculate the correct offset to bottom, taking in account all the things GetTransformForOverlayCoordinates() does not
        float width;
        vr::VROverlay()->GetOverlayWidthInMeters(m_OvrlHandleCurrentUITarget, &width);
        float uv_width  = bounds.uMax - bounds.uMin;
        float uv_height = bounds.vMax - bounds.vMin;
        float cropped_width  = ovrl_pixel_width  * uv_width;
        float cropped_height = ovrl_pixel_height * uv_height * height_factor_3d;
        float aspect_ratio_orig = (float)ovrl_pixel_width / ovrl_pixel_height;
        float aspect_ratio_new = cropped_height / cropped_width;
        float height = (aspect_ratio_orig * width);
        float offset_to_bottom = -( (aspect_ratio_new * width) - (aspect_ratio_orig * width) ) / 2.0f;
        offset_to_bottom -= height / 2.0f;

        //Y-coordinate from this function is pretty much unpredictable if not pixel_height / 2
        vr::VROverlay()->GetTransformForOverlayCoordinates(m_OvrlHandleCurrentUITarget, origin, { (float)ovrl_pixel_width, (float)ovrl_pixel_height/2.0f }, &matrix);

        //Move to bottom first, vertically centering the floating UI overlay on the bottom end of the target overlay (previous function already got the X in a predictable spot)
        OffsetTransformFromSelf(matrix, 0.0f, offset_to_bottom, 0.0f);
        //Offset further to get the desired postion at the edge of the overlay
        OffsetTransformFromSelf(matrix, -1.235f, 0.1340f, 0.025f);

        vr::VROverlay()->SetOverlayTransformAbsolute(ovrl_handle_floating_ui, origin, &matrix);
    }
    
    if ( (ovrl_handle_hover_target == vr::k_ulOverlayHandleInvalid) || (m_OvrlHandleCurrentUITarget == vr::k_ulOverlayHandleInvalid) ) //If not even the UI itself is being hovered, fade out
    {
        m_FadeOutDelayCount++;

        //Delay normal fade in order to not flicker when switching hover target between mirror overlay and floating UI
        if (m_FadeOutDelayCount > 20)
        {
            //Hide
            m_Visible = false;
            m_FadeOutDelayCount = 0;
        }
    }
    else
    {
        m_Visible = true;
    }
}

FloatingUI::FloatingUI() : m_OvrlHandleCurrentUITarget(vr::k_ulOverlayHandleInvalid),
                           m_OvrlIDCurrentUITarget(0),
                           m_Alpha(0.0f),
                           m_Visible(false),
                           m_IsSwitchingTarget(false),
                           m_FadeOutDelayCount(0)
{
    m_WindowActionBar.Hide(true); //Visible by default, so hide
}

void FloatingUI::Update()
{
    UpdateUITargetState();

    if ( (m_Alpha != 0.0f) || (m_Visible) )
    {
        vr::VROverlayHandle_t ovrl_handle_floating_ui = UIManager::Get()->GetOverlayHandleFloatingUI();

        if (m_Alpha == 0.0f) //Overlay was hidden before
        {
            vr::VROverlay()->ShowOverlay(ovrl_handle_floating_ui);

            OverlayConfigData& overlay_data = OverlayManager::Get().GetConfigData(m_OvrlIDCurrentUITarget);

            //Instantly adjust action bar visibility to overlay state before fading in
            if (overlay_data.ConfigBool[configid_bool_overlay_actionbar_enabled])
            {
                m_WindowActionBar.Show(true);
            }
            else
            {
                m_WindowActionBar.Hide(true);
            }
        }

        //Alpha fade animation
        m_Alpha += (m_Visible) ? 0.1f : -0.1f;

        if (m_Alpha > 1.0f)
            m_Alpha = 1.0f;
        else if (m_Alpha < 0.0f)
            m_Alpha = 0.0f;

        vr::VROverlay()->SetOverlayAlpha(ovrl_handle_floating_ui, m_Alpha);

        if (m_Alpha == 0.0f) //Overlay was visible before
        {
            vr::VROverlay()->HideOverlay(ovrl_handle_floating_ui);
            m_WindowActionBar.Hide(true);
            //In case we were switching targets, reset switching state and target overlay
            m_IsSwitchingTarget = false;
            m_OvrlHandleCurrentUITarget = vr::k_ulOverlayHandleInvalid;
            m_OvrlIDCurrentUITarget = 0;
        }
    }

    if ( (m_Alpha != 0.0f) )
    {
        OverlayConfigData& overlay_data = OverlayManager::Get().GetConfigData(m_OvrlIDCurrentUITarget);

        //If action bar state was changed
        if (overlay_data.ConfigBool[configid_bool_overlay_actionbar_enabled])
        {
            m_WindowActionBar.Show();
        }
        else
        {
            m_WindowActionBar.Hide();
        }

        m_WindowActionBar.Update();
        m_WindowSideBar.Update(m_WindowActionBar.GetSize().y, m_OvrlIDCurrentUITarget);
    }
}

bool FloatingUI::IsVisible() const
{
    return ((m_Visible) || (m_Alpha != 0.0f));
}