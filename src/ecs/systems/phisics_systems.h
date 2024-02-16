#pragma once
#include <ecs/components/game_state_component.h>
#include <ecs/systems/details/coordinates_transformer.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

class PhysicsSystem
{
    entt::registry& registry;
    GameState& gameState;
    std::shared_ptr<b2World> physicsWorld;
    CoordinatesTransformer coordinatesTransformer;
    float deltaTime;
public:
    PhysicsSystem(entt::registry& registry, float deltaTime);
private:
    void RemoveDistantObjectsSystem();
    void UpdatePlayersWeaponDirection();
};
