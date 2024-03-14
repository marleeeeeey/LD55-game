#include "player_control_systems.h"
#include "entt/entity/fwd.hpp"
#include "glm/fwd.hpp"
#include "my_common_cpp_utils/config.h"
#include "utils/entt_registry_wrapper.h"
#include <SDL.h>
#include <ecs/components/game_components.h>
#include <imgui_impl_sdl2.h>
#include <my_common_cpp_utils/logger.h>
#include <utils/coordinates_transformer.h>
#include <utils/factories/box2d_body_creator.h>
#include <utils/systems/input_event_manager.h>

PlayerControlSystem::PlayerControlSystem(
    EnttRegistryWrapper& registryWrapper, InputEventManager& inputEventManager,
    Box2dEnttContactListener& contactListener, ObjectsFactory& objectsFactory)
  : registryWrapper(registryWrapper), registry(registryWrapper.GetRegistry()), inputEventManager(inputEventManager),
    transformer(registry), gameState(registry.get<GameOptions>(registry.view<GameOptions>().front())),
    box2dBodyCreator(registry), contactListener(contactListener), objectsFactory(objectsFactory)
{
    SubscribeToInputEvents();
    SubscribeToContactListener();
}

void PlayerControlSystem::SubscribeToInputEvents()
{
    inputEventManager.Subscribe(
        InputEventManager::EventType::ButtonHold,
        [this](const InputEventManager::EventInfo& eventInfo) { HandlePlayerMovement(eventInfo); });

    inputEventManager.Subscribe(
        InputEventManager::EventType::ButtonReleaseAfterHold,
        [this](const InputEventManager::EventInfo& eventInfo) { HandlePlayerAttack(eventInfo); });

    inputEventManager.Subscribe(
        InputEventManager::EventType::RawSdlEvent,
        [this](const InputEventManager::EventInfo& eventInfo) { HandlePlayerBuildingAction(eventInfo); });

    inputEventManager.Subscribe(
        InputEventManager::EventType::RawSdlEvent,
        [this](const InputEventManager::EventInfo& eventInfo) { HandlePlayerWeaponDirection(eventInfo); });
};

void PlayerControlSystem::SubscribeToContactListener()
{
    contactListener.SubscribeContact(
        Box2dEnttContactListener::ContactType::BeginSensor,
        [this](entt::entity entityA, entt::entity entityB) { HandlePlayerBeginPlayerContact(entityA, entityB); });
    contactListener.SubscribeContact(
        Box2dEnttContactListener::ContactType::EndSensor,
        [this](entt::entity entityA, entt::entity entityB) { HandlePlayerEndPlayerContact(entityA, entityB); });
};

void PlayerControlSystem::HandlePlayerMovement(const InputEventManager::EventInfo& eventInfo)
{
    float movingForce = 10.0f;
    float jumpForce = 50.0f;

    auto& originalEvent = eventInfo.originalEvent;

    const auto& players = registry.view<PlayerInfo, PhysicsInfo>();
    for (auto entity : players)
    {
        const auto& [playerInfo, physicalBody] = players.get<PlayerInfo, PhysicsInfo>(entity);
        auto body = physicalBody.bodyRAII->GetBody();

        // If the player is not on the ground, then don't allow to move or jump.
        bool playerControlsWorksOnGroundOnly =
            utils::GetConfig<bool, "PlayerControlSystem.playerControlsWorksOnGroundOnly">();
        if (playerControlsWorksOnGroundOnly && playerInfo.countOfGroundContacts <= 0)
            continue;

        auto mass = body->GetMass();
        movingForce *= mass;
        jumpForce *= mass;

        if (originalEvent.key.keysym.scancode == SDL_SCANCODE_W)
        {
            if (playerInfo.countOfGroundContacts > 0)
                body->ApplyForceToCenter(b2Vec2(0, -jumpForce), true);
        }

        if (originalEvent.key.keysym.scancode == SDL_SCANCODE_A)
        {
            body->ApplyForceToCenter(b2Vec2(-movingForce, 0), true);
        }

        if (originalEvent.key.keysym.scancode == SDL_SCANCODE_D)
        {
            body->ApplyForceToCenter(b2Vec2(movingForce, 0), true);
        }
    }
}

