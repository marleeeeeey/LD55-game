#include "render_hud_systems.h"
#include "ecs/components/all_components.h"
#include <imgui.h>
#include <my_common_cpp_utils/Logger.h>
#include <utils/colors.h>
#include <utils/draw.h>

void RenderHUDSystem(entt::registry& registry, SDL_Renderer* renderer)
{
    auto& gameState = registry.get<GameState>(registry.view<GameState>().front());

    ImGui::Begin("HUD");

    // Caclulare count of tiles, players and dynamic bodies:
    auto tiles = registry.view<TileInfo>();
    auto players = registry.view<PlayerNumber>();
    auto dynamicBodies = registry.view<PhysicalBody>();
    size_t dynamicBodiesCount = 0;
    for (auto entity : dynamicBodies)
    {
        auto body = dynamicBodies.get<PhysicalBody>(entity).value->GetBody();
        if (body->GetType() == b2_dynamicBody)
            dynamicBodiesCount++;
    }

    // Print the gravity and camera scale, count of tiles, players and dynamic bodies in one line.
    auto gravity = gameState.physicsWorld->GetGravity().Length();
    auto cameraScale = gameState.cameraScale;
    ImGui::Text(MY_FMT("{:.2f}/{:.2f} (Gr/Sc)", gravity, cameraScale).c_str());
    ImGui::Text(MY_FMT("{}/{}/{} (Ts/Ps/DB)", tiles.size(), players.size(), dynamicBodiesCount).c_str());

    // Draw controls the physics world.
    ImGui::SliderInt("Velocity Iterations", (int*)&gameState.velocityIterations, 1, 10);
    ImGui::SliderInt("Position Iterations", (int*)&gameState.positionIterations, 1, 10);
    ImGui::SliderInt("Mini Tile Resolution", (int*)&gameState.miniTileResolution, 1, 8);
    ImGui::SliderFloat("Dynamic Body Probability", &gameState.dynamicBodyProbability, 0.0f, 1.0f);
    ImGui::SliderFloat("Gap Between Physical And Visual", &gameState.gapBetweenPhysicalAndVisual, 0.0f, 1.0f);
    ImGui::Checkbox("Prevent Creation Invisible Tiles", &gameState.preventCreationInvisibleTiles);
    if (ImGui::Button("Reload Map"))
        gameState.reloadMap = true;
    ImGui::End();
}

void DrawGridSystem(SDL_Renderer* renderer, const GameState& gameState)
{
    const int gridSize = 32;
    const SDL_Color gridColor = GetSDLColor(ColorName::Grey);
    const SDL_Color screenCenterColor = GetSDLColor(ColorName::Red);
    const SDL_Color originColor = GetSDLColor(ColorName::Green);

    // Get the window size to determine the drawing area
    int windowWidth = static_cast<int>(gameState.windowSize.x);
    int windowHeight = static_cast<int>(gameState.windowSize.y);

    auto& cameraCenter = gameState.cameraCenter;

    // Calculate the start and end points for drawing the grid
    int startX = static_cast<int>(cameraCenter.x - windowWidth / 2 / gameState.cameraScale);
    int startY = static_cast<int>(cameraCenter.y - windowHeight / 2 / gameState.cameraScale);
    int endX = startX + windowWidth / gameState.cameraScale;
    int endY = startY + windowHeight / gameState.cameraScale;

    // Align the beginning of the grid with the cell boundaries and add 1.4 of the grid size to center the grid.
    // This shift is necessary to align Box2D bodies with the grid.
    startX -= startX % gridSize + gridSize / 4;
    startY -= startY % gridSize + gridSize / 4;

    // Draw vertical grid lines
    SetRenderDrawColor(renderer, gridColor);
    for (int x = startX; x <= endX; x += gridSize)
    {
        int screenX = static_cast<int>((x - cameraCenter.x) * gameState.cameraScale + windowWidth / 2);
        SDL_RenderDrawLine(renderer, screenX, 0, screenX, windowHeight);
    }

    // Draw horizontal grid lines
    for (int y = startY; y <= endY; y += gridSize)
    {
        int screenY = static_cast<int>((y - cameraCenter.y) * gameState.cameraScale + windowHeight / 2);
        SDL_RenderDrawLine(renderer, 0, screenY, windowWidth, screenY);
    }

    // Draw the center of screen point
    DrawCross(renderer, gameState.windowSize / 2.0f, 20, screenCenterColor);
}
