#include "sound_player.hpp"

#include "sound_effect.hpp"

#include <SFML/Audio/Listener.hpp>

#include <cmath>

namespace
{
	// X = left, Y= up, Z = forward (out of the screen}
	const float ListenerZ = 300.f;
	const float Attenuation = 1.f;
	const float MinDistance2D = 200.f;
	const float MinDistance3D = std::sqrt(MinDistance2D * MinDistance2D + ListenerZ * ListenerZ);
}

SoundPlayer::SoundPlayer()
{
	m_sound_buffers.Load(SoundEffect::kTankBulletFire, "Media/Sound/tankBullet.flac");
	m_sound_buffers.Load(SoundEffect::kExplosionDestroy, "Media/Sound/explosion1.wav");
	m_sound_buffers.Load(SoundEffect::kTankCollision, "Media/Sound/tankCollision.mp3");
	m_sound_buffers.Load(SoundEffect::kExplosion1, "Media/Sound/bulletHit.wav");
	m_sound_buffers.Load(SoundEffect::kButton, "Media/Sound/button.wav");
	m_sound_buffers.Load(SoundEffect::kButtonSelect, "Media/Sound/buttonSelect.wav");
	m_sound_buffers.Load(SoundEffect::kPickup, "Media/Sound/pickup.wav");
	m_sound_buffers.Load(SoundEffect::kWall, "Media/Sound/wallHit.wav");
	m_sound_buffers.Load(SoundEffect::kTurretFire, "Media/Sound/laser.wav");
	m_sound_buffers.Load(SoundEffect::kMissile, "Media/Sound/missile.wav");
	

	sf::Listener::setDirection({ 0.f, 0.f, -1.f });
}

void SoundPlayer::Play(SoundEffect effect)
{
	Play(effect, GetListenerPosition());
}

void SoundPlayer::Play(SoundEffect effect, sf::Vector2f position)
{
	m_sounds.emplace_back(m_sound_buffers.Get(effect));
	sf::Sound& sound = m_sounds.back();

	sound.setBuffer(m_sound_buffers.Get(effect));
	sound.setPosition({ position.x, -position.y, 0.f });
	sound.setAttenuation(Attenuation);
	sound.setMinDistance(MinDistance3D);
	sound.setVolume(100.f);

	sound.play();
}

void SoundPlayer::RemoveStoppedSounds()
{
	m_sounds.remove_if([](const sf::Sound& s)
		{
			return s.getStatus() == sf::Sound::Status::Stopped;
		});
}

void SoundPlayer::SetListenerPosition(sf::Vector2f position)
{
	sf::Listener::setPosition({ position.x, -position.y, ListenerZ });
}

sf::Vector2f SoundPlayer::GetListenerPosition() const
{
	sf::Vector3f position = sf::Listener::getPosition();
	return { position.x, -position.y };
}