void PlayerControlSystem::HandlePlayerAttack(const InputEventManager::EventInfo& eventInfo)
{
    if (eventInfo.originalEvent.button.button == SDL_BUTTON_LEFT)
    {
        auto& gameState = registry.get<GameOptions>(registry.view<GameOptions>().front());
        auto physicsWorld = gameState.physicsWorld;
        const auto& players = registry.view<PlayerInfo, PhysicsInfo, AnimationInfo>();
        CoordinatesTransformer transformer(registry);

        for (auto entity : players)
        {
            const auto& playerInfo = players.get<PlayerInfo>(entity);
            const auto& playerBody = players.get<PhysicsInfo>(entity).bodyRAII->GetBody();
            const auto& animationInfo = players.get<AnimationInfo>(entity);
            // TODO2: use the size from specific bounding box.
            const auto& playerSize = animationInfo.animation.frames.front().renderingInfo.sdlSize;
            const auto& weaponDirection = playerInfo.weaponDirection;

            // Calculate the position of the grenade slightly in front of the player.
            glm::vec2 playerWorldPos = transformer.PhysicsToWorld(playerBody->GetPosition());
            glm::vec2 positionInFrontOfPlayer = playerWorldPos + weaponDirection * playerSize.x;
            glm::vec2 projectileSize(5, 5);

            // Spawn flying entity.
            float force = std::min(eventInfo.holdDuration * 10.0f, 3.0f);
            auto flyingEntity =
                objectsFactory.SpawnFlyingEntity(positionInFrontOfPlayer, projectileSize, weaponDirection, force);

            // Apply the explosion component to the flying entity.
            if (playerInfo.currentWeapon == PlayerInfo::Weapon::Bazooka)
            {
                registry.emplace<ContactExplosionComponent>(flyingEntity);
                registry.emplace<ExplosionImpactComponent>(flyingEntity);
            }
            else if (playerInfo.currentWeapon == PlayerInfo::Weapon::Grenade)
            {
                registry.emplace<TimerExplosionComponent>(flyingEntity);
                registry.emplace<ExplosionImpactComponent>(flyingEntity);
            }
        }
    }
}

void PlayerControlSystem::HandlePlayerBuildingAction(const InputEventManager::EventInfo& eventInfo)
{
    auto event = eventInfo.originalEvent;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
    {
        auto physicsWorld = gameState.physicsWorld;

        auto windowPos = glm::vec2(event.button.x, event.button.y);
        auto worldPos = transformer.ScreenToWorld(windowPos);

        auto entity = registryWrapper.Create("buildingBlock");
        glm::vec2 sdlSize(10.0f, 10.0f);
        auto physicsBody = box2dBodyCreator.CreatePhysicsBody(entity, worldPos, sdlSize);
        registry.emplace<RenderingInfo>(entity, sdlSize, nullptr, SDL_Rect{}, ColorName::Green);
        registry.emplace<PhysicsInfo>(entity, physicsBody);
    }
};

void PlayerControlSystem::HandlePlayerWeaponDirection(const InputEventManager::EventInfo& eventInfo)
{
    auto event = eventInfo.originalEvent;

    if (event.type == SDL_MOUSEMOTION)
    {
        const auto& players = registry.view<PlayerInfo, PhysicsInfo>();
        for (auto entity : players)
        {
            const auto& [playerInfo, physicalBody] = players.get<PlayerInfo, PhysicsInfo>(entity);
            auto playerBody = physicalBody.bodyRAII->GetBody();

            glm::vec2 mouseWindowPos{event.motion.x, event.motion.y};
            glm::vec2 playerWindowPos = transformer.PhysicsToScreen(playerBody->GetPosition());
            glm::vec2 directionVec = mouseWindowPos - playerWindowPos;
            playerInfo.weaponDirection = glm::normalize(directionVec);
        }
    }
};

void PlayerControlSystem::HandlePlayerEndPlayerContact(entt::entity entityA, entt::entity entityB)
{
    SetGroundContactFlagIfEntityIsPlayer(entityA, false);
    SetGroundContactFlagIfEntityIsPlayer(entityB, false);
};

void PlayerControlSystem::HandlePlayerBeginPlayerContact(entt::entity entityA, entt::entity entityB)
{
    SetGroundContactFlagIfEntityIsPlayer(entityA, true);
    SetGroundContactFlagIfEntityIsPlayer(entityB, true);
};

void PlayerControlSystem::SetGroundContactFlagIfEntityIsPlayer(entt::entity entity, bool value)
{
    auto playerInfo = registry.try_get<PlayerInfo>(entity);
    if (playerInfo)
    {
        playerInfo->countOfGroundContacts += value ? 1 : -1;
        MY_LOG_FMT(debug, "Player {} countOfGroundContacts: {}", playerInfo->number, playerInfo->countOfGroundContacts);
    }
};