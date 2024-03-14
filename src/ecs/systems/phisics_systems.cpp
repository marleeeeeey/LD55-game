#include "phisics_systems.h"
#include <ecs/components/game_components.h>
#include <glm/glm.hpp>
#include <utils/box2d_helpers.h>
#include <utils/entt_registry_wrapper.h>
#include <utils/glm_box2d_conversions.h>
#include <utils/math_utils.h>

PhysicsSystem::PhysicsSystem(EnttRegistryWrapper& registryWrapper)
  : registryWrapper(registryWrapper), registry(registryWrapper.GetRegistry()),
    gameState(registry.get<GameOptions>(registry.view<GameOptions>().front())), physicsWorld(gameState.physicsWorld),
    coordinatesTransformer(registry)
{}

void PhysicsSystem::Update(float deltaTime)
{
    // Update the physics world with Box2D engine.
    physicsWorld->Step(
        deltaTime, gameState.physicsOptions.velocityIterations, gameState.physicsOptions.positionIterations);

    SetPlayersRotationToZero(); // players should not rotate.
    UpdatePlayersWeaponDirection();
    RemoveDistantObjects();
};

void PhysicsSystem::RemoveDistantObjects()
{
    auto levelBounds = gameState.levelOptions.levelBox2dBounds;

    auto physicalBodies = registry.view<PhysicsInfo>();
    for (auto entity : physicalBodies)
    {
        auto& physicalBody = physicalBodies.get<PhysicsInfo>(entity);
        b2Vec2 pos = physicalBody.bodyRAII->GetBody()->GetPosition();

        if (!utils::IsPointInsideBounds(pos, levelBounds))
        {
            registryWrapper.Destroy(entity);
        }
    }
}

// Set the direction of the weapon of the player to the last mouse position.
void PhysicsSystem::UpdatePlayersWeaponDirection()
{
    auto players = registry.view<PhysicsInfo, PlayerInfo>();
    for (auto entity : players)
    {
        const auto& [physicalBody, playerInfo] = players.get<PhysicsInfo, PlayerInfo>(entity);

        auto& lastMousePosInWindow = gameState.windowOptions.lastMousePosInWindow;
        glm::vec2 playerPosInWindow =
            coordinatesTransformer.PhysicsToScreen(physicalBody.bodyRAII->GetBody()->GetPosition());

        playerInfo.weaponDirection = glm::normalize(lastMousePosInWindow - playerPosInWindow);
    }
}

void PhysicsSystem::SetPlayersRotationToZero()
{
    auto players = registry.view<PlayerInfo, PhysicsInfo>();
    for (auto entity : players)
    {
        auto& playerInfo = players.get<PlayerInfo>(entity);
        auto& physicalBody = players.get<PhysicsInfo>(entity);
        auto body = physicalBody.bodyRAII->GetBody();
        body->SetAngularVelocity(0);
        body->SetTransform(body->GetPosition(), 0);
    }
};