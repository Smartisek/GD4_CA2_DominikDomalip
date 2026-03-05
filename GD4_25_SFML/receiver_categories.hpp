#pragma once
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kPlayer1Tank = 1 << 1,
	kPlayer2Tank = 1 << 2,
	kPlayer1Projectile = 1 << 3,
	kPlayer2Projectile = 1 << 4,
	kObstacle = 1 << 5,
	kPickup = 1 << 6,
	kParticleSystem = 1 << 7,
	kSoundEffect = 1 << 8,
	kEnemy = 1 << 9,
	kEnemyProjectile = 1 << 10
};

//A message that would be sent to all aircraft would be
//unsigned int all_aircraft = ReceiverCategories::kPlayer | ReceiverCategories::kAlloedAircraft | ReceiverCategories::kEnemyAircraft