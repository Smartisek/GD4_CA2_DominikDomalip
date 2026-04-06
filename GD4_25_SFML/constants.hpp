#pragma once
constexpr auto kPlayerSpeed = 100.f;
constexpr auto kTimePerFrame = 1.f / 120.f;
constexpr auto kMaxFireRate = 5;
constexpr auto kMaxSpread = 3;
constexpr auto kPickupDropChance = 3;
constexpr auto kMissileRefill = 3;
constexpr auto kGameOverToMenuPause = 3;

constexpr float kReferenceWidth = 1920.f;
constexpr float kReferenceHeight = 1080.f;
constexpr float kMinimapScale = 0.5f;
constexpr unsigned int kMinimumFontSize = 12;

//network constants 
constexpr float kNetworkUpdateRate = 120.f;
constexpr float kServerPhysicsRate = 120.f;
constexpr float kNetworkInterpolation = 0.5f;
constexpr float kClientTimeout = 30.f;
constexpr float kSpawnRadius = 800.f; //how far from center to spawn the tanks
constexpr auto kMaxPickups = 10;