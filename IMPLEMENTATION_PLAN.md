# Implementation Plan to Fix Menu UI and Camera "Teleport"

## Goal Description
The user reported three issues after the previous fix:
1. **Menu items disappear (only title visible)**: The `UIRenderSystem` re-maps and overwrites the exact same Vulkan vertex buffer for both the buttons and the title in the same frame *before* the GPU processes the first draw call. The GPU only sees the final overwrite (the title) when drawing.
2. **Pause menu options side-by-side & "V" visual bug**: The pause menu options are horizontal, and the "Menu Principal" text is too large for its 100px button, causing it to overflow to the left. Because the background quad is small, the first letter "V" (from "Voltar ao Menu") renders outside the button quad.
3. **Camera "Teleport" issue**: The `KeyboardMovementController` is fine. The issue is that the camera moves outside the Shadow Map's orthogonal projection bounds (`orthoSize = 20.0f`). When outside, the fragment shader treats the scene as being completely covered in shadow (black). This sudden blackout feels like a teleport.

## Proposed Changes

### src/systems/ui_render_system.cpp & hpp
- Refactor `UIRenderSystem` to accumulate UI elements per-frame rather than issuing separate mapping calls.
- Add `uiVertices` and `uiIndices` as class members.
- `renderGameObjects` and `renderText` will only append to these member vectors (without taking `FrameInfo`).
- Add a new `finishRender(FrameInfo &frameInfo)` method which uploads the accumulated vectors to the Vulkan buffer and issues a single draw call.

### src/first_app.cpp
- Update `FirstApp::renderMenu` and `FirstApp::renderPauseModal` to use the new method signatures (omitting `frameInfo` from `renderGameObjects` and `renderText`), and call `uiRenderSystem->finishRender(frameInfo)` at the end.
- Change the pause menu buttons to be stacked vertically (e.g. `HEIGHT / 2.0f - 25.0f` and `HEIGHT / 2.0f + 50.0f`) instead of side-by-side.
- Increase the width of the pause menu buttons from 100px to 250px so that "Voltar ao Menu" fits properly and doesn't spill over to the left.

### shaders/simple_shader.frag
- Update `ShadowCalculation` to handle fragments that fall outside the light's orthogonal projection box in all axes (X, Y, and Z).
- Specifically, check `projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0` and return `0.0` (not in shadow) so the world remains lit when exploring beyond the shadow map radius instead of turning completely black.

## Verification Plan
### Manual Verification
1. Open the game. The main menu should display the buttons correctly alongside the title.
2. In the Pause menu, the buttons should be stacked vertically and the text "Voltar ao Menu" should be completely contained within the button quad without spilling the "V".
3. When playing, hold W to walk far away from the origin. The world should remain visible (lit) when exiting the shadow map bounds instead of suddenly turning black.